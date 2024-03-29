/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "../libgebr-gettext.h"
#include <glib/gi18n-lib.h>

#include "gebr-gui-file-entry.h"
#include "gebr-gui-utils.h"
#include <libgebr/utils.h>

/*
 * Prototypes
 */

static void __gebr_gui_file_entry_entry_changed(GtkEntry * entry, GebrGuiFileEntry * file_entry);

static void
__gebr_gui_file_entry_browse_button_clicked(GtkButton * button, GtkEntryIconPosition icon_pos, GdkEvent * event,
					    GebrGuiFileEntry * file_entry);

static gboolean on_mnemonic_activate(GebrGuiFileEntry * file_entry);

/*
 * gobject stuff
 */

enum {
	CUSTOMIZE_FUNCTION = 1,
	LAST_PROPERTY
};

enum {
	PATH_CHANGED = 0,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
gebr_gui_file_entry_set_property(GebrGuiFileEntry * file_entry, guint property_id, const GValue * value,
				 GParamSpec * param_spec)
{
	switch (property_id) {
	case CUSTOMIZE_FUNCTION:
		file_entry->customize_function = g_value_get_pointer(value);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(file_entry, property_id, param_spec);
		break;
	}
}

static void
gebr_gui_file_entry_get_property(GebrGuiFileEntry * file_entry, guint property_id, GValue * value,
				     GParamSpec * param_spec)
{
	switch (property_id) {
	case CUSTOMIZE_FUNCTION:
		g_value_set_pointer(value, file_entry->customize_function);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(file_entry, property_id, param_spec);
		break;
	}
}

static void gebr_gui_file_entry_class_init(GebrGuiFileEntryClass * klass)
{
	GObjectClass *gobject_class;
	GParamSpec *param_spec;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = (typeof(gobject_class->set_property)) gebr_gui_file_entry_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property)) gebr_gui_file_entry_get_property;

	param_spec = g_param_spec_pointer("customize-function",
					  "Customize function", "Customize GtkFileChooser of dialog",
					  (GParamFlags)(G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, CUSTOMIZE_FUNCTION, param_spec);

	/* signals */
	object_signals[PATH_CHANGED] = g_signal_new("path-changed", GEBR_GUI_TYPE_FILE_ENTRY, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrGuiFileEntryClass, path_changed), NULL, NULL,	/* acumulators */
						    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gebr_gui_file_entry_init(GebrGuiFileEntry * file_entry)
{
	GtkWidget *entry;

	/* entry */
	entry = gtk_entry_new();
	file_entry->entry = entry;
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(file_entry), entry, TRUE, TRUE, 0);
	g_signal_connect(GTK_ENTRY(entry), "changed", G_CALLBACK(__gebr_gui_file_entry_entry_changed), file_entry);
	g_signal_connect(entry, "icon-release",
			 G_CALLBACK(__gebr_gui_file_entry_browse_button_clicked), file_entry);
	g_signal_connect(file_entry, "mnemonic-activate", G_CALLBACK(on_mnemonic_activate), NULL);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);

	file_entry->choose_directory = FALSE;
	file_entry->do_overwrite_confirmation = TRUE;
	file_entry->prefix = NULL;
	file_entry->line = NULL;
}

G_DEFINE_TYPE(GebrGuiFileEntry, gebr_gui_file_entry, GTK_TYPE_HBOX);

/*
 * Internal functions
 */

static void __gebr_gui_file_entry_entry_changed(GtkEntry * entry, GebrGuiFileEntry * file_entry)
{
	g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
}

