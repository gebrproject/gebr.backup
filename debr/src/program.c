/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/gui/utils.h>
#include <libgebr/gui/programedit.h>

#include "program.h"
#include "debr.h"
#include "callbacks.h"
#include "help.h"

/*
 * File: program.c
 * Construct interfaces for programs
 */

/*
 * Declarations
 */

#define RESPONSE_REFRESH	101

struct program_preview_data {
	struct gebr_gui_program_edit *	program_edit;
	GebrGeoXmlProgram *				program;
	GtkWidget *				title_label;
	GtkWidget *				hbox;
};

static void
program_details_update(void);
static gboolean
program_get_selected(GtkTreeIter * iter, gboolean warn_user);
static void
program_load_iter(GtkTreeIter * iter);
static void
program_load_selected(void);
static GtkTreeIter
program_append_to_ui(GebrGeoXmlProgram * program);
static void
program_selected(void);
static void
program_select_iter(GtkTreeIter iter);
static GtkMenu *
program_popup_menu(GtkWidget * tree_view);

static void
program_stdin_changed(GtkToggleButton * togglebutton);
static void
program_stdout_changed(GtkToggleButton * togglebutton);
static void
program_stderr_changed(GtkToggleButton * togglebutton);
static gboolean
program_title_changed(GtkEntry * entry);
static gboolean
program_binary_changed(GtkEntry * entry);
static gboolean
program_description_changed(GtkEntry * entry);
static void
program_url_open(GtkButton * button);
static void
program_help_view(GtkButton * button, GebrGeoXmlProgram * program);
static void
program_help_edit(GtkButton * button);
static void
program_help_refresh(GtkButton * button);
static gboolean
program_url_changed(GtkEntry * entry);

static void
program_preview_on_response(GtkWidget * dialog, gint arg1, struct program_preview_data * data);
static gboolean
program_preview_on_delete_event(GtkWidget * dialog, GdkEventAny * event, struct program_preview_data * data);

/*
 * Section: Public
 */

/*
 * Function: program_setup_ui
 * Set interface and its callbacks
 */
void
program_setup_ui(void)
{
	GtkWidget *		hpanel;
	GtkWidget *		scrolled_window;
	GtkWidget *		details;
	GtkWidget *		hbox;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	hpanel = gtk_hpaned_new();
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 200, -1);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	debr.ui_program.list_store = gtk_list_store_new(PROGRAM_N_COLUMN,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_POINTER);
	debr.ui_program.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(debr.ui_program.list_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_program.tree_view)),
		GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(debr.ui_program.tree_view),
		(GebrGuiGtkPopupCallback)program_popup_menu, NULL);
	gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable(GTK_TREE_VIEW(debr.ui_program.tree_view), PROGRAM_XMLPOINTER,
		(GebrGuiGtkTreeViewMoveSequenceCallback)menu_saved_status_set_unsaved, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_program.tree_view);
	g_signal_connect(debr.ui_program.tree_view, "cursor-changed",
		(GCallback)program_selected, NULL);
	g_signal_connect(debr.ui_program.tree_view, "row-activated",
		G_CALLBACK(program_dialog_setup_ui), NULL);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(debr.ui_program.tree_view), FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(col, 24);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", PROGRAM_STATUS);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_program.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PROGRAM_TITLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_program.tree_view), col);

	/*
	 * Info panel
	 */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window, TRUE, FALSE);

	debr.ui_program.details.frame = gtk_frame_new(NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
		debr.ui_program.details.frame);

	details = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(debr.ui_program.details.frame), details);

	debr.ui_program.details.title_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.title_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.title_label, FALSE, TRUE, 0);

	debr.ui_program.details.description_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.description_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.description_label, FALSE, TRUE, 0);

	debr.ui_program.details.nparams_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.nparams_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.nparams_label, FALSE, TRUE, 10);

	debr.ui_program.details.binary_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.binary_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.binary_label, FALSE, TRUE, 0);

	debr.ui_program.details.url_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.url_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.url_label, FALSE, TRUE, 10);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(details), hbox, FALSE, TRUE, 0);

	debr.ui_program.details.url_button = gtk_button_new();
	gtk_box_pack_start(GTK_BOX(hbox),debr.ui_program.details.url_button, FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(debr.ui_program.details.url_button), "clicked",
		G_CALLBACK(program_url_open), debr.program);

	debr.ui_program.details.help_button = gtk_button_new_from_stock(GTK_STOCK_INFO);
	gtk_box_pack_end(GTK_BOX(details), debr.ui_program.details.help_button, FALSE, TRUE, 0);

	debr.ui_program.widget = hpanel;
	gtk_widget_show_all(debr.ui_program.widget);

	program_details_update();
}

