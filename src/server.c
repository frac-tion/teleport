/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003, Ximian, Inc.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libsoup/soup.h>
#include <glib/gstdio.h>

#include "get.h"

static int port;
static SoupServer *server;
static const char *tls_cert_file, *tls_key_file;

  static int
compare_strings (gconstpointer a, gconstpointer b)
{
  const char **sa = (const char **)a;
  const char **sb = (const char **)b;

  return strcmp (*sa, *sb);
}

  static GString *
get_directory_listing (const char *path)
{
  GPtrArray *entries;
  GString *listing;
  char *escaped;
  GDir *dir;
  const gchar *d_name;
  int i;

  entries = g_ptr_array_new ();
  dir = g_dir_open (path, 0, NULL);
  if (dir) {
    while ((d_name = g_dir_read_name (dir))) {
      if (!strcmp (d_name, ".") ||
          (!strcmp (d_name, "..") &&
           !strcmp (path, "./")))
        continue;
      escaped = g_markup_escape_text (d_name, -1);
      g_ptr_array_add (entries, escaped);
    }
    g_dir_close (dir);
  }

  g_ptr_array_sort (entries, (GCompareFunc)compare_strings);

  listing = g_string_new ("<html>\r\n");
  escaped = g_markup_escape_text (strchr (path, '/'), -1);
  g_string_append_printf (listing, "<head><title>Index of %s</title></head>\r\n", escaped);
  g_string_append_printf (listing, "<body><h1>Index of %s</h1>\r\n<p>\r\n", escaped);
  g_free (escaped);
  for (i = 0; i < entries->len; i++) {
    g_string_append_printf (listing, "<a href=\"%s\">%s</a><br>\r\n",
        (char *)entries->pdata[i], 
        (char *)entries->pdata[i]);
    g_free (entries->pdata[i]);
  }
  g_string_append (listing, "</body>\r\n</html>\r\n");

  g_ptr_array_free (entries, TRUE);
  return listing;
}

  static void
do_get (SoupServer *server, SoupMessage *msg, const char *path)
{
  char *slash;
  GStatBuf st;

  printf("paths: %s", path);
  if (g_stat (path, &st) == -1) {
    if (errno == EPERM)
      soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
    else if (errno == ENOENT)
      soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
    else
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
    GString *listing;
    char *index_path;

    slash = strrchr (path, '/');
    if (!slash || slash[1]) {
      char *redir_uri;

      redir_uri = g_strdup_printf ("%s/", soup_message_get_uri (msg)->path);
      soup_message_set_redirect (msg, SOUP_STATUS_MOVED_PERMANENTLY,
          redir_uri);
      g_free (redir_uri);
      return;
    }

    index_path = g_strdup_printf ("%s/index.html", path);
    if (g_stat (path, &st) != -1) {
      do_get (server, msg, index_path);
      g_free (index_path);
      return;
    }
    g_free (index_path);

    listing = get_directory_listing (path);
    soup_message_set_response (msg, "text/html",
        SOUP_MEMORY_TAKE,
        listing->str, listing->len);
    soup_message_set_status (msg, SOUP_STATUS_OK);
    g_string_free (listing, FALSE);
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
do_get_response_json (SoupServer *server, SoupMessage *msg, const char *path)
{
  char *slash;
  GStatBuf st;

  printf("paths: %s", path);
  if (g_stat (path, &st) == -1) {
    if (errno == EPERM)
      soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
    else if (errno == ENOENT)
      soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
    else
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
    GString *listing;
    char *index_path;

    slash = strrchr (path, '/');
    if (!slash || slash[1]) {
      char *redir_uri;

      redir_uri = g_strdup_printf ("%s/", soup_message_get_uri (msg)->path);
      soup_message_set_redirect (msg, SOUP_STATUS_MOVED_PERMANENTLY,
          redir_uri);
      g_free (redir_uri);
      return;
    }

    index_path = g_strdup_printf ("%s/index.html", path);
    if (g_stat (path, &st) != -1) {
      do_get (server, msg, index_path);
      g_free (index_path);
      return;
    }
    g_free (index_path);

    listing = get_directory_listing (path);
    soup_message_set_response (msg, "text/html",
        SOUP_MEMORY_TAKE,
        listing->str, listing->len);
    soup_message_set_status (msg, SOUP_STATUS_OK);
    g_string_free (listing, FALSE);
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
static void handle_incoming_file(const char * hash, const char * name, const char * size, const char * origin) {
  //If the user accepts the file
  do_downloading(g_strdup_printf("http://%s:%d/transfer/%s", origin, port, hash), name);
  g_print("Got a new file form %s with size:%s with title: %s\n", origin, size, name);
}

  static void
server_callback (SoupServer *server, SoupMessage *msg,
    const char *path, GHashTable *query,
    SoupClientContext *context, gpointer data)
{
  char *file_path;
  SoupMessageHeadersIter iter;
  GString * response;
  const char *name, *value, *token, *size, *file_name;

  g_print ("%s %s HTTP/1.%d\n", msg->method, path,
      soup_message_get_http_version (msg));
  soup_message_headers_iter_init (&iter, msg->request_headers);
  while (soup_message_headers_iter_next (&iter, &name, &value))
    g_print ("%s: %s\n", name, value);
  if (msg->request_body->length)
    g_print ("%s\n", msg->request_body->data);

  if (data != NULL) {
    g_print("File to share %s\n", data);
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
        handle_incoming_file(token, file_name, size, "localhost");
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

    //do_get_response_json (server, msg, "hello world");
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
quit (int sig)
{
  /* Exit cleanly on ^C in case we're valgrinding. */
  exit (0);
}


int addRouteToServer(char *name, char *file_to_send, char *destination) {
  soup_server_add_handler (server, g_strdup_printf("/transfer/%s", name),
      server_callback, g_strdup(file_to_send), NULL);
  //send notification of avabile file to the client
  //For getting file size
  //https://developer.gnome.org/gio/stable/GFile.html#g-file-query-info
  do_client_notify(g_strdup_printf("http://%s:%d/?token=%s&size=0&name=%s\n", destination, port, 
        name, file_to_send));
  return 0;
}

int run_http_server() {
  //GMainLoop *loop;
  GSList *uris, *u;
  char *str;
  GTlsCertificate *cert;
  GError *error = NULL;

  port = 3000;
  server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "teleport-httpd ",
      NULL);
  soup_server_listen_all (server, port, 0, &error);

  soup_server_add_handler (server, NULL,
      server_callback, NULL, NULL);

  uris = soup_server_get_uris (server);
  for (u = uris; u; u = u->next) {
    str = soup_uri_to_string (u->data, FALSE);
    g_print ("Listening on %s\n", str);
    g_free (str);
    soup_uri_free (u->data);
  }
  g_slist_free (uris);

  g_print ("\nWaiting for requests...\n");

  //loop = g_main_loop_new (NULL, TRUE);
  //g_main_loop_run (loop);

  return 0;
}

/*  int
main (int argc, char **argv)
{
  char *file_to_send = "/home/julian/teleport/docs/flow-diagram.svg";
  createServer(file_to_send);
  return 0;
}
*/
