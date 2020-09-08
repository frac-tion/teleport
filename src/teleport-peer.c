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
#include "teleport-peer.h"

struct _TeleportPeer
{
  GObject parent;

  gchar *name;
  gchar *ip;
  guint port;
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