/*
 * Function: program_load_menu
 * Load programs of the current menu into the tree view
 */
void
program_load_menu(void)
{
	GebrGeoXmlSequence *		program;
	GtkTreeIter			iter;

	gtk_list_store_clear(debr.ui_program.list_store);
	if (debr.menu == NULL) {
		debr.program = NULL;
		parameter_load_program();
		program_details_update();
		return;
	}

	gebr_geoxml_flow_get_program(debr.menu, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		program_append_to_ui(GEBR_GEOXML_PROGRAM(program));

	debr.program = NULL;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(debr.ui_program.list_store), &iter) == TRUE)
		program_select_iter(iter);
}

/*
 * Function: program_new
 * Append a new program and selects it
 */
void
program_new(gboolean edit)
{
	GebrGeoXmlProgram *		program;
	GtkTreeIter		iter;

	program = gebr_geoxml_flow_append_program(debr.menu);
	/* default settings */
	gebr_geoxml_program_set_title(program, _("New program"));
	gebr_geoxml_program_set_stdin(program, TRUE);
	gebr_geoxml_program_set_stdout(program, TRUE);
	gebr_geoxml_program_set_stderr(program, TRUE);

	iter = program_append_to_ui(program);
	gtk_list_store_set(debr.ui_program.list_store, &iter,
		PROGRAM_STATUS, debr.pixmaps.stock_no,
		-1);

	program_select_iter(iter);
	if (edit)
		on_program_properties_activate();
	menu_details_update();
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: program_preview
 * Show GeBR's program edit view
 */
void
program_preview(void)
{
	struct program_preview_data *		data;
	GtkWidget *				dialog;

	if (!program_get_selected(NULL, TRUE))
		return;

	data = g_malloc(sizeof(struct program_preview_data));
	data->program = debr.program;
	dialog = gtk_dialog_new_with_buttons(_("Parameter's dialog preview"),
		GTK_WINDOW(NULL),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_REFRESH, RESPONSE_REFRESH,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	g_object_set(dialog, "type-hint", GDK_WINDOW_TYPE_HINT_NORMAL, NULL);
	g_signal_connect(dialog, "response",
		G_CALLBACK(program_preview_on_response), data);
	g_signal_connect(dialog, "delete-event",
		G_CALLBACK(program_preview_on_delete_event), data);

	data->program_edit = gebr_gui_gebr_gui_program_edit_setup_ui(
		GEBR_GEOXML_PROGRAM(gebr_geoxml_object_copy(GEBR_GEOXML_OBJECT(debr.program))),
		NULL, (GebrGuiShowHelpCallback)program_help_view, TRUE);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), data->program_edit->widget, TRUE, TRUE, 0);
	gtk_widget_show(dialog);

	/* adjust window size to fit parameters horizontally */
	gdouble width;
	width = GTK_BIN(GTK_BIN(data->program_edit->scrolled_window)->child)->child->allocation.width + 30;
	if (width >= gdk_screen_get_width(gdk_screen_get_default()))
		gtk_window_maximize(GTK_WINDOW(dialog));
	else
		gtk_widget_set_size_request(dialog, width, 500);
}

/*
 * Function: program_remove
 * Confirm action and if confirmed removed selected program from XML and UI
 */
void
program_remove(void)
{
	GtkTreeIter		iter;

	if (!program_get_selected(NULL, TRUE))
		return;
	if (gebr_gui_confirm_action_dialog(_("Delete program"), _("Are you sure you want to delete selected(s) program(s)?"))
	== FALSE)
		return;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_program.tree_view) {
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(debr.program));
		debr.program = NULL;
		gtk_list_store_remove(debr.ui_program.list_store, &iter);
		g_signal_emit_by_name(debr.ui_program.tree_view, "cursor-changed");
	}

	gebr_gui_gtk_tree_view_select_sibling(GTK_TREE_VIEW(debr.ui_program.tree_view));
	menu_details_update();
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: program_top
 * Action move top
 */
