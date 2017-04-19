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
  GtkWidget *remote_devices_list;
};

G_DEFINE_TYPE_WITH_PRIVATE(TeleportAppWindow, teleport_app_window, GTK_TYPE_APPLICATION_WINDOW);

  static void
teleport_app_window_init (TeleportAppWindow *win)
{
  TeleportAppWindowPrivate *priv;
  GtkBuilder *builder;
  GtkBuilder *builder_remote_list;
  GtkWidget *menu;
  GtkWidget *remote_list_row;
  GtkLabel *remote_name;
  GtkWidget *line;
  GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  //GAction *action;

  priv = teleport_app_window_get_instance_private (win);
  gtk_widget_init_template (GTK_WIDGET (win));

  builder = gtk_builder_new_from_resource ("/org/gtk/teleportapp/settings.ui");
  menu = GTK_WIDGET (gtk_builder_get_object (builder, "settings"));

  gtk_menu_button_set_popover(GTK_MENU_BUTTON (priv->gears), menu);

  builder_remote_list = gtk_builder_new_from_resource ("/org/gtk/teleportapp/remote_list.ui");

  remote_list_row = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_device_row"));
  remote_name = GTK_LABEL (gtk_builder_get_object (builder_remote_list, "device_name"));
  gtk_label_set_text(remote_name, "Tobias's Laptop");
  gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), remote_list_row, -1);


  line = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_space_row"));
  gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), line, -1);

builder_remote_list = gtk_builder_new_from_resource ("/org/gtk/teleportapp/remote_list.ui");

  remote_list_row = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_device_row"));
  remote_name = GTK_LABEL (gtk_builder_get_object (builder_remote_list, "device_name"));
  gtk_label_set_text(remote_name, "Tobias's Laptop");
  gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), remote_list_row, -1);


  g_object_unref (builder);
  g_object_unref (menu);
  g_object_unref (remote_list_row);
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
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportAppWindow, remote_devices_list);

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
