/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/geoxml.h>
#include <libgebr/comm.h>
#include <libgebr/gui/utils.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>

#include "flow.h"
#include "line.h"
#include "gebr.h"
#include "menu.h"
#include "document.h"
#include "server.h"
#include "callbacks.h"
#include "ui_flow.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"

gboolean flow_new(void)
{
	GtkTreeIter iter;

	const gchar *line_title;
	const gchar *line_filename;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlSequence *line_flow;

	if (!project_line_get_selected(NULL, LineSelection))
		return FALSE;

	line_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line));
	line_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.line));

	/* Create a new flow */
	flow = GEBR_GEOXML_FLOW(document_new(GEBR_GEOXML_DOCUMENT_TYPE_FLOW));
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(flow), _("New Flow"));
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(flow), gebr.config.username->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(flow), gebr.config.email->str);
	/* and add to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow)),
			   FB_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(flow)), -1);

	/* and add to current line */
	gebr_geoxml_line_append_flow(gebr.line, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(flow)));
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE);
	document_save(GEBR_GEOXML_DOC(flow), TRUE);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(flow));

	flow_browse_select_iter(&iter);

	gebr_geoxml_line_get_flow(gebr.line, &line_flow, gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter, FB_LINE_FLOW_POINTER, line_flow, -1);

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("New flow added to line '%s'."), line_title);
	if (!on_document_properties_activate())
		flow_delete(FALSE);

	return TRUE;
}

void flow_free(void)
{
	if (gebr.flow != NULL) {
		gebr_geoxml_document_free(GEBR_GEOXML_DOC(gebr.flow));
		gebr.flow = NULL;
	}
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	gtk_combo_box_set_model(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), NULL);
	gtk_widget_set_sensitive(gebr.ui_flow_edition->queue_combobox, FALSE);
	gtk_widget_set_sensitive(gebr.ui_flow_edition->server_combobox, FALSE);
	gtk_container_foreach(GTK_CONTAINER(gebr.ui_flow_browse->revisions_menu),
			      (GtkCallback) gtk_widget_destroy, NULL);
	flow_browse_info_update();
}

void flow_delete(gboolean confirm)
{
	GtkTreeIter iter;
	gboolean valid = FALSE;

	gchar *title;
	gchar *filename;

	GebrGeoXmlSequence *line_flow;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;
	if (confirm && gebr_gui_confirm_action_dialog(_("Delete flow"),
						      _("Are you sure you want to delete the selected flow(s)?")) ==
	    FALSE)
		return;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
				   FB_TITLE, &title, FB_FILENAME, &filename, -1);

		/* Some feedback */
		if (confirm) {
			gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Deleting flow '%s'."), title);
			gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting flow '%s' from line '%s'."),
				     title, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line)));
		}

		/* Seek and destroy */
		gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
		for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
			if (strcmp(filename, gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow))) == 0) {
				gebr_geoxml_sequence_remove(line_flow);
				document_save(GEBR_GEOXML_DOC(gebr.line), TRUE);
				break;
			}
		}

		/* Free and delete flow from the disk */
		valid = gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter);
		flow_free();
		document_delete(filename);
		g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");

		g_free(title);
		g_free(filename);
	}
	if (valid)
		flow_browse_select_iter(&iter);
}

