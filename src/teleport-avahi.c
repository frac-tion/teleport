/* teleport-browser.c
 *
 * Copyright 2020 Julian Sparber <julian@sparber.net>
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

#include <glib-object.h>
#include <avahi-gobject/ga-error.h>
#include <avahi-gobject/ga-client.h>
#include <avahi-gobject/ga-entry-group.h>
#include <avahi-gobject/ga-service-browser.h>
#include <avahi-gobject/ga-service-resolver.h>

#include "teleport-avahi.h"
#include "teleport-peer.h"
#include "enum-types.h"

struct _TeleportAvahi
{
  GObject parent;

  GaClient *client;
  GaEntryGroup *entry_group;
  GaServiceBrowser *browser;
  GHashTable *devices;
  guint watcher_id;

  gchar *name;
  guint16 port;
  TeleportAvahiState state;
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_PORT,
  PROP_STATE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

enum {                       
  NEW_DEVICE,
  DEVICE_DISAPPEARED,
  N_SIGNALS
};

static gint signals [N_SIGNALS];

G_DEFINE_TYPE (TeleportAvahi, teleport_avahi, G_TYPE_OBJECT)

static void
set_state (TeleportAvahi *self,
           TeleportAvahiState state)
{
  if (self->state == state)
    return;

  self->state = state;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);
}

static void
service_resolver_found_cb (GaServiceResolver *resolver,
                           AvahiIfIndex instance,
                           AvahiProtocol *protocol,
                           const gchar *name,
                           const gchar *type,
                           const gchar *domain,
                           const gchar *host_name,
                           const AvahiAddress *address,
                           uint16_t port,
                           AvahiStringList *txt,
                           GaLookupResultFlags *flags,
                           TeleportAvahi *self)
{
  TeleportPeer *peer;
  char a[AVAHI_ADDRESS_STR_MAX];

  /* FIXME: this function requires avahi-client as dependency */
  avahi_address_snprint (a, sizeof(a), address);

  g_debug ("Found service at %s:%d with name %s\n", a, port, name);

  if (!g_hash_table_contains (self->devices, name)) {
    peer = teleport_peer_new (name, a, port);
    g_hash_table_insert (self->devices, g_strdup (name), peer);
    g_signal_emit (self, signals[NEW_DEVICE], 0, peer);
  }
  g_object_unref (resolver);
}

static void
service_resolver_failure_cb (GaServiceResolver *resolver,
                             GError *error)
{
  if (error != NULL) {
    g_warning ("Error resolving service: %s", error->message);
  }
  g_object_unref (resolver);
}

static void
new_service_cb (GaServiceBrowser *browser,
                AvahiIfIndex instance,
                AvahiProtocol protocol,
                const gchar *name,
                const gchar *type,
                const gchar *domain,
                GaLookupResultFlags *flags,
                TeleportAvahi *self)
{
  g_autoptr (GError) error = NULL;
  GaServiceResolver *resolver;

  if (g_strcmp0 (self->name, name) == 0)
    return;

  if (g_hash_table_contains (self->devices, name))
    return;

  resolver = ga_service_resolver_new (instance,
                                      protocol,
                                      name,
                                      type,
                                      domain,
                                      GA_PROTOCOL_INET, /* Only lookup IPv4 */
                                      GA_LOOKUP_NO_TXT); /* Don't lookup the txt */

  g_signal_connect (resolver, "found", G_CALLBACK (service_resolver_found_cb), self); 
  g_signal_connect (resolver, "failure", G_CALLBACK (service_resolver_failure_cb), self); 
  ga_service_resolver_attach (resolver, self->client, &error);

  if (error != NULL) {
    g_warning ("Couldn't resolve the service: %s", error->message);
  }
}

static void
removed_service_cb (GaServiceBrowser *browser,
                    AvahiIfIndex instance,
                    AvahiProtocol protocol,
                    const gchar *name,
                    const gchar *type,
                    const gchar *domain,
                    GaLookupResultFlags *flags,
                    TeleportAvahi *self)
{
  gpointer peer;
  gpointer key;
  g_debug ("Remove service with name: %s\n", name);
  if (g_hash_table_steal_extended (self->devices, name,  &key, &peer)) {
    g_signal_emit (self, signals[DEVICE_DISAPPEARED], 0, peer);
    g_object_unref (peer);
    g_free ((gchar *) key);
  }
}

