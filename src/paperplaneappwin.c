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
  GtkBuilder *builder;
  GMenuModel *menu;
  GAction *action;

  gtk_widget_init_template (GTK_WIDGET (win));

  builder = gtk_builder_new_from_resource ("/org/gtk/paperplaneapp/settings.ui");
  menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
  g_object_unref (builder);

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