static void
__gebr_gui_file_entry_browse_button_clicked(GtkButton *button,
					    GtkEntryIconPosition icon_pos,
					    GdkEvent *event,
					    GebrGuiFileEntry *file_entry)
{
	GtkWidget *chooser_dialog;

	chooser_dialog = gtk_file_chooser_dialog_new(file_entry->choose_directory == FALSE
						     ? _("Choose file") : _("Choose directory"), NULL,
						     file_entry->choose_directory == FALSE
						     ? GTK_FILE_CHOOSER_ACTION_SAVE :
						     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL,
						     GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser_dialog), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog),
						       file_entry->do_overwrite_confirmation);

	/* call customize funtion for user changes */
	if (file_entry->customize_function != NULL)
		file_entry->customize_function(GTK_FILE_CHOOSER(chooser_dialog), file_entry->customize_user_data);

	gchar ***paths = gebr_geoxml_line_get_paths(file_entry->line);

	gchar *new_text;
	const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(file_entry->entry));
	gint response = gebr_file_chooser_set_remote_navigation(chooser_dialog,
	                                                        entry_text,
	                                                        file_entry->prefix,
	                                                        paths, TRUE,
	                                                        &new_text);

	if (response == GTK_RESPONSE_OK) {
		gchar *path = gebr_relativise_path(new_text, file_entry->prefix, paths);
		gtk_entry_set_text(GTK_ENTRY(file_entry->entry), path);
		g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
		g_free(path);
	}

	gtk_widget_grab_focus (file_entry->entry);

	g_free(new_text);
	gebr_pairstrfreev(paths);
}

static gboolean on_mnemonic_activate(GebrGuiFileEntry * file_entry)
{
	gtk_widget_mnemonic_activate(file_entry->entry, TRUE);
	return TRUE;
}

/*
 * Library functions
 */

GtkWidget *gebr_gui_file_entry_new(GebrGuiFileEntryCustomize customize_function, gpointer user_data)
{
	GebrGuiFileEntry *file_entry;

	file_entry = g_object_new(GEBR_GUI_TYPE_FILE_ENTRY, "customize-function", customize_function, NULL);
	file_entry->customize_user_data = user_data;

	return GTK_WIDGET(file_entry);
}

void gebr_gui_file_entry_set_choose_directory(GebrGuiFileEntry * file_entry, gboolean choose_directory)
{
	file_entry->choose_directory = choose_directory;
}

gboolean gebr_gui_file_entry_get_choose_directory(GebrGuiFileEntry * file_entry)
{
	return file_entry->choose_directory;
}

void
gebr_gui_file_entry_set_do_overwrite_confirmation(GebrGuiFileEntry * file_entry,
						      gboolean do_overwrite_confirmation)
{
	file_entry->do_overwrite_confirmation = do_overwrite_confirmation;
}

gboolean gebr_gui_file_entry_get_do_overwrite_confirmation(GebrGuiFileEntry * file_entry)
{
	return file_entry->do_overwrite_confirmation;
}

void gebr_gui_file_entry_set_path(GebrGuiFileEntry * file_entry, const gchar * path)
{
	gtk_entry_set_text(GTK_ENTRY(file_entry->entry), path);
}

const gchar *gebr_gui_file_entry_get_path(GebrGuiFileEntry * file_entry)
{
	return gtk_entry_get_text(GTK_ENTRY(file_entry->entry));
}

void gebr_gui_file_entry_set_activates_default (GebrGuiFileEntry * self, gboolean setting)
{
	g_return_if_fail (GEBR_GUI_IS_FILE_ENTRY (self));

	gtk_entry_set_activates_default (GTK_ENTRY (self->entry), setting);
}

void gebr_gui_file_entry_set_warning(GebrGuiFileEntry * self, const gchar * tooltip){

	g_return_if_fail (GEBR_GUI_IS_FILE_ENTRY (self));

	gtk_entry_set_icon_from_stock(GTK_ENTRY(self->entry), GTK_ENTRY_ICON_SECONDARY, "folder-warning");
	gtk_entry_set_icon_tooltip_markup(GTK_ENTRY(self->entry), GTK_ENTRY_ICON_SECONDARY, tooltip);
}

void gebr_gui_file_entry_unset_warning(GebrGuiFileEntry * self, const gchar * tooltip){

	g_return_if_fail (GEBR_GUI_IS_FILE_ENTRY (self));

	gtk_entry_set_icon_from_stock(GTK_ENTRY(self->entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
	gtk_entry_set_icon_tooltip_text(GTK_ENTRY(self->entry), GTK_ENTRY_ICON_SECONDARY, tooltip);
}

void
gebr_gui_file_entry_set_paths_from_line(GebrGuiFileEntry *self,
					const gchar *prefix,
					GebrGeoXmlLine *line)
{
	self->line = line;
	self->prefix = g_strdup(prefix);
}
