#include <gtk/gtk.h>

#include "teleportapp.h"
#include "teleportappwin.h"

struct _TeleportApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE(TeleportApp, teleport_app, GTK_TYPE_APPLICATION);

  static void
teleport_app_init (TeleportApp *app)
{
}

  static void
teleport_app_activate (GApplication *app)
{
  TeleportAppWindow *win;

  win = teleport_app_window_new (TELEPORT_APP (app));
  gtk_window_present (GTK_WINDOW (win));
}

  static void
teleport_app_open (GApplication  *app,
    GFile        **files,
    gint           n_files,
    const gchar   *hint)
{
  GList *windows;
  TeleportAppWindow *win;
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
