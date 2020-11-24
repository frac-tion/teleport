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
  HdyApplicationWindow parent;

  GtkStack  *this_device_stack;
  GtkButton *this_device_submit_button;
  GtkWidget *this_device_name_label;
  GtkEntry  *this_device_name_entry;
  GtkWidget *remote_devices_stack;
  GtkWidget *remote_devices;
  GtkFileChooserButton *settings_download_directory;
};

G_DEFINE_TYPE (TeleportWindow, teleport_window, HDY_TYPE_APPLICATION_WINDOW)

static GSettings *
get_settings (TeleportWindow *self)
{
  TeleportApp *app = TELEPORT_APP (g_application_get_default());
  return teleport_app_get_settings (app);
}

static void
change_download_directory_cb (GtkWidget *widget,
                              GSettings *settings)
{
  g_autofree gchar *newDownloadDir;

  newDownloadDir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
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
edit_device_name_changed_cb (TeleportWindow *self)
{
  const gchar *name;
  name = gtk_entry_get_text (self->this_device_name_entry);
  gtk_widget_set_sensitive (GTK_WIDGET (self->this_device_submit_button),
                            valid_device_name (name));
}

static void
edit_clicked_cb (TeleportWindow *self,
                 GtkButton *button)
{
  g_autofree gchar *name = NULL;

  name = g_settings_get_string (get_settings (self),
                                "device-name");
  gtk_entry_set_text (self->this_device_name_entry, name);
  edit_device_name_changed_cb (self);
  gtk_stack_set_visible_child_name (self->this_device_stack, "edit");
}

static void
edit_submit_clicked_cb (TeleportWindow *self,
                        GtkButton *button)
{
  gtk_stack_set_visible_child_name (self->this_device_stack, "normal");
  g_settings_set_string (get_settings (self),
                         "device-name",
                         gtk_entry_get_text (self->this_device_name_entry));
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
teleport_window_constructed (GObject *object)
{
  TeleportWindow *self;
  GSettings *settings;

  G_OBJECT_CLASS (teleport_window_parent_class)->constructed (object);

  self = TELEPORT_WINDOW (object);
  settings = get_settings (self);

  g_settings_bind (settings,
                   "device-name",
                   self->this_device_name_label,
                   "label",
                   G_SETTINGS_BIND_GET);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (self->settings_download_directory),
                                       g_settings_get_string(settings, "download-dir"));

  g_signal_connect (self->settings_download_directory,
                    "file-set",
                    G_CALLBACK (change_download_directory_cb),
                    settings);

  g_signal_connect (settings,
                    "changed",
                    G_CALLBACK (update_download_directory),
                    self->settings_download_directory);
}

static void
teleport_window_init (TeleportWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
teleport_window_class_init (TeleportWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = teleport_window_constructed;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/frac_tion/teleport/window.ui");

  gtk_widget_class_bind_template_child (widget_class, TeleportWindow, this_device_name_label);
  gtk_widget_class_bind_template_child (widget_class, TeleportWindow, this_device_name_entry);
  gtk_widget_class_bind_template_child (widget_class, TeleportWindow, this_device_submit_button);
  gtk_widget_class_bind_template_child (widget_class, TeleportWindow, this_device_stack);
  gtk_widget_class_bind_template_child (widget_class, TeleportWindow, remote_devices);
  gtk_widget_class_bind_template_child (widget_class, TeleportWindow, remote_devices_stack);
  gtk_widget_class_bind_template_child (widget_class, TeleportWindow, settings_download_directory);
  gtk_widget_class_bind_template_callback (widget_class, edit_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, edit_submit_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, edit_device_name_changed_cb);
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
