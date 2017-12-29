/* teleport-window.h
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

#ifndef __TELEPORT_WINDOW_H
#define __TELEPORT_WINDOW_H

#include <gtk/gtk.h>
#include "teleport-app.h"
#include "teleport-peer.h"

#define TELEPORT_WINDOW_TYPE (teleport_window_get_type ())
G_DECLARE_FINAL_TYPE (TeleportWindow, 
                      teleport_window, 
                      TELEPORT, 
                      WINDOW, 
                      GtkApplicationWindow)

TeleportWindow     *teleport_window_new             (TeleportApp       *);
void               teleport_window_open             (TeleportWindow    *,
                                                     GFile             *);
void               update_remote_device_list        (TeleportWindow    *,
                                                     Peer      *);
void               update_remote_device_list_remove (TeleportWindow    *,
                                                     Peer      *);

gchar *            teleport_get_download_directory  (void); 
gchar *            teleport_get_device_name         (void); 
void               teleport_show_no_device_message  (TeleportWindow *,
                                                     gboolean);
void               teleport_show_no_avahi_message   (TeleportWindow *,
                                                     gboolean);

#endif /* __TELEPORT_WINDOW_H */
