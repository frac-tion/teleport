/* teleport-remote-device.c
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
#include "teleport-remote-device.h"
#include "teleport-peer.h"
#include "teleport-server.h"

enum {
  TARGET_INT32,
  TARGET_STRING,
  TARGET_URIS,
  TARGET_ROOTWIN
};

enum                                                                            
{                                                                               
  PROP_0,                                                                       
  PROP_PEER,                                                                    
  LAST_PROP                                                                     
}; 

/* datatype (string), restrictions on DnD (GtkTargetFlags), datatype (int) */
static GtkTargetEntry target_list[] = {
  //{ "INTEGER",    0, TARGET_INT32 },
  //{ "STRING",     0, TARGET_STRING },
  //{ "text/plain", 0, TARGET_STRING },
    { "text/uri-list", 0, TARGET_URIS }
  //{ "application/octet-stream", 0, TARGET_STRING },
  //{ "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = G_N_ELEMENTS (target_list);

struct _TeleportRemoteDevice
{
  GtkFrame      parent;
  GtkWidget 	*remote_device_row;
  GtkWidget 	*device_name;
  GtkWidget 	*send_btn;

  /* data */
  Peer          *peer;
};

G_DEFINE_TYPE (TeleportRemoteDevice, teleport_remote_device, GTK_TYPE_FRAME)

    /*
     * GObject overrides
     */

static void
teleport_remote_device_finalize (GObject *object)
{
  //TeleportRemoteDevice *self = TELEPORT_REMOTE_DEVICE (object);

  G_OBJECT_CLASS (teleport_remote_device_parent_class)->finalize (object);
}

static void
teleport_remote_device_dispose (GObject *object)
{
  G_OBJECT_CLASS (teleport_remote_device_parent_class)->dispose (object);
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
teleport_remote_device_class_init (TeleportRemoteDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = teleport_remote_device_dispose;
  object_class->finalize = teleport_remote_device_finalize;
  object_class->get_property = teleport_remote_device_get_property;
  object_class->set_property = teleport_remote_device_set_property;

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
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /*g_param_spec_object ("peer",
    "Task of the row",
    "The task that this row represents",
    (Peer *),
    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
    */

  gtk_widget_class_set_template_from_resource (widget_class, "/com/frac_tion/teleport/remote_list.ui");
  gtk_widget_class_bind_template_child (widget_class, TeleportRemoteDevice, remote_device_row);
  gtk_widget_class_bind_template_child (widget_class, TeleportRemoteDevice, device_name);
  gtk_widget_class_bind_template_child (widget_class, TeleportRemoteDevice, send_btn);
}

static void
open_file_picker(GtkButton *btn,
                 Peer *device) {
  GtkWidget *dialog;
  GtkWidget * window;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  gint res;
  g_print("Open file chooser for submitting a file to %s with Address %s\n", device->name, device->ip);

  window = gtk_widget_get_toplevel (GTK_WIDGET (btn));
  if (gtk_widget_is_toplevel (window))
    {
      dialog =  gtk_file_chooser_dialog_new ("Open File",
                                             GTK_WINDOW(window),
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
          gtk_widget_destroy (dialog);
          teleport_server_add_route (g_compute_checksum_for_string (G_CHECKSUM_SHA256, filename,  -1), filename, device->ip);
          g_free (filename);
        }
      else
        {
          gtk_widget_destroy (dialog);
        }

    }
}


static void 
send_file_to_device (gchar *uri, gpointer data)
{
  Peer *device = (Peer *) data;
  GFile *file = g_file_new_for_uri (uri);
  gchar *filename  = NULL;
  if (g_file_query_exists (file, NULL)) {
    filename = g_file_get_path (file);
    teleport_server_add_route (g_compute_checksum_for_string (G_CHECKSUM_SHA256, filename,  -1), g_strdup(filename), device->ip);
    g_free (filename);
  }
  else {
    g_print ("File doesn't exist: %s\n", uri);
  }
  g_object_unref(file);
}


static void
drag_data_received_handl
(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
 GtkSelectionData *selection_data, guint target_type, guint time,
 gpointer data)
{
  glong   *_idata;
  gchar   *_sdata;
  gchar   **uris;

  gboolean dnd_success = FALSE;
  gboolean delete_selection_data = FALSE;

  /* Deal with what we are given from source */
  if((selection_data != NULL) && (gtk_selection_data_get_length(selection_data) >= 0))
    {
      if (gdk_drag_context_get_suggested_action(context) == GDK_ACTION_ASK)
        {
          /* Ask the user to move or copy, then set the context action. */
        }

      if (gdk_drag_context_get_suggested_action(context) == GDK_ACTION_MOVE)
        delete_selection_data = TRUE;

      /* Check that we got the format we can use */
      switch (target_type)
        {
        case TARGET_INT32:
          _idata = (glong*)gtk_selection_data_get_data(selection_data);
          g_print ("integer: %ld", *_idata);
          dnd_success = TRUE;
          break;

        case TARGET_STRING:
          _sdata = (gchar*)gtk_selection_data_get_data(selection_data);
          g_print ("string: %s", _sdata);
          dnd_success = TRUE;
          break;

        case TARGET_URIS:
          uris = gtk_selection_data_get_uris(selection_data);
          if (uris != NULL && uris[1] == NULL) {
            send_file_to_device (uris[0], data);
            dnd_success = TRUE;
          }
          break;

        default:
          g_print ("Something bad!");
        }
    }

  if (dnd_success == FALSE)
    {
      g_print ("DnD data transfer failed!\n");
      g_print ("You can not drag more then one file!\n");
    }

  gtk_drag_finish (context, dnd_success, delete_selection_data, time);
}

/* Emitted when a drag is over the destination */
static gboolean
drag_motion_handl
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint t,
 gpointer user_data)
{
  // Fancy stuff here. This signal spams the console something horrible.
  //const gchar *name = gtk_widget_get_name (widget);
  //g_print ("%s: drag_motion_handl\n", name);
  return  FALSE;
}

/* Emitted when a drag leaves the destination */
static void
drag_leave_handl
(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data)
{
}

/* Emitted when the user releases (drops) the selection. It should check that
 * the drop is over a valid part of the widget (if its a complex widget), and
 * itself to return true if the operation should continue. Next choose the
 * target type it wishes to ask the source for. Finally call gtk_drag_get_data
 * which will emit "drag-data-get" on the source. */
static gboolean
drag_drop_handl
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time,
 gpointer user_data)
{
  gboolean        is_valid_drop_site;
  GdkAtom         target_type;

  /* Check to see if (x,y) is a valid drop site within widget */
  is_valid_drop_site = TRUE;

  /* If the source offers a target */
  if (gdk_drag_context_list_targets (context))
    {
      /* Choose the best target type */
      target_type = GDK_POINTER_TO_ATOM
       (g_list_nth_data (gdk_drag_context_list_targets(context), TARGET_INT32));


      /* Request the data from the source. */
      gtk_drag_get_data
       (
        widget,         /* will receive 'drag-data-received' signal */
        context,        /* represents the current state of the DnD */
        target_type,    /* the target type we want */
        time            /* time stamp */
       );
    }
  /* No target offered by source => error */
  else
    {
      is_valid_drop_site = FALSE;
    }

  return  is_valid_drop_site;
}

