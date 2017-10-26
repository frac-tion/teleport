#include <gtk/gtk.h>

#include "teleport-app.h"
#include "teleport-window.h"
#include "teleport-server.h"
#include "teleport-peer.h"

GtkWidget *find_child(GtkWidget *, const gchar *);

enum {
  TARGET_INT32,
  TARGET_STRING,
  TARGET_URIS,
  TARGET_ROOTWIN
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
  GtkWidget *remote_devices_list;
  GtkWidget *this_device_name_label;
  GtkWidget *remote_no_devices;
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

  gtk_label_set_text (GTK_LABEL (priv->this_device_name_label),
                      g_settings_get_string (priv->settings, "device-name"));

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
}


/*Doing Dnd init stuff */
static void
add_dnd (GtkWidget *widget, gpointer data)
{
  /* Make the widget a DnD destination. */
  gtk_drag_dest_set
   (
    widget,              /* widget that will accept a drop */
    GTK_DEST_DEFAULT_ALL,
    target_list,            /* lists of target to support */
    n_targets,              /* size of list */
    GDK_ACTION_COPY         /* what to do with data after dropped */
   );

  /* All possible destination signals */
  g_signal_connect (widget, "drag-data-received",
                    G_CALLBACK(drag_data_received_handl), data);

  g_signal_connect (widget, "drag-leave",
                    G_CALLBACK (drag_leave_handl), data);

  g_signal_connect (widget, "drag-motion",
                    G_CALLBACK (drag_motion_handl), data);

  g_signal_connect (widget, "drag-drop",
                    G_CALLBACK (drag_drop_handl), data);
}

static void
open_file_picker(GtkButton *btn,
                 Peer *device) {
  GtkWidget *dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  gint res;
  g_print("Open file chooser for submitting a file to %s with Address %s\n", device->name, device->ip);

  dialog =  gtk_file_chooser_dialog_new ("Open File",
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
      gtk_widget_destroy (dialog);
      teleport_server_add_route (g_compute_checksum_for_string (G_CHECKSUM_SHA256, filename,  -1), filename, device->ip);
      g_free (filename);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

void
update_remote_device_list(TeleportWindow *win,
                          Peer *device)
{
  TeleportWindowPrivate *priv;
  GtkBuilder *builder_remote_list;
  GtkWidget *row;
  GtkLabel *name_label;
  GtkButton *send_btn;
  //GtkWidget *line;

  priv = teleport_window_get_instance_private (win);

  gtk_widget_hide (priv->remote_no_devices);

  builder_remote_list = gtk_builder_new_from_resource ("/com/frac_tion/teleport/remote_list.ui");

  row = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_device_row"));
  name_label = GTK_LABEL (gtk_builder_get_object (builder_remote_list, "device_name"));
  gtk_label_set_text(name_label, device->name);
  gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), row, -1);
  send_btn = GTK_BUTTON (gtk_builder_get_object (builder_remote_list, "send_btn"));
  g_signal_connect (send_btn, "clicked", G_CALLBACK (open_file_picker), device);

  //Add drag n drop
  add_dnd (row, device);

  //line = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_space_row"));
  //gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), line, -1);
  g_object_unref (builder_remote_list);
}


void
update_remote_device_list_remove(TeleportWindow *win,
                                 Peer *device)
{
  TeleportWindowPrivate *priv;
  GtkWidget *box;
  GtkListBoxRow *remote_row;
  GtkLabel *name_label;
  gint i = 0;

  priv = teleport_window_get_instance_private (win);
  box = priv->remote_devices_list;

  remote_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), i);

  while (remote_row != NULL) {
    name_label = GTK_LABEL(find_child(GTK_WIDGET(remote_row), "GtkLabel"));
    if (name_label != NULL && g_strcmp0(device->name, gtk_label_get_text(name_label)) == 0) {
      gtk_container_remove (GTK_CONTAINER(box), GTK_WIDGET(remote_row));
    }
    i++;
    remote_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), i);
  }
  // the last row got removed and we have to display the searching box
  if (i <= 2) {
    gtk_widget_show (priv->remote_no_devices);
  }
}

GtkWidget *
find_child(GtkWidget *parent, const gchar *name)
{
  if (g_strcmp0(gtk_widget_get_name((GtkWidget *)parent), (gchar *)name) == 0) {
    return parent;
  }

  if (GTK_IS_BIN(parent)) {
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
    return find_child(child, name);
  }

  if (GTK_IS_CONTAINER(parent)) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
    while ((children = g_list_next(children)) != NULL) {
      GtkWidget *widget = find_child(children->data, name);
      if (widget != NULL) {
        return widget;
      }
    }
  }

  return NULL;
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
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, this_device_name_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, remote_no_devices);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, remote_devices_list);
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
teleport_window_open (TeleportWindow *win,
                      GFile *file)
{
  //TeleportWindowPrivate *priv;
  //priv = teleport_window_get_instance_private (win);
}
