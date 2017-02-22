#include <gtk/gtk.h>

#include "teleportapp.h"
#include "teleportappwin.h"

struct _TeleportAppWindow
{
  GtkApplicationWindow parent;
};

typedef struct _TeleportAppWindowPrivate TeleportAppWindowPrivate;

struct _TeleportAppWindowPrivate
{
  GSettings *settings;
  GtkWidget *gears;
};

G_DEFINE_TYPE_WITH_PRIVATE(TeleportAppWindow, teleport_app_window, GTK_TYPE_APPLICATION_WINDOW);

  static void
teleport_app_window_init (TeleportAppWindow *win)
{
  TeleportAppWindowPrivate *priv;
  GtkBuilder *builder;
  GMenuModel *menu;
  //GAction *action;

  priv = teleport_app_window_get_instance_private (win);
  gtk_widget_init_template (GTK_WIDGET (win));

  builder = gtk_builder_new_from_resource ("/org/gtk/teleportapp/settings.ui");
  menu = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (priv->gears), menu);

  //action = g_settings_create_action (priv->settings, "show-words");
  //g_action_map_add_action (G_ACTION_MAP (win), action);
  /*
     const gchar *authors[] = {
     "Julian Sparber <julian@sparber.net>",
     NULL
     };

     gtk_show_about_dialog (parent,
     "authors", authors,
     "comments", "",
     "copyright", "Copyright \xc2\xa9 2017 Julian Sparber",
     "license-type", GTK_LICENSE_AGPL_3_0,
     "logo-icon-name", "",
     "program-name", "",
     "translator-credits", "translator-credits",
     "version", "20",
     "website", "https://wiki.gnome.org/Apps/teleport",
     "website-label", "Teleport website",
     NULL);
     */

  //g_object_unref (action);
  //g_object_unref (builder);
}

  static void
teleport_app_window_dispose (GObject *object)
{
  TeleportAppWindow *win;
  TeleportAppWindowPrivate *priv;

  win = TELEPORT_APP_WINDOW (object);
  priv = teleport_app_window_get_instance_private (win);

  //g_clear_object (&priv->settings);

  G_OBJECT_CLASS (teleport_app_window_parent_class)->dispose (object);
}

  static void
teleport_app_window_class_init (TeleportAppWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = teleport_app_window_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
      "/org/gtk/teleportapp/window.ui");

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportAppWindow, gears);
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
  TeleportAppWindowPrivate *priv;
  priv = teleport_app_window_get_instance_private (win);
}
