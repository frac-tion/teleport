/* teleport-server.c
 *
 * Copyright 2017 Julian Sparber <julian@sparber.com>
 *
 * Teleport is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>

#include <libsoup/soup.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "teleport-server.h"
#include "teleport-get.h"
#include "teleport-app.h"

static int port;
static SoupServer *glob_server;
//static const char *tls_cert_file, *tls_key_file;

static void
do_get (SoupServer *server, SoupMessage *msg, const char *path)
{
  GStatBuf st;

  if (g_stat (path, &st) == -1) {
    if (errno == EPERM)
      soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
    else if (errno == ENOENT)
      soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
    else
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  if (msg->method == SOUP_METHOD_GET) {
    GMappedFile *mapping;
    SoupBuffer *buffer;

    mapping = g_mapped_file_new (path, FALSE, NULL);
    if (!mapping) {
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
      return;
    }

    buffer = soup_buffer_new_with_owner (g_mapped_file_get_contents (mapping),
                                         g_mapped_file_get_length (mapping),
                                         mapping, (GDestroyNotify)g_mapped_file_unref);
    soup_message_body_append_buffer (msg->response_body, buffer);
    soup_buffer_free (buffer);
  } else /* msg->method == SOUP_METHOD_HEAD */ {
    char *length;

    /* We could just use the same code for both GET and
     * HEAD (soup-message-server-io.c will fix things up).
     * But we'll optimize and avoid the extra I/O.
     */
    length = g_strdup_printf ("%lu", (gulong)st.st_size);
    soup_message_headers_append (msg->response_headers,
                                 "Content-Length", length);
    g_free (length);
  }

  soup_message_set_status (msg, SOUP_STATUS_OK);
}

static void 
handle_incoming_file(const char *hash,
                     const char *filename,
                     const int size,
                     const char *origin) {
  GVariantBuilder *builder;
  GVariant *value;

  g_print("Got a new file form %s with size:%d with title: %s\n", origin, size, filename);
  builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  g_variant_builder_add (builder, "s", origin);
  g_variant_builder_add (builder,
                         "s",
                         g_strdup_printf("http://%s:%d/transfer/%s",
                                         origin,
                                         port,
                                         hash)),
                        g_variant_builder_add (builder, "s", filename);
  value = g_variant_new ("as", builder);
  g_variant_builder_unref (builder);

  // create_user_notification startes the download
  // if the user wants to save the file
  create_user_notification(filename, size, origin, value);
}

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *context, gpointer data)
{
  char *file_path;
  SoupMessageHeadersIter iter;
  GString * response;
  const char *name, *value, *token, *size, *file_name, *origin_addr;

  origin_addr = soup_client_context_get_host (context);

  g_print ("%s %s HTTP/1.%d\n", msg->method, path,
           soup_message_get_http_version (msg));
  soup_message_headers_iter_init (&iter, msg->request_headers);
  while (soup_message_headers_iter_next (&iter, &name, &value))
    g_print ("%s: %s\n", name, value);
  if (msg->request_body->length)
    g_print ("%s\n", msg->request_body->data);

  if (data != NULL) {
    g_print("File to share %s\n", (char *)data);
    file_path = g_strdup(data);
    response = g_string_new("{\"error\": false, \"message\": \"Success\"}");
  }
  else {
    file_path = NULL;
    if (query != NULL) {
      token = g_hash_table_lookup (query, "token");
      size = g_hash_table_lookup (query, "size");
      file_name = g_hash_table_lookup (query, "name");

      if (token != NULL && size != NULL && file_name != NULL) {
        g_print("Token: %s, Size: %s, Name: %s\n", token, size, file_name);
        response = g_string_new("{\"error\": false, \"message\": \"Success\"}");
        //handle_incoming_file(token, file_name, g_ascii_strtoull (size, NULL, 0), origin_addr);
      }
      else 
        response = g_string_new("{\"error\": true, \"message\": \"query malformed\"}");
    }
    else {
      g_print("No query passed");
      response = g_string_new("{\"error\": true, \"message\": \"No query passed\"}");
    }
  }

  if (g_strcmp0(path, "/") == 0) {
  }

  if (msg->method == SOUP_METHOD_GET || msg->method == SOUP_METHOD_HEAD) {
    if (file_path != NULL)
      do_get (server, msg, file_path);
    else {
      soup_message_set_response (msg, "application/json",
                                 SOUP_MEMORY_TAKE,
                                 response->str, response->len);
      soup_message_set_status (msg, SOUP_STATUS_OK);
      g_print("Handle response\n");
    }
  }
  else
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);

  if (file_path != NULL)
    g_free (file_path);
  if (response != NULL)
    g_string_free (response, FALSE);
  g_print ("  -> %d %s\n\n", msg->status_code, msg->reason_phrase);
}

static void
remove_server_route (SoupServer *server, const gchar *path)
{
  soup_server_remove_handler (server, path);
  g_print ("Route %s has expired, removing it\n", path);
}

static gboolean
do_server_timeout (gpointer user_data)
{
  gchar *path = user_data;
  remove_server_route(glob_server, path);
  g_free(path);
  return FALSE;
}

int
teleport_server_add_route (gchar *name,
                           gchar *file_to_send,
                           gchar *destination) {
  GFile *file;
  GFileInfo *fileInfo;
  gchar *path;

  path = g_strdup_printf("/transfer/%s", name);
  soup_server_add_handler (glob_server, path,
                           server_callback, g_strdup(file_to_send), NULL);

  /* send notification of available file to the client */
  /* getting file size */
  file = g_file_new_for_path(file_to_send);
  fileInfo = g_file_query_info(file,
                               "standard::display-name,standard::size",
                               G_FILE_QUERY_INFO_NONE,
                               NULL,
                               NULL);

  teleport_get_do_client_notify(g_strdup_printf("http://%s:%d/?token=%s&size=%jd&name=%s\n",
                                                destination,
                                                port,
                                                name,
                                                g_file_info_get_size(fileInfo),
                                                g_file_info_get_display_name(fileInfo)));

  /* Add timeout of 2 min which removes the route again */
  g_timeout_add_seconds (2 * 60, do_server_timeout, path);

  g_object_unref(fileInfo);
  g_object_unref(file);
  return 0;
}

int
teleport_server_run (void) {
  GSList *uris, *u;
  char *str;
  //GTlsCertificate *cert;
  GError *error = NULL;

  port = 3000;
  glob_server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "teleport-httpd ",
                                 NULL);
  soup_server_listen_all (glob_server, port, 0, &error);

  soup_server_add_handler (glob_server, NULL,
                           server_callback, NULL, NULL);

  uris = soup_server_get_uris (glob_server);
  for (u = uris; u; u = u->next) {
    str = soup_uri_to_string (u->data, FALSE);
    g_print ("Listening on %s\n", str);
    g_free (str);
    soup_uri_free (u->data);
  }
  g_slist_free (uris);

  return 0;
}