void flow_import(void)
{
	GtkTreeIter iter;

	GtkWidget *chooser_dialog;
	GtkFileFilter *file_filter;
	gchar *dir;

	gchar *flow_title;

	GebrGeoXmlFlow *imported_flow;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	/* assembly a file chooser dialog */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose filename to open"),
						     GTK_WINDOW(gebr.window),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_OPEN, GTK_RESPONSE_YES,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(file_filter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	/* load flow */
	dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	if (document_load_path((GebrGeoXmlDocument**)(&imported_flow), dir))
		goto out2;

	/* initialization */
	flow_title = (gchar*)gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(imported_flow));

	/* feedback */
	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Flow '%s' imported to line '%s' from file '%s'."),
		     flow_title, gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.line)), dir);

	document_import(GEBR_GEOXML_DOCUMENT(imported_flow));
	/* and add it to the line */
	gebr_geoxml_line_append_flow(gebr.line, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(imported_flow)));
	document_save(GEBR_GEOXML_DOC(gebr.line), FALSE);
	/* and to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_TITLE, flow_title, 
			   FB_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(imported_flow)), -1);
	flow_browse_select_iter(&iter);

	gebr_geoxml_document_free(GEBR_GEOXML_DOC(imported_flow));
out2:	g_free(dir);
out:	gtk_widget_destroy(chooser_dialog);
}

void flow_export(void)
{
	GtkTreeIter iter;

	GtkWidget *chooser_dialog;
	GtkWidget *check_box;
	GtkFileFilter *file_filter;
	GString *title;
	gchar *tmp;
	gchar *filename;

	GebrGeoXmlDocument *flow;
	gchar *flow_filename;

	if (!flow_browse_get_selected(&iter, TRUE))
		return;

	flow_browse_single_selection();

	title = g_string_new(NULL);

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_FILENAME, &flow_filename, -1);
	if (document_load((GebrGeoXmlDocument**)(&flow), flow_filename))
		goto out;

	/* run file chooser */
	g_string_printf(title, _("Choose filename to save flow '%s'"), gebr_geoxml_document_get_title(flow));
	check_box = gtk_check_button_new_with_label(_("Make this flow user-independent."));
	chooser_dialog = gebr_gui_save_dialog_new(title->str, GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), ".flw");

	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(file_filter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser_dialog), check_box);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box), TRUE);

	/* show file chooser */
	tmp = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!tmp)
		goto out;

	flow_set_paths_to(GEBR_GEOXML_FLOW(flow), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_box)));
	filename = g_path_get_basename(tmp);

	/* export current flow to disk */
	gebr_geoxml_document_set_filename(flow, filename);
	document_save_at(flow, tmp, FALSE);

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Flow '%s' exported to %s."),
		     (gchar *) gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow)), tmp);

	/* frees */
	g_free(tmp);
	g_free(filename);

	gebr_geoxml_document_free(flow);
out:
	g_free(flow_filename);
	g_string_free(title, TRUE);
}

/**
 * \internal
 * Cleanup (if group recursively) parameters value.
 * If _use_value_as_default_ is TRUE the value is made default
 */
static void flow_export_parameters_cleanup(GebrGeoXmlParameters * parameters, gboolean use_value_as_default)
{
	GebrGeoXmlSequence *parameter;

	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter)) {
		if (gebr_geoxml_parameter_get_is_program_parameter(GEBR_GEOXML_PARAMETER(parameter)) == TRUE) {
			GebrGeoXmlSequence *value;

			if (use_value_as_default == TRUE) {
				GebrGeoXmlSequence *default_value;

				gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
									FALSE, &value, 0);
				gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
									TRUE, &default_value, 0);
				for (; value != NULL; gebr_geoxml_sequence_next(&value),
				     gebr_geoxml_sequence_next(&default_value)) {
					if (default_value == NULL)
						default_value =
						    GEBR_GEOXML_SEQUENCE(gebr_geoxml_program_parameter_append_value
									 (GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
									  TRUE));
					gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(default_value),
								       gebr_geoxml_value_sequence_get
								       (GEBR_GEOXML_VALUE_SEQUENCE(value)));
				}

				/* remove extras default values */
				while (default_value != NULL) {
					GebrGeoXmlSequence *tmp;

					tmp = default_value;
					gebr_geoxml_sequence_next(&tmp);
					gebr_geoxml_sequence_remove(default_value);
					default_value = tmp;
				}
			}

			gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
								FALSE, &value, 0);
			for (; value != NULL; gebr_geoxml_sequence_next(&value))
				gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(value), "");
		} else {	/* a group, time for recursion! */
			GebrGeoXmlSequence *instance;

			gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
			for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
				flow_export_parameters_cleanup(GEBR_GEOXML_PARAMETERS(instance), use_value_as_default);
		}
	}
}

