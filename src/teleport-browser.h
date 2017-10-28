#ifndef __TELEPORT_BROWSER_H
#define __TELEPORT_BROWSER_H

#include "teleport-peer.h"

int teleport_browser_run_avahi_service(TeleportPeer *);
void teleport_browser_avahi_shutdown(void);

#endif /* __TELEPORT_BROWSER_H */
