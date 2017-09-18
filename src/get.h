#ifndef __GET_H
#define __GET_H


extern int do_downloading(const char *, const char *, const char *);
extern int do_client_notify(char *);

int saveFile (SoupMessage *, const gchar *, const gchar *);
gchar * getFilePath (const gchar *, const gchar *);

#endif /* __GET_H */
