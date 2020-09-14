/* teleport-app.c
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
#define HANDY_USE_UNSTABLE_API
#include <handy.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "teleport-app.h"
#include "teleport-peer.h"
#include "teleport-window.h"
#include "teleport-avahi.h"
#include "teleport-server.h"


static void save_file_callback          (GSimpleAction *simple,
                                         GVariant      *parameter,
                                         gpointer       user_data);

static void do_nothing_callback         (GSimpleAction *simple,
                                         GVariant      *parameter,
                                         gpointer       user_data);

static void open_file_callback          (GSimpleAction *simple,
                                         GVariant      *parameter,
                                         gpointer       user_data);


static void open_folder_callback        (GSimpleAction        *simple,
                                         GVariant             *parameter,
                                         gpointer              user_data);


static void teleport_app_show_about     (GSimpleAction        *simple,
                                         GVariant             *parameter,
                                         gpointer              user_data);

static void teleport_app_quit           (GSimpleAction        *simple,
                                         GVariant             *parameter,
                                         gpointer              user_data);

static const GActionEntry app_entries[] =
{
    { "save", save_file_callback, "s", NULL, NULL },
    { "decline", do_nothing_callback, "s", NULL, NULL },
    { "do-nothing", do_nothing_callback, "s", NULL, NULL },
    { "open-file", open_file_callback, "s", NULL, NULL },
    { "open-folder", open_folder_callback, "s", NULL, NULL },
    { "about", teleport_app_show_about },
    { "quit",   teleport_app_quit }
};

struct _TeleportApp {
  GtkApplication        parent;

  GSettings      *settings;
  TeleportWindow *window;
  TeleportServer *server;
  TeleportAvahi  *avahi;
  GListStore     *peer_list;
  GHashTable     *files;
};

G_DEFINE_TYPE (TeleportApp, teleport_app, GTK_TYPE_APPLICATION)

static void
create_user_notification (TeleportApp *self,
                          TeleportFile *file,
                          TeleportPeer *device)
{
  g_autoptr (GNotification) notification;
  GVariant *target;
  const gchar *notification_id;

  notification = g_notification_new ("Teleport");
  notification_id = teleport_file_get_id (file);
  target = g_variant_new_string (notification_id);
  g_notification_set_body (notification,
                           g_strdup_printf("%s is sending %s (%s)",
                                           teleport_peer_get_name(device),
                                           teleport_file_get_destination_path(file),
                                           g_format_size (teleport_file_get_size(file))));

  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Decline", "app.decline", target);
  g_notification_add_button_with_target_value (notification, "Save", "app.save", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification (G_APPLICATION (self), notification_id, notification);
  g_hash_table_insert (self->files, g_strdup (notification_id), file);
}

/*
void
create_finished_notification (const char *origin, const int filesize, const char *filename, GVariant *target) {
  GNotification *notification = g_notification_new ("Teleport");

  g_notification_set_body (notification,
                           g_strdup_printf("Transfer of %s from %s is complete", 
                                           filename,
                                           teleport_peer_get_name_by_addr (self->peerList, origin)));
  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Show in folder", "app.open-folder", target);
  g_notification_add_button_with_target_value (notification, "Open", "app.open-file", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification ((GApplication *) mainApplication, NULL, notification);
  g_object_unref (notification);
}
*/
 
static void
save_file_callback (GSimpleAction *simple,
                    GVariant      *parameter,
                    gpointer       user_data) {
  TeleportApp *self;
  TeleportFile *file;
  g_autofree gchar *download_directory = NULL;
  const gchar *file_id;

  self = TELEPORT_APP (user_data);
  file_id = g_variant_get_string (parameter, NULL);
  file = g_hash_table_lookup (self->files, file_id);

  if (TELEPORT_IS_FILE (file)) {
    g_print ("The file will be downloaded and saved.\n");
    download_directory = teleport_get_download_directory (self);
    teleport_file_download (file, download_directory);
  }
}

static void
do_nothing_callback (GSimpleAction *simple,
                     GVariant      *parameter,
                     gpointer       user_data) {
  g_print ("No action\n");
}