static void
teleport_avahi_browse (TeleportAvahi *self)
{
  g_autoptr (GError) error = NULL;
  GaClientState state;

  g_return_if_fail (TELEPORT_IS_AVAHI (self));

  if (self->client == NULL)
    return;

  g_object_get (self->client,
                "state", &state,
                NULL);

  if (state != GA_CLIENT_STATE_S_RUNNING)
    return;

  self->browser = ga_service_browser_new_full (AVAHI_IF_UNSPEC, 
                                               GA_PROTOCOL_INET,
                                               "_teleport._tcp",
                                               NULL,
                                               GA_LOOKUP_NO_FLAGS);
  ga_service_browser_attach (self->browser, self->client, &error);

  if (error != NULL) {
    g_warning ("Can't start looking for other teleport instances: %s", error->message);
    return;
  }

  g_signal_connect (self->browser, "new-service", G_CALLBACK (new_service_cb), self); 
  g_signal_connect (self->browser, "removed-service", G_CALLBACK (removed_service_cb), self); 
}

static void
teleport_avahi_publish (TeleportAvahi *self)
{
  g_autoptr (GError) error = NULL;
  GaClientState state;

  g_return_if_fail (TELEPORT_IS_AVAHI (self));

  if (self->client == NULL)
    return;

  g_object_get (self->client,
                "state", &state,
                NULL);

  if (state != GA_CLIENT_STATE_S_RUNNING)
    return;

  if (self->entry_group == NULL) {
    self->entry_group = ga_entry_group_new ();
    ga_entry_group_attach (self->entry_group, self->client, &error);

    if (error != NULL) {
      g_warning ("Unable to publish a new avahi service: %s", error->message);
      return;
    }
  } else {
    ga_entry_group_reset (self->entry_group, &error);

    if (error != NULL) {
      g_warning ("Couldn't reset the published service %s", error->message);
      return;
    }
  }

  ga_entry_group_add_service (self->entry_group, self->name, "_teleport._tcp", self->port, &error, NULL);

  if (error != NULL) {
    g_warning ("Unable to publish a new avahi service: %s", error->message);
    return;
  }

  ga_entry_group_commit (self->entry_group, &error);

  if (error != NULL) {
    g_warning ("Unable to publish a new avahi service: %s", error->message);
    return;
  }
}

static void
client_state_changed_cb (GaClient *client,
                         GaClientState state,
                         TeleportAvahi *self)
{
  switch (state) {
  case GA_CLIENT_STATE_NOT_STARTED:
  case GA_CLIENT_STATE_S_REGISTERING:
  case GA_CLIENT_STATE_CONNECTING:
    break;
  case GA_CLIENT_STATE_S_RUNNING:
    teleport_avahi_publish (self);
    teleport_avahi_browse (self);
    set_state (self, TELEPORT_AVAHI_STATE_RUNNING);
    break;
  case GA_CLIENT_STATE_S_COLLISION:
    set_state (self, TELEPORT_AVAHI_STATE_NAME_COLLISION);
    break;
  case GA_CLIENT_STATE_FAILURE:
    set_state (self, TELEPORT_AVAHI_STATE_UNKNOWN);
    break;
  }
}

static void
on_avahi_appeared (GDBusConnection *connection,
                  const gchar      *name,
                  const gchar      *name_owner,
                  TeleportAvahi    *self)
{
  g_autoptr (GError) error = NULL;

  self->client = ga_client_new (GA_CLIENT_FLAG_NO_FLAGS);
  g_signal_connect (self->client, "state-changed", G_CALLBACK (client_state_changed_cb), self);

  ga_client_start (self->client, &error);

  if (error != NULL) {
    g_warning ("Couldn't connect to Avahi service: %s", error->message);
    g_clear_object (&self->client);
    if (error->code == AVAHI_ERR_NO_DAEMON) {
      set_state (self, TELEPORT_AVAHI_STATE_NO_DEAMON);
    }
    return;
  }

  set_state (self, TELEPORT_AVAHI_STATE_NEW);

}

static void
cleanup_client_data (TeleportAvahi *self)
{
  g_clear_object (&self->client);
  g_clear_object (&self->entry_group);
  g_clear_object (&self->browser);
}

static gboolean
device_remove_cb (gpointer key,
                  TeleportPeer *peer,
                  TeleportAvahi *self)
{
  g_signal_emit (self, signals[DEVICE_DISAPPEARED], 0, peer);
  return TRUE;
}

