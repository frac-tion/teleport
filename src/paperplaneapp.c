#include <gtk/gtk.h>

#include "paperplaneapp.h"
#include "paperplaneappwin.h"

struct _PaperplaneApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE(PaperplaneApp, paperplane_app, GTK_TYPE_APPLICATION);

  static void
paperplane_app_init (PaperplaneApp *app)
{
}

  static void
paperplane_app_activate (GApplication *app)
{
  PaperplaneAppWindow *win;

  win = paperplane_app_window_new (PAPERPLANE_APP (app));
  gtk_window_present (GTK_WINDOW (win));
}

  static void
paperplane_app_open (GApplication  *app,
    GFile        **files,
    gint           n_files,
    const gchar   *hint)
{
  GList *windows;
  PaperplaneAppWindow *win;
  int i;

  windows = gtk_application_get_windows (GTK_APPLICATION (app));
  if (windows)
    win = PAPERPLANE_APP_WINDOW (windows->data);
  else
    win = paperplane_app_window_new (PAPERPLANE_APP (app));

  for (i = 0; i < n_files; i++)
    paperplane_app_window_open (win, files[i]);

  gtk_window_present (GTK_WINDOW (win));
}

  static void
paperplane_app_class_init (PaperplaneAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = paperplane_app_activate;
  G_APPLICATION_CLASS (class)->open = paperplane_app_open;
}

  PaperplaneApp *
paperplane_app_new (void)
{
  return g_object_new (PAPERPLANE_APP_TYPE,
      "application-id", "org.gtk.paperplaneapp",
      "flags", G_APPLICATION_HANDLES_OPEN,
      NULL);
}
