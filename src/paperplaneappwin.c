#include <gtk/gtk.h>

#include "paperplaneapp.h"
#include "paperplaneappwin.h"

struct _PaperplaneAppWindow
{
  GtkApplicationWindow parent;
};

G_DEFINE_TYPE(PaperplaneAppWindow, paperplane_app_window, GTK_TYPE_APPLICATION_WINDOW);

  static void
paperplane_app_window_init (PaperplaneAppWindow *win)
{
  gtk_widget_init_template (GTK_WIDGET (win));
}

  static void
paperplane_app_window_class_init (PaperplaneAppWindowClass *class)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
      "/org/gtk/paperplaneapp/window.ui");
}

  PaperplaneAppWindow *
paperplane_app_window_new (PaperplaneApp *app)
{
  return g_object_new (PAPERPLANE_APP_WINDOW_TYPE, "application", app, NULL);
}

  void
paperplane_app_window_open (PaperplaneAppWindow *win,
    GFile            *file)
{
}