static void
on_avahi_vanished (GDBusConnection *connection,
                   const gchar      *name,
                   TeleportAvahi    *self)
{
  cleanup_client_data (self);
  g_hash_table_foreach_remove (self->devices, (GHRFunc) device_remove_cb, self);
  set_state (self, TELEPORT_AVAHI_STATE_NO_DEAMON);
}

static void
teleport_avahi_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  TeleportAvahi *self = TELEPORT_AVAHI (object);

  switch (property_id) {
  case PROP_NAME:
    teleport_avahi_set_name (self, g_value_get_string (value));
    break;
  case PROP_PORT:
    self->port = g_value_get_uint (value);
    break;
  case PROP_STATE:
    self->state = g_value_get_enum (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
teleport_avahi_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  TeleportAvahi *self = TELEPORT_AVAHI (object);

  switch (property_id) {
  case PROP_NAME:
    g_value_set_string (value, self->name);
    break;
  case PROP_PORT:
    g_value_set_uint (value, self->port);
    break;
  case PROP_STATE:
    g_value_set_enum (value, teleport_avahi_get_state (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}
static void
teleport_avahi_dispose (GObject *object)
{
  TeleportAvahi *self = TELEPORT_AVAHI (object);

  g_bus_unwatch_name (self->watcher_id);
  cleanup_client_data (self);
  g_hash_table_destroy (self->devices);
  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (teleport_avahi_parent_class)->dispose (object);
}

static void
teleport_avahi_class_init (TeleportAvahiClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = teleport_avahi_dispose;
  object_class->get_property = teleport_avahi_get_property;
  object_class->set_property = teleport_avahi_set_property;

  signals[NEW_DEVICE] = g_signal_new ("new-device",
                                      G_TYPE_OBJECT,
                                      G_SIGNAL_RUN_LAST,
                                      0,
                                      NULL /* accumulator */,
                                      NULL /* accumulator data */,
                                      NULL /* C marshaller */,
                                      G_TYPE_NONE /* return_type */,
                                      1,
                                      TELEPORT_TYPE_PEER);

  signals[DEVICE_DISAPPEARED] = g_signal_new ("device-disappeared",
                                              G_TYPE_OBJECT,
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL /* accumulator */,
                                              NULL /* accumulator data */,
                                              NULL /* C marshaller */,
                                              G_TYPE_NONE /* return_type */,
                                              1,
                                              TELEPORT_TYPE_PEER);
  props[PROP_NAME] =
   g_param_spec_string ("name",
                        "Name",
                        "The name of the published device",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_PORT] =
   g_param_spec_uint ("port",
                      "Port",
                      "The port used by the published device",
                      0, G_MAXINT16, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

   props[PROP_STATE] =
   g_param_spec_enum ("state",
                      "State",
                      "The state of avahi",
                      TELEPORT_TYPE_AVAHI_STATE,
                      TELEPORT_AVAHI_STATE_UNKNOWN,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
teleport_avahi_init (TeleportAvahi *self)
{
  self->client = NULL;
  self->entry_group = NULL;
  self->browser = NULL;
  self->name = NULL;
  self->devices = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

}

TeleportAvahi *
teleport_avahi_new (const gchar *name,
                    guint16 port)
{
  return g_object_new (TELEPORT_TYPE_AVAHI,
                       "name", name,
                       "port", port,
                       NULL);
}

void
teleport_avahi_start (TeleportAvahi *self)
{
  self->watcher_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                       "org.freedesktop.Avahi",
                                       G_BUS_NAME_WATCHER_FLAGS_NONE,
                                       (GBusNameAppearedCallback) on_avahi_appeared,
                                       (GBusNameVanishedCallback) on_avahi_vanished,
                                       self,
                                       NULL);
}

void
teleport_avahi_set_name (TeleportAvahi *self,
                         const gchar *name)
{
  g_return_if_fail (TELEPORT_IS_AVAHI (self));

  if (name == NULL)
    return;

  if (g_strcmp0 (self->name, name) == 0)
    return;

  g_clear_pointer (&self->name, g_free);
  self->name = g_strdup (name);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NAME]);

  teleport_avahi_publish (self);
}

TeleportAvahiState
teleport_avahi_get_state (TeleportAvahi *self)
{
  g_return_val_if_fail (TELEPORT_IS_AVAHI (self), TELEPORT_AVAHI_STATE_UNKNOWN);

  return self->state;
}
