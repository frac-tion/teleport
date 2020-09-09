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
#include "teleport-file.h"

#define TELEPORT_TYPE_PEER teleport_peer_get_type ()
G_DECLARE_FINAL_TYPE (TeleportPeer, teleport_peer, TELEPORT, PEER, GObject)

TeleportPeer* teleport_peer_new (const gchar        *name,
                                 const gchar        *ip,
                                 guint                port);
const gchar* teleport_peer_get_name (TeleportPeer   *self);
void teleport_peer_set_name (TeleportPeer           *self,
                            const gchar             *name);
const gchar* teleport_peer_get_ip (TeleportPeer     *self);
void teleport_peer_set_ip (TeleportPeer             *self,
                             const gchar            *ip);
guint teleport_peer_get_port (TeleportPeer          *self);
void teleport_peer_set_port  (TeleportPeer          *self,
                              guint                 port);
gchar *teleport_peer_get_incoming_address (TeleportPeer *self);
void teleport_peer_add_file (TeleportPeer *self,
                             TeleportFile *file);
void teleport_peer_send_file (TeleportPeer *self,
                             TeleportFile *file);
GListStore *teleport_peer_get_files (TeleportPeer *self);

#endif /* __TELEPORT_PEER_H */
