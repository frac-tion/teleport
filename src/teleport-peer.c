/* teleport-peer.c
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

#include <gtk/gtk.h>
#include <libsoup/soup.h>

#include "teleport-peer.h"

struct _TeleportPeer
{
  GObject parent;

  gchar *name;
  gchar *ip;
  guint port;

  GListStore *files;
};

enum {
  PROP_0,
  PROP_NAME,
  PROP_IP,
  PROP_PORT,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


G_DEFINE_TYPE (TeleportPeer, teleport_peer, G_TYPE_OBJECT)

static void
incoming_file_notification (TeleportPeer *self,
                            TeleportFile *file)
{
  g_autoptr (GNotification) notification;
  GVariant *target;
  const gchar *notification_id;

  notification = g_notification_new ("Teleport");
  notification_id = teleport_file_get_id (file);
  target = g_variant_new_string (notification_id);
  g_notification_set_body (notification,
                           g_strdup_printf ("%s is sending %s (%s)",
                                           teleport_peer_get_name (self),
                                           teleport_file_get_destination_path (file),
                                           g_format_size (teleport_file_get_size (file))));

  g_notification_add_button_with_target_value (notification, "Decline", "app.abort-file", target);
  g_notification_add_button_with_target_value (notification, "Save", "app.save", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification (g_application_get_default (), notification_id, notification);
}

static void
file_ready_notification (TeleportPeer *self,
                         TeleportFile *file)
{
  g_autoptr (GNotification) notification;
  GVariant *target;
  const gchar *notification_id;

  notification = g_notification_new ("Teleport");
  notification_id = teleport_file_get_id (file);
  target = g_variant_new_string (notification_id);
  g_notification_set_body (notification,
                           g_strdup_printf ("Transfer of %s from %s is complete",
                                            teleport_file_get_destination_path (file),
                                            teleport_peer_get_name (self)));
  g_notification_add_button_with_target_value (notification, "Show in folder", "app.open-folder", target);
  g_notification_add_button_with_target_value (notification, "Open", "app.open-file", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification (g_application_get_default (), notification_id, notification);
}

static void
file_state_changed_cb (TeleportPeer *self,
                       GParamSpec   *pspec,
                       TeleportFile *file)
{
  const gchar *notification_id;
  notification_id = teleport_file_get_id (file);

  switch (teleport_file_get_state (file)) {
  case TELEPORT_FILE_STATE_TRANSFAIR:
    g_application_withdraw_notification (g_application_get_default (), notification_id);
    break;
  case TELEPORT_FILE_STATE_FINISH:
    file_ready_notification (self, file);
    break;
  case TELEPORT_FILE_STATE_REJECT:
  case TELEPORT_FILE_STATE_CANCELLED:
    teleport_peer_remove_file (self, file);
    break;
  case TELEPORT_FILE_STATE_UNKNOWN:
  case TELEPORT_FILE_STATE_NEW:
    break;
  case TELEPORT_FILE_STATE_DESTINATION_ERROR:
    g_debug ("The file exists already in the download folder: %s",
              teleport_file_get_destination_path (file));
    break;
  case TELEPORT_FILE_STATE_ERROR:
    g_warning ("There has been an error with a file: %s",
               teleport_file_get_destination_path (file));
    break;
  }
}

static void
teleport_peer_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  TeleportPeer *self = TELEPORT_PEER (object);

  switch (property_id) {
  case PROP_NAME:
    teleport_peer_set_name (self, g_value_get_string (value));
    break;
  case PROP_IP:
    teleport_peer_set_ip (self, g_value_get_string (value));
    break;
  case PROP_PORT:
    teleport_peer_set_port (self, g_value_get_uint (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
teleport_peer_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  TeleportPeer *self = TELEPORT_PEER (object);

  switch (property_id) {
  case PROP_NAME:
    g_value_set_string (value, teleport_peer_get_name (self));
    break;
  case PROP_IP:
    g_value_set_string (value, teleport_peer_get_ip (self));
    break;
  case PROP_PORT:
    g_value_set_uint (value, teleport_peer_get_port (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void 
teleport_peer_finalize (GObject *object)
{
  TeleportPeer *self = TELEPORT_PEER (object);

  g_free (self->name);
  g_free (self->ip);

  G_OBJECT_CLASS (teleport_peer_parent_class)->finalize (object);
}

static void
teleport_peer_class_init (TeleportPeerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = teleport_peer_set_property;
  object_class->get_property = teleport_peer_get_property;
  object_class->finalize = teleport_peer_finalize;

  props[PROP_NAME] =
   g_param_spec_string ("name",
                        "Name",
                        "The name of the peer",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_IP] =
   g_param_spec_string ("ip",
                        "Ip address",
                        "The ip address of the peer",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_PORT] =
   g_param_spec_uint ("port",
                      "Port",
                      "The port used by the peer",
                      0, G_MAXINT16, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
teleport_peer_init (TeleportPeer *self)
{
  self->name = NULL;
  self->ip = NULL;
  self->port = 0;
  self->files = g_list_store_new (TELEPORT_TYPE_FILE);
}

TeleportPeer *
teleport_peer_new (const gchar *name, const gchar *ip, guint port)
{
  return g_object_new (TELEPORT_TYPE_PEER, "name", name, "ip", ip, "port", port, NULL);
}

void
teleport_peer_set_name (TeleportPeer *self, const gchar *name)
{
  g_return_if_fail (TELEPORT_IS_PEER (self));

  if (g_strcmp0 (name, self->name) == 0)
    return;

  g_free(self->name);
  self->name = g_strdup(name);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NAME]);
}

const gchar *
teleport_peer_get_name (TeleportPeer *self)
{
  g_return_val_if_fail (TELEPORT_IS_PEER (self), NULL);

  return self->name;
}

void
teleport_peer_set_ip (TeleportPeer *self, const gchar *ip)
{
  g_return_if_fail (TELEPORT_IS_PEER (self));

  if (g_strcmp0 (ip, self->ip) == 0)
    return;

  g_free(self->ip);
  self->ip = g_strdup(ip);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IP]);
}

const gchar *
teleport_peer_get_ip (TeleportPeer *self)
{
  g_return_val_if_fail (TELEPORT_IS_PEER (self), NULL);

  return self->ip;
}

void
teleport_peer_set_port (TeleportPeer *self, guint port)
{
  g_return_if_fail (TELEPORT_IS_PEER (self));

  if (port == self->port)
    return;

  self->port = port;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PORT]);
}

guint
teleport_peer_get_port (TeleportPeer *self)
{
  g_return_val_if_fail (TELEPORT_IS_PEER (self), 0);

  return self->port;
}

gchar *
teleport_peer_get_incoming_address (TeleportPeer *self)
{
  return g_strdup_printf ("http://%s:%d/incoming", self->ip, self->port);
}

static void
send_file_cb (SoupSession *session,
              SoupMessage *msg,
              TeleportFile *file)
{
  g_print ("File was actually send\n");
}

void
teleport_peer_send_file (TeleportPeer *destination,
                         TeleportFile *file)
{
  SoupSession *session;
  SoupMessage *msg;
  g_autofree gchar *data = NULL; 

  session = g_object_new (SOUP_TYPE_SESSION,
                          SOUP_SESSION_ADD_FEATURE_BY_TYPE,
                          SOUP_TYPE_CONTENT_DECODER,
                          SOUP_SESSION_USER_AGENT,
                          "teleport",
                          SOUP_SESSION_ACCEPT_LANGUAGE_AUTO,
                          TRUE,
                          NULL);

  msg = soup_message_new ("POST", teleport_peer_get_incoming_address (destination));
  data = teleport_file_serialize (file);
  g_print ("Data: %s\n%s\n", data, teleport_peer_get_incoming_address (destination));
  soup_message_set_request (msg,
                            "application/json",
                            SOUP_MEMORY_COPY,
                            data,
                            strlen (data));
  soup_message_set_flags (msg, SOUP_MESSAGE_NO_REDIRECT);
  soup_session_queue_message (session, g_object_ref (msg), (SoupSessionCallback) send_file_cb, file);
}

void
teleport_peer_add_file (TeleportPeer *self,
                        TeleportFile *file)
{
  g_list_store_append (self->files, file);
  g_signal_connect_swapped (file, "notify::state", G_CALLBACK (file_state_changed_cb), self);
  incoming_file_notification (self, file);
}

void
teleport_peer_remove_file (TeleportPeer *self,
                           TeleportFile *file)
{
  guint position = 0;

  if (g_list_store_find (self->files, file, &position))
    g_list_store_remove (self->files, position);
}

GListStore *
teleport_peer_get_files (TeleportPeer *self)
{
  return self->files;
}