static void
open_folder_callback (GSimpleAction *simple,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  GDBusProxy      *proxy;
  GVariant        *retval;
  GVariantBuilder *builder;
  const gchar     *uri;
  GError **error = NULL;
  const gchar     *path;

  path = g_strdup_printf("%s/%s",
                         g_variant_get_string (g_variant_get_child_value (parameter, 3), NULL),
                         g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));

  uri = g_filename_to_uri (path, NULL, error);
  g_debug ("Show file: %s", uri);

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.freedesktop.FileManager1",
                                         "/org/freedesktop/FileManager1",
                                         "org.freedesktop.FileManager1",
                                         NULL, error);

  if (!proxy) {
    g_prefix_error (error,
                    ("Connecting to org.freedesktop.FileManager1 failed: "));
  }
  else {

    builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
    g_variant_builder_add (builder, "s", uri);

    retval = g_dbus_proxy_call_sync (proxy,
                                     "ShowItems",
                                     g_variant_new ("(ass)",
                                                    builder,
                                                    ""),
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1, NULL, error);

    g_variant_builder_unref (builder);
    g_object_unref (proxy);

    if (!retval)
      {
        g_prefix_error (error, ("Calling ShowItems failed: "));
      }
    else
      g_variant_unref (retval);
  }
}


static void
open_file_callback (GSimpleAction *simple,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  g_print ("TODO: open file");
  //gtk_show_uri_on_window (NULL, g_filename_to_uri(path, NULL, NULL), GDK_CURRENT_TIME, NULL);
}

static void
recived_file_cb (TeleportApp *self,
                 TeleportPeer *peer,
                 TeleportFile *file,
                 TeleportServer *server)
{
  g_print ("%s is sending %s (%s)\n",
           teleport_peer_get_name (peer),
           teleport_file_get_destination_path (file),
           g_format_size (teleport_file_get_size (file)));


  teleport_peer_add_file (peer, file);
  create_user_notification (self, file, peer);
}

static void
avahi_state_changed_cb (TeleportApp *self)
{
  TeleportAvahiState state = teleport_avahi_get_state (self->avahi);

  /* TODO: handle all errors separatly */
  switch (state) {
  case TELEPORT_AVAHI_STATE_UNKNOWN:
  case TELEPORT_AVAHI_STATE_ERROR:
  case TELEPORT_AVAHI_STATE_NAME_COLLISION:
    teleport_window_set_view (self->window, "unknown-error");
    break;
  case TELEPORT_AVAHI_STATE_NEW:
  case TELEPORT_AVAHI_STATE_RUNNING:
    teleport_window_set_view (self->window, "normal");
    break;
    break;
  case TELEPORT_AVAHI_STATE_NO_DEAMON:
    teleport_window_set_view (self->window, "no-avahi-daemon");
    break;
  }
}

static void
teleport_app_add_peer (TeleportApp *self, TeleportPeer *peer) {
  g_list_store_append (self->peer_list, peer);
  teleport_server_add_peer (self->server, peer);
}

static void
teleport_app_remove_peer (TeleportApp *self, TeleportPeer *peer) {
  guint position = 0;

  if (g_list_store_find (self->peer_list, peer, &position))
    g_list_store_remove (self->peer_list, position);

  teleport_server_remove_peer (self->server, peer);
}

static void
init_settings (GSettings *settings) {
  if (g_settings_get_user_value (settings, "download-dir") == NULL) {
    g_print ("Download dir set to XDG DOWNLOAD directory\n");
    if (g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD) != NULL) {
      g_settings_set_string (settings,
                             "download-dir",
                             g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
    }
    else {
      g_print ("Error: XDG DOWNLOAD is not set.\n");
    }
  }

  if (g_settings_get_user_value (settings, "device-name") == NULL) {
    g_settings_set_string (settings,
                           "device-name",
                           g_get_host_name());
  }
}

static gchar * 
get_device_name (TeleportApp *self) {
  return g_settings_get_string (self->settings, "device-name");
}

gchar * 
teleport_get_download_directory (TeleportApp *self) 
{
  return g_settings_get_string (self->settings, "download-dir");
}

