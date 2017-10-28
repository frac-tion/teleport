/* teleport-peer.h
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

#ifndef __TELEPORT_PEER_H
#define __TELEPORT_PEER_H

#include <gtk/gtk.h>

#define TELEPORT_TYPE_PEER teleport_peer_get_type ()
G_DECLARE_FINAL_TYPE (TeleportPeer, teleport_peer, TELEPORT, PEER, GObject)

typedef struct Peers {
  char *name;
  char *ip;
  gint port;
} Peer;


gchar * teleport_peer_get_name (TeleportPeer  *self, gint index, GError **error);
gchar * teleport_peer_get_ip (TeleportPeer  *self, gint index, GError **error);
gint teleport_peer_get_port (TeleportPeer  *self, gint index, GError **error);
void teleport_peer_add_peer (TeleportPeer *self, gchar * name, gchar * ip, gint port);
void teleport_peer_remove_peer (TeleportPeer *, Peer *);
void teleport_peer_remove_peer_by_name (TeleportPeer *, const gchar *);
gchar * teleport_peer_get_name_by_addr (TeleportPeer *, const gchar *);
int  teleport_peer_get_number (TeleportPeer *self);

#endif /* __TELEPORT_PEER_H */
