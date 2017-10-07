#include <gtk/gtk.h>

#include "teleport-app.h"

int
main (int argc, char *argv[])
{
  return g_application_run (G_APPLICATION (teleport_app_new ()), argc, argv);
}
