/* teleport-remote-device.c
 *
 * Copyright 2020 Julian Sparber <julian@sparber.net>
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
#define HANDY_USE_UNSTABLE_API
#include <handy.h>
#include "teleport-remote-device.h"
#include "teleport-app.h"
#include "teleport-peer.h"
#include "teleport-file.h"
#include "teleport-file-row.h"

enum                                                                            
{                                                                               
  PROP_0,                                                                       
  PROP_PEER,                                                                    
  LAST_PROP                                                                     
}; 

static GtkTargetEntry entries[] = {
    { "text/uri-list", GTK_TARGET_OTHER_APP, 0 }
};

struct _TeleportRemoteDevice
{
  GtkListBoxRow  parent;

  GtkWidget     *name_label;
  GtkWidget 	*send_btn;
  GtkWidget     *files_revealer;
  GtkWidget     *files;

  /* data */
  TeleportPeer  *peer;
};

G_DEFINE_TYPE (TeleportRemoteDevice, teleport_remote_device, GTK_TYPE_LIST_BOX_ROW)

static void
number_of_files_changed_cb (GListModel *list,
               guint       position,
               guint       removed,
               guint       added,
               TeleportRemoteDevice *self)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->files_revealer),
                                 g_list_model_get_item (list, 0) != NULL);
}