void
program_top(void)
{
	GtkTreeIter	iter;

	program_get_selected(&iter, TRUE);

	gtk_list_store_move_after(debr.ui_program.list_store, &iter, NULL);
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(debr.program), NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: program_bottom
 * Move bottom current selected program
 */
void
program_bottom(void)
{
	GtkTreeIter		iter;

	program_get_selected(&iter, TRUE);

	gtk_list_store_move_before(debr.ui_program.list_store, &iter, NULL);
	gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(debr.program), NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/* Function: program_copy
 * Copy selected(s) program to clipboard
 */
void
program_copy(void)
{
	GtkTreeIter		iter;

	gebr_geoxml_clipboard_clear();
	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_program.tree_view) {
		GebrGeoXmlObject *	program;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), &iter,
			PROGRAM_XMLPOINTER, &program,
			-1);
		gebr_geoxml_clipboard_copy(GEBR_GEOXML_OBJECT(program));
	}
}

/* Function: program_paste
 * Paste programs on clipboard
 */
void
program_paste(void)
{
	GebrGeoXmlSequence *	pasted;
	GtkTreeIter		pasted_iter;

	pasted = GEBR_GEOXML_SEQUENCE(gebr_geoxml_clipboard_paste(GEBR_GEOXML_OBJECT(debr.menu)));
	if (pasted == NULL) {
		debr_message(LOG_ERROR, _("Could not paste program"));
		return;
	}

	do {
		gtk_list_store_append(debr.ui_program.list_store, &pasted_iter);
		gtk_list_store_set(debr.ui_program.list_store, &pasted_iter,
			PROGRAM_XMLPOINTER, pasted,
			-1);
		program_load_iter(&pasted_iter);
	} while (!gebr_geoxml_sequence_next(&pasted));

	program_select_iter(pasted_iter);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: program_dialog_setup_ui
 * Open dialog to configure current program
 */
void
program_dialog_setup_ui(void)
{
	GtkWidget *	dialog;
	GtkWidget *	table;

	GtkWidget *	io_vbox;
	GtkWidget *	io_depth_hbox;
	GtkWidget *	io_label;
	GtkWidget *	io_stdin_checkbutton;
	GtkWidget *	io_stdout_checkbutton;
	GtkWidget *	io_stderr_checkbutton;

	GtkWidget *	title_label;
	GtkWidget *	title_entry;
	GtkWidget *	binary_label;
	GtkWidget *	binary_entry;
	GtkWidget *	description_label;
	GtkWidget *	description_entry;
	GtkWidget *	help_hbox;
	GtkWidget *	help_label;
	GtkWidget *	help_view_button;
	GtkWidget *	help_edit_button;
	GtkWidget *	help_refresh_button;
	GtkWidget *     url_label;
	GtkWidget *     url_entry;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(debr.ui_program.tree_view));
	if (program_get_selected(NULL, TRUE) == FALSE)
		return;

	dialog = gtk_dialog_new_with_buttons(_("Edit program"),
		GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 400, 350);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	/*
	 * IO
	 */
	io_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(io_vbox);
	gtk_table_attach(GTK_TABLE(table), io_vbox, 0, 2, 0, 1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);

	io_label = gtk_label_new(_("This program:"));
	gtk_misc_set_alignment(GTK_MISC(io_label), 0, 0);
	gtk_widget_show(io_label);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_label, FALSE, FALSE, 0);

	io_depth_hbox = gebr_gui_gtk_container_add_depth_hbox(io_vbox);
	io_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(io_vbox);
	gtk_box_pack_start(GTK_BOX(io_depth_hbox), io_vbox, FALSE, TRUE, 0);

	io_stdin_checkbutton = gtk_check_button_new_with_label(_("reads from standard input"));
	gtk_widget_show(io_stdin_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stdin_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stdin_checkbutton, "clicked",
		G_CALLBACK(program_stdin_changed), debr.program);

	io_stdout_checkbutton = gtk_check_button_new_with_label(_("writes to standard output"));
	gtk_widget_show(io_stdout_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stdout_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stdout_checkbutton, "clicked",
		G_CALLBACK(program_stdout_changed), debr.program);

	io_stderr_checkbutton = gtk_check_button_new_with_label(_("appends to standard error"));
	gtk_widget_show(io_stderr_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stderr_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stderr_checkbutton, "clicked",
		G_CALLBACK(program_stderr_changed), debr.program);

	/*
	 * Title
	 */
	title_label = gtk_label_new(_("Title:"));
	gtk_widget_show(title_label);
	gtk_table_attach(GTK_TABLE(table), title_label, 0, 1, 1, 2,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(title_label), 0, 0.5);

	title_entry = gtk_entry_new();
	gtk_widget_show(title_entry);
	gtk_table_attach(GTK_TABLE(table), title_entry, 1, 2, 1, 2,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(title_entry, "changed",
		(GCallback)program_title_changed, debr.program);

	/*
	 * Binary
	 */
	binary_label = gtk_label_new(_("Executable:"));
	gtk_widget_show(binary_label);
	gtk_table_attach(GTK_TABLE(table), binary_label, 0, 1, 2, 3,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(binary_label), 0, 0.5);

	binary_entry = gtk_entry_new();
	gtk_widget_show(binary_entry);
	gtk_table_attach(GTK_TABLE(table), binary_entry, 1, 2, 2, 3,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(binary_entry, "changed",
		(GCallback)program_binary_changed, debr.program);

	/*
	 * Description
	 */
	description_label = gtk_label_new(_("Description:"));
	gtk_widget_show(description_label);
	gtk_table_attach(GTK_TABLE(table), description_label, 0, 1, 3, 4,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(description_label), 0, 0.5);

	description_entry = gtk_entry_new();
	gtk_widget_show(description_entry);
	gtk_table_attach(GTK_TABLE(table), description_entry, 1, 2, 3, 4,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(description_entry, "changed",
		(GCallback)program_description_changed, debr.program);

	/*
	 * Help
	 */
	help_label = gtk_label_new(_("Help"));
	gtk_widget_show(help_label);
	gtk_table_attach(GTK_TABLE(table), help_label, 0, 1, 4, 5,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(help_label), 0, 0.5);

	help_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(help_hbox);
	gtk_table_attach(GTK_TABLE(table), help_hbox, 1, 2, 4, 5,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);

	help_view_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_widget_show(help_view_button);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_view_button, FALSE, FALSE, 0);
	g_signal_connect(help_view_button, "clicked",
		G_CALLBACK(program_help_view), debr.program);
	g_object_set(G_OBJECT(help_view_button), "relief", GTK_RELIEF_NONE, NULL);

	help_edit_button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_widget_show(help_edit_button);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_edit_button, FALSE, FALSE, 0);
	g_signal_connect(help_edit_button, "clicked",
		G_CALLBACK(program_help_edit), debr.program);
	g_object_set(G_OBJECT(help_edit_button), "relief", GTK_RELIEF_NONE, NULL);

	help_refresh_button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
        gebr_gui_gtk_widget_set_tooltip(help_refresh_button, _("Help edition with all possible information refreshed"));
	gtk_widget_show(help_refresh_button);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_refresh_button, FALSE, FALSE, 0);
	g_signal_connect(help_refresh_button, "clicked",
		G_CALLBACK(program_help_refresh), debr.program);
	g_object_set(G_OBJECT(help_refresh_button), "relief", GTK_RELIEF_NONE, NULL);

	/*
	 * URL
	 */
	url_label = gtk_label_new(_("URL:"));
	gtk_widget_show(url_label);
	gtk_table_attach(GTK_TABLE(table), url_label, 0, 1, 5, 6,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(url_label), 0, 0.5);

	url_entry = gtk_entry_new();
	gtk_widget_show(url_entry);
	gtk_table_attach(GTK_TABLE(table), url_entry, 1, 2, 5, 6,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(url_entry, "changed",
		(GCallback)program_url_changed, debr.program);

	/*
	 * Load program into UI
	 */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdin_checkbutton),
		gebr_geoxml_program_get_stdin(debr.program));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdout_checkbutton),
		gebr_geoxml_program_get_stdout(debr.program));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stderr_checkbutton),
		gebr_geoxml_program_get_stderr(debr.program));
	gtk_entry_set_text(GTK_ENTRY(title_entry), gebr_geoxml_program_get_title(debr.program));
	gtk_entry_set_text(GTK_ENTRY(binary_entry), gebr_geoxml_program_get_binary(debr.program));
	gtk_entry_set_text(GTK_ENTRY(description_entry), gebr_geoxml_program_get_description(debr.program));
	gtk_entry_set_text(GTK_ENTRY(url_entry), gebr_geoxml_program_get_url(debr.program));

	gtk_widget_show(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	program_load_selected();
}

