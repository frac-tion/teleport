#include <gtk/gtk.h>

#include "teleport-app.h"

int
main (int argc, char *argv[])
{
  if (g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD) == NULL) {
    g_print("Update XDG user dirs\n");
    g_spawn_command_line_sync("xdg-user-dirs-update", NULL, NULL, NULL, NULL);
  }
  g_print("Download dir: %s\n", g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));

  return g_application_run (G_APPLICATION (teleport_app_new ()), argc, argv);
}
