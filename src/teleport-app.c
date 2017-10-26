#include <gtk/gtk.h>

#include "teleport-app.h"
#include "teleport-peer.h"
#include "teleport-window.h"
#include "teleport-browser.h"
#include "teleport-publish.h"
#include "teleport-server.h"
#include "teleport-get.h"


void save_file_callback (GSimpleAction *simple,
                         GVariant      *parameter,
                         gpointer       user_data);

void do_nothing_callback (GSimpleAction *simple,
                          GVariant      *parameter,
                          gpointer       user_data);

void open_file_callback (GSimpleAction *simple,
                         GVariant      *parameter,
                         gpointer       user_data);


enum {
  NOTIFY_USER, NOTIFY_FINISED, N_SIGNALS
};

static GActionEntry app_entries[] =
{
    { "save", save_file_callback, "as", NULL, NULL },
    { "decline", do_nothing_callback, "as", NULL, NULL },
    { "do-nothing", do_nothing_callback, "as", NULL, NULL },
    { "open-file", open_file_callback, "as", NULL, NULL }
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


void save_file_callback (GSimpleAction *simple,
                         GVariant      *parameter,
                         gpointer       user_data) {
  teleport_get_do_downloading(g_variant_get_string (g_variant_get_child_value (parameter, 0), NULL),
                              g_variant_get_string (g_variant_get_child_value (parameter, 1), NULL),
                              g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));
}

void do_nothing_callback (GSimpleAction *simple,
                          GVariant      *parameter,
                          gpointer       user_data) {
  g_print ("No action\n");
}

void open_file_callback (GSimpleAction *simple,
                         GVariant      *parameter,
                         gpointer       user_data) {
  g_print("Open file\n %s%s",
          g_variant_get_string (g_variant_get_child_value (parameter, 3), NULL),
          g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));

  g_spawn_command_line_async(g_strdup_printf("xdg-open %s/%s",
                                             g_variant_get_string (g_variant_get_child_value (parameter, 3), NULL),
                                             g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL)), NULL);
}

void create_user_notification (const char *file_name, const int file_size, const char *origin_device, GVariant *target) {
  GIcon *icon;
  GNotification *notification = g_notification_new ("Teleport");
  TeleportAppPrivate *priv = mainApplication->priv;
  g_notification_set_body (notification,
                           g_strdup_printf("%s is sending %s (%s)",
                                           teleport_peer_get_name_by_addr (priv->peerList, origin_device),
                                           file_name,
                                           g_format_size (file_size)));
  icon = g_themed_icon_new ("dialog-information");
  g_notification_set_icon (notification, icon);
  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Decline", "app.decline", target);
  g_notification_add_button_with_target_value (notification, "Save", "app.save", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification ((GApplication *) mainApplication, NULL, notification);
  g_object_unref (icon);
  g_object_unref (notification);
  //the example says I have to unref it but it gives a critival error
  //https://developer.gnome.org/glib/stable/gvariant-format-strings.html
  //g_variant_unref (target);
}

void create_finished_notification (const char *origin, const int filesize, const char *filename, GVariant *target) {
  GIcon *icon;
  GNotification *notification = g_notification_new ("Teleport");
  TeleportAppPrivate *priv = mainApplication->priv;

  g_notification_set_body (notification,
                           g_strdup_printf("Transfer of %s from %s is complete", 
                                           filename,
                                           teleport_peer_get_name_by_addr (priv->peerList, origin)));
  icon = g_themed_icon_new ("dialog-information");
  g_notification_set_icon (notification, icon);
  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Open", "app.open-file", target);
  g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_HIGH);
  g_application_send_notification ((GApplication *) mainApplication, NULL, notification);
  g_object_unref (icon);
  g_object_unref (notification);
}


gboolean mainLoopAddPeerCallback (gpointer peer) {
  TeleportAppPrivate *priv = mainApplication->priv;
  GtkWidget *window = priv->window;

  update_remote_device_list((TeleportWindow *) window, (Peer *) peer);
  return G_SOURCE_REMOVE;
}

gboolean mainLoopRemovePeerCallback (gpointer peer)  {
  TeleportAppPrivate *priv = mainApplication->priv;
  GtkWidget *window = priv->window;

  update_remote_device_list_remove((TeleportWindow *) window, (Peer *) peer);
  return G_SOURCE_REMOVE;
}

void callback_add_peer (GObject *instance, Peer *peer, gpointer window) {
  g_idle_add(mainLoopAddPeerCallback, peer);
}

void callback_remove_peer (GObject *instance, Peer *peer, gpointer window) {
  g_idle_add(mainLoopRemovePeerCallback, peer);
}

void callback_notify_user (GObject *instance, gchar *name, gpointer window) {
  //create_user_notification("icon.png", 2000, "Mark's laptop");
}

static void
teleport_app_startup (GApplication *app) {
  TeleportAppPrivate *priv;
  mainApplication = TELEPORT_APP (app);
  priv = mainApplication->priv;

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  G_APPLICATION_CLASS (teleport_app_parent_class)->startup (app);

  priv->window = GTK_WIDGET (teleport_window_new (TELEPORT_APP (app)));

  priv->peerList = g_object_new (TELEPORT_TYPE_PEER, NULL);

  g_signal_connect (priv->peerList, "addpeer", (GCallback)callback_add_peer, priv->window);
  g_signal_connect (priv->peerList, "removepeer", (GCallback)callback_remove_peer, priv->window);
  g_signal_connect (app, "notify_user", (GCallback)callback_notify_user, priv->window);

  teleport_server_run();
  teleport_publish_run (teleport_get_device_name());
  teleport_browser_run_avahi_service(priv->peerList);
}

static void
teleport_app_activate (GApplication *app) {
  TeleportAppPrivate *priv;
  priv = mainApplication->priv;

  gtk_widget_show (priv->window);
  gtk_window_present (GTK_WINDOW (priv->window));
}

static void
teleport_app_open (GApplication  *app,
                   GFile        **files,
                   gint           n_files,
                   const gchar   *hint)
{
  /*GList *windows;
  int i;

  windows = gtk_application_get_windows (GTK_APPLICATION (app));
  if (windows)
    win = TELEPORT_WINDOW (windows->data);
  else
    win = teleport_window_new (TELEPORT_APP (app));

  for (i = 0; i < n_files; i++)
    teleport_window_open (win, files[i]);

  gtk_window_present (GTK_WINDOW (win));
  */
}

static void
teleport_app_class_init (TeleportAppClass *class)
{
  G_APPLICATION_CLASS (class)->startup = teleport_app_startup;
  G_APPLICATION_CLASS (class)->activate = teleport_app_activate;
  G_APPLICATION_CLASS (class)->open = teleport_app_open;

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


