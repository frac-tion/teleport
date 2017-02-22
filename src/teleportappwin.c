#include <gtk/gtk.h>

#include "teleportapp.h"
#include "teleportappwin.h"

struct _TeleportAppWindow
{
  GtkApplicationWindow parent;
};

G_DEFINE_TYPE(TeleportAppWindow, teleport_app_window, GTK_TYPE_APPLICATION_WINDOW);

  static void
teleport_app_window_init (TeleportAppWindow *win)
{
  gtk_widget_init_template (GTK_WIDGET (win));
  GtkBuilder *builder;
  GMenuModel *menu;
  GAction *action;

  gtk_widget_init_template (GTK_WIDGET (win));

  builder = gtk_builder_new_from_resource ("/org/gtk/teleportapp/settings.ui");
  menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
  g_object_unref (builder);

}

  static void
teleport_app_window_class_init (TeleportAppWindowClass *class)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
      "/org/gtk/teleportapp/window.ui");
}

  TeleportAppWindow *
teleport_app_window_new (TeleportApp *app)
{
  return g_object_new (TELEPORT_APP_WINDOW_TYPE, "application", app, NULL);
}

  void
teleport_app_window_open (TeleportAppWindow *win,
    GFile            *file)
{
}
