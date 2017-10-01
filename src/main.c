#include <gtk/gtk.h>

#include "teleportapp.h"

int
main (int argc, char *argv[])
{
  if (g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD) == NULL) {
    g_spawn_command_line_sync("xdg-user-dirs-update", NULL, NULL, NULL, NULL);
  }

  return g_application_run (G_APPLICATION (teleport_app_new ()), argc, argv);
}