void flow_export_as_menu(void)
{
	GtkWidget *dialog;
	GtkFileFilter *file_filter;

	GString *path;
	gchar *filename;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlSequence *program;
	gboolean use_value;
	gint i;
	gchar *tmp;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;
	flow_browse_single_selection();

	/* run file chooser */
	dialog = gtk_file_chooser_dialog_new(_("Choose filename to save"),
					     GTK_WINDOW(gebr.window),
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_SAVE, GTK_RESPONSE_YES,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), gebr.config.usermenus->str);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Menu files (*.mnu)"));
	gtk_file_filter_add_pattern(file_filter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);

	/* show file chooser */
	gtk_widget_show(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_YES)
		goto out;
	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	path = g_string_new(tmp);
	gebr_append_filename_extension(path, ".mnu");
	filename = g_path_get_basename(path->str);

	/* first clone it */
	flow = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOC(gebr.flow)));

	gtk_widget_destroy(dialog);
	dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
					(GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL),
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					"Do you want to use your parameters' values as default values?");
	gtk_window_set_title(GTK_WINDOW(dialog), _("Default values"));

	gebr_geoxml_flow_get_program(flow, &program, 0);
	use_value = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES ? TRUE : FALSE;
	i = 0;
	for (; program != NULL; gebr_geoxml_sequence_next(&program)) {

		flow_export_parameters_cleanup(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)),
					       use_value);
		gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
	}

	gebr_geoxml_flow_io_set_input(flow, "");
	gebr_geoxml_flow_io_set_output(flow, "");
	gebr_geoxml_flow_io_set_error(flow, "");

	gebr_geoxml_flow_set_date_last_run(flow, "");
	gebr_geoxml_document_set_date_created(GEBR_GEOXML_DOC(flow), gebr_iso_date());
	gebr_geoxml_document_set_date_modified(GEBR_GEOXML_DOC(flow), gebr_iso_date());

	gebr_geoxml_document_set_help(GEBR_GEOXML_DOC(flow), "");
	gebr_geoxml_document_set_filename(GEBR_GEOXML_DOC(flow), filename);
	document_save_at(GEBR_GEOXML_DOC(flow), path->str, FALSE);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(flow));

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Flow '%s' exported as menu to %s."),
		     (gchar *) gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.flow)), path);

	/* frees */
	g_string_free(path, TRUE);
	g_free(filename);
	g_free(tmp);

out:	gtk_widget_destroy(dialog);
}

static void flow_copy_from_dicts_parse_parameters(GebrGeoXmlParameters * parameters);

/** 
 * \internal
 * Parse a parameter.
 */
static void flow_copy_from_dicts_parse_parameter(GebrGeoXmlParameter * parameter)
{
	GebrGeoXmlDocument *documents[] = {
		GEBR_GEOXML_DOCUMENT(gebr.project), GEBR_GEOXML_DOCUMENT(gebr.line),
		GEBR_GEOXML_DOCUMENT(gebr.flow), NULL
	};

	if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GebrGeoXmlSequence *instance;

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
			flow_copy_from_dicts_parse_parameters(GEBR_GEOXML_PARAMETERS(instance));

		return;
	}

	for (int i = 0; documents[i] != NULL; i++) {
		GebrGeoXmlProgramParameter *dict_parameter;

		dict_parameter =
			gebr_geoxml_program_parameter_find_dict_parameter(GEBR_GEOXML_PROGRAM_PARAMETER
									  (parameter), documents[i]);
		if (dict_parameter != NULL)
			gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER
								      (parameter), FALSE,
								      gebr_geoxml_program_parameter_get_first_value
								      (dict_parameter, FALSE));
	}
	gebr_geoxml_program_parameter_set_value_from_dict(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
							  NULL);
}

/** 
 * \internal
 * Parse a set of parameters.
 */
static void flow_copy_from_dicts_parse_parameters(GebrGeoXmlParameters * parameters)
{
	GebrGeoXmlSequence *parameter;

	parameter = GEBR_GEOXML_SEQUENCE(gebr_geoxml_parameters_get_first_parameter(parameters));
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter))
		flow_copy_from_dicts_parse_parameter(GEBR_GEOXML_PARAMETER(parameter));
}

