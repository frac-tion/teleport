/* teleport-remote-device.c
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

#ifndef TELEPORT_REMOTE_DEVICE_H
#define TELEPORT_REMOTE_DEVICE_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "teleport-peer.h"

G_BEGIN_DECLS

#define TELEPORT_TYPE_REMOTE_DEVICE (teleport_remote_device_get_type())

G_DECLARE_FINAL_TYPE (TeleportRemoteDevice, teleport_remote_device, TELEPORT, REMOTE_DEVICE, GtkFrame)

GtkWidget *     teleport_remote_device_new      (Peer         *         );

Peer *  teleport_remote_device_get_peer (GtkWidget            *         );
void    teleport_remote_device_set_peer (TeleportRemoteDevice *, Peer  *);
void    teleport_remote_device_destroy  (TeleportRemoteDevice *);


G_END_DECLS

#endif /* TELEPORT_REMOTE_DEVICE_H */