static void
teleport_app_startup (GApplication *application) {
  TeleportApp *self = TELEPORT_APP (application);
  g_autoptr(GtkCssProvider) provider = NULL;
  TeleportPeer *dummy_peer;
  const gchar *quit_accels[] = { "<Primary>q", NULL };

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   self);

  G_APPLICATION_CLASS (teleport_app_parent_class)->startup (application);

  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "app.quit", quit_accels);

  hdy_init ();

  /* CSS style */
  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider,
                                       "com/frac_tion/teleport/style.css");
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default(),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  /* ListStore to store peers */
  self->peer_list = g_list_store_new (TELEPORT_TYPE_PEER);

  /* TODO: randomize the server port */
  self->server = teleport_server_new (3000);
  self->avahi = teleport_avahi_new (get_device_name (self), 3000);
  g_signal_connect_swapped (self->server, "recived_file", G_CALLBACK (recived_file_cb), self);
  g_signal_connect_swapped (self->avahi, "notify::state", G_CALLBACK (avahi_state_changed_cb), self);
  g_signal_connect_swapped (self->avahi, "new-device", G_CALLBACK (teleport_app_add_peer), self);
  g_signal_connect_swapped (self->avahi,
                            "device-disappeared",
                            G_CALLBACK (teleport_app_remove_peer),
                            self);
  g_settings_bind (self->settings,
                   "device-name",
                   self->avahi,
                   "name",
                   G_SETTINGS_BIND_GET);

  /* Add dummy devie */
  dummy_peer = teleport_peer_new("Dummy Device", "192.168.1.57", 3000);
  teleport_app_add_peer (self, dummy_peer);

  /* window */
  self->window = teleport_window_new (self);
  teleport_window_bind_device_list (self->window, self->peer_list);

  teleport_avahi_start (self->avahi);
}

static void
teleport_app_activate (GApplication *application) {
  TeleportApp *self = TELEPORT_APP (application);

  gtk_widget_show (GTK_WIDGET (self->window));
  gtk_window_present (GTK_WINDOW (self->window));
}

static void
teleport_app_finalize (GObject *object)
{
  TeleportApp *self = TELEPORT_APP (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->peer_list);
  g_clear_object (&self->window);
  g_hash_table_destroy (self->files);

  G_OBJECT_CLASS (teleport_app_parent_class)->finalize (object);
}

static void
teleport_app_class_init (TeleportAppClass *class)
{
  G_APPLICATION_CLASS (class)->startup = teleport_app_startup;
  G_APPLICATION_CLASS (class)->activate = teleport_app_activate;

  G_OBJECT_CLASS (class)->finalize = teleport_app_finalize;
}

static void
teleport_app_show_about (GSimpleAction *simple,
                         GVariant      *parameter,
                         gpointer       user_data)
{
  TeleportApp *self = TELEPORT_APP (user_data);
  char *copyright;
  GDateTime *date;
  int created_year = 2017;

  static const gchar *authors[] = {
    "Julian Sparber <julian@sparber.net>",
    NULL
  };

  static const gchar *artists[] = {
    "Tobias Bernard <tbernard@gnome.org>",
    NULL
  };

  date = g_date_time_new_now_local ();

  if (g_date_time_get_year (date) <= created_year)
    {
      copyright = g_strdup_printf (("Copyright \xC2\xA9 %d "
                                     "The Teleport authors"), created_year);
    }
  else
    {
      copyright = g_strdup_printf (("Copyright \xC2\xA9 %d\xE2\x80\x93%d "
                                     "The Telport authors"), created_year, g_date_time_get_year (date));
    }

  gtk_show_about_dialog (GTK_WINDOW (self->window),
                         "program-name", ("Teleport"),
                         "version", VERSION,
                         "copyright", copyright,
                         "license-type", GTK_LICENSE_AGPL_3_0,
                         "authors", authors,
                         "artists", artists,
                         "logo-icon-name", "com.frac_tion.teleport",
                         "translator-credits", ("translator-credits"),
                         NULL);
  g_free (copyright);
  g_date_time_unref (date);
}

static void
teleport_app_init (TeleportApp *self) {
  self->settings = g_settings_new ("com.frac_tion.teleport");
  init_settings (self->settings);

  self->avahi = NULL;
  self->files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
teleport_app_quit (GSimpleAction *simple,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  TeleportApp *self = TELEPORT_APP (user_data);

  gtk_widget_destroy (GTK_WIDGET (self->window));
}

TeleportApp *
teleport_app_new (void)
{
  return g_object_new (TELEPORT_APP_TYPE,
                       "application-id", "com.frac_tion.teleport",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

GSettings *
teleport_app_get_settings (TeleportApp *self)
{
  g_return_val_if_fail (TELEPORT_IS_APP (self), NULL);
  return self->settings;
}

void
teleport_app_send_file (TeleportApp *self,
                        TeleportFile *file,
                        TeleportPeer *device)
{
  teleport_server_add_file (self->server, file);
  teleport_peer_send_file (device, file);
}
