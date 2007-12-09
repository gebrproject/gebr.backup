/*   G�BR - An environment for seismic processing.
 *   Copyright(C) 2007 G�BR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or *(at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_flow_browse.c
 * Responsible for UI for browsing a line's flows.
 *
 */

#include <string.h>

#include "ui_flow_browse.h"
#include "gebr.h"
#include "support.h"
#include "document.h"
#include "flow.h"
#include "ui_flow.h"
#include "ui_help.h"

/*
 * Prototypes
 */

static void
flow_browse_load(void);

static void
flow_browse_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text, struct ui_flow_browse * ui_flow_browse);

static void
flow_browse_show_help(void);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: add_flow_browse
 * Assembly the flow browse page.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_flow_browse *
flow_browse_setup_ui(void)
{
	struct ui_flow_browse *		ui_flow_browse;

	GtkTreeSelection *		selection;
	GtkTreeViewColumn *		col;
	GtkCellRenderer *		renderer;

	GtkWidget *			page;
	GtkWidget *			hpanel;
	GtkWidget *			scrolledwin;
	GtkWidget *			frame;
	GtkWidget *			infopage;

	gchar *				label;

	/* alloc */
	ui_flow_browse = g_malloc(sizeof(struct ui_flow_browse));

	/* Create flow browse page */
	page = gtk_vbox_new(FALSE, 0);
	ui_flow_browse->widget = page;
	label = _("Flows");
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(page), hpanel);

	/*
	 * Left side: flow list
	 */
	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolledwin, FALSE, FALSE);
	gtk_widget_set_size_request(scrolledwin, 300, -1);

	ui_flow_browse->store = gtk_list_store_new(FB_N_COLUMN,
					G_TYPE_STRING,  /* Name(title for libgeoxml) */
					G_TYPE_STRING); /* Filename */

	ui_flow_browse->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_browse->store));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(GTK_OBJECT(renderer), "edited",
			GTK_SIGNAL_FUNC(flow_browse_rename), ui_flow_browse);

	col = gtk_tree_view_column_new_with_attributes(_("Index"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, FB_TITLE);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", FB_TITLE);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	gtk_container_add(GTK_CONTAINER(scrolledwin), ui_flow_browse->view);

	g_signal_connect(GTK_OBJECT(ui_flow_browse->view), "cursor-changed",
			GTK_SIGNAL_FUNC(flow_browse_load), ui_flow_browse);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->view), "cursor-changed",
			GTK_SIGNAL_FUNC(flow_browse_info_update), ui_flow_browse);

	/*
	 * Right side: flow info
	 */
	frame = gtk_frame_new(_("Details"));
	gtk_paned_pack2(GTK_PANED(hpanel), frame, TRUE, FALSE);

	infopage = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), infopage);

	/* Title */
	ui_flow_browse->info.title = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.title), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->info.title, FALSE, TRUE, 0);

	/* Description */
	ui_flow_browse->info.description = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.description), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->info.description, FALSE, TRUE, 10);

	/* I/O */
	GtkWidget *table;
	table = gtk_table_new(3, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(infopage), table, FALSE, TRUE, 0);

	ui_flow_browse->info.input_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.input_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.input_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	ui_flow_browse->info.input = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.input), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.input, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	ui_flow_browse->info.output_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.output_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.output_label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	ui_flow_browse->info.output = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.output), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.output, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	ui_flow_browse->info.error_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.error_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.error_label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

	ui_flow_browse->info.error = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.error), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.error, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

	/* Help */
	ui_flow_browse->info.help = gtk_button_new_from_stock(GTK_STOCK_INFO);
	gtk_box_pack_end(GTK_BOX(infopage), ui_flow_browse->info.help, FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->info.help), "clicked",
			GTK_SIGNAL_FUNC(flow_browse_show_help), ui_flow_browse);

	/* Author */
	ui_flow_browse->info.author = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.author), 0, 0);
	gtk_box_pack_end(GTK_BOX(infopage), ui_flow_browse->info.author, FALSE, TRUE, 0);

	return ui_flow_browse;
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: flow_browse_load
 * Load a selected flow from file
 *
 * Load a selected flow from file when selected in "Flow Browse"
 */
