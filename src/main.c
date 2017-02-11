#include <gtk/gtk.h>

#include "paperplaneapp.h"

int
main (int argc, char *argv[])
{
  return g_application_run (G_APPLICATION (paperplane_app_new ()), argc, argv);
}
