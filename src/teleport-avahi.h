/* teleport-avahi.h
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

#ifndef __TELEPORT_AVAHI_H
#define __TELEPORT_AVAHI_H

#include <gtk/gtk.h>

#define TELEPORT_TYPE_AVAHI teleport_avahi_get_type ()
G_DECLARE_FINAL_TYPE (TeleportAvahi, teleport_avahi, TELEPORT, AVAHI, GObject)

typedef enum {
  TELEPORT_AVAHI_STATE_UNKNOWN = -1,
  TELEPORT_AVAHI_STATE_NEW = 0,
  TELEPORT_AVAHI_STATE_RUNNING,
  TELEPORT_AVAHI_STATE_NAME_COLLISION,
  TELEPORT_AVAHI_STATE_NO_DEAMON,
  TELEPORT_AVAHI_STATE_ERROR,
} TeleportAvahiState;

TeleportAvahi* teleport_avahi_new (const gchar *name, guint16 port);

void teleport_avahi_start (TeleportAvahi *self);
void teleport_avahi_set_name (TeleportAvahi *self, const gchar *name);

#endif /* __TELEPORT_AVAHI_H */
