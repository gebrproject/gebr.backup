/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 20072-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <string.h>

#include <gui/utils.h>

#include "program.h"
#include "gebrme.h"
#include "support.h"
#include "help.h"

/*
 * File: program.c
 * Construct interfaces for programs
 */

/*
 * Declarations
 */

enum {
        PROGRAM_TITLE,
	PROGRAM_DESCRIPTION,
	PROGRAM_XMLPOINTER,
	PROGRAM_N_COLUMN
};

static GtkTreeIter
program_append_to_ui(GeoXmlProgram * program);

static void
program_selected(void);

static void
program_select_iter(GtkTreeIter iter);

static GtkMenu *
program_popup_menu(GtkWidget * tree_view);

static void
program_move_up(GtkMenuItem * menu_item);

static void
program_move_down(GtkMenuItem * menu_item);

static void
program_dialog_setup_ui(void);

static void
program_stdin_changed(GtkToggleButton * togglebutton, GeoXmlProgram * program);

static void
program_stdout_changed(GtkToggleButton * togglebutton, GeoXmlProgram * program);

static void
program_stderr_changed(GtkToggleButton * togglebutton, GeoXmlProgram * program);

static gboolean
program_title_changed(GtkEntry * entry, GeoXmlProgram * program);

static gboolean
program_binary_changed(GtkEntry * entry, GeoXmlProgram * program);

static gboolean
program_description_changed(GtkEntry * entry, GeoXmlProgram * program);

static void
program_help_view(GtkButton * button, GeoXmlProgram * program);

static void
program_help_edit(GtkButton * button, GeoXmlProgram * program);

static gboolean
program_url_changed(GtkEntry * entry, GeoXmlProgram * program);

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
	GtkWidget *		scrolled_window;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	gebrme.ui_program.list_store = gtk_list_store_new(PROGRAM_N_COLUMN,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_POINTER);
	gebrme.ui_program.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(gebrme.ui_program.list_store));
	gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(gebrme.ui_program.tree_view),
		(GtkPopupCallback)program_popup_menu, NULL);
	gtk_widget_show(gebrme.ui_program.tree_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), gebrme.ui_program.tree_view);
	g_signal_connect(gebrme.ui_program.tree_view, "cursor-changed",
		(GCallback)program_selected, NULL);
	g_signal_connect(gebrme.ui_program.tree_view, "row-activated",
		GTK_SIGNAL_FUNC(program_dialog_setup_ui), NULL);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PROGRAM_TITLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gebrme.ui_program.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PROGRAM_DESCRIPTION);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gebrme.ui_program.tree_view), col);

	gebrme.ui_program.widget = scrolled_window;
	gtk_widget_show_all(gebrme.ui_program.widget);
}

/*
 * Function: program_load_menu
 * Load programs of the current menu into the tree view
 */
void
program_load_menu(void)
{
	GeoXmlSequence *		program;
	GtkTreeIter			iter;

	gtk_list_store_clear(gebrme.ui_program.list_store);

	geoxml_flow_get_program(gebrme.menu, &program, 0);
	for (; program != NULL; geoxml_sequence_next(&program))
		program_append_to_ui(GEOXML_PROGRAM(program));

	gebrme.program = NULL;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebrme.ui_program.list_store), &iter) == TRUE)
		program_select_iter(iter);
}

/*
 * Function: program_new
 * Append a new program and selects it
 */
void
program_new(void)
{
	GeoXmlProgram *		program;

	program = geoxml_flow_append_program(gebrme.menu);
	/* default settings */
	geoxml_program_set_title(program, _("New program"));
	geoxml_program_set_stdin(program, TRUE);
	geoxml_program_set_stdout(program, TRUE);
	geoxml_program_set_stderr(program, TRUE);

	program_select_iter(program_append_to_ui(program));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: program_remove
 * Confirm action and if confirmed removed selected program from XML and UI
 */
void
program_remove(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	if (gebrme.program == NULL) {
		gebrme_message(LOG_ERROR, _("No program is selected"));
		return;
	}
	if (confirm_action_dialog(_("Delete program"), _("Are you sure you want to delete program '%s'?"),
	geoxml_program_get_title(gebrme.program)) == FALSE)
		return;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_program.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	gtk_list_store_remove(gebrme.ui_program.list_store, &iter);
	geoxml_sequence_remove(GEOXML_SEQUENCE(gebrme.program));
	gebrme.program = NULL;

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Section: Private
 */

static GtkTreeIter
program_append_to_ui(GeoXmlProgram * program)
{
	GtkTreeIter	iter;

	gtk_list_store_append(gebrme.ui_program.list_store, &iter);
	gtk_list_store_set(gebrme.ui_program.list_store, &iter,
		PROGRAM_TITLE, geoxml_program_get_title(program),
		PROGRAM_DESCRIPTION, geoxml_program_get_description(program),
		PROGRAM_XMLPOINTER, program,
		-1);

	return iter;
}

/*
 * Function: program_selected
 * Load user selected program
 */
static void
program_selected(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_program.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_program.list_store), &iter,
		PROGRAM_XMLPOINTER, &gebrme.program,
		-1);
	parameter_load_program();
}

/*
 * Function: program_select_iter
 * Select _iter_ loading it
 */
static void
program_select_iter(GtkTreeIter iter)
{
	GtkTreeSelection *	tree_selection;

	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_program.list_store), &iter,
		PROGRAM_XMLPOINTER, &gebrme.program,
		-1);
	tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_program.tree_view));
	gtk_tree_selection_select_iter(tree_selection, &iter);

	parameter_load_program();
}

