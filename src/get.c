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
//finished (SoupSession *session, SoupMessage *msg, gpointer loop)
finished (GObject *session, GAsyncResult *res, gpointer loop)
{
  SoupMessage *msg = g_async_result_get_user_data(res);
  g_print("Output %s", soup_message_get_uri (msg)->path);
  //g_main_loop_quit (loop);
}

  char* 
concat(const char *s1, const char *s2)
{
  char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
  //in real code you would check for errors in malloc here
  strcpy(result, s1);
  strcat(result, s2);
  return result;
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

  const char *name;
  SoupMessage *msg;
  const char *header;
  FILE *output_file = NULL;

  msg = soup_message_new ("GET", url);
  soup_message_set_flags (msg, SOUP_MESSAGE_NO_REDIRECT);

  g_object_ref (msg);
  //instate of NULL it should have a gcancellable object
  soup_session_send_async (session, msg, NULL, finished, loop);
  g_main_loop_run (loop);

  name = soup_message_get_uri (msg)->path;

  if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code))
    g_print ("%s: %d %s\n", name, msg->status_code, msg->reason_phrase);

  if (SOUP_STATUS_IS_REDIRECTION (msg->status_code)) {
      g_print ("%s: %d %s\n", name, msg->status_code, msg->reason_phrase);
  } else if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
    output_file = fopen (concat(output_file_path, name), "w");
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

  g_main_loop_unref (loop);

  return 0;
}

int main ()
{
  get ("http://juliansparber.com/index.html", "./test_download");
  return 0;
}
