#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libsoup/soup.h>
#include "teleport-app.h"
#include "teleport-window.h"
#include "teleport-get.h"

static int saveFile (SoupMessage *, const gchar *, const gchar *);
static gchar * getFilePath (const gchar *, const gchar *);
static int get (const gchar *, const gchar *, const gchar *, const gchar *);

static gboolean debug;

static void
finished (SoupSession *session,
          SoupMessage *msg,
          gpointer target)
{
  //GVariant *target array: {originDevice, url, filename, downloadDirectory}
  if ((char *) g_variant_get_string (
                                     g_variant_get_child_value ((GVariant *) target, 2), NULL) != NULL) {
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
}

static int
get (const gchar *url,
     const gchar *originDevice,
     const gchar *downloadDirectory,
     const gchar *outputFilename) {
  SoupSession *session;
  SoupLogger *logger = NULL;
  SoupMessage *msg;

  if (!soup_uri_new (url)) {
    g_printerr ("Could not parse '%s' as a URL\n", url);
    return (1);
  }

  session = g_object_new (SOUP_TYPE_SESSION,
                          SOUP_SESSION_ADD_FEATURE_BY_TYPE,
                          SOUP_TYPE_CONTENT_DECODER,
                          SOUP_SESSION_USER_AGENT,
                          "teleport ",
                          SOUP_SESSION_ACCEPT_LANGUAGE_AUTO,
                          TRUE,
                          NULL);

  if (debug) {
    logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
    soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));
    g_object_unref (logger);
  }

  msg = soup_message_new ("GET", url);
  soup_message_set_flags (msg, SOUP_MESSAGE_NO_REDIRECT);

  g_object_ref (msg);
  //soup_session_queue_message (session, msg, finished, loop);
  if (outputFilename == NULL) {
    soup_session_queue_message (session, msg, NULL, NULL);
  }
  else {
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
  }

  return 0;
}

static int 
saveFile (SoupMessage *msg,
          const gchar *outputDirectory,
          const gchar *outputFilename) {
  const char *name;
  FILE *outputFile = NULL;

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
      outputFile = fopen (getFilePath(outputDirectory, outputFilename), "w");

      if (!outputFile)
        g_printerr ("Error trying to create file %s.\n", outputFilename);

      if (outputFile) {
        fwrite (msg->response_body->data,
                1,
                msg->response_body->length,
                outputFile);

        fclose (outputFile);
      }
    }
  }
  return 0;
}


static gchar *
getFilePath (const gchar *outputDirectory,
             const gchar *outputFilename) {
  return g_strdup_printf("%s/%s", outputDirectory,
                         g_uri_escape_string(outputFilename, NULL, TRUE));
}

int
teleport_get_do_client_notify (const gchar *url) {
  get (g_strdup(url), NULL, NULL, NULL);
  g_print("Offering selected file to other machine.\n");
  return 0;
}

int 
teleport_get_do_downloading (const char *originDevice,
                             const char *url,
                             const char *filename) {
  const gchar *outputDirectory = teleport_get_download_directory();
  g_print("Downloading %s to %s\n", url, g_uri_escape_string(filename, NULL, TRUE));
  get (g_strdup(url), originDevice, outputDirectory, filename);
  return 0;
}
