/* teleport-server.c
 *
 * Copyright 2020 Julian Sparber <julian@sparber.net>
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
#include "teleport-file.h"
#include "teleport-app.h"

struct _TeleportServer {
  SoupServer parent;

  GHashTable *send_files;
  GHashTable *devices;
};

enum {                       
  RECIVED_FILE, N_SIGNALS
};

static gint signals [N_SIGNALS];

G_DEFINE_TYPE (TeleportServer, teleport_server, SOUP_TYPE_SERVER)

static void
finished_cb (TeleportFile *file,
             SoupMessage  *msg)
{
  /* TODO: remove file from send_files hash table */
  /* teleport_file_set_status (file, TELEPORT_FILE_COMPLETED); */
  teleport_file_set_progress (file, 100);
}

static void
start_sending_cb (TeleportFile *file,
                  SoupMessage  *msg)
{
  /* teleport_file_set_status (file, TELEPORT_FILE_STARTED); */
  teleport_file_set_progress (file, 0);
}

static void
update_progress_cb (TeleportFile *file,
                    SoupBuffer   *chunk,
                    SoupMessage  *msg)
{
  guint progress;

  progress = teleport_file_get_progress (file) + (chunk->length / 100) * teleport_file_get_size (file);
  teleport_file_set_progress (file, progress);
}

static void
file_request_cb (TeleportServer *self,
                 SoupMessage *msg,
                 const char *path,
                 GHashTable *query,
                 SoupClientContext *context,
                 gpointer data)
{
  if (msg->method == SOUP_METHOD_GET) {
    GMappedFile *mapping;
    SoupBuffer *buffer = NULL;
    gchar *key;
    TeleportFile *file;

    key = g_strrstr (path, "/") + 1;
    file = g_hash_table_lookup (self->send_files, key);

    if (!TELEPORT_IS_FILE (file)) {
      soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
      return;
    }

    g_signal_connect_swapped (msg, "finished", G_CALLBACK (finished_cb), file);
    g_signal_connect_swapped (msg, "starting", G_CALLBACK (start_sending_cb), file);
    g_signal_connect_swapped (msg, "wrote-body-data", G_CALLBACK (update_progress_cb), file);

    mapping = g_mapped_file_new (teleport_file_get_source_path (file), FALSE, NULL);
    if (!mapping) {
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
      return;
    }

    buffer = soup_buffer_new_with_owner (g_mapped_file_get_contents (mapping),
                                         g_mapped_file_get_length (mapping),
                                         mapping, (GDestroyNotify)g_mapped_file_unref);
    soup_message_body_append_buffer (msg->response_body, buffer);
    soup_message_set_status (msg, SOUP_STATUS_OK);
  } else {
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
  }
}

static void
incoming_files_cb (TeleportServer *self,
                   SoupMessage *msg,
                   const char *path,
                   GHashTable *query,
                   SoupClientContext *context,
                   gpointer data)
{
  TeleportFile *file;
  TeleportPeer *peer;
  g_autofree gchar *source_host = NULL;
  g_autofree gchar *source_path = NULL;

  if (msg->method == SOUP_METHOD_POST) {
    soup_message_set_status (msg, SOUP_STATUS_OK);

    file = teleport_file_new_from_serialized (msg->request_body->data);

    source_host = g_strdup_printf ("%s", soup_client_context_get_host (context));
    /* TODO: The sender should already set the correct source path */
    source_path = g_strdup_printf ("http://%s:3000/transfer/%s",
                                   source_host,
                                   teleport_file_get_id (file));
    teleport_file_set_source_path (file, source_path);

    /* TODO: this should contain also the port */
    peer = g_hash_table_lookup (self->devices, source_host);
    if (TELEPORT_IS_PEER (peer) && TELEPORT_IS_FILE (file))
      g_signal_emit (self, signals[RECIVED_FILE], 0, peer, file);
    else
      g_warning ("Recived file offer but couldn't find the origin device or something while parsing went wrong");
  } else {
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
  }
}

static void
teleport_server_finalize (GObject *object)
{
  TeleportServer *self = TELEPORT_SERVER (object);
  g_hash_table_destroy (self->send_files);
  g_hash_table_destroy (self->devices);

  G_OBJECT_CLASS (teleport_server_parent_class)->finalize (object);
} 

static void
teleport_server_class_init (TeleportServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = teleport_server_finalize;

  signals[RECIVED_FILE] = g_signal_new ("recived_file",
                                         G_TYPE_OBJECT,
                                         G_SIGNAL_RUN_LAST,
                                         0,
                                         NULL /* accumulator */,
                                         NULL /* accumulator data */,
                                         NULL /* C marshaller */,
                                         G_TYPE_NONE /* return_type */,
                                         2,
                                         TELEPORT_TYPE_PEER,
                                         TELEPORT_TYPE_FILE);
}

static void
teleport_server_init (TeleportServer *self)
{
  self->send_files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  self->devices = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

TeleportServer *
teleport_server_new (guint port) {
  TeleportServer *self;
  g_autoptr(GError) error = NULL;
  
  self = g_object_new (TELEPORT_TYPE_SERVER,
                              SOUP_SERVER_SERVER_HEADER, "teleport-httpd ",
                              NULL);

  /* add handler for incoming files */
  soup_server_add_handler (SOUP_SERVER (self),
                           "/incoming",
                           (SoupServerCallback) incoming_files_cb,
                           NULL, NULL);

  /* add handler for sent files */
  soup_server_add_handler (SOUP_SERVER (self),
                           "/transfer",
                           (SoupServerCallback) file_request_cb,
                           NULL, NULL);

  if (!soup_server_listen_all (SOUP_SERVER (self), port, 0, &error))
    g_warning ("Couldn't start http server: %s", error->message);

  return self;
}

void
teleport_server_add_file (TeleportServer *self,
                          TeleportFile *file)
{
  g_hash_table_insert (self->send_files, g_strdup (teleport_file_get_id (file)), file);
}

void
teleport_server_add_peer (TeleportServer *self,
                          TeleportPeer *device)
{
  g_return_if_fail (TELEPORT_IS_SERVER (self));
  g_return_if_fail (TELEPORT_IS_PEER (device));

  g_hash_table_insert (self->devices,
                       g_strdup (teleport_peer_get_ip (device)),
                       g_object_ref (device));
}

void
teleport_server_remove_peer (TeleportServer *self,
                             TeleportPeer *device)
{
  g_return_if_fail (TELEPORT_IS_SERVER (self));
  g_return_if_fail (TELEPORT_IS_PEER (device));

  g_hash_table_remove (self->devices, g_strdup (teleport_peer_get_ip (device)));
}
