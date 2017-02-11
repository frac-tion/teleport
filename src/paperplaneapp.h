#ifndef __PAPERPLANEAPP_H
#define __PAPERPLANEAPP_H

#include <gtk/gtk.h>


#define PAPERPLANE_APP_TYPE (paperplane_app_get_type ())
G_DECLARE_FINAL_TYPE (PaperplaneApp, paperplane_app, PAPERPLANE, APP, GtkApplication)


PaperplaneApp     *paperplane_app_new         (void);


#endif /* __PAPERPLANEAPP_H */
