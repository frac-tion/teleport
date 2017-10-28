/* teleport-browser.c
 *
 * Copyright 2017 Julian Sparber <julian@sparber.com>
 *
 * Teleport is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/thread-watch.h>

#include "teleport-browser.h"
#include "teleport-app.h"
#include "teleport-peer.h"

static AvahiThreadedPoll *threaded_poll = NULL;
static AvahiClient *client = NULL;
static TeleportPeer *peerList = NULL;

static void
resolve_callback (AvahiServiceResolver *r,
                  AVAHI_GCC_UNUSED AvahiIfIndex interface,
                  AVAHI_GCC_UNUSED AvahiProtocol protocol,
                  AvahiResolverEvent event,
                  const char *name,
                  const char *type,
                  const char *domain,
                  const char *host_name,
                  const AvahiAddress *address,
                  uint16_t port,
                  AvahiStringList *txt,
                  AvahiLookupResultFlags flags,
                  AVAHI_GCC_UNUSED void* userdata)
{
  assert(r);
  /* Called whenever a service has been resolved successfully or timed out */
  switch (event) {
  case AVAHI_RESOLVER_FAILURE:
    fprintf(stderr,
            "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n",
            name,
            type,
            domain,
            avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
    break;
  case AVAHI_RESOLVER_FOUND: {
                               char a[AVAHI_ADDRESS_STR_MAX], *t;
                               fprintf(stderr,
                                       "Service '%s' of type '%s' in domain '%s':\n",
                                       name,
                                       type,
                                       domain);
                               avahi_address_snprint(a, sizeof(a), address);
                               t = avahi_string_list_to_string(txt);
                               teleport_peer_add_peer(peerList, g_strdup(name), g_strdup(a), port);
                               avahi_free(t);
                             }
  }

  avahi_service_resolver_free(r);
}

static void
browse_callback(
                AvahiServiceBrowser *b,
                AvahiIfIndex interface,
                AvahiProtocol protocol,
                AvahiBrowserEvent event,
                const char *name,
                const char *type,
                const char *domain,
                AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                void* userdata)
{
  AvahiClient *c = userdata;
  assert(b);

  /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
  switch (event) {
  case AVAHI_BROWSER_FAILURE:
    fprintf(stderr,
            "(Browser) %s\n",
            avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
    teleport_browser_avahi_shutdown();
    return;
  case AVAHI_BROWSER_NEW:
    fprintf(stderr,
            "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n",
            name,
            type,
            domain);
    if (flags & AVAHI_LOOKUP_RESULT_LOCAL)
      break;
    /* We ignore the returned resolver object. In the callback
       function we free it. If the server is terminated before
       the callback function is called the server will free
       the resolver for us. */
    if (!(avahi_service_resolver_new(c,
                                     interface,
                                     protocol,
                                     name,
                                     type,
                                     domain,
                                     AVAHI_PROTO_UNSPEC,
                                     0,
                                     resolve_callback,
                                     c)))

      fprintf(stderr,
              "Failed to resolve service '%s': %s\n",
              name,
              avahi_strerror(avahi_client_errno(c)));
    break;
  case AVAHI_BROWSER_REMOVE:
    fprintf(stderr,
            "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n",
            name,
            type,
            domain);
    teleport_peer_remove_peer_by_name(peerList, g_strdup(name));
    break;
  case AVAHI_BROWSER_ALL_FOR_NOW:
  case AVAHI_BROWSER_CACHE_EXHAUSTED:
    fprintf(stderr,
            "(Browser) %s\n",
            event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
    break;
  }
}

static void
client_callback (AvahiClient *c,
                 AvahiClientState state,
                 AVAHI_GCC_UNUSED void * userdata)
{
  assert(c);
  /* Called whenever the client or server state changes */
  if (state == AVAHI_CLIENT_FAILURE) {
    fprintf(stderr,
            "Server connection failure: %s\n",
            avahi_strerror(avahi_client_errno(c)));
    teleport_browser_avahi_shutdown();
  }
}

int
teleport_browser_run_avahi_service (TeleportPeer *peers)
{
  int error;
  peerList = peers;

  /* Call this when the application starts up. */

  if (!(threaded_poll = avahi_threaded_poll_new())) {
    /* do something bad */
    return 1;
  }

  if (!(client = avahi_client_new(avahi_threaded_poll_get(threaded_poll),
                                  0,
                                  client_callback,
                                  NULL,
                                  &error))) {
    fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
    /* do something bad */
    return 1;
  }

  /* create some browsers on the client object here, if you wish */
  if (!(avahi_service_browser_new(client,
                                  AVAHI_IF_UNSPEC,
                                  AVAHI_PROTO_INET,
                                  "_teleport._tcp",
                                  NULL,
                                  0,
                                  browse_callback,
                                  client))) {
    /* so something bad */
    return 1;
  }

  /* Finally, start the event loop thread */
  if (avahi_threaded_poll_start(threaded_poll) < 0) {
    /* do something bad */
    return 1;
  }
  return 0;
}

void
teleport_browser_avahi_shutdown(void)
{
  /* Call this when the app shuts down */

  avahi_threaded_poll_stop(threaded_poll);
  avahi_client_free(client);
  avahi_threaded_poll_free(threaded_poll);
}