static void
teleport_remote_device_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  TeleportRemoteDevice *self = TELEPORT_REMOTE_DEVICE (object);

  switch (prop_id)
    {
    case PROP_PEER:
      g_value_set_pointer (value, self->peer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
teleport_remote_device_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  TeleportRemoteDevice *self = TELEPORT_REMOTE_DEVICE (object);

  switch (prop_id)
    {
    case PROP_PEER:
      teleport_remote_device_set_peer (self, g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
teleport_remote_device_finalize (GObject *object)
{
  TeleportRemoteDevice *self = TELEPORT_REMOTE_DEVICE (object);

  g_object_unref (self->peer);

  G_OBJECT_CLASS (teleport_remote_device_parent_class)->finalize (object);
}

static void
teleport_remote_device_class_init (TeleportRemoteDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = teleport_remote_device_get_property;
  object_class->set_property = teleport_remote_device_set_property;
  object_class->finalize = teleport_remote_device_finalize;

  /**
   * TeleportRemoteDevice::peer:
   *
   * The peer that this row represents, or %NULL.
   */
  g_object_class_install_property (object_class,
                                   PROP_PEER,
                                   g_param_spec_pointer ("peer",
                                                         "Peer of the row",
                                                         "The peer that this row represents",
                                                         G_PARAM_READWRITE));

  gtk_widget_class_set_template_from_resource (widget_class, "/com/frac_tion/teleport/remote_list.ui");
  gtk_widget_class_bind_template_child (widget_class, TeleportRemoteDevice, name_label);
  gtk_widget_class_bind_template_child (widget_class, TeleportRemoteDevice, send_btn);
  gtk_widget_class_bind_template_child (widget_class, TeleportRemoteDevice, files_revealer);
  gtk_widget_class_bind_template_child (widget_class, TeleportRemoteDevice, files);
}

static void
send_file_to_device (TeleportPeer *device,
                     GFile *source_file)
{
  g_autoptr (GFileInfo) file_info = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar* source_path = NULL;
  TeleportFile *file;

  file_info = g_file_query_info (source_file,
                                 "standard::display-name,standard::size",
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL,
                                 &error);
  if (error != NULL)
    g_warning ("Couldn't query file info: %s", error->message);

  source_path = g_file_get_path (source_file);
  file = teleport_file_new (source_path,
                            g_file_info_get_display_name (file_info),
                            g_file_info_get_size (file_info));
  teleport_app_send_file (TELEPORT_APP (g_application_get_default ()), file, device);
}

static void
open_file_picker (TeleportRemoteDevice *self)
{
  g_autoptr (GtkFileChooserNative) dialog = NULL;
  g_autoptr (GFile) source_file = NULL;
  GtkWidget *window;

  window = gtk_widget_get_toplevel (GTK_WIDGET (self));
  if (gtk_widget_is_toplevel (window)) {
    dialog =  gtk_file_chooser_native_new ("Select file",
                                           GTK_WINDOW (window),
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           ("_Open"),
                                           ("_Cancel"));

    if (gtk_native_dialog_run (GTK_NATIVE_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
      source_file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      send_file_to_device (teleport_remote_device_get_peer (self), source_file);
    }
  }
}

static void
drag_data_received_handle (TeleportRemoteDevice *self,
                           GdkDragContext *context,
                           gint x,
                           gint y,
                           GtkSelectionData *selection_data,
                           guint target_type,
                           guint time,
                           gpointer user_data)
{
  g_autoptr (GFile) file = NULL;
  gboolean success = FALSE;
  gchar   **uris;

  /* TODO: allow sending multiple files */
  uris = gtk_selection_data_get_uris (selection_data);
  if (uris != NULL && uris[0] != NULL) {
    file = g_file_new_for_uri (uris[0]);
    send_file_to_device (teleport_remote_device_get_peer (self), file);
    g_strfreev (uris);
    success = TRUE;
  }

  gtk_drag_finish (context, success, FALSE, time);
}

static gboolean
drag_drop_handle (GtkWidget *widget,
                  GdkDragContext *context,
                  gint x,
                  gint y,
                  guint time,
                  gpointer user_data)
{
  gtk_drag_get_data (widget, context, gdk_atom_intern_static_string ("text/uri-list"), time);

  return  TRUE;
}

static void
teleport_remote_device_init (TeleportRemoteDevice *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));


  gtk_drag_dest_set (GTK_WIDGET (self),
                     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
                     entries,
                     1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (self->send_btn,
                            "clicked",
                            G_CALLBACK (open_file_picker),
                            self);

  g_signal_connect (self, "drag-data-received",
                    G_CALLBACK(drag_data_received_handle),
                    NULL);

  g_signal_connect (self, "drag-drop",
                    G_CALLBACK (drag_drop_handle),
                    NULL);
}

GtkWidget*
teleport_remote_device_new (TeleportPeer *peer)
{
  return g_object_new (TELEPORT_TYPE_REMOTE_DEVICE,
                       "peer", peer,
                       NULL);
}

/**
 * teleport_remote_device_get_peer:
 * @self: a #TeleportRemoteDevice
 *
 * Retrieves the #Peer that @row manages, or %NULL if none
 * is set.
 *
 * Returns: (transfer none): the internal peer of @row
 */
TeleportPeer *
teleport_remote_device_get_peer (TeleportRemoteDevice *self)
{
  g_return_val_if_fail (TELEPORT_IS_REMOTE_DEVICE (self), NULL);

  return self->peer;
}

/**
 * teleport_remote_device_get_peer:
 * @self: a #TeleportRemoteDevice
 *
 * Sets the peer for a row
 *
 */
void
teleport_remote_device_set_peer (TeleportRemoteDevice *self,
                                 TeleportPeer         *peer)
{
  GListModel *list;
  g_return_if_fail (TELEPORT_IS_REMOTE_DEVICE (self));

  if (peer == self->peer)
    return;

  g_clear_object (&self->peer);

  self->peer = g_object_ref (peer);

  g_object_bind_property (peer,
                          "name",
                          self->name_label,
                          "label",
                          G_BINDING_SYNC_CREATE);

  list = G_LIST_MODEL (teleport_peer_get_files (self->peer));
  gtk_list_box_bind_model (GTK_LIST_BOX (self->files),
                           list,
                           (GtkListBoxCreateWidgetFunc) teleport_file_row_new,
                           NULL,
                           NULL);

  g_signal_connect (list,
                            "items-changed",
                            G_CALLBACK (number_of_files_changed_cb),
                            self);
  number_of_files_changed_cb (list, 0, 0, 0, self);
}
