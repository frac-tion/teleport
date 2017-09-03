#include <gtk/gtk.h>

#include "teleportapp.h"
#include "teleportappwin.h"
#include "server.h"


GtkWidget* find_child(GtkWidget* , const gchar* );
TeleportAppWindow* mainWin;

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
  GtkWidget *menu;
  mainWin = win;

  priv = teleport_app_window_get_instance_private (win);

  gtk_widget_init_template (GTK_WIDGET (win));

  builder = gtk_builder_new_from_resource ("/org/gtk/teleportapp/settings.ui");
  menu = GTK_WIDGET (gtk_builder_get_object (builder, "settings"));

  gtk_menu_button_set_popover(GTK_MENU_BUTTON (priv->gears), menu);

  g_object_unref (builder);
}

static void
open_file_picker(GtkButton * btn, GString* deviceName) {
  GtkWidget *dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  gint res;
  g_print("Open file chooser for submitting a file to %s\n", deviceName);

  dialog = gtk_file_chooser_dialog_new ("Open File",
      GTK_WINDOW(mainWin),
      action,
      ("_Cancel"),
      GTK_RESPONSE_CANCEL,
      ("_Open"),
      GTK_RESPONSE_ACCEPT,
      NULL);

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
    filename = gtk_file_chooser_get_filename (chooser);
    g_print("Choosen file is %s\n", filename);
    addRouteToServer(g_compute_checksum_for_string (G_CHECKSUM_SHA256, filename,  -1), filename, "localhost");
    //open_file (filename);
    g_free (filename);
  }

  gtk_widget_destroy (dialog);
}

void update_remote_device_list(TeleportAppWindow *win, char * name) {
  TeleportAppWindowPrivate *priv;
  GtkBuilder *builder_remote_list;
  GtkWidget *row;
  GtkLabel *name_label;
  GtkButton *send_btn;
  //GtkWidget *line;

  priv = teleport_app_window_get_instance_private (win);

  builder_remote_list = gtk_builder_new_from_resource ("/org/gtk/teleportapp/remote_list.ui");

  row = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_device_row"));
  name_label = GTK_LABEL (gtk_builder_get_object (builder_remote_list, "device_name"));
  gtk_label_set_text(name_label, name);
  gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), row, -1);
  send_btn = GTK_BUTTON (gtk_builder_get_object (builder_remote_list, "send_btn"));
  g_signal_connect (send_btn, "clicked", G_CALLBACK (open_file_picker), name);

  //line = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_space_row"));
  //gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), line, -1);
  g_object_unref (builder_remote_list);
}


void update_remote_device_list_remove(TeleportAppWindow *win, char * name) {
  TeleportAppWindowPrivate *priv;
  GtkWidget *box;
  GtkListBoxRow *remote_row;
  GtkLabel *name_label;
  GtkWidget *line;
  gint i = 0;

  priv = teleport_app_window_get_instance_private (win);
  box = priv->remote_devices_list;

  remote_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), i);

  while(remote_row != NULL) {
    name_label = GTK_LABEL(find_child(GTK_WIDGET(remote_row), "GtkLabel"));
    if (name_label != NULL && g_strcmp0(name, gtk_label_get_text(name_label)) == 0) {
      gtk_container_remove (GTK_CONTAINER(box), GTK_WIDGET(remote_row));
    }
    i++;
    remote_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), i);
  }
}

  GtkWidget*
find_child(GtkWidget* parent, const gchar* name)
{
  if (g_strcmp0(gtk_widget_get_name((GtkWidget*)parent), (gchar*)name) == 0) {
    return parent;
  }

  if (GTK_IS_BIN(parent)) {
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
    return find_child(child, name);
  }

  if (GTK_IS_CONTAINER(parent)) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
    while ((children = g_list_next(children)) != NULL) {
      GtkWidget* widget = find_child(children->data, name);
      if (widget != NULL) {
        return widget;
      }
    }
  }

  return NULL;
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
