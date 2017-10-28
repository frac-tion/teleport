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

enum {
  ADD, REMOVE, N_SIGNALS
};

static gint signalIds [N_SIGNALS];

struct _TeleportPeer
{
  GObject parent;
  GArray *list;
  /* instance members */
};

G_DEFINE_TYPE (TeleportPeer, teleport_peer, G_TYPE_OBJECT)

static void
teleport_peer_constructed (GObject *obj)
{
  G_OBJECT_CLASS (teleport_peer_parent_class)->constructed (obj);
}

static void
teleport_peer_class_init (TeleportPeerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = teleport_peer_constructed;
  signalIds[ADD] = g_signal_new ("addpeer",
                                 G_TYPE_OBJECT,
                                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                 0,
                                 NULL /* accumulator */,
                                 NULL /* accumulator data */,
                                 NULL /* C marshaller */,
                                 G_TYPE_NONE /* return_type */,
                                 1,
                                 G_TYPE_POINTER);

  signalIds[REMOVE] = g_signal_new ("removepeer",
                                    G_TYPE_OBJECT,
                                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                    0,
                                    NULL /* accumulator */,
                                    NULL /* accumulator data */,
                                    NULL /* C marshaller */,
                                    G_TYPE_NONE /* return_type */,
                                    1,
                                    G_TYPE_POINTER);

}

static void
teleport_peer_init (TeleportPeer *self)
{
  self->list = g_array_new (FALSE, FALSE, sizeof(Peer *));
}

gchar *teleport_peer_get_name (TeleportPeer *self, gint index, GError **error)
{
  Peer *element;
  //g_return_if_fail (TELEPORT_IS_PEER (self));
  //g_return_if_fail (error == NULL || *error == NULL);
  if (index > self->list->len-1)
    return NULL;
  element = g_array_index(self->list, Peer *, index);
  return element->name;
}

gchar *teleport_peer_get_ip (TeleportPeer *self, gint index, GError **error)
{
  //g_return_if_fail (TELEPORT_IS_PEER (self));
  //g_return_if_fail (error == NULL || *error == NULL);
  Peer *element = g_array_index(self->list, Peer *, index);
  if (index > self->list->len-1)
    return NULL;
  return element->ip;
}
gint teleport_peer_get_port (TeleportPeer *self, gint index, GError **error)
{
  Peer *element = g_array_index(self->list, Peer*, index);
  if (index > self->list->len-1)
    return 0;
  return element->port;
}

void teleport_peer_add_peer (TeleportPeer *self, gchar *name, gchar *ip, gint port)
{
  Peer *new = g_new(Peer, 1);
  new->ip = ip;
  new->port = port;
  new->name = name;
  g_array_append_val(self->list, new);

  g_signal_emit (self, signalIds[ADD], 0, new);
}

void teleport_peer_remove_peer (TeleportPeer *self, Peer *device)
{
  Peer *element;
  gboolean found = FALSE;
  //Maybe I could just compare the addresses
  for (int i = 0; i < self->list->len && !found; i++) {
    element = g_array_index(self->list, Peer *, i);
    if (g_strcmp0(element->name, device->name) == 0) {
      found = TRUE;
      g_array_remove_index(self->list, i);
    }
  }
  g_signal_emit (self, signalIds[REMOVE], 0, device);
}

void
teleport_peer_remove_peer_by_name (TeleportPeer *self, const gchar *name)
{
  Peer *element = NULL;
  gboolean found = FALSE;
  g_print("Remove this device %s", name);
  for (int i = 0; i < self->list->len && !found; i++) {
    element = g_array_index(self->list, Peer *, i);
    if (g_strcmp0(element->name, name) == 0) {
      found = TRUE;
      g_array_remove_index(self->list, i);
    }
  }
  g_signal_emit (self, signalIds[REMOVE], 0, element);
}

gchar *
teleport_peer_get_name_by_addr (TeleportPeer *self, const gchar *addr)
{
  Peer *element = NULL;
  gchar *name = NULL;
  gboolean found = FALSE;
  for (int i = 0; i < self->list->len && !found; i++) {
    element = g_array_index(self->list, Peer *, i);
    if (g_strcmp0(element->ip, addr) == 0) {
      found = TRUE;
      name = element->name;
    }
  }
  return name;
}
int 
teleport_peer_get_number (TeleportPeer *self)
{
  return (self->list->len);
}