/*
 * Section: Private
 */

/*
 * Function: program_details_update
 * Load details of selected program to the details view
 */
static void
program_details_update(void)
{
	GString *               text;
	GebrGeoXmlParameters *      parameters;
	gchar *		        markup;

	g_object_set(debr.ui_program.details.url_button, "visible", debr.program != NULL, NULL);
	gtk_widget_set_sensitive(debr.ui_program.details.help_button, debr.program != NULL);
	if (debr.program == NULL) {
		gtk_frame_set_label(GTK_FRAME(debr.ui_program.details.frame), NULL);
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.title_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.description_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.nparams_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.binary_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.url_label), "");
		return;
	}

	gtk_frame_set_label(GTK_FRAME(debr.ui_program.details.frame), _("Details"));

	markup = g_markup_printf_escaped("<b>%s</b>", gebr_geoxml_program_get_title(debr.program));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.title_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<i>%s</i>", gebr_geoxml_program_get_description(debr.program));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.description_label), markup);
	g_free(markup);

	parameters = gebr_geoxml_program_get_parameters(debr.program);
	text = g_string_new(NULL);
	switch (gebr_geoxml_parameters_get_number(parameters)){
	case 0:
		g_string_printf(text, _("This program has no parameters"));
		break;
	case 1:
		g_string_printf(text, _("This program has 1 parameter"));
		break;
	default:
		g_string_printf(text, _("This program has %li parameters"),
			gebr_geoxml_parameters_get_number(parameters));
	}
	gtk_label_set_text(GTK_LABEL(debr.ui_program.details.nparams_label), text->str);
	g_string_free(text, TRUE);

	markup = g_markup_printf_escaped("<b>%s</b>: %s", _("Binary"), gebr_geoxml_program_get_binary(debr.program));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.binary_label), markup);
	g_free(markup);

	markup = g_markup_printf_escaped("<b>%s</b>:", _("URL"));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.url_label), markup);
	g_free(markup);

	if (strlen(gebr_geoxml_program_get_url(debr.program)) > 0)
		gtk_button_set_label(GTK_BUTTON(debr.ui_program.details.url_button),
			gebr_geoxml_program_get_url(debr.program));
	else
		gtk_button_set_label(GTK_BUTTON(debr.ui_program.details.url_button),
			_("URL not set"));

	g_object_set(G_OBJECT(debr.ui_program.details.url_button), "sensitive",
		(strlen(gebr_geoxml_program_get_url(debr.program))>0) ? TRUE : FALSE, NULL);

	g_signal_handlers_disconnect_matched(G_OBJECT(debr.ui_program.details.help_button),
		G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (GCallback)program_help_view, NULL);
	g_signal_connect(GTK_OBJECT(debr.ui_program.details.help_button), "clicked",
		G_CALLBACK(program_help_view), debr.program);
	g_object_set(G_OBJECT(debr.ui_program.details.help_button),
		"sensitive", (strlen(gebr_geoxml_program_get_help(debr.program)) > 1) ? TRUE : FALSE, NULL);
}

