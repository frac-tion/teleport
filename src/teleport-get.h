#ifndef __TELEPORT_GET_H
#define __TELEPORT_GET_H

#include <libsoup/soup.h>

int teleport_get_do_downloading   (const gchar *,
                                   const gchar *,
                                   const gchar *);
int teleport_get_do_client_notify (const gchar *);

#endif /* __TELEPORT_GET_H */