static GtkMenu *
program_popup_menu(GtkWidget * tree_view)
{
	GtkWidget *	menu;
	GtkWidget *	menu_item;

	GtkTreeIter	iter;

	menu = gtk_menu_new();

	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(gebrme.actions.program.new));
	/* Move up */
	if (gtk_list_store_can_move_up(gebrme.ui_program.list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GO_UP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)program_move_up, NULL);
	}
	/* Move down */
	if (gtk_list_store_can_move_down(gebrme.ui_program.list_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GO_DOWN, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)program_move_down, NULL);
	}
	/* Remove */
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(gebrme.actions.program.delete));

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static void
program_move_up(GtkMenuItem * menu_item)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_program.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	gtk_list_store_move_up(gebrme.ui_program.list_store, &iter);
	geoxml_sequence_move_up(GEOXML_SEQUENCE(gebrme.program));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
program_move_down(GtkMenuItem * menu_item)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_program.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	gtk_list_store_move_down(gebrme.ui_program.list_store, &iter);
	geoxml_sequence_move_down(GEOXML_SEQUENCE(gebrme.program));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
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
	GtkWidget *     url_label;
	GtkWidget *     url_entry;

	dialog = gtk_dialog_new_with_buttons(_("Edit menu"),
		GTK_WINDOW(gebrme.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 400, 300);

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

	io_depth_hbox = gtk_container_add_depth_hbox(io_vbox);
	io_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(io_vbox);
	gtk_box_pack_start(GTK_BOX(io_depth_hbox), io_vbox, FALSE, TRUE, 0);

	io_stdin_checkbutton = gtk_check_button_new_with_label(_("reads from standard input"));
	gtk_widget_show(io_stdin_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stdin_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stdin_checkbutton, "clicked",
		GTK_SIGNAL_FUNC(program_stdin_changed), gebrme.program);

	io_stdout_checkbutton = gtk_check_button_new_with_label(_("writes to standard output"));
	gtk_widget_show(io_stdout_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stdout_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stdout_checkbutton, "clicked",
		GTK_SIGNAL_FUNC(program_stdout_changed), gebrme.program);

	io_stderr_checkbutton = gtk_check_button_new_with_label(_("appends to standard error"));
	gtk_widget_show(io_stderr_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stderr_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stderr_checkbutton, "clicked",
		GTK_SIGNAL_FUNC(program_stderr_changed), gebrme.program);

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
		(GCallback)program_title_changed, gebrme.program);

	/*
	 * Binary
	 */
	binary_label = gtk_label_new(_("Binary:"));
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
		(GCallback)program_binary_changed, gebrme.program);

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
		(GCallback)program_description_changed, gebrme.program);
	
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
		GTK_SIGNAL_FUNC(program_help_view), gebrme.program);
	g_object_set(G_OBJECT(help_view_button), "relief", GTK_RELIEF_NONE, NULL);

	help_edit_button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_widget_show(help_edit_button);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_edit_button, FALSE, FALSE, 5);
	g_signal_connect(help_edit_button, "clicked",
		GTK_SIGNAL_FUNC(program_help_edit), gebrme.program);
	g_object_set(G_OBJECT(help_edit_button), "relief", GTK_RELIEF_NONE, NULL);

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
		(GCallback)program_url_changed, gebrme.program);

	/*
	 * Load program into UI
	 */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdin_checkbutton),
		geoxml_program_get_stdin(gebrme.program));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdout_checkbutton),
		geoxml_program_get_stdout(gebrme.program));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stderr_checkbutton),
		geoxml_program_get_stderr(gebrme.program));
	gtk_entry_set_text(GTK_ENTRY(title_entry), geoxml_program_get_title(gebrme.program));
	gtk_entry_set_text(GTK_ENTRY(binary_entry), geoxml_program_get_binary(gebrme.program));
	gtk_entry_set_text(GTK_ENTRY(description_entry), geoxml_program_get_description(gebrme.program));
	gtk_entry_set_text(GTK_ENTRY(url_entry), geoxml_program_get_url(gebrme.program));
	
	gtk_widget_show(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void
program_stdin_changed(GtkToggleButton * togglebutton, GeoXmlProgram * program)
{
	geoxml_program_set_stdin(program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
program_stdout_changed(GtkToggleButton * togglebutton, GeoXmlProgram * program)
{
	geoxml_program_set_stdout(program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
program_stderr_changed(GtkToggleButton * togglebutton, GeoXmlProgram * program)
{
	geoxml_program_set_stderr(program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
program_title_changed(GtkEntry * entry, GeoXmlProgram * program)
{
	GtkWidget *	program_label;

	g_object_get(G_OBJECT(entry), "user-data", &program_label, NULL);

	geoxml_program_set_title(program, gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_label_set_text(GTK_LABEL(program_label), gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

static gboolean
program_binary_changed(GtkEntry * entry, GeoXmlProgram * program)
{
	geoxml_program_set_binary(program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

static gboolean
program_description_changed(GtkEntry * entry, GeoXmlProgram * program)
{
	geoxml_program_set_description(program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

static void
program_help_view(GtkButton * button, GeoXmlProgram * program)
{
	help_show(geoxml_program_get_help(program));
}

static void
program_help_edit(GtkButton * button, GeoXmlProgram * program)
{
	GString *	help;

	help = help_edit(geoxml_program_get_help(program), program);
	geoxml_program_set_help(program, help->str);
	g_string_free(help, TRUE);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
program_url_changed(GtkEntry * entry, GeoXmlProgram * program)
{
	geoxml_program_set_url(program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}
