#ifndef __TELEPORTAPPWIN_H
#define __TELEPORTAPPWIN_H

#include <gtk/gtk.h>
#include "teleportapp.h"


#define TELEPORT_APP_WINDOW_TYPE (teleport_app_window_get_type ())
G_DECLARE_FINAL_TYPE (TeleportAppWindow, teleport_app_window, TELEPORT, APP_WINDOW, GtkApplicationWindow)

  TeleportAppWindow       *teleport_app_window_new          (TeleportApp *app);
  void                    teleport_app_window_open         (TeleportAppWindow *win,
      GFile            *file);


#endif /* __TELEPORTAPPWIN_H */
