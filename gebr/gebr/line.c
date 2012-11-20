/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but gebr.THOUT ANY gebr.RRANTY; without even the implied warranty
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "line.h"
#include "gebr.h"
#include "document.h"
#include "project.h"
#include "flow.h"
#include "callbacks.h"
#include "ui_project_line.h"
#include "ui_document.h"

static void on_properties_response(gboolean accept)
{
	if (!accept)
		line_delete(FALSE);
}

void line_new(void)
{
	GtkTreeSelection *selection;
	GtkTreeIter project_iter, line_iter;

	gchar *project_title;

	GebrGeoXmlLine *line;

	if (!project_line_get_selected(NULL, ProjectLineSelection))
		return;

	/* get project iter */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		gtk_tree_selection_get_selected(selection, NULL, &project_iter);
	else {
		gtk_tree_selection_get_selected(selection, NULL, &line_iter);
		gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &project_iter, &line_iter);
	}

	/* create it */
	line = GEBR_GEOXML_LINE(document_new(GEBR_GEOXML_DOCUMENT_TYPE_LINE));
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(line), _("New Line"));
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(line), gebr.config.username->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(line), gebr.config.email->str);
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &project_iter,
			   PL_TITLE, &project_title, -1);
	line_iter = project_append_line_iter(&project_iter, line);
	gebr_geoxml_project_append_line(gebr.project, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(line)));
	document_save(GEBR_GEOXML_DOC(gebr.project), TRUE, FALSE);
	document_save(GEBR_GEOXML_DOC(line), TRUE, FALSE);

	/* feedback */
	gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("New line created in project '%s'."), project_title);
	g_free(project_title);

	project_line_select_iter(&line_iter);

	document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.line), on_properties_response);
}

gboolean line_delete(gboolean confirm)
{
	GtkTreeIter iter;

	GebrGeoXmlSequence *line_flow;
	const gchar *line_filename;

	if (!project_line_get_selected(&iter, LineSelection))
		return FALSE;

	if (confirm
	    && gebr_gui_confirm_action_dialog(_("Delete line"),
					      _("Are you sure you want to delete line '%s' and all its flows?"),
					      gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.line))) == FALSE)
		return FALSE;

	GtkTreeIter tmpiter;
	gpointer document;
	gebr_gui_gtk_tree_model_foreach(tmpiter, GTK_TREE_MODEL(gebr.ui_flow_browse->store)) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &tmpiter,
				   FB_XMLPOINTER, &document, -1);
		gebr_remove_help_edit_window(document);
	}
	gebr_remove_help_edit_window(gebr.line);

	/* Removes its flows */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
	for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		GString *path;
		const gchar *flow_source;

		flow_source = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		path = document_get_path(flow_source);
		g_unlink(path->str);
		g_string_free(path, TRUE);

		/* log action */
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting child flow '%s'."), flow_source);
	}

	/* Remove the line from its project */
	line_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(gebr.line));
	if (gebr_geoxml_project_remove_line(gebr.project, line_filename)) {
		document_save(GEBR_GEOXML_DOC(gebr.project), TRUE, FALSE);
		document_delete(line_filename);
	}

	/* inform the user */
	if (confirm) {
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Deleting line '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.line)));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting line '%s' from project '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.line)),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.project)));
	}

	/* remove from the GUI */
	if (gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), &iter))
		project_line_select_iter(&iter);

	return TRUE;
}

void line_set_paths_to(GebrGeoXmlLine * line, gboolean relative)
{
	GebrGeoXmlSequence *line_path;
	GString *path;

	path = g_string_new(NULL);
	gebr_geoxml_line_get_path(line, &line_path, 0);
	for (; line_path != NULL; gebr_geoxml_sequence_next(&line_path)) {
		g_string_assign(path, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path)));
		gebr_path_set_to(path, relative);
		gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(line_path), path->str);
	}

	g_string_free(path, TRUE);
}

GtkTreeIter line_append_flow_iter(GebrGeoXmlFlow * flow, GebrGeoXmlLineFlow * line_flow)
{
	GtkTreeIter iter;

	/* add to the flow browser. */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow)),
			   FB_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(flow)),
			   FB_LINE_FLOW_POINTER, line_flow,
			   FB_XMLPOINTER, flow,
			   -1);

	return iter;
}

void line_load_flows(void)
{
	GebrGeoXmlSequence *line_flow;
	GtkTreeIter iter;
	gboolean error = FALSE;

	flow_free();
	project_line_get_selected(&iter, DontWarnUnselection);

	/* iterate over its flows */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
	while (line_flow != NULL) {
		GebrGeoXmlFlow *flow;

		GebrGeoXmlSequence * next = line_flow;
		gebr_geoxml_sequence_next(&next);

		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		int ret = document_load_with_parent((GebrGeoXmlDocument**)(&flow), filename, &iter, TRUE);
		if (ret) {
			line_flow = next;
			error = TRUE;
			continue;
		}

		line_append_flow_iter(flow, GEBR_GEOXML_LINE_FLOW(line_flow));

		line_flow = next;
	}

	if (!error)
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Flows loaded."));

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter) == TRUE)
		flow_browse_select_iter(&iter);
}

void line_move_flow_top(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_after(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}

void line_move_flow_bottom(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_before(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}