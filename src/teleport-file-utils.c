/* teleport-file-utils.c
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

#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include "teleport-file-utils.h"


/*  This function was copied from nautilus
 *  The code is licensed under the GPLv2-or-later with Copyright (C) 1999, 2000 Eazel, Inc.
 *  https://gitlab.gnome.org/GNOME/nautilus/-/blob/
 *  43b356b03b62e3611472654be5ffc8928ee98915/eel/eel-vfs-extensions.c
 */
static char *
eel_filename_get_extension_offset (const char *filename)
{
    char *end, *end2; 
    const char *start;
    
    if (filename == NULL || filename[0] == '\0')
    {
        return NULL;
    }
    
    /* basename must have at least one char */
    start = filename + 1;
    
    end = strrchr (start, '.');
    if (end == NULL || end[1] == '\0')
    {
        return NULL;
    }
    
    if (end != start)
    {                                   
        if (strcmp (end, ".gz") == 0 || 
            strcmp (end, ".bz2") == 0 ||
            strcmp (end, ".sit") == 0 ||
            strcmp (end, ".Z") == 0 || 
            strcmp (end, ".bz") == 0 ||
            strcmp (end, ".xz") == 0)
        {
            end2 = end - 1;
            while (end2 > start &&
                   *end2 != '.')
            {
                end2--;
            }
            if (end2 != start)
            {
                end = end2;
            }
        }
    }
    
    return end;
}

gchar *
teleport_file_utils_get_incremented_name (const gchar *filename)
{
  g_autofree gchar *basename = NULL;
  gchar *str;
  gchar *extension;
  gint count = 1;

  extension = eel_filename_get_extension_offset (filename);
  if (extension && extension != filename)
    basename = g_strndup (filename, extension - filename);
  else
    basename = g_strdup (filename);

  str = g_strrstr (basename, " (");
  if (str && sscanf (str, " (%d)", &count) == 1) {
    count++;
    /* Remove the previous count from the end of the filename */
    *str = '\0';
  }

  if (extension && extension != filename)
    return g_strdup_printf ("%s (%d)%s", basename, count, extension);
  else
    return g_strdup_printf ("%s (%d)", basename, count);
}
