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

static SoupSession *session;
static GMainLoop *loop;
static gboolean debug;

  static void
finished (SoupSession *session, SoupMessage *msg, gpointer loop)
{
  g_main_loop_quit (loop);
}

  int
get (char *url, const gchar *output_file_path)
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

  loop = g_main_loop_new (NULL, TRUE);

  SoupMessage *msg;
  const char *header;
  const char *name;
  FILE *output_file = NULL;

  msg = soup_message_new ("GET", url);
  soup_message_set_flags (msg, SOUP_MESSAGE_NO_REDIRECT);

  g_object_ref (msg);
  soup_session_queue_message (session, msg, finished, loop);
  // soup_session_queue_message (session, msg, finished, NULL);
  g_main_loop_run (loop);

  name = soup_message_get_uri (msg)->path;

  if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code))
    g_print ("%s: %d %s\n", name, msg->status_code, msg->reason_phrase);

  if (SOUP_STATUS_IS_REDIRECTION (msg->status_code)) {
    g_print ("%s: %d %s\n", name, msg->status_code, msg->reason_phrase);
  } else if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
    //if there is no file name and path the page will not get saved
    if (output_file_path == NULL) {
      g_print ("Got a file offered form a other peer. Will not save anything.\n");
    }
    else {
      output_file = fopen (output_file_path, "w");
      if (!output_file)
        g_printerr ("Error trying to create file %s.\n", output_file_path);

      if (output_file) {
        fwrite (msg->response_body->data,
            1,
            msg->response_body->length,
            output_file);

        fclose (output_file);
      }
    }
  }

  g_main_loop_unref (loop);

  return 0;
}

int do_client_notify (char * url)
{
  get (g_strdup(url), NULL);
  return 0;
}

int do_downloading (char * url, char * file_name)
{
  g_print("Downloading url %s\n", url);
  //get (g_strdup(url),"./test_download");
  //get ("http://juliansparber.com/index.html", "./test_download");
  return 0;
}


/*int main () {
  get ("http://juliansparber.com/index.html", "./test_download");
  return 0;
  }
  */