void flow_copy_from_dicts(GebrGeoXmlFlow * flow)
{
	GebrGeoXmlSequence *program;

	gebr_geoxml_flow_get_program(flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		flow_copy_from_dicts_parse_parameters(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));
}

/**
 * \internal
 */
static void path_set_to(GString * path, gboolean relative)
{
	if (relative)
		gebr_path_use_home_variable(path);
	else
		gebr_path_resolve_home_variable(path);
}

/**
 * \internal
 */
static void flow_paths_foreach_parameter(GebrGeoXmlParameter * parameter, gboolean relative)
{
	if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE) {
		GebrGeoXmlSequence *value;

		gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE, &value, 0);
		for (; value != NULL; gebr_geoxml_sequence_next(&value)) {
			GString *path;

			path = g_string_new(gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(value)));
			path_set_to(path, relative);
			gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(value), path->str);

			g_string_free(path, TRUE);
		}
	}
}

void flow_set_paths_to(GebrGeoXmlFlow * flow, gboolean relative)
{
	GString *path;
	GebrGeoXmlSequence *server;

	path = g_string_new(NULL);

	/* flow's IO */
	g_string_assign(path, gebr_geoxml_flow_io_get_input(flow));
	path_set_to(path, relative);
	gebr_geoxml_flow_io_set_input(flow, path->str);
	g_string_assign(path, gebr_geoxml_flow_io_get_output(flow));
	path_set_to(path, relative);
	gebr_geoxml_flow_io_set_output(flow, path->str);
	g_string_assign(path, gebr_geoxml_flow_io_get_error(flow));
	path_set_to(path, relative);
	gebr_geoxml_flow_io_set_error(flow, path->str);

	/* servers IO */
	gebr_geoxml_flow_get_server(flow, &server, 0);
	for (; server != NULL; gebr_geoxml_sequence_next(&server)) {
		g_string_assign(path, gebr_geoxml_flow_server_io_get_input(GEBR_GEOXML_FLOW_SERVER(server)));
		path_set_to(path, relative);
		gebr_geoxml_flow_server_io_set_input(GEBR_GEOXML_FLOW_SERVER(server), path->str);
		g_string_assign(path, gebr_geoxml_flow_server_io_get_output(GEBR_GEOXML_FLOW_SERVER(server)));
		path_set_to(path, relative);
		gebr_geoxml_flow_server_io_set_output(GEBR_GEOXML_FLOW_SERVER(server), path->str);
		g_string_assign(path, gebr_geoxml_flow_server_io_get_error(GEBR_GEOXML_FLOW_SERVER(server)));
		path_set_to(path, relative);
		gebr_geoxml_flow_server_io_set_error(GEBR_GEOXML_FLOW_SERVER(server), path->str);
	}

	/* all parameters */
	gebr_geoxml_flow_foreach_parameter(flow, (GebrGeoXmlCallback)flow_paths_foreach_parameter, GINT_TO_POINTER(relative));

	g_string_free(path, TRUE);
}

void flow_run(struct server *server, GebrCommServerRun * config)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlSequence *i;

	flow = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(gebr.flow)));
	gebr_geoxml_flow_get_program(flow, &i, 0);
	if (i == NULL) {
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Flow '%s' is empty."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow)));
		goto out;
	}

	/* Strip flow: remove helps and revisions */
	gebr_geoxml_document_set_help(GEBR_GEOXML_DOCUMENT(flow), "");
	for (; i != NULL; gebr_geoxml_sequence_next(&i))
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(i), "");
	/* clear all revisions */
	gebr_geoxml_flow_get_revision(flow, &i, 0);
	while (i != NULL) {
		GebrGeoXmlSequence *tmp;

		tmp = i;
		gebr_geoxml_sequence_next(&tmp);
		gebr_geoxml_sequence_remove(i);
		i = tmp;
	}

	flow_copy_from_dicts(flow);

	/* RUN */
	config->flow = flow;
	gebr_comm_server_run_flow(server->comm, config);

	gebr_geoxml_flow_set_date_last_run(gebr.flow, gebr_iso_date());
	document_save(GEBR_GEOXML_DOC(gebr.flow), FALSE);
	flow_browse_info_update();

	gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking server to run flow '%s'."),
		     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(config->flow)));
	if (gebr_comm_server_is_local(server->comm) == FALSE) {
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server '%s' to run flow '%s'."),
			     server->comm->address->str, gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(config->flow)));
	} else {
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking local server to run flow '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(config->flow)));
	}

	/* frees */