/*
 * Function: program_get_selected
 * Return true if there is a program selected and write it to _iter_
 */
static gboolean
program_get_selected(GtkTreeIter * iter, gboolean warn_user)
{
	if (gebr_gtk_tree_view_get_selected(GTK_TREE_VIEW(debr.ui_program.tree_view), iter) == FALSE) {
		if (warn_user)
			debr_message(LOG_ERROR, _("No program is selected"));
		return FALSE;
	}

	return TRUE;
}

/*
 * Function: program_load_iter
 * Load stuff into _iter_
 */
static void
program_load_iter(GtkTreeIter * iter)
{
	GebrGeoXmlProgram *	program;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), iter,
		PROGRAM_XMLPOINTER, &program,
		-1);

	gtk_list_store_set(debr.ui_program.list_store, iter,
		PROGRAM_TITLE, gebr_geoxml_program_get_title(program),
		-1);
	program_details_update();
}

/*
 * Function: program_load_selected
 * Reload selected program contents to its iter
 */
static void
program_load_selected(void)
{
	GtkTreeIter	iter;

	if (!program_get_selected(&iter, FALSE))
		return;
	program_load_iter(&iter);

	/* program is now considered as reviewed by the user */
	gtk_list_store_set(debr.ui_program.list_store, &iter,
		PROGRAM_STATUS, NULL,
		-1);
}

static GtkTreeIter
program_append_to_ui(GebrGeoXmlProgram * program)
{
	GtkTreeIter	iter;

	gtk_list_store_append(debr.ui_program.list_store, &iter);
	gtk_list_store_set(debr.ui_program.list_store, &iter,
		PROGRAM_XMLPOINTER, program,
		-1);
	program_load_iter(&iter);

	return iter;
}

/*
 * Function: program_selected
 * Load user selected program
 */
