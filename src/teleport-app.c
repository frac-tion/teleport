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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "teleport-app.h"
#include "teleport-peer.h"
#include "teleport-window.h"
#include "teleport-browser.h"
#include "teleport-publish.h"
#include "teleport-server.h"
#include "teleport-get.h"


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

enum {
  NOTIFY_USER, NOTIFY_FINISED, N_SIGNALS
};

static GActionEntry app_entries[] =
{
    { "save", save_file_callback, "as", NULL, NULL },
    { "decline", do_nothing_callback, "as", NULL, NULL },
    { "do-nothing", do_nothing_callback, "as", NULL, NULL },
    { "open-file", open_file_callback, "as", NULL, NULL },
    { "open-folder", open_folder_callback, "as", NULL, NULL },
    { "about", teleport_app_show_about },
    { "quit",   teleport_app_quit }
};

static TeleportApp *mainApplication;
static gint signalIds [N_SIGNALS];

typedef struct
{
  GtkWidget     *window;
  TeleportPeer  *peerList;
} TeleportAppPrivate;


struct _TeleportApp {
  GtkApplication        parent;

  /*< private >*/
  TeleportAppPrivate    *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (TeleportApp, teleport_app, GTK_TYPE_APPLICATION);

static void
on_avahi_appeared (GDBusConnection *connection,
                  const gchar     *name,
                  const gchar     *name_owner,
                  gpointer         user_data)
{
  TeleportApp *app = user_data;
  TeleportAppPrivate *priv = app->priv;
  GtkWidget *window = priv->window;

  teleport_show_no_device_message (TELEPORT_WINDOW (window), TRUE);
  teleport_publish_run (teleport_get_device_name());
  teleport_browser_run_avahi_service(priv->peerList);
  teleport_show_no_avahi_message (TELEPORT_WINDOW (window), FALSE);
}

static void
on_avahi_vanished (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  TeleportApp *app = user_data;
  TeleportAppPrivate *priv = app->priv;
  GtkWidget *window = priv->window;

  teleport_show_no_device_message (TELEPORT_WINDOW (window), FALSE);
  teleport_show_no_avahi_message (TELEPORT_WINDOW (window), TRUE);
}

static void
watch_for_avahi_service (TeleportApp *application) {
    g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                 "org.freedesktop.Avahi",
                                 G_BUS_NAME_WATCHER_FLAGS_NONE,
                                 on_avahi_appeared,
                                 on_avahi_vanished,
                                 application,
                                 NULL);
}

static void 
save_file_callback (GSimpleAction *simple,
                    GVariant      *parameter,
                    gpointer       user_data) {
  teleport_get_do_downloading(g_variant_get_string (g_variant_get_child_value (parameter, 0), NULL),
                              g_variant_get_string (g_variant_get_child_value (parameter, 1), NULL),
                              g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));
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
  const gchar *path;
  g_print("Open file\n %s%s",
          g_variant_get_string (g_variant_get_child_value (parameter, 3), NULL),
          g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));

  path = g_strdup_printf("%s/%s",
                         g_variant_get_string (g_variant_get_child_value (parameter, 3), NULL),
                         g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));

  gtk_show_uri_on_window (NULL, g_filename_to_uri(path, NULL, NULL), GDK_CURRENT_TIME, NULL);
}

