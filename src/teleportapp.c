#include <gtk/gtk.h>

#include "teleportapp.h"
#include "teleportpeer.h"
#include "teleportappwin.h"
#include "browser.h"
#include "publish.h"
#include "server.h"
#include "get.h"


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

static TeleportAppWindow *win;
static GApplication *application;
static gint signalIds [N_SIGNALS];

struct _TeleportApp {
  GtkApplication parent;
};

G_DEFINE_TYPE (TeleportApp, teleport_app, GTK_TYPE_APPLICATION);


void save_file_callback (GSimpleAction *simple,
    GVariant      *parameter,
    gpointer       user_data) {
  do_downloading(g_variant_get_string (g_variant_get_child_value (parameter, 0), NULL),
                 g_variant_get_string (g_variant_get_child_value (parameter, 1), NULL),
                 g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));
}

void do_nothing_callback (GSimpleAction *simple,
    GVariant      *parameter,
    gpointer       user_data) {
}

void open_file_callback (GSimpleAction *simple,
    GVariant      *parameter,
    gpointer       user_data) {
  g_print("Open file\n %s%s",
                 g_variant_get_string (g_variant_get_child_value (parameter, 3), NULL),
                 g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL));

  g_spawn_command_line_async(g_strdup_printf("xdg-open /home/julian/Projects/teleport/src/%s%s",
                 g_variant_get_string (g_variant_get_child_value (parameter, 3), NULL),
                 g_variant_get_string (g_variant_get_child_value (parameter, 2), NULL)), NULL);
}

void create_user_notification (const char *file_name, const int file_size, const char *origin_device, GVariant *target) {
    GNotification *notification = g_notification_new ("Teleport");
  g_notification_set_body (notification,
      g_strdup_printf("%s is sending %s (%s)",
        origin_device,
        file_name,
        g_format_size (file_size)));
  GIcon *icon = g_themed_icon_new ("dialog-information");
  g_notification_set_icon (notification, icon);
  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Decline", "app.decline", target);
  g_notification_add_button_with_target_value (notification, "Save", "app.save", target);
  g_application_send_notification (application, NULL, notification);
  g_object_unref (icon);
  g_object_unref (notification);
  //the example says I have to unref it but it gives a critival error
  //https://developer.gnome.org/glib/stable/gvariant-format-strings.html
  //g_variant_unref (target);
}

void create_finished_notification (const char *origin, const int filesize, const char *filename, GVariant *target) {
  GNotification *notification = g_notification_new ("Teleport");
  g_notification_set_body (notification, g_strdup_printf("Transfer of %s from %s is complete", filename, origin));
  GIcon *icon = g_themed_icon_new ("dialog-information");
  g_notification_set_icon (notification, icon);
  g_notification_set_default_action_and_target_value (notification, "app.do-nothing", target);
  g_notification_add_button_with_target_value (notification, "Open", "app.open-file", target);
  g_application_send_notification (application, NULL, notification);
  g_object_unref (icon);
  g_object_unref (notification);
}


gboolean mainLoopAddPeerCallback (gpointer peer) {
  //g_print("new New device name is %p\n", ((Peer *)peer));
  //g_print("new New device name is %s\n", ((Peer *)peer)->name);
  update_remote_device_list(win, (Peer *) peer);
  return G_SOURCE_REMOVE;
}

gboolean mainLoopRemovePeerCallback (gpointer peer)  {
  update_remote_device_list_remove(win, (Peer *) peer);
  return G_SOURCE_REMOVE;
}

void callback_add_peer (GObject *instance, Peer *peer, TeleportAppWindow *win ) {
  g_idle_add(mainLoopAddPeerCallback, peer);
}

void callback_remove_peer (GObject *instance, Peer *peer, TeleportAppWindow *win ) {
  g_idle_add(mainLoopRemovePeerCallback, peer);
}

void callback_notify_user (GObject *instance, char *name, TeleportAppWindow *win ) {
  //create_user_notification("icon.png", 2000, "Mark's laptop");
}

static void
teleport_app_init (TeleportApp *app) {

}

static void
teleport_app_activate (GApplication *app) {
  //TeleportAppWindow *win;
  application = app;
  TeleportPeer *peerList = g_object_new (TELEPORT_TYPE_PEER, NULL);

  win = teleport_app_window_new (TELEPORT_APP (app));
  gtk_window_present (GTK_WINDOW (win));

  g_action_map_add_action_entries (G_ACTION_MAP (app),
      app_entries, G_N_ELEMENTS (app_entries),
      app);

  g_signal_connect (peerList, "addpeer", (GCallback)callback_add_peer, win);
  g_signal_connect (peerList, "removepeer", (GCallback)callback_remove_peer, win);
  g_signal_connect (app, "notify_user", (GCallback)callback_notify_user, win);
  /*teleport_peer_add_peer(peerList, "julian", "192.168.0.1", 3000);
    g_print("Data: %s\n", teleport_peer_get_name(peerList, 0, NULL));
    g_print("Data: %s\n", teleport_peer_get_ip(peerList, 0, NULL));
    g_print("Data: %d\n", teleport_peer_get_port(peerList, 0, NULL));
    */

  /*GVariantBuilder *builder;
  GVariant *value;

  builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  g_variant_builder_add (builder, "s", "devicename");
  g_variant_builder_add (builder, "s", "https://downloadlink");
  g_variant_builder_add (builder, "s", "filename");
  value = g_variant_new ("as", builder);
  g_variant_builder_unref (builder);

  create_finished_notification ("USER", 2000, "FILENAME", value);
  */
  run_http_server();
  run_avahi_publish_service((char *) g_get_host_name());
  run_avahi_service(peerList);
}

  static void
teleport_app_open (GApplication  *app,
    GFile        **files,
    gint           n_files,
    const gchar   *hint)
{
  GList *windows;
  int i;

  windows = gtk_application_get_windows (GTK_APPLICATION (app));
  if (windows)
    win = TELEPORT_APP_WINDOW (windows->data);
  else
    win = teleport_app_window_new (TELEPORT_APP (app));

  for (i = 0; i < n_files; i++)
    teleport_app_window_open (win, files[i]);

  gtk_window_present (GTK_WINDOW (win));
}

  static void
teleport_app_class_init (TeleportAppClass *class)
{
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
      "application-id", "org.gtk.teleportapp",
      "flags", G_APPLICATION_HANDLES_OPEN,
      NULL);
}