static void
program_selected(void)
{
	GtkTreeIter	iter;

	if (program_get_selected(&iter, FALSE) == FALSE) {
		debr.program = NULL;
		return;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), &iter,
		PROGRAM_XMLPOINTER, &debr.program,
		-1);

	parameter_load_program();
	program_details_update();
	do_navigation_bar_update();
}

/*
 * Function: program_select_iter
 * Select _iter_ loading it
 */
static void
program_select_iter(GtkTreeIter iter)
{
	GtkTreeSelection *	tree_selection;

	tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_program.tree_view));
	gtk_tree_selection_unselect_all(tree_selection);
	gtk_tree_selection_select_iter(tree_selection, &iter);
	program_selected();
}

/*
 * Function: program_popup_menu
 * Agregate action to the popup menu and shows it.
 */
static GtkMenu *
program_popup_menu(GtkWidget * tree_view)
{
	GtkWidget *	menu;

	GtkTreeIter	iter;

	menu = gtk_menu_new();

	if (program_get_selected(&iter, FALSE) == FALSE) {
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
			gtk_action_group_get_action(debr.action_group, "program_new")));
		goto out;
	}

	if (gebr_gui_gtk_list_store_can_move_up(debr.ui_program.list_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_top")));
	if (gebr_gui_gtk_list_store_can_move_down(debr.ui_program.list_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_bottom")));
	if (gebr_gui_gtk_list_store_can_move_up(debr.ui_program.list_store, &iter) == TRUE ||
	gebr_gui_gtk_list_store_can_move_down(debr.ui_program.list_store, &iter) == TRUE)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_new")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_delete")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_properties")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_copy")));
	gtk_container_add(GTK_CONTAINER(menu), gtk_action_create_menu_item(
		gtk_action_group_get_action(debr.action_group, "program_paste")));

out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/*
 * Function: program_stdin_changed
 * Sync UI to XML
 */
static void
program_stdin_changed(GtkToggleButton * togglebutton)
{
	gebr_geoxml_program_set_stdin(debr.program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: program_stdout_changed
 * Sync UI to XML
 */
static void
program_stdout_changed(GtkToggleButton * togglebutton)
{
	gebr_geoxml_program_set_stdout(debr.program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: program_stderr_changed
 * Sync UI to XML
 */
static void
program_stderr_changed(GtkToggleButton * togglebutton)
{
	gebr_geoxml_program_set_stderr(debr.program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
program_title_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_title(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

static gboolean
program_binary_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_binary(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

static gboolean
program_description_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_description(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

static void
program_url_open(GtkButton * button)
{
	GString * cmdline;

	cmdline = g_string_new(NULL);
	g_string_printf(cmdline, "%s %s&", debr.config.browser->str, gebr_geoxml_program_get_url(debr.program));
	system(cmdline->str);
	g_string_free(cmdline, TRUE);
}

static void
program_help_view(GtkButton * button, GebrGeoXmlProgram * program)
{
        gchar * help;

        help = (gchar *) gebr_geoxml_program_get_help(program);
        if (strlen(help) > 1)
                help_show(help);
        else
                help_show(" ");

}

static void
program_help_edit(GtkButton * button)
{
	GString *	help;

	help = help_edit(gebr_geoxml_program_get_help(debr.program), debr.program, FALSE);
	gebr_geoxml_program_set_help(debr.program, help->str);
	g_string_free(help, TRUE);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
program_help_refresh(GtkButton * button)
{
	GString *	help;

	help = help_edit(gebr_geoxml_program_get_help(debr.program), debr.program, TRUE);
	gebr_geoxml_program_set_help(debr.program, help->str);
	g_string_free(help, TRUE);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
program_url_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_url(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

static void
program_preview_on_response(GtkWidget * dialog, gint response, struct program_preview_data * data)
{
	switch (response) {
	case RESPONSE_REFRESH:
		gebr_gui_gebr_gui_program_edit_reload(data->program_edit,
			GEBR_GEOXML_PROGRAM(gebr_geoxml_object_copy(GEBR_GEOXML_OBJECT(data->program))));
		break;
	case GTK_RESPONSE_CLOSE: default:
		gebr_gui_gebr_gui_program_edit_destroy(data->program_edit);
		gtk_widget_destroy(dialog);
		g_free(data);
		break;
	}
}

static gboolean
program_preview_on_delete_event(GtkWidget * dialog, GdkEventAny * event, struct program_preview_data * data)
{
	program_preview_on_response(dialog, GTK_RESPONSE_CLOSE, data);
	return FALSE;
}
