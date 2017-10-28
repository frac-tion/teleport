/* teleport-get.h
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

#ifndef __TELEPORT_GET_H
#define __TELEPORT_GET_H

#include <libsoup/soup.h>

int teleport_get_do_downloading   (const gchar *,
                                   const gchar *,
                                   const gchar *);
int teleport_get_do_client_notify (const gchar *);

#endif /* __TELEPORT_GET_H */
