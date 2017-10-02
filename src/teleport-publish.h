#ifndef __TELEPORT_PUBLISH_H
#define __TELEPORT_PUBLISH_H


extern int run_avahi_publish_service(char *);
extern void shutdown_avahi_publish_service(void);
extern void update_service(char *);

#endif /* __TELEPORT_PUBLISH_H */
