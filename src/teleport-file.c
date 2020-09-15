/* teleport-file.c
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
#include <glib.h>
#include <json-glib/json-glib.h>

#include <libsoup/soup.h>

#include "teleport-file.h"
#include "teleport-peer.h"
#include "enum-types.h"

struct _TeleportFile {
  GObject parent;

  gchar *id;
  gchar *source_path;
  gchar *destination_path;
  /* Size is 0 when this TeleportFile is a dicontary */
  gint64 size;

  gfloat progress;
  
  TeleportFileState state;

  SoupSession *session;
  SoupMessage *msg;

  /* Files is only set when this TeleportFile is a dicontary */ 
  /* TODO: allow sending files GList *subfiles; */
};

enum {
  PROP_0,
  PROP_ID,
  PROP_SOURCE_PATH,
  PROP_DESTINATION_PATH,
  PROP_SIZE,
  PROP_PROGRESS,
  PROP_STATE,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

G_DEFINE_TYPE (TeleportFile, teleport_file, G_TYPE_OBJECT)

static void
teleport_file_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  TeleportFile *self = TELEPORT_FILE (object);

  switch (property_id) {
  case PROP_ID:
    if (self->id != NULL)
      g_free (self->id);
    self->id = g_value_dup_string (value);
  case PROP_SOURCE_PATH:
    teleport_file_set_source_path (self, g_value_get_string (value));
    break;
  case PROP_DESTINATION_PATH:
    teleport_file_set_destination_path (self, g_value_get_string (value));
    break;
  case PROP_SIZE:
    teleport_file_set_size (self, g_value_get_int64 (value));
    break;
  case PROP_PROGRESS:
    teleport_file_set_progress (self, g_value_get_float (value));
    break;
  case PROP_STATE:
    teleport_file_set_state (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
teleport_file_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  TeleportFile *self = TELEPORT_FILE (object);

  switch (property_id) {
  case PROP_ID:
    g_value_set_string (value, teleport_file_get_id (self));
    break;
  case PROP_SOURCE_PATH:
    g_value_set_string (value, teleport_file_get_source_path (self));
    break;
  case PROP_DESTINATION_PATH:
    g_value_set_string (value, teleport_file_get_destination_path (self));
    break;
  case PROP_SIZE:
    g_value_set_int64 (value, teleport_file_get_size (self));
    break;
  case PROP_PROGRESS:
    g_value_set_float (value, teleport_file_get_progress (self));
    break;
  case PROP_STATE:
    g_value_set_enum (value, teleport_file_get_state (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
teleport_file_finalize (GObject *object)
{
  TeleportFile *self = TELEPORT_FILE (object);
  g_free (self->source_path);
  g_free (self->destination_path);
  g_free (self->id);
  
  G_OBJECT_CLASS (teleport_file_parent_class)->finalize (object);
} 

static void
teleport_file_class_init (TeleportFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = teleport_file_set_property;
  object_class->get_property = teleport_file_get_property;
  object_class->finalize = teleport_file_finalize;

  props[PROP_ID] =
   g_param_spec_string ("id",
                        "ID",
                        "The id of this file",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_SOURCE_PATH] =
   g_param_spec_string ("source_path",
                        "Source path",
                        "The path or url to the source file",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_DESTINATION_PATH] =
   g_param_spec_string ("destination_path",
                        "Destination path",
                        "The path to the destination of for this file",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_SIZE] =
   g_param_spec_int64 ("size",
                      "Size",
                      "The size of this file",
                      0, G_MAXINT64, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_PROGRESS] =
   g_param_spec_float ("progress",
                      "Progress",
                      "The download or upload progress",
                      0, 1.0, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_STATE] =
   g_param_spec_enum ("state",
                      "State",
                      "The state of this file",
                      TELEPORT_TYPE_FILE_STATE,
                      TELEPORT_FILE_STATE_NEW,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
teleport_file_init (TeleportFile *self)
{
  self->id = NULL;
}

TeleportFile *
teleport_file_new (const gchar *source_path,
                   const gchar *destination_path,
                   gint64 size) {
  return g_object_new (TELEPORT_TYPE_FILE,
                       "source_path", source_path,
                       "destination_path", destination_path,
                       "size", size,
                       NULL);
}

TeleportFile *
teleport_file_new_from_serialized (const gchar *data)
{
  g_autoptr(GError) error = NULL;
  GObject *self;

  self = json_gobject_from_data (TELEPORT_TYPE_FILE,
                                 data,
                                 -1,
                                 &error);

  if (error != NULL)
    g_warning ("Couldn't parse JSON: %s", error->message);

  return TELEPORT_FILE (self);
}

gchar *
teleport_file_serialize (TeleportFile *self)
{
  return json_gobject_to_data (G_OBJECT (self), NULL);
}

const gchar *
teleport_file_get_id (TeleportFile *self)
{
  g_return_val_if_fail (TELEPORT_IS_FILE (self), NULL);

  if (self->id == NULL)
    self->id = g_uuid_string_random ();

  return self->id;
}

const gchar *
teleport_file_get_destination_path (TeleportFile *self)
{
  g_return_val_if_fail (TELEPORT_IS_FILE (self), NULL);

  return self->destination_path;
}

void
teleport_file_set_destination_path (TeleportFile *self, const gchar *destination_path)
{
  g_return_if_fail (TELEPORT_IS_FILE (self));

  if (!g_strcmp0 (self->destination_path, destination_path))
    return;
  
  g_free (self->destination_path);
  self->destination_path = g_strdup (destination_path);
  
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DESTINATION_PATH]);
}

const gchar *
teleport_file_get_source_path (TeleportFile *self)
{
  g_return_val_if_fail (TELEPORT_IS_FILE (self), NULL);

  return self->source_path;
}

void
teleport_file_set_source_path (TeleportFile *self, const gchar *source_path)
{
  g_return_if_fail (TELEPORT_IS_FILE (self));

  if (!g_strcmp0 (self->source_path, source_path))
    return;
  
  g_free (self->source_path);
  self->source_path = g_strdup (source_path);
  
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SOURCE_PATH]);
}

gint64
teleport_file_get_size (TeleportFile *self)
{
  g_return_val_if_fail (TELEPORT_IS_FILE (self), 0);

  return self->size;
}

void
teleport_file_set_size (TeleportFile *self, gint64 size)
{
  g_return_if_fail (TELEPORT_IS_FILE (self));

  if (self->size == size)
    return;
  
  self->size = size;
  
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIZE]);
}

gfloat
teleport_file_get_progress (TeleportFile *self)
{
  g_return_val_if_fail (TELEPORT_IS_FILE (self), -1);

  return self->progress;
}

void
teleport_file_set_progress (TeleportFile *self, gfloat progress)
{
  g_return_if_fail (TELEPORT_IS_FILE (self));

  if (self->progress == progress)
    return;
  
  self->progress = progress;
  
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PROGRESS]);
}

TeleportFileState
teleport_file_get_state (TeleportFile *self)
{
  g_return_val_if_fail (TELEPORT_IS_FILE (self), TELEPORT_FILE_STATE_UNKNOWN);

  return self->state;
}

void
teleport_file_set_state (TeleportFile *self,
                         TeleportFileState state)
{
  g_return_if_fail (TELEPORT_IS_FILE (self));

  if (self->state == state)
    return;
  
  self->state = state;
  
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);
}

static void
got_chunk_cb (SoupMessage  *msg,
              SoupBuffer  *chunk,
              GIOStream *stream)
{
  g_return_if_fail (G_IS_IO_STREAM (stream));

  g_output_stream_write (g_io_stream_get_output_stream (stream),
                         chunk->data,
                         chunk->length,
                         NULL,
                         NULL);
}

static void
progress_cb (TeleportFile  *file,
             SoupBuffer  *chunk,
             SoupMessage *msg)
{
  gfloat progress;
  gfloat increment;

  progress = teleport_file_get_progress (file);
  increment = ((gfloat) chunk->length / (gfloat) teleport_file_get_size (file));
  teleport_file_set_progress (file, progress + increment);
}

static void
got_body_cb (SoupMessage  *msg,
             GIOStream *stream)
{
  g_return_if_fail (G_IS_IO_STREAM (stream));

  g_output_stream_close (g_io_stream_get_output_stream(stream), NULL, NULL);
  g_object_unref (stream);
}


static void
get_file_cb (SoupSession *session,
             SoupMessage *msg,
             TeleportFile *file)
{
  if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
    teleport_file_set_progress (file, 1.0);
    g_print ("Finished downloading file: %s\n", soup_status_get_phrase (msg->status_code));
    teleport_file_set_state (file, TELEPORT_FILE_STATE_FINISH);
  } else if (msg->status_code == SOUP_STATUS_CANCELLED) {
    teleport_file_set_state (file, TELEPORT_FILE_STATE_CANCELLED);
  } else {
    teleport_file_set_state (file, TELEPORT_FILE_STATE_ERROR);
  }

  g_clear_object (&file->msg);
}

void
teleport_file_download (TeleportFile *file, SoupSession *session, gchar *download_folder)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GFile) destination_file = NULL;
  GFileIOStream *io_stream = NULL;

  g_return_if_fail (TELEPORT_IS_FILE (file));
  g_return_if_fail (SOUP_IS_SESSION (session));

  destination_file = g_file_new_build_filename (download_folder, teleport_file_get_destination_path (file), NULL);
  io_stream = g_file_create_readwrite (destination_file, G_FILE_CREATE_PRIVATE, NULL, &error);
  if (error != NULL) {
    g_warning ("Couldn't create file: %s", error->message);
    /* TODO: show error to user and allow overriding files or rename */
    teleport_file_set_state (file, TELEPORT_FILE_STATE_ERROR);
    return;
  }

  file->msg = soup_message_new ("GET", teleport_file_get_source_path (file));
  soup_message_set_flags (file->msg, SOUP_MESSAGE_NO_REDIRECT);
  soup_message_body_set_accumulate (file->msg->response_body, FALSE);

  teleport_file_set_progress (file, 0.0);

  g_signal_connect (file->msg, "got-chunk", G_CALLBACK (got_chunk_cb), io_stream);
  g_signal_connect (file->msg, "got-body", G_CALLBACK (got_body_cb), io_stream);
  g_signal_connect_swapped (file->msg, "got-chunk", G_CALLBACK (progress_cb), file);

  teleport_file_set_state (file, TELEPORT_FILE_STATE_TRANSFAIR);
  soup_session_queue_message (session, g_object_ref (file->msg), (SoupSessionCallback) get_file_cb, file);
}

void
teleport_file_cancel_transfer (TeleportFile *file, SoupSession *session)
{
  g_return_if_fail (TELEPORT_IS_FILE (file));
  g_return_if_fail (SOUP_IS_SESSION (session));

  if (file->msg != NULL) {
    soup_session_cancel_message (session,
                                 file->msg,
                                 SOUP_STATUS_CANCELLED);
  }
}