static void
teleport_remote_device_init (TeleportRemoteDevice *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* Make the widget a DnD destination. */
  gtk_drag_dest_set
   (
    GTK_WIDGET (self),              /* widget that will accept a drop */
    GTK_DEST_DEFAULT_ALL,
    target_list,            /* lists of target to support */
    n_targets,              /* size of list */
    GDK_ACTION_COPY         /* what to do with data after dropped */
   );
}

GtkWidget*
teleport_remote_device_new (Peer *peer)
{
  return g_object_new (TELEPORT_TYPE_REMOTE_DEVICE,
                       "peer", peer,
                       NULL);
}

/**
 * teleport_remote_device_get_peer:
 * @row: a #TeleportRemoteDevice
 *
 * Retrieves the #Peer that @row manages, or %NULL if none
 * is set.
 *
 * Returns: (transfer none): the internal peer of @row
 */
Peer *
      teleport_remote_device_get_peer (GtkWidget *widget)
{
  TeleportRemoteDevice *row;

  g_return_val_if_fail (TELEPORT_IS_REMOTE_DEVICE (widget), NULL);

  row = TELEPORT_REMOTE_DEVICE (widget);

  return row->peer;
}

void
teleport_remote_device_set_peer (TeleportRemoteDevice *widget,
                                 Peer    *peer)
{
  g_return_if_fail (TELEPORT_IS_REMOTE_DEVICE (widget));
  widget->peer = peer;

  /*
     we need to create a peer object instate of a struct to be able to bind it
  if (!g_set_object (&widget->peer, peer))
    return;

  g_object_bind_property (peer,
                          "name",
                          widget->device_name,
                          "label",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
                          */

  if (peer)
    {
      gtk_label_set_text(GTK_LABEL (widget->device_name), peer->name);

      g_signal_connect (widget->send_btn, "clicked", G_CALLBACK (open_file_picker), peer);


      g_signal_connect (widget, "drag-data-received",
                        G_CALLBACK(drag_data_received_handl), peer);

      g_signal_connect (widget, "drag-leave",
                        G_CALLBACK (drag_leave_handl), peer);

      g_signal_connect (widget, "drag-motion",
                        G_CALLBACK (drag_motion_handl), peer);

      g_signal_connect (widget, "drag-drop",
                        G_CALLBACK (drag_drop_handl), peer);
    }
}

/**
 * teleport_remote_device_destroy:
 * @self: a #TeleportRemoteDevice
 *
 * Destroy @self after hiding it.
 */
void
teleport_remote_device_destroy (TeleportRemoteDevice *self)
{
}

