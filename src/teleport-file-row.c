/* teleport-file-row.c
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

#include <gtk/gtk.h>
#include "teleport-file-row.h"
#include "teleport-file.h"
#include "enum-types.h"

enum                                                                            
{                                                                               
  PROP_0,                                                                       
  PROP_FILE,                                                                    
  LAST_PROP                                                                     
}; 

struct _TeleportFileRow
{
  GtkListBoxRow  parent;

  GtkWidget     *button_revealer;
  GtkWidget 	*button_stack;
  GtkWidget 	*file_label;
  GtkWidget 	*progressbar;
  GtkWidget 	*save_button;
  GtkWidget     *abort_button;
  GtkWidget     *open_button;

  /* data */
  TeleportFile  *file;
};

G_DEFINE_TYPE (TeleportFileRow, teleport_file_row, GTK_TYPE_LIST_BOX_ROW)


static void
update_state_cb (TeleportFileRow *self,
                 GParamSpec  *pspec,
                 TeleportFile *file)
{
  g_autofree gchar *label = NULL;
  GtkWidget *button = NULL;
  gboolean show_progressbar = FALSE;

  g_print ("Update_file state \n");

  switch (teleport_file_get_state (file)) {
  case TELEPORT_FILE_STATE_NEW:
    button = self->save_button;
    label = g_strdup_printf ("Received \"%s\"", teleport_file_get_destination_path (file));
    break;
  case TELEPORT_FILE_STATE_DESTINATION_ERROR:
    button = self->save_button;
    label = g_strdup_printf ("File \"%s\" already exists", teleport_file_get_destination_path (file));
    break;
  case TELEPORT_FILE_STATE_TRANSFAIR:
    show_progressbar = TRUE;
    label = g_strdup_printf ("Downloading \"%s\"", teleport_file_get_destination_path (file));
    break;
  case TELEPORT_FILE_STATE_FINISH:
    button = self->open_button;
    label = g_strdup_printf ("Ready \"%s\"", teleport_file_get_destination_path (file));
    break;
  case TELEPORT_FILE_STATE_CANCELLED:
    label = g_strdup_printf ("Download for \"%s\" was cancelled", teleport_file_get_destination_path (file));
    break;
  case TELEPORT_FILE_STATE_REJECT:
  case TELEPORT_FILE_STATE_ERROR:
  case TELEPORT_FILE_STATE_UNKNOWN:
  default:
    g_print ("State is something wrong\n");
    label = g_strdup_printf ("Something went terrible wrong with file \"%s\"", teleport_file_get_destination_path (file));
    break;
  }

  gtk_label_set_text (GTK_LABEL (self->file_label), label);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->button_revealer), button != NULL);
  if (button != NULL)
    gtk_stack_set_visible_child (GTK_STACK (self->button_stack), button);
  gtk_widget_set_visible (self->progressbar, show_progressbar);
}

static void
teleport_file_row_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  TeleportFileRow *self = TELEPORT_FILE_ROW (object);

  switch (prop_id)
    {
    case PROP_FILE:
      g_value_set_pointer (value, self->file);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
teleport_file_row_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  TeleportFileRow *self = TELEPORT_FILE_ROW (object);

  switch (prop_id)
    {
    case PROP_FILE:
      teleport_file_row_set_file (self, g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
teleport_file_row_finalize (GObject *object)
{
  TeleportFileRow *self = TELEPORT_FILE_ROW (object);

  g_object_unref (self->file);

  G_OBJECT_CLASS (teleport_file_row_parent_class)->finalize (object);
}

static void
teleport_file_row_class_init (TeleportFileRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = teleport_file_row_get_property;
  object_class->set_property = teleport_file_row_set_property;
  object_class->finalize = teleport_file_row_finalize;

  g_object_class_install_property (object_class,
                                   PROP_FILE,
                                   g_param_spec_pointer ("file",
                                                         "File of the row",
                                                         "The file that this row represents",
                                                         G_PARAM_READWRITE));

  gtk_widget_class_set_template_from_resource (widget_class, "/com/frac_tion/teleport/file_row.ui");
  gtk_widget_class_bind_template_child (widget_class, TeleportFileRow, button_stack);
  gtk_widget_class_bind_template_child (widget_class, TeleportFileRow, button_revealer);
  gtk_widget_class_bind_template_child (widget_class, TeleportFileRow, file_label);
  gtk_widget_class_bind_template_child (widget_class, TeleportFileRow, progressbar);
  gtk_widget_class_bind_template_child (widget_class, TeleportFileRow, save_button);
  gtk_widget_class_bind_template_child (widget_class, TeleportFileRow, abort_button);
  gtk_widget_class_bind_template_child (widget_class, TeleportFileRow, open_button);
}

static void
teleport_file_row_init (TeleportFileRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

TeleportFileRow*
teleport_file_row_new (TeleportFile *file)
{
  return g_object_new (TELEPORT_TYPE_FILE_ROW,
                       "file", file,
                       NULL);
}

TeleportFile *
teleport_file_row_get_file (TeleportFileRow *self)
{
  g_return_val_if_fail (TELEPORT_IS_FILE_ROW (self), NULL);

  return self->file;
}

void
teleport_file_row_set_file (TeleportFileRow *self,
                            TeleportFile    *file)
{
  g_return_if_fail (TELEPORT_IS_FILE_ROW (self));

  if (file == self->file)
    return;

  g_clear_object (&self->file);

  self->file = g_object_ref (file);

  g_object_bind_property (self->file,
                          "progress",
                          self->progressbar,
                          "fraction",
                          G_BINDING_SYNC_CREATE);

  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->save_button),
                                    "s",
                                    teleport_file_get_id (self->file));
  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->open_button),
                                    "s",
                                    teleport_file_get_id (self->file));
  gtk_actionable_set_action_target (GTK_ACTIONABLE (self->abort_button),
                                    "s",
                                    teleport_file_get_id (self->file));

  update_state_cb (self, NULL, self->file);
  g_signal_connect_swapped (self->file,
                            "notify::state",
                            G_CALLBACK (update_state_cb),
                            self);
}
