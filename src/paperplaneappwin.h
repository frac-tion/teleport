#ifndef __PAPERPLANEAPPWIN_H
#define __PAPERPLANEAPPWIN_H

#include <gtk/gtk.h>
#include "paperplaneapp.h"


#define PAPERPLANE_APP_WINDOW_TYPE (paperplane_app_window_get_type ())
G_DECLARE_FINAL_TYPE (PaperplaneAppWindow, paperplane_app_window, PAPERPLANE, APP_WINDOW, GtkApplicationWindow)

  PaperplaneAppWindow       *paperplane_app_window_new          (PaperplaneApp *app);
  void                    paperplane_app_window_open         (PaperplaneAppWindow *win,
      GFile            *file);


#endif /* __PAPERPLANEAPPWIN_H */