out:	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(config->flow));
}

gboolean flow_revision_save(void)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *align;
	gboolean ret = FALSE;

	if (gebr.flow == NULL)
		return FALSE;

	dialog = gtk_dialog_new_with_buttons(_("Save flow state"),
					     GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	align = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 10, 10, 10, 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(align), vbox);

	label = gtk_label_new(_("Make a comment for this state:"));
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		GebrGeoXmlRevision *revision;

		revision = gebr_geoxml_flow_append_revision(gebr.flow, gtk_entry_get_text(GTK_ENTRY(entry)));
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE);
		flow_browse_load_revision(revision, TRUE);
		ret = TRUE;
	}

	gtk_widget_destroy(dialog);

	return ret;
}

void flow_program_remove(void)
{
	GtkTreeIter iter;
	gboolean valid = FALSE;
	GebrGeoXmlProgram *program;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
			continue;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(program));
		valid = gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store), &iter);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE);

	if (valid)
		flow_edition_select_component_iter(&iter);
}

void flow_program_move_top(void)
{
	GtkTreeIter iter;

	flow_edition_get_selected_component(&iter, FALSE);
	/* Update flow */
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(gebr.program), NULL);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE);
	/* Update GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
				  &iter, &gebr.ui_flow_edition->input_iter);
}

void flow_program_move_bottom(void)
{
	GtkTreeIter iter;

	flow_edition_get_selected_component(&iter, FALSE);
	/* Update flow */
	gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(gebr.program), NULL);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE);
	/* Update GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
				   &iter, &gebr.ui_flow_edition->output_iter);
}

void flow_copy(void)
{
	GtkTreeIter iter;

	if (gebr.flow_clipboard != NULL) {
		g_list_foreach(gebr.flow_clipboard, (GFunc) g_free, NULL);
		g_list_free(gebr.flow_clipboard);
		gebr.flow_clipboard = NULL;
	}

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
		gchar *filename;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_FILENAME, &filename, -1);
		gebr.flow_clipboard = g_list_prepend(gebr.flow_clipboard, filename);
	}
	gebr.flow_clipboard = g_list_reverse(gebr.flow_clipboard);
}

void flow_paste(void)
{
	GList *i;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	for (i = g_list_first(gebr.flow_clipboard); i != NULL; i = g_list_next(i)) {
		GebrGeoXmlDocument *flow;

		if (document_load((GebrGeoXmlDocument**)(&flow), (gchar *)i->data))
			continue;

		document_import(flow);
		line_append_flow_iter(GEBR_GEOXML_FLOW(flow),
				      GEBR_GEOXML_LINE_FLOW(gebr_geoxml_line_append_flow(gebr.line,
											 gebr_geoxml_document_get_filename(flow))));
		document_save(GEBR_GEOXML_DOCUMENT(gebr.line), TRUE);

		gebr_geoxml_document_free(flow);
	}
}

void flow_program_copy(void)
{
	GtkTreeIter iter;

	gebr_geoxml_clipboard_clear();
	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		GebrGeoXmlObject *program;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		gebr_geoxml_clipboard_copy(GEBR_GEOXML_OBJECT(program));
	}
}

void flow_program_paste(void)
{
	GebrGeoXmlSequence *pasted;

	pasted = GEBR_GEOXML_SEQUENCE(gebr_geoxml_clipboard_paste(GEBR_GEOXML_OBJECT(gebr.flow)));
	if (pasted == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not paste component."));
		return;
	}

	flow_add_program_sequence_to_view(GEBR_GEOXML_SEQUENCE(pasted), TRUE);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE);
}