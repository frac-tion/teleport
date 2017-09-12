/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003, Ximian, Inc.
 * Copyright (C) 2013 Igalia, S.L.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libsoup/soup.h>
#include "teleportapp.h"

static SoupSession *session;
static gboolean debug;

int saveFile (SoupMessage *, const gchar *, const gchar *);
gchar * getFilePath (const gchar *, const gchar *);

  static void
finished (SoupSession *session, SoupMessage *msg, gpointer target)
{
  //GVariant *target array: {originDevice, url, filename, downloadDirectory}
  saveFile(msg,
      (char *) g_variant_get_string (
        g_variant_get_child_value ((GVariant *) target, 3), NULL),
      (char *) g_variant_get_string (
        g_variant_get_child_value ((GVariant *) target, 2), NULL));

  create_finished_notification ((char *) g_variant_get_string (
        g_variant_get_child_value ((GVariant *) target, 0), NULL),
      0,
      g_variant_get_string (
        g_variant_get_child_value ((GVariant *) target, 2), NULL),
      target);
}

  int
get (char *url, const gchar *originDevice, const gchar *downloadDirectory, const gchar *outputFilename)
{
  GError *error = NULL;
  SoupLogger *logger = NULL;

  if (!soup_uri_new (url)) {
    g_printerr ("Could not parse '%s' as a URL\n", url);
    return (1);
  }

  session = g_object_new (SOUP_TYPE_SESSION,
      SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
      SOUP_SESSION_USER_AGENT, "teleport ",
      SOUP_SESSION_ACCEPT_LANGUAGE_AUTO, TRUE,
      NULL);

  if (debug) {
    logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
    soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));
    g_object_unref (logger);
  }

  SoupMessage *msg;
  const char *header;

  msg = soup_message_new ("GET", url);
  soup_message_set_flags (msg, SOUP_MESSAGE_NO_REDIRECT);

  g_object_ref (msg);
  //soup_session_queue_message (session, msg, finished, loop);
  GVariantBuilder *builder;
  GVariant *target;

  builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  g_variant_builder_add (builder, "s", originDevice);
  g_variant_builder_add (builder, "s", url);
  g_variant_builder_add (builder, "s", outputFilename);
  g_variant_builder_add (builder, "s", downloadDirectory);
  target = g_variant_new ("as", builder);
  g_variant_builder_unref (builder);

  soup_session_queue_message (session, msg, finished, target);

  return 0;
}

int saveFile (SoupMessage *msg, const gchar *outputDirectory, const gchar *outputFilename) {
  const char *name;
  FILE *output_file = NULL;

  //get (g_strdup(url), g_strdup_printf("./test_download/%s", g_uri_escape_string(filename, NULL, TRUE)));
  name = soup_message_get_uri (msg)->path;

  if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code))
    g_print ("%s: %d %s\n", name, msg->status_code, msg->reason_phrase);

  if (SOUP_STATUS_IS_REDIRECTION (msg->status_code)) {
    g_print ("%s: %d %s\n", name, msg->status_code, msg->reason_phrase);
  } else if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
    //if there is no file name and path the page will not get saved
    if (outputFilename == NULL) {
      //g_print ("%s: Got a file offered form a other peer. Will not save anything.\n", name);
    }
    else {
      output_file = fopen (getFilePath(outputDirectory, outputFilename), "w");

      if (!output_file)
        g_printerr ("Error trying to create file %s.\n", outputFilename);

      if (output_file) {
        fwrite (msg->response_body->data,
            1,
            msg->response_body->length,
            output_file);

        fclose (output_file);
      }
    }
  }

  return 0;
}

int do_client_notify (char *url)
{
  get (g_strdup(url), NULL, NULL, NULL);
  g_print("Offering selected file to other machine.\n");
  return 0;
}


gchar * getFilePath (const gchar *outputDirectory, const gchar *outputFilename) {
    return g_strdup_printf("%s%s", outputDirectory,
                           g_uri_escape_string(outputFilename, NULL, TRUE));
}
  int 
do_downloading (const char *originDevice, const char *url, const char *filename)
{
  gchar *outputDirectory = "./test_download/";
  g_print("Downloading %s to %s\n", url, g_uri_escape_string(filename, NULL, TRUE));
  get (g_strdup(url), originDevice, outputDirectory, filename);
  return 0;
}
