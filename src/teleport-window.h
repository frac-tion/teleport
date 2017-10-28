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

#endif /* __TELEPORT_WINDOW_H */
