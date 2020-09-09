/* teleport-file-row.c
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

#ifndef TELEPORT_FILE_ROW_H
#define TELEPORT_FILE_ROW_H

#include <gtk/gtk.h>
#include "teleport-file.h"

G_BEGIN_DECLS

#define TELEPORT_TYPE_FILE_ROW (teleport_file_row_get_type())
G_DECLARE_FINAL_TYPE (TeleportFileRow, teleport_file_row, TELEPORT, FILE_ROW, GtkListBoxRow)

TeleportFileRow *teleport_file_row_new      (TeleportFile    *file);
TeleportFile    *teleport_file_row_get_file (TeleportFileRow *self);
void             teleport_file_row_set_file (TeleportFileRow *self,
                                             TeleportFile    *file);
G_END_DECLS

#endif /* TELEPORT_FILE_ROW_H */
