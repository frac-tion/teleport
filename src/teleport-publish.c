#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include <avahi-common/thread-watch.h>

#include "teleport-app.h"
#include "teleport-peer.h"
#include "teleport-publish.h"

static AvahiThreadedPoll *threaded_poll = NULL;
static AvahiEntryGroup *group = NULL;
static AvahiClient *client = NULL;
//static AvahiSimplePoll *simple_poll = NULL;

static char *name = NULL;

static void create_services(AvahiClient *c);

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
  assert(g == group || group == NULL);
  group = g;

  /* Called whenever the entry group state changes */

  switch (state) {
  case AVAHI_ENTRY_GROUP_ESTABLISHED :
    /* The entry group has been established successfully */
    fprintf(stderr, "Service '%s' successfully established.\n", name);
    break;

  case AVAHI_ENTRY_GROUP_COLLISION : {
                                       char *n;

                                       /* A service name collision with a remote service
                                        * happened. Let's pick a new name */
                                       n = avahi_alternative_service_name(name);
                                       avahi_free(name);
                                       name = n;

                                       fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

                                       /* And recreate the services */
                                       create_services(avahi_entry_group_get_client(g));
                                       break;
                                     }

  case AVAHI_ENTRY_GROUP_FAILURE :

                                   fprintf(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

                                   /* Some kind of failure happened while we were registering our services */
                                   //avahi_simple_poll_quit(simple_poll);
                                   shutdown_avahi_publish_service();
                                   break;

  case AVAHI_ENTRY_GROUP_UNCOMMITED:
  case AVAHI_ENTRY_GROUP_REGISTERING:
                                   ;
  }
}

static void create_services(AvahiClient *c) {
  char *n, r[128];
  int ret;
  assert(c);

  /* If this is the first time we're called, let's create a new
   * entry group if necessary */

  if (!group)
    if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
      fprintf(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
      goto fail;
    }

  /* If the group is empty (either because it was just created, or
   * because it was reset previously, add our entries.  */

  if (avahi_entry_group_is_empty(group)) {
    fprintf(stderr, "Adding service '%s'\n", name);

    /* Create some random TXT data */
    snprintf(r, sizeof(r), "random=%i", rand());

    /* We will now add two services and one subtype to the entry
     * group. The two services have the same name, but differ in
     * the service type (IPP vs. BSD LPR). Only services with the
     * same name should be put in the same entry group. */

    /* Add the service for Teleport */
    if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name, "_http._tcp", NULL, NULL, 3000, "test=blah", r, NULL)) < 0) {

      if (ret == AVAHI_ERR_COLLISION)
        goto collision;

      fprintf(stderr, "Failed to add _http._tcp service: %s\n", avahi_strerror(ret));
      goto fail;
    }

    /* Tell the server to register the service */
    if ((ret = avahi_entry_group_commit(group)) < 0) {
      fprintf(stderr, "Failed to commit entry group: %s\n", avahi_strerror(ret));
      goto fail;
    }
  }

  return;

collision:

  /* A service name collision with a local service happened. Let's
   * pick a new name */
  n = avahi_alternative_service_name(name);
  avahi_free(name);
  name = n;

  fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

  avahi_entry_group_reset(group);

  create_services(c);
  return;

fail:
  //avahi_simple_poll_quit(simple_poll);
  shutdown_avahi_publish_service();
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
  assert(c);

  /* Called whenever the client or server state changes */

  switch (state) {
  case AVAHI_CLIENT_S_RUNNING:

    /* The server has startup successfully and registered its host
     * name on the network, so it's time to create our services */
    create_services(c);
    break;

  case AVAHI_CLIENT_FAILURE:

    fprintf(stderr, "Client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
    //avahi_simple_poll_quit(simple_poll);
    shutdown_avahi_publish_service();

    break;

  case AVAHI_CLIENT_S_COLLISION:

    /* Let's drop our registered services. When the server is back
     * in AVAHI_SERVER_RUNNING state we will register them
     * again with the new host name. */

  case AVAHI_CLIENT_S_REGISTERING:

    /* The server records are now being established. This
     * might be caused by a host name change. We need to wait
     * for our own records to register until the host name is
     * properly esatblished. */

    if (group)
      avahi_entry_group_reset(group);

    break;

  case AVAHI_CLIENT_CONNECTING:
    ;
  }
}

extern void update_service(char * service_name) {
  avahi_free(name);
  name = avahi_strdup(service_name);

  /* If the server is currently running, we need to remove our
   * service and create it anew */
  if (avahi_client_get_state(client) == AVAHI_CLIENT_S_RUNNING) {

    /* Remove the old services */
    if (group)
      avahi_entry_group_reset(group);

    /* And create them again with the new name */
    create_services(client);
  }
}

int run_avahi_publish_service(char * service_name) {
  int error;

  if (!(threaded_poll = avahi_threaded_poll_new())) {
    fprintf(stderr, "Failed to create threaded poll object.\n");
    return 1;
  }

  name = avahi_strdup(service_name);

  /* Allocate a new client */

  if (!(client = avahi_client_new(avahi_threaded_poll_get(threaded_poll), 0, client_callback, NULL, &error))) {
    fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
    return 1;
  }

  if (avahi_threaded_poll_start(threaded_poll) < 0) {
    return 1;
  }

  return 0;
}

void shutdown_avahi_publish_service(void) {
  /* Call this when the app shuts down */

  avahi_threaded_poll_stop(threaded_poll);
  avahi_client_free(client);
  avahi_threaded_poll_free(threaded_poll);
  avahi_free(name);
}
