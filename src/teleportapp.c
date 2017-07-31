#include <gtk/gtk.h>

#include "teleportapp.h"
#include "teleportpeer.h"
#include "teleportappwin.h"
#include "browser.h"
#include "publish.h"
#include "server.h"

static TeleportAppWindow *win;

struct _TeleportApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE(TeleportApp, teleport_app, GTK_TYPE_APPLICATION);


gboolean mainLoopAddPeerCallback (gpointer name)  {
  update_remote_device_list(win, (char*)name);
  return G_SOURCE_REMOVE;
}

gboolean mainLoopRemovePeerCallback (gpointer name)  {
  update_remote_device_list_remove(win, (char*)name);
  return G_SOURCE_REMOVE;
}

void callback_add_peer(GObject * instance, char * name, TeleportAppWindow * win ) {
  g_idle_add(mainLoopAddPeerCallback, g_strdup(name));
}

void callback_remove_peer(GObject * instance, char * name, TeleportAppWindow * win ) {
  g_idle_add(mainLoopRemovePeerCallback, g_strdup(name));
}

  static void
teleport_app_init (TeleportApp *app)
{

}

  static void
teleport_app_activate (GApplication *app)
{
  //TeleportAppWindow *win;
  TeleportPeer *peerList = g_object_new (TELEPORT_TYPE_PEER, NULL);

  win = teleport_app_window_new (TELEPORT_APP (app));
  gtk_window_present (GTK_WINDOW (win));

  g_signal_connect (peerList, "addpeer", (GCallback)callback_add_peer, win);
  g_signal_connect (peerList, "removepeer", (GCallback)callback_remove_peer, win);
  /*teleport_peer_add_peer(peerList, "julian", "192.168.0.1", 3000);
  g_print("Data: %s\n", teleport_peer_get_name(peerList, 0, NULL));
  g_print("Data: %s\n", teleport_peer_get_ip(peerList, 0, NULL));
  g_print("Data: %d\n", teleport_peer_get_port(peerList, 0, NULL));
  */
  run_http_server();
  run_avahi_publish_service("Angela's (self)");
  run_avahi_service(peerList);
	GNotification *notification = g_notification_new ("Hello world!");
	g_notification_set_body (notification, "This is an example notification.");
	GIcon *icon = g_themed_icon_new ("dialog-information");
	g_notification_set_icon (notification, icon);
	g_notification_add_button (notification, "5 minutes", "app.reply-5-minutes");
	g_application_send_notification (app, NULL, notification);
	g_object_unref (icon);
	g_object_unref (notification);
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
}

  TeleportApp *
teleport_app_new (void)
{
  return g_object_new (TELEPORT_APP_TYPE,
      "application-id", "org.gtk.teleportapp",
      "flags", G_APPLICATION_HANDLES_OPEN,
      NULL);
}
