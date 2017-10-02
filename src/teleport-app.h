#ifndef __TELEPORT_APP_H
#define __TELEPORT_APP_H

#include <gtk/gtk.h>
#include "teleport-peer.h"

#define TELEPORT_APP_TYPE (teleport_app_get_type ())
G_DECLARE_FINAL_TYPE (TeleportApp, teleport_app, TELEPORT, APP, GtkApplication)


TeleportApp     *teleport_app_new            (void);
void            create_user_notification     (const char *,
                                              const int,
                                              const char *,
                                              GVariant *);

void            create_finished_notification (const char *,
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


#endif /* __TELEPORT_APP_H */
