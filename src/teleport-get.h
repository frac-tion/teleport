#ifndef __TELEPORT_GET_H
#define __TELEPORT_GET_H

#include <libsoup/soup.h>

extern int do_downloading(const char *, const char *, const char *);
extern int do_client_notify(char *);

int saveFile (SoupMessage *, const gchar *, const gchar *);
gchar * getFilePath (const gchar *, const gchar *);
int get (char *, const gchar *, const gchar *, const gchar *);

#endif /* __TELEPORT_GET_H */
