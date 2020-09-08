/* teleport-file.h
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

#ifndef __TELEPORT_FILE_H
#define __TELEPORT_FILE_H

#include <gtk/gtk.h>

#include "teleport-peer.h"

#define TELEPORT_TYPE_FILE teleport_file_get_type ()
G_DECLARE_FINAL_TYPE (TeleportFile, teleport_file, TELEPORT, FILE, GObject)

TeleportFile *teleport_file_new (const gchar *source_path,
                                 const gchar *destination_path,
                                 gint64 size);
TeleportFile *teleport_file_new_from_serialized (const gchar *data);

gchar *teleport_file_serialize (TeleportFile *self);
const gchar *teleport_file_get_id (TeleportFile *self);
const gchar *teleport_file_get_destination_path (TeleportFile *self);
void teleport_file_set_destination_path (TeleportFile *self,
                                         const gchar *destination_path);
const gchar* teleport_file_get_source_path (TeleportFile *self);
void teleport_file_set_source_path (TeleportFile *self,
                                    const gchar *source_path);
void teleport_file_set_size (TeleportFile *self,
                             gint64 size);
gint64 teleport_file_get_size (TeleportFile *self);
gfloat teleport_file_get_progress (TeleportFile *self);
void teleport_file_set_progress (TeleportFile *self,
                                 gfloat progress);

void teleport_file_download (TeleportFile *file,
                             gchar *download_directory);
void teleport_file_send (TeleportFile *file,
                         TeleportPeer *destination);



#endif /* __TELEPORT_FILE_H */