void 
create_user_notification (const char *file_name, const int file_size, const char *origin_device, GVariant *target) {
  GNotification *notification = g_notification_new ("Teleport");
  TeleportAppPrivate *priv = mainApplication->priv;

  g_notification_set_body (notification,
                           g_strdup_printf("%s is sending %s (%s)",
                                           teleport_peer_get_name_by_addr (priv->peerList, origin_device),
                                           file_name,
                                           g_format_size (file_size)));

  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Decline", "app.decline", target);
  g_notification_add_button_with_target_value (notification, "Save", "app.save", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification ((GApplication *) mainApplication, NULL, notification);
  g_object_unref (notification);
  //the example says I have to unref it but it gives a critival error
  //https://developer.gnome.org/glib/stable/gvariant-format-strings.html
  //g_variant_unref (target);
}

void
create_finished_notification (const char *origin, const int filesize, const char *filename, GVariant *target) {
  GNotification *notification = g_notification_new ("Teleport");
  TeleportAppPrivate *priv = mainApplication->priv;

  g_notification_set_body (notification,
                           g_strdup_printf("Transfer of %s from %s is complete", 
                                           filename,
                                           teleport_peer_get_name_by_addr (priv->peerList, origin)));
  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Show in folder", "app.open-folder", target);
  g_notification_add_button_with_target_value (notification, "Open", "app.open-file", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification ((GApplication *) mainApplication, NULL, notification);
  g_object_unref (notification);
}


gboolean
mainLoopAddPeerCallback (gpointer peer) {
  TeleportAppPrivate *priv = mainApplication->priv;
  GtkWidget *window = priv->window;

  update_remote_device_list((TeleportWindow *) window, (Peer *) peer);
  return G_SOURCE_REMOVE;
}

gboolean
mainLoopRemovePeerCallback (gpointer peer)  {
  TeleportAppPrivate *priv = mainApplication->priv;
  GtkWidget *window = priv->window;

  update_remote_device_list_remove((TeleportWindow *) window, (Peer *) peer);
  //if (teleport_peer_get_number (priv->peerList) == 0)
  if (teleport_peer_get_number (priv->peerList) == 0)
    teleport_show_no_device_message (TELEPORT_WINDOW (window), TRUE);
  return G_SOURCE_REMOVE;
}

void
callback_add_peer (GObject *instance, Peer *peer, gpointer window) {
  g_idle_add(mainLoopAddPeerCallback, peer);
}

void
callback_remove_peer (GObject *instance, Peer *peer, gpointer window) {
  g_idle_add(mainLoopRemovePeerCallback, peer);
}

void
callback_notify_user (GObject *instance, gchar *name, gpointer window) {
  //create_user_notification("icon.png", 2000, "Mark's laptop");
}

static void
teleport_app_startup (GApplication *app) {
  TeleportAppPrivate *priv;
  GtkStyleProvider *provider;

  mainApplication = TELEPORT_APP (app);
  priv = mainApplication->priv;

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   app);

  G_APPLICATION_CLASS (teleport_app_parent_class)->startup (app);

  /* window */
  priv->window = GTK_WIDGET (teleport_window_new (TELEPORT_APP (app)));

  /* CSS style */
  provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_resource (GTK_CSS_PROVIDER (provider),
                                       "com/frac_tion/teleport/style.css");
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default(),
                                             provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  /* PeerList */
  priv->peerList = g_object_new (TELEPORT_TYPE_PEER, NULL);

  /* Add signals */
  g_signal_connect (priv->peerList, "addpeer", (GCallback)callback_add_peer, priv->window);
  g_signal_connect (priv->peerList, "removepeer", (GCallback)callback_remove_peer, priv->window);
  g_signal_connect (app, "notify_user", (GCallback)callback_notify_user, priv->window);

  teleport_server_run();

  watch_for_avahi_service (TELEPORT_APP (app));
}

static void
teleport_app_activate (GApplication *app) {
  TeleportAppPrivate *priv;
  priv = mainApplication->priv;

  gtk_widget_show (priv->window);
  gtk_window_present (GTK_WINDOW (priv->window));
}

static void
teleport_app_finalize (GObject *object)
{
  G_OBJECT_CLASS (teleport_app_parent_class)->finalize (object);
}

static void
teleport_app_class_init (TeleportAppClass *class)
{
  G_APPLICATION_CLASS (class)->startup = teleport_app_startup;
  G_APPLICATION_CLASS (class)->activate = teleport_app_activate;

  G_OBJECT_CLASS (class)->finalize = teleport_app_finalize;

  signalIds[NOTIFY_USER] = g_signal_new ("notify_user",
                                         G_TYPE_OBJECT,
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                         0,
                                         NULL /* accumulator */,
                                         NULL /* accumulator data */,
                                         NULL /* C marshaller */,
                                         G_TYPE_NONE /* return_type */,
                                         1,
                                         G_TYPE_STRING);
}

static void
teleport_app_show_about (GSimpleAction *simple,
                         GVariant      *parameter,
                         gpointer       user_data)
{
  TeleportAppPrivate *priv = TELEPORT_APP (user_data)->priv;
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

  gtk_show_about_dialog (GTK_WINDOW (priv->window),
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
teleport_app_quit (GSimpleAction *simple,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  TeleportAppPrivate *priv = TELEPORT_APP (user_data)->priv;

  gtk_widget_destroy (priv->window);
}

TeleportApp *
teleport_app_new (void)
{
  return g_object_new (TELEPORT_APP_TYPE,
                       "application-id", "com.frac_tion.teleport",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

static void
teleport_app_init (TeleportApp *app) {
  TeleportAppPrivate *priv = teleport_app_get_instance_private (app);
  app->priv = priv;
}


