#ifndef __TELEPORT_PEER_H
#define __TELEPORT_PEER_H

#include <gtk/gtk.h>

#define TELEPORT_TYPE_PEER teleport_peer_get_type ()
G_DECLARE_FINAL_TYPE (TeleportPeer, teleport_peer, TELEPORT, PEER, GObject)

typedef struct Peers {
  char *name;
  char *ip;
  gint port;
} Peer;


gchar* teleport_peer_get_name (TeleportPeer  *self, gint index, GError **error);
gchar* teleport_peer_get_ip (TeleportPeer  *self, gint index, GError **error);
gint teleport_peer_get_port (TeleportPeer  *self, gint index, GError **error);
void teleport_peer_add_peer (TeleportPeer *self, gchar * name, gchar * ip, gint port);
void teleport_peer_remove_peer (TeleportPeer *, Peer *);
void teleport_peer_remove_peer_by_name (TeleportPeer *, gchar *);

#endif /* __TELEPORT_PEER_H */
