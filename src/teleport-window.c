/* teleport-window.c
 *
 * Copyright 2017 Julian Sparber <julian@sparber.com>
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

#include <gtk/gtk.h>

#include "teleport-app.h"
#include "teleport-window.h"
#include "teleport-server.h"
#include "teleport-peer.h"
#include "teleport-remote-device.h"

TeleportWindow *mainWin;

struct _TeleportWindow
{
  GtkApplicationWindow parent;
};

typedef struct _TeleportWindowPrivate TeleportWindowPrivate;

struct _TeleportWindowPrivate
{
  GSettings *settings;
  GtkWidget *gears;
  GtkWidget *this_device_settings_button;
  GtkWidget *remote_devices_box;
  GtkWidget *this_device_name_label;
  GtkWidget *remote_no_devices;
  GtkWidget *remote_no_avahi;
  GtkWidget *this_device_settings_entry;
};

G_DEFINE_TYPE_WITH_PRIVATE(TeleportWindow, teleport_window, GTK_TYPE_APPLICATION_WINDOW);

static void
change_download_directory_cb (GtkWidget *widget,
                              gpointer user_data) {
  GSettings *settings;
  gchar * newDownloadDir;
  settings = (GSettings *)user_data;

  newDownloadDir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  g_print ("Change download directory\n");
  g_settings_set_string (settings,
                         "download-dir",
                         newDownloadDir);
  g_free(newDownloadDir);
}

static void
on_click_this_device_settings_button (GtkWidget *widget,
                                      gpointer user_data) {
  TeleportWindowPrivate *priv = (TeleportWindowPrivate *) user_data;
  g_settings_set_string (priv->settings,
                         "device-name",
                         gtk_entry_get_text (GTK_ENTRY (priv->this_device_settings_entry)));
}

static void
teleport_window_init (TeleportWindow *win)
{
  TeleportWindowPrivate *priv;
  GtkBuilder *builder;
  GtkWidget *menu;
  GtkFileChooserButton *downloadDir;
  mainWin = win;

  priv = teleport_window_get_instance_private (win);
  priv->settings = g_settings_new ("com.frac_tion.teleport");

  if (g_settings_get_user_value (priv->settings, "download-dir") == NULL) {
    g_print ("Download dir set to XDG DOWNLOAD directory\n");
    if (g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD) != NULL) {
      g_settings_set_string (priv->settings,
                             "download-dir",
                             g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
    }
    else {
      g_print ("Error: XDG DOWNLOAD is not set.\n");
    }
  }

  if (g_settings_get_user_value (priv->settings, "device-name") == NULL) {
    g_settings_set_string (priv->settings,
                           "device-name",
                           g_get_host_name());
  }

  gtk_widget_init_template (GTK_WIDGET (win));

  builder = gtk_builder_new_from_resource ("/com/frac_tion/teleport/settings.ui");
  menu = GTK_WIDGET (gtk_builder_get_object (builder, "settings"));
  downloadDir = GTK_FILE_CHOOSER_BUTTON (gtk_builder_get_object (builder, "settings_download_directory"));

  gtk_menu_button_set_popover(GTK_MENU_BUTTON (priv->gears), menu);


  g_settings_bind (priv->settings, "device-name",
                   priv->this_device_name_label, "label",
                   G_SETTINGS_BIND_DEFAULT);

  /*gtk_label_set_text (GTK_LABEL (priv->this_device_name_label),
    g_settings_get_string (priv->settings, "device-name"));
    */

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (downloadDir),
                                       g_settings_get_string(priv->settings,
                                                             "download-dir"));

  g_signal_connect (downloadDir, "file-set", G_CALLBACK (change_download_directory_cb), priv->settings);
  /*g_settings_bind (priv->settings, "download-dir",
    GTK_FILE_CHOOSER (downloadDir), "current-folder",
    G_SETTINGS_BIND_DEFAULT);
    */

  //g_object_unref (menu);
  //g_object_unref (label);
  g_object_unref (builder);

  /* Add popover for device settings */
  builder = gtk_builder_new_from_resource ("/com/frac_tion/teleport/device_settings.ui");
  menu = GTK_WIDGET (gtk_builder_get_object (builder, "device-settings"));
  gtk_menu_button_set_popover(GTK_MENU_BUTTON (priv->this_device_settings_button), menu);

  priv->this_device_settings_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                         "this_device_settings_entry"));
  g_settings_bind (priv->settings,
                   "device-name",
                   priv->this_device_settings_entry,
                   "text",
                   G_SETTINGS_BIND_GET);

  g_signal_connect (GTK_WIDGET (gtk_builder_get_object (builder, "this_device_settings_button")),
                    "clicked", G_CALLBACK (on_click_this_device_settings_button), priv);

  g_signal_connect (priv->this_device_settings_entry,
                    "activate", G_CALLBACK (on_click_this_device_settings_button), priv);

  g_object_unref (builder);
}


void
update_remote_device_list(TeleportWindow *win,
                          Peer           *device)
{
  TeleportWindowPrivate *priv;
  GtkWidget *remote_device;

  priv = teleport_window_get_instance_private (win);

  gtk_widget_hide (priv->remote_no_devices);

  remote_device = teleport_remote_device_new (device);

  gtk_box_pack_end (GTK_BOX (priv->remote_devices_box),
                    remote_device,
                    TRUE,
                    TRUE,
                    0);
}

static void
remove_remote_peer (GtkWidget *widget,
                    gpointer data)
{
  if (TELEPORT_IS_REMOTE_DEVICE (widget) && teleport_remote_device_get_peer(widget) == ((Peer *) data)) {
    gtk_widget_destroy (widget);
  }
}

void
update_remote_device_list_remove(TeleportWindow *win,
                                 Peer *device)
{
  TeleportWindowPrivate *priv;

  priv = teleport_window_get_instance_private (win);

  gtk_container_foreach (GTK_CONTAINER(priv->remote_devices_box),
                         remove_remote_peer,
                         device);
}

static void
teleport_window_dispose (GObject *object)
{
  TeleportWindow *win;
  TeleportWindowPrivate *priv;

  win = TELEPORT_WINDOW (object);
  priv = teleport_window_get_instance_private (win);

  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (teleport_window_parent_class)->dispose (object);
}

static void
teleport_window_class_init (TeleportWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = teleport_window_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/com/frac_tion/teleport/window.ui");

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, gears);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, this_device_settings_button);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, this_device_name_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, remote_no_devices);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, remote_devices_box);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, remote_no_avahi);
}

TeleportWindow *
teleport_window_new (TeleportApp *app)
{
  return g_object_new (TELEPORT_WINDOW_TYPE, "application", app, NULL);
}

gchar * 
teleport_get_device_name (void) 
{
  TeleportWindowPrivate *priv;
  priv = teleport_window_get_instance_private (mainWin);

  return g_settings_get_string (priv->settings, "device-name");
}

gchar * 
teleport_get_download_directory (void) 
{
  TeleportWindowPrivate *priv;
  priv = teleport_window_get_instance_private (mainWin);

  return g_settings_get_string (priv->settings, "download-dir");
}

void
teleport_show_no_device_message (TeleportWindow *self, gboolean show)
{
  TeleportWindowPrivate *priv;
  priv = teleport_window_get_instance_private (self);
  if (show)
    gtk_widget_show (priv->remote_no_devices);
  else
    gtk_widget_hide (priv->remote_no_devices);
}

void
teleport_show_no_avahi_message (TeleportWindow *self, gboolean show)
{
  TeleportWindowPrivate *priv;
  priv = teleport_window_get_instance_private (self);
  if (show)
    gtk_widget_show (priv->remote_no_avahi);
  else
    gtk_widget_hide (priv->remote_no_avahi);
}

void
teleport_window_open (TeleportWindow *win,
                      GFile *file)
{
  //TeleportWindowPrivate *priv;
  //priv = teleport_window_get_instance_private (win);
}
