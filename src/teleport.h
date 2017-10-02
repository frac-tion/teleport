#ifndef __TELEPORTAPP_H
#define __TELEPORTAPP_H

#include <gtk/gtk.h>
#include "teleportpeer.h"

#define TELEPORT_APP_TYPE (teleport_app_get_type ())
G_DECLARE_FINAL_TYPE (TeleportApp, teleport_app, TELEPORT, APP, GtkApplication)


TeleportApp     *teleport_app_new             (void);
extern void     create_user_notification      (const char *,
                                               const int,
                                               const char *,
                                               GVariant *);

extern void     create_finished_notification (const char *,
                                              const int,
                                              const char *,
                                              GVariant *);

gboolean        mainLoopAddPeerCallback      (gpointer);
gboolean        mainLoopRemovePeerCallback   (gpointer);

void            callback_add_peer            (GObject *,
                                              Peer *,
                                              gpointer);
void            callback_remove_peer         (GObject *,
                                              Peer *peer,
                                              gpointer);
void            callback_notify_user         (GObject *,
                                              gchar *,
                                              gpointer);


#endif /* __TELEPORTAPP_H */
