/* teleport-server.h
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

#ifndef __TELEPORT_SERVER_H
#define __TELEPORT_SERVER_H

#include <libsoup/soup.h>
#include "teleport-file.h"

#define TELEPORT_TYPE_SERVER teleport_server_get_type ()
G_DECLARE_FINAL_TYPE (TeleportServer, teleport_server, TELEPORT, SERVER, SoupServer)

TeleportServer *teleport_server_new (guint port);

void teleport_server_add_file (TeleportServer *self,
                               TeleportFile *file);
void teleport_server_add_peer (TeleportServer *self,
                               TeleportPeer *device);
void teleport_server_remove_peer (TeleportServer *self,
                                  TeleportPeer *device);

#endif /* __TELEPORT_SERVER_H */