static void
flow_browse_load(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			filename;
	gchar *                 title;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		if (gebr.flow != NULL)
			flow_free();
		return;
	}
	gtk_tree_model_get(GTK_TREE_MODEL (gebr.ui_flow_browse->store), &iter,
			   FB_FILENAME, &filename,
			   FB_TITLE, &title,
			-1);

	/* free previous flow */
	if (gebr.flow != NULL)
		flow_free();

	/* load it */
	gebr.flow = GEOXML_FLOW(document_load(filename));
	if (gebr.flow == NULL){
		gebr_message(ERROR, TRUE, FALSE, _("Unable to load flow '%s'"), title);
		gebr_message(ERROR, FALSE, TRUE, _("Unable to load flow '%s' from file '%s'"), title, filename);
		goto out;
	}

	/* now into GUI */
	flow_add_programs_to_view(gebr.flow);
	flow_browse_info_update();

out:	g_free(filename);
	g_free(title);
}

/*
 * Function: flow_browse_rename
 * Rename a flow upon double click.
 */
static void
flow_browse_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text, struct ui_flow_browse * ui_flow_browse)
{
	GtkTreeIter	iter;
	gchar *         old_title;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ui_flow_browse->store),
					    &iter,
					    path_string);
	old_title = (gchar *)geoxml_document_get_title(GEOXML_DOC(gebr.flow));

	/* was it really renamed? */
	if (g_ascii_strcasecmp(old_title, new_text) == 0)
		return;

	/* update store */
	gtk_list_store_set(ui_flow_browse->store, &iter,
			   FB_TITLE, new_text,
			   -1);
	/* update XML */
	geoxml_document_set_title(GEOXML_DOC(gebr.flow), new_text);
	flow_save();

	/* send feedback */
	gebr_message(INFO, FALSE, TRUE, _("Flow '%s' renamed to '%s'"), old_title, new_text);
}

/*
 * Function: flow_browse_info_update
 * Update information shown about the selected flow
 */
void
flow_browse_info_update(void)
{
	if (gebr.flow == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.title), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.author), "");

		g_object_set(gebr.ui_flow_browse->info.help, "sensitive", FALSE, NULL);
		return;
	}

	gchar *		markup;
	GString *	text;

	/* Title in bold */
	markup = g_markup_printf_escaped("<b>%s</b>", geoxml_document_get_title(GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.title), markup);
	g_free(markup);

	/* Description in italic */
	markup = g_markup_printf_escaped("<i>%s</i>", geoxml_document_get_description(GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.description), markup);
	g_free(markup);

	/* I/O labels */
	markup = g_markup_printf_escaped("<b>%s</b>", _("Input:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.input_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Output:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.output_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Error log:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.error_label), markup);
	g_free(markup);

	/* Input file */
	if (strlen(geoxml_flow_io_get_input(gebr.flow)))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), geoxml_flow_io_get_input(gebr.flow));
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), _("(none)"));
	/* Output file */
	if (strlen(geoxml_flow_io_get_output(gebr.flow)))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), geoxml_flow_io_get_output(gebr.flow));
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), _("(none)"));
	/* Error file */
	if(strlen(geoxml_flow_io_get_error(gebr.flow)))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), geoxml_flow_io_get_error(gebr.flow));
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), _("(none)"));

	/* Author and email */
	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
			geoxml_document_get_author(GEOXML_DOC(gebr.flow)),
			geoxml_document_get_email(GEOXML_DOC(gebr.flow)));
	gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.author), text->str);
	g_string_free(text, TRUE);

	/* Info button */
	g_object_set(gebr.ui_flow_browse->info.help,
		"sensitive", strlen(geoxml_document_get_help(gebr.flow)) ? TRUE : FALSE, NULL);
}

static void
flow_browse_show_help(void)
{
	help_show((gchar*)geoxml_document_get_help(GEOXML_DOC(gebr.flow)),
		_("Flow help"), (gchar*)geoxml_document_get_filename(GEOXML_DOC(gebr.flow)));
}
