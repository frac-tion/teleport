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
#include <string.h>
#define HANDY_USE_UNSTABLE_API
#include <handy.h>

#include "teleport-app.h"
#include "teleport-window.h"
#include "teleport-server.h"
#include "teleport-peer.h"
#include "teleport-remote-device.h"

struct _TeleportWindow
{
  GtkApplicationWindow parent;

  GtkWidget *gears;
  GtkWidget *this_device_settings_button;
  GtkWidget *this_device_name_label;
  GtkWidget *this_device_settings_entry;
  GtkWidget *remote_devices_stack;
  GtkWidget *remote_devices;
};

G_DEFINE_TYPE (TeleportWindow, teleport_window, GTK_TYPE_APPLICATION_WINDOW)

static void
change_download_directory_cb (GtkWidget *widget,
                              GSettings *settings)
{
  g_autofree gchar *newDownloadDir;

  newDownloadDir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  g_print ("Change download directory\n");
  g_settings_set_string (settings,
                         "download-dir",
                         newDownloadDir);
}

static gboolean
valid_device_name (const gchar *name)
{
  if (strlen (name) == 0)
    return FALSE;
  return TRUE;
}

static void
on_new_device_name (TeleportWindow *self,
                    GtkWidget *entry)
{
  g_autofree gchar *text = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  g_strstrip (text);
  gtk_widget_set_sensitive (self->this_device_settings_button, valid_device_name (text));
  if (!valid_device_name (text)) {
    g_signal_stop_emission_by_name (entry, "insert-text");
  }
}

static void
update_download_directory (GSettings    *settings,
                           gchar        *key,
                           gpointer     *data)
{
  if (g_strcmp0 (key, "download-dir") == 0) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (data),
                                         g_settings_get_string(settings, key));
  }
}

static void
teleport_window_init (TeleportWindow *self)
{
  GtkBuilder *builder;
  GtkWidget *menu;
  GtkFileChooser *download_directory;
  GApplication *app = g_application_get_default ();
  GSettings *settings = teleport_app_get_settings (TELEPORT_APP (app));

  gtk_widget_init_template (GTK_WIDGET (self));

  builder = gtk_builder_new_from_resource ("/com/frac_tion/teleport/settings.ui");
  menu = GTK_WIDGET (gtk_builder_get_object (builder, "settings"));
  download_directory = GTK_FILE_CHOOSER (gtk_builder_get_object (builder, 
                                                                 "settings_download_directory"));

  gtk_menu_button_set_popover (GTK_MENU_BUTTON (self->gears), menu);

  g_settings_bind (settings, "device-name",
                   self->this_device_name_label, "label",
                   G_SETTINGS_BIND_GET);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (download_directory),
                                       g_settings_get_string(settings,
                                                             "download-dir"));

  g_signal_connect (download_directory, "file-set", G_CALLBACK (change_download_directory_cb), settings);
  g_signal_connect (settings, "changed", G_CALLBACK (update_download_directory), download_directory);

  g_object_unref (builder);

  /* Add popover for device settings */
  builder = gtk_builder_new_from_resource ("/com/frac_tion/teleport/device_settings.ui");
  menu = GTK_WIDGET (gtk_builder_get_object (builder, "device-settings"));
  gtk_menu_button_set_popover(GTK_MENU_BUTTON (self->this_device_settings_button), menu);

  self->this_device_settings_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                         "this_device_settings_entry"));
  g_settings_bind (settings,
                   "device-name",
                   self->this_device_settings_entry,
                   "text",
                   G_SETTINGS_BIND_DEFAULT);

  // TODO: implemt the button press
  /*
     g_signal_connect (GTK_WIDGET (gtk_builder_get_object (builder, "this_device_settings_button")),
     "clicked", G_CALLBACK (on_click_this_device_settings_button), self);
     */

  /*
     g_signal_connect (self->this_device_settings_entry,
     "activate", G_CALLBACK (on_click_this_device_settings_button), self);
     */

  g_signal_connect_swapped (G_OBJECT (self->this_device_settings_entry),
                            "insert-text",
                            G_CALLBACK (on_new_device_name),
                            self);
  g_object_unref (builder);
}

static void
teleport_window_dispose (GObject *object)
{
  G_OBJECT_CLASS (teleport_window_parent_class)->dispose (object);
}

static void
teleport_window_class_init (TeleportWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = teleport_window_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/com/frac_tion/teleport/window.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), TeleportWindow, gears);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), TeleportWindow, this_device_settings_button);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), TeleportWindow, this_device_name_label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), TeleportWindow, remote_devices);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), TeleportWindow, remote_devices_stack);
}

TeleportWindow *
teleport_window_new (TeleportApp *app)
{
  return g_object_new (TELEPORT_WINDOW_TYPE, "application", app, NULL);
}

void
teleport_window_bind_device_list (TeleportWindow *self,
                                  GListStore *list)
{
  g_return_if_fail (TELEPORT_IS_WINDOW (self));

  gtk_list_box_bind_model (GTK_LIST_BOX (self->remote_devices),
                           G_LIST_MODEL (list),
                           (GtkListBoxCreateWidgetFunc) teleport_remote_device_new,
                           NULL,
                           NULL);
}

void
teleport_window_set_view (TeleportWindow *self,
                          const gchar *view)
{
  g_return_if_fail (TELEPORT_IS_WINDOW (self));

  gtk_stack_set_visible_child_name (GTK_STACK (self->remote_devices_stack), view);
}
