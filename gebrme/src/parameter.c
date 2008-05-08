/*   GeBR ME - GeBR Menu Editor
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <geoxml.h>
#include <gui/gtkfileentry.h>
#include <gui/gtkenhancedentry.h>
#include <gui/utils.h>
#include <gui/parameter.h>
#include <gui/valuesequenceedit.h>

#include "parameter.h"
#include "support.h"
#include "gebrme.h"
#include "enumoptionedit.h"
#include "groupparameters.h"
#include "interface.h"
#include "menu.h"

/*
 * File: program.c
 * Construct interfaces for parameter
 */

/*
 * Declarations
 */

enum {
        PARAMETER_LABEL,
	PARAMETER_TYPE,
	PARAMETER_XMLPOINTER,
	PARAMETER_N_COLUMN
};

static GtkTreeIter
paramater_append_to_ui(GeoXmlParameter * parameter, GtkTreeIter * parent);
static void
parameter_select_iter(GtkTreeIter iter);

static void
parameter_create_ui_type_general(GtkWidget * table, struct parameter_data * data);
static void
parameter_data_free(GtkObject * expander, struct parameter_data * data);
static void
parameter_up(GtkButton * button, struct parameter_data * data);
static void
parameter_down(GtkButton * button, struct parameter_data * data);
static void
parameter_required_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_separator_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_label_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_keyword_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_default_widget_changed(struct parameter_widget * widget, struct parameter_data * data);
static void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_data * data);
static void
parameter_range_min_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_range_max_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_range_digits_changed(GtkEntry * entry, struct parameter_data * data);
static void
parameter_enum_options_changed(EnumOptionEdit * enum_option_edit, struct parameter_data * data);
static gboolean
parameter_is_exclusive(struct parameter_data * data);
static void
parameter_change_exclusive(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_group_exclusive_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_group_expanded_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static void
parameter_group_multiple_changed(GtkToggleButton * toggle_button, struct parameter_data * data);
static gboolean
parameter_group_instances_changed(GtkSpinButton * spinbutton, struct parameter_data * data);
static void
parameter_uilabel_update(struct parameter_data * data);

/*
 * Section: Public
 */

/*
 * Function: parameter_setup_ui
 * Set interface and its callbacks
 */
void
parameter_setup_ui(void)
{
	GtkWidget *		scrolled_window;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	gebrme.ui_parameter.tree_store = gtk_tree_store_new(PARAMETER_N_COLUMN,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_POINTER);
	gebrme.ui_parameter.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(gebrme.ui_parameter.tree_store));
	gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(gebrme.ui_parameter.tree_view),
		(GtkPopupCallback)parameter_popup_menu, NULL);
	gtk_widget_show(gebrme.ui_parameter.tree_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), gebrme.ui_parameter.tree_view);
	g_signal_connect(gebrme.ui_parameter.tree_view, "cursor-changed",
		(GCallback)parameter_selected, NULL);
	g_signal_connect(gebrme.ui_parameter.tree_view, "row-activated",
		GTK_SIGNAL_FUNC(parameter_dialog_setup_ui), NULL);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Label"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PROGRAM_TITLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gebrme.ui_parameter.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PROGRAM_DESCRIPTION);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gebrme.ui_parameter.tree_view), col);

	gebrme.ui_parameter.widget = scrolled_window;
	gtk_widget_show_all(gebrme.ui_parameter.widget);
}

/*
 * Function: parameter_load_program
 * Load current program parameters' to the UI
 */
void
parameter_load_program(void)
{
	GeoXmlSequence *		parameter;
	GtkTreeIter			iter;

	gtk_tree_store_clear(gebrme.ui_parameter.tree_store);

	geoxml_parameters_get_parameter(geoxml_program_get_parameters(gebrme.program), &parameter, 0);
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		parameter_append_to_ui(GEOXML_PARAMETER(parameter), NULL);

		if (geoxml_parameter_get_is_program_parameter(GEOXML_PARAMETER(parameter)) == FALSE) {
			GeoXmlSequence *	first_instance;

			geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(PARAMETER), &first_instance, 0);
			geoxml_parameters_get_parameter(gebrme.parameter, &parameter, 0);
			for (; parameter != NULL; geoxml_sequence_next(&parameter))
				parameter_append_to_ui(GEOXML_PARAMETER(parameter), NULL);
		}
	}

	gebrme.parameter = NULL;
}

/*
 * Function: parameter_new
 * Append a new parameter
 */
void
parameter_new(void)
{
	GeoXmlParameter *		parameter;

	parameter = geoxml_parameter_append_parameter(geoxml_program_get_parameters(gebrme.program));
	parameter_select_iter(parameter_append_to_ui(parameter));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_remove
 * Confirm action and if confirmed removed selected parameter from XML and UI
 */
void
parameter_remove(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	if (gebrme.parameter == NULL) {
		gebrme_message(LOG_ERROR, _("No parameter is selected"));
		return;
	}
	if (confirm_action_dialog(_("Delete parameter"), _("Are you sure you want to delete parameter '%s'?"),
	geoxml_parameter_get_label(gebrme.parameter)) == FALSE)
		return;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_parameter.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	gtk_tree_store_remove(gebrme.ui_parameter.tree_store, &iter);
	geoxml_sequence_remove(GEOXML_SEQUENCE(gebrme.parameter));
	gebrme.parameter = NULL;

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_duplicate
 * Append a duplicated parameter
 */
void
parameter_duplicate(void)
{
	GeoXmlParameter *		parameter;

	parameter = GEOXML_PARAMETER(geoxml_sequence_append_clone(GEOXML_SEQUENCE(gebrme.parameter)));
	parameter_select_iter(parameter_append_to_ui(parameter));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: parameter_change_type
 * Open dialog to change type of current selected parameter
 */

void
parameter_change_type(void)
{
	GtkWidget *			type_label;
	GtkWidget *			type_combo;
// 	GtkWidget *			label_label;
// 	GtkWidget *			label_entry;

	dialog = gtk_dialog_new_with_buttons(_("Edit menu"),
		GTK_WINDOW(gebrme.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 400, 300);

	table = gtk_table_new(1, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	type_label = gtk_label_new)_("Type:"));
	gtk_widget_show)type_label);
	gtk_table_attach)GTK_TABLE)table), type_label, 0, 1, 0, 1,
		(GtkAttachOptions))GTK_FILL),
		(GtkAttachOptions))0), 0, 0);
	gtk_misc_set_alignment)GTK_MISC)type_label), 0, 0.5);

	type_combo = gtk_combo_box_new_text));
	gtk_widget_show)type_combo);
	gtk_table_attach)GTK_TABLE)table), type_combo, 1, 2, 0, 1,
		(GtkAttachOptions))GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions))0), 0, 0);
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_STRING));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_INT));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_FILE));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_FLAG));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_FLOAT));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_RANGE));
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_ENUM));
	/* does not allow group-in-group */
	if (geoxml)
	gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo),
		g_datalist_get_data(gebrme.parameter_types, GEOXML_PARAMETERTYPE_GROUP));
	gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), type);
	g_signal_connect(type_combo, "changed",
		(GCallback)parameter_type_changed, data);

	gtk_widget_show(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
		
	/* TODO: do not use combobox index directly */
	geoxml_parameter_set_type(&data->parameter, (enum GEOXML_PARAMETERTYPE)gtk_combo_box_get_active(combo));
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	gtk_widget_destroy(dialog);

// 	/*
// 	 * Label
// 	 */
// 	label_label = gtk_label_new(_("Label:"));
// 	gtk_widget_show(label_label);
// 	gtk_table_attach(GTK_TABLE(parameter_table), label_label, 0, 1, 1, 2,
// 			(GtkAttachOptions)(GTK_FILL),
// 			(GtkAttachOptions)(0), 0, 0);
// 	gtk_misc_set_alignment(GTK_MISC(label_label), 0, 0.5);
// 
// 	label_entry = gtk_entry_new();
// 	gtk_widget_show(label_entry);
// 	gtk_table_attach(GTK_TABLE(parameter_table), label_entry, 1, 2, 1, 2,
// 		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
// 		(GtkAttachOptions)(0), 0, 0);
// 	/* read */
// 	gtk_entry_set_text(GTK_ENTRY(label_entry), geoxml_parameter_get_label(parameter));
// 	/* signal */
// 	g_signal_connect(label_entry, "changed",
// 		(GCallback)parameter_label_changed, data);
}

/*
 * Section: Private
 */

/*
 * Function: parameter_append_to_ui
 * Create an item for _paramater_ on parameter tree view
 */
static GtkTreeIter
paramater_append_to_ui(GeoXmlParameter * parameter, GtkTreeIter * parent)
{
	GtkTreeIter	iter;

	gtk_tree_store_append(gebrme.ui_parameter.tree_store, &iter, parent);
	gtk_tree_store_set(gebrme.ui_parameter.tree_store, &iter,
		PARAMETER_PARAMETER, geoxml_parameter_get_label(parameter),
		PARAMETER_TYPE, g_datalist_get_data(gebrme.parameter_types,
			geoxml_parameter_get_type(parameter)),
		PARAMETER_XMLPOINTER, parameter,
		-1);

	return iter;
}

/*
 * Function: parameter_select_iter
 * Select _iter_ loading its pointer
 */
static void
parameter_select_iter(GtkTreeIter iter)
{
	GtkTreeSelection *	tree_selection;

	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.ui_parameter.list_store), &iter,
		PARAMETER_XMLPOINTER, &gebrme.parameter,
		-1);
	tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.ui_parameter.tree_view));
	gtk_tree_selection_select_iter(tree_selection, &iter);
}


GtkWidget *
parameter_create_ui(GeoXmlParameter * parameter, struct parameters_data * parameters_data, gboolean expanded)
{
	struct parameter_data *		data;
	enum GEOXML_PARAMETERTYPE	type;

	GtkWidget *			frame;

	GtkWidget *			parameter_expander;
	GtkWidget *			parameter_label_widget;
	GtkWidget *			parameter_label;
	GtkWidget *			depth_hbox;
	GtkWidget *			parameter_vbox;
	GtkWidget *			parameter_table;

	GtkWidget *			general_table;

	GtkWidget *			button_hbox;
	GtkWidget *			up_button;
	GtkWidget *			down_button;
	GtkWidget *			duplicate_button;
	GtkWidget *			remove_button;
	GtkWidget *			align;

	type = geoxml_parameter_get_type(parameter);

	data = g_malloc(sizeof(struct parameter_data));
	data->parameter = parameter;
	data->parameters_data = parameters_data;

	data->frame = frame = gtk_frame_new("");
	gtk_widget_show(frame);
	g_object_set(G_OBJECT(frame), "shadow-type", GTK_SHADOW_OUT, NULL);
	geoxml_object_set_user_data(GEOXML_OBJECT(data->parameter), data);

	parameter_expander = gtk_expander_new("");
	gtk_container_add(GTK_CONTAINER(frame), parameter_expander);
	gtk_expander_set_expanded(GTK_EXPANDER(parameter_expander), !expanded);
	gtk_widget_show(parameter_expander);
	depth_hbox = gtk_container_add_depth_hbox(parameter_expander);
	g_signal_connect(parameter_expander, "destroy",
		GTK_SIGNAL_FUNC(parameter_data_free), data);

	parameter_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(parameter_expander), parameter_label_widget);
	gtk_expander_hacked_define(parameter_expander, parameter_label_widget);

	if (parameters_data->is_group == TRUE) {
		GtkWidget *	radio_button;

		radio_button = gtk_radio_button_new(
			((struct group_parameters_data *)parameters_data)->radio_group);
		((struct group_parameters_data *)parameters_data)->radio_group =
			gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button));
		gtk_box_pack_start(GTK_BOX(parameter_label_widget), radio_button, FALSE, FALSE, 0);
		g_signal_connect(radio_button, "toggled",
			(GCallback)parameter_change_exclusive, data);

		parameter_label = gtk_label_new("");
		data->label = parameter_label;
		gtk_widget_show(parameter_label);
		gtk_box_pack_start(GTK_BOX(parameter_label_widget), parameter_label, FALSE, FALSE, 0);
		g_object_set(G_OBJECT(parameter_label), "user-data", radio_button, NULL);

		gtk_widget_show_all(parameter_label_widget);
	} else {
		parameter_label_widget = gtk_hbox_new(FALSE, 0);
		gtk_expander_set_label_widget(GTK_EXPANDER(parameter_expander), parameter_label_widget);
		gtk_widget_show(parameter_label_widget);
		gtk_expander_hacked_define(parameter_expander, parameter_label_widget);

		parameter_label = gtk_label_new("");
		data->label = parameter_label;
		gtk_widget_show(parameter_label);
		gtk_box_pack_start(GTK_BOX(parameter_label_widget), parameter_label, FALSE, FALSE, 0);
	}

	parameter_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(depth_hbox), parameter_vbox, TRUE, TRUE, 0);
	gtk_widget_show(parameter_vbox);

	parameter_table = gtk_table_new (3, 2, FALSE);
	gtk_widget_show (parameter_table);
	gtk_box_pack_start(GTK_BOX(parameter_vbox), parameter_table, FALSE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (parameter_table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (parameter_table), 5);


	/*
	 * General parameters fields
	 */
	general_table = gtk_table_new(5, 2, FALSE);
	gtk_widget_show(general_table);
	gtk_box_pack_start(GTK_BOX(parameter_vbox), general_table, FALSE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(general_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(general_table), 5);
	g_object_set(G_OBJECT(type_combo), "user-data", general_table, NULL);

	/* create parameter fields on GUI */
	parameter_create_ui_type_general(general_table, data);
	parameter_uilabel_update(data);

	/* finishing...
	 * Buttons Up, Down and Remove
	 */
	button_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(button_hbox);
	gtk_box_pack_end(GTK_BOX(parameter_vbox), button_hbox, TRUE, FALSE, 0);

	up_button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_widget_show(up_button);
	gtk_box_pack_start(GTK_BOX(button_hbox), up_button, FALSE, FALSE, 5);
	g_signal_connect(up_button, "clicked",
		GTK_SIGNAL_FUNC(parameter_up), data);
	g_object_set(G_OBJECT(up_button),
		"relief", GTK_RELIEF_NONE,
		NULL);

	down_button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_widget_show(down_button);
	gtk_box_pack_start(GTK_BOX(button_hbox), down_button, FALSE, FALSE, 5);
	g_signal_connect(down_button, "clicked",
		GTK_SIGNAL_FUNC(parameter_down), data);
	g_object_set(G_OBJECT(down_button),
		"relief", GTK_RELIEF_NONE,
		NULL);

	duplicate_button = gtk_button_new();
	gtk_widget_show(duplicate_button);
	gtk_box_pack_start(GTK_BOX(button_hbox), duplicate_button, FALSE, FALSE, 5);
	g_signal_connect(duplicate_button, "clicked",
		GTK_SIGNAL_FUNC(parameter_duplicate), data);
	g_object_set(G_OBJECT(duplicate_button),
		"label", _("Duplicate"),
		"image", gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_SMALL_TOOLBAR),
		"relief", GTK_RELIEF_NONE,
		NULL);

	align = gtk_alignment_new(1, 0, 0, 1);
	gtk_widget_show(align);
	remove_button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_show(remove_button);
	gtk_box_pack_start(GTK_BOX(button_hbox), align, TRUE, TRUE, 5);
	gtk_container_add(GTK_CONTAINER(align), remove_button);
	g_signal_connect(remove_button, "clicked",
		GTK_SIGNAL_FUNC(parameter_remove), data);
	g_object_set(G_OBJECT(remove_button),
		"relief", GTK_RELIEF_NONE,
		NULL);

	if (data->parameters_data->is_group == TRUE) {
		struct parameter_data *	group_data;
		gboolean		one_instance;

		group_data = ((struct group_parameters_data*)data->parameters_data)->parameter;
		one_instance = geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(group_data->parameter)) == 1;

		gtk_widget_set_sensitive(up_button, one_instance);
		gtk_widget_set_sensitive(down_button, one_instance);
		gtk_widget_set_sensitive(duplicate_button, one_instance);
		gtk_widget_set_sensitive(remove_button, one_instance);
	}

	return frame;
}

void
parameter_create_ui_type_specific(GtkWidget * table, struct parameter_data * data)
{
	GtkWidget *			default_label;
	GtkWidget *			default_widget_hbox;
	GtkWidget *			default_widget;
	GtkWidget *			widget;

	enum GEOXML_PARAMETERTYPE	type;
	GeoXmlProgramParameter *	program_parameter;

	type = geoxml_parameter_get_type(data->parameter);
	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		if (geoxml_parameter_group_get_can_instanciate(GEOXML_PARAMETER_GROUP(data->parameter)) == TRUE) {
			GtkWidget *	instances_label;
			GtkWidget *	instances_spin;

			instances_label = gtk_label_new(_("Instances:"));
			gtk_widget_show(instances_label);
			gtk_table_attach(GTK_TABLE(table), instances_label, 0, 1, 0, 1,
				(GtkAttachOptions)(GTK_FILL),
				(GtkAttachOptions)(GTK_FILL), 0, 0);
			gtk_misc_set_alignment(GTK_MISC(instances_label), 0, 0.5);

			instances_spin = gtk_spin_button_new_with_range(1, 999999999, 1);
			gtk_widget_show(instances_spin);
			gtk_table_attach(GTK_TABLE(table), instances_spin, 1, 2, 0, 1,
				(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(instances_spin),
				geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(data->parameter)));
			g_signal_connect(instances_spin, "output",
				(GCallback)parameter_group_instances_changed, data);
		}

		data->specific.group.parameters_data = group_parameters_create_ui(data, TRUE);
		gtk_table_attach(GTK_TABLE(table), data->specific.group.parameters_data->widget, 0, 2, 1, 2,
			(GtkAttachOptions)(GTK_FILL|GTK_EXPAND),
			(GtkAttachOptions)(GTK_FILL|GTK_EXPAND), 0, 0);

		return;
	}

	default_label = gtk_label_new(_("Default:"));
	gtk_widget_show(default_label);
	widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(widget), default_label, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(default_label), 0, 0.5);

	default_widget_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(default_widget_hbox);

	program_parameter = GEOXML_PROGRAM_PARAMETER(data->parameter);
	switch (type) {
	case GEOXML_PARAMETERTYPE_STRING:
		data->specific.widget = parameter_widget_new_string(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_INT:
		data->specific.widget = parameter_widget_new_int(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_FLOAT:
		data->specific.widget = parameter_widget_new_float(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		data->specific.widget = parameter_widget_new_flag(data->parameter, TRUE);
		break;
	case GEOXML_PARAMETERTYPE_FILE: {
		GtkWidget *		type_label;
		GtkWidget *		type_combo;

		data->specific.widget = parameter_widget_new_file(data->parameter, NULL, TRUE);

		type_label = gtk_label_new (_("Type:"));
		gtk_widget_show (type_label);
		gtk_table_attach (GTK_TABLE (table), type_label, 0, 1, 1, 2,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_misc_set_alignment (GTK_MISC (type_label), 0, 0.5);

		type_combo = gtk_combo_box_new_text ();
		gtk_widget_show (type_combo);
		gtk_table_attach (GTK_TABLE (table), type_combo, 1, 2, 1, 2,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("File"));
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), _("Directory"));
		g_signal_connect(type_combo, "changed",
				(GCallback)parameter_file_type_changed, data);

		/* file or directory? */
		gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo),
			geoxml_program_parameter_get_file_be_directory(program_parameter) == TRUE ? 1 : 0);
		break;
	} case GEOXML_PARAMETERTYPE_RANGE: {
		GtkWidget *	min_label;
		GtkWidget *	min_entry;
		GtkWidget *	max_label;
		GtkWidget *	max_entry;
		GtkWidget *	inc_label;
		GtkWidget *	inc_entry;
		GtkWidget *	digits_label;
		GtkWidget *	digits_entry;
		gchar *		min_str;
		gchar *		max_str;
		gchar *		inc_str;
		gchar *		digits_str;

		geoxml_program_parameter_get_range_properties(program_parameter,
			&min_str, &max_str, &inc_str, &digits_str);
		data->specific.widget = parameter_widget_new_range(data->parameter, TRUE);

		min_label = gtk_label_new(_("Minimum:"));
		gtk_widget_show(min_label);
		gtk_table_attach(GTK_TABLE(table), min_label, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(min_label), 0, 0.5);
		min_entry = gtk_entry_new();
		gtk_widget_show(min_entry);
		gtk_table_attach(GTK_TABLE(table), min_entry, 1, 2, 1, 2,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(min_entry), min_str);
		g_signal_connect(min_entry, "changed",
			(GCallback)parameter_range_min_changed, data);

		max_label = gtk_label_new(_("Maximum:"));
		gtk_widget_show(max_label);
		gtk_table_attach(GTK_TABLE(table), max_label, 0, 1, 2, 3,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(max_label), 0, 0.5);
		max_entry = gtk_entry_new();
		gtk_widget_show(max_entry);
		gtk_table_attach(GTK_TABLE(table), max_entry, 1, 2, 2, 3,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(max_entry), max_str);
		g_signal_connect(max_entry, "changed",
			(GCallback)parameter_range_max_changed, data);

		inc_label = gtk_label_new(_("Increment:"));
		gtk_widget_show(inc_label);
		gtk_table_attach(GTK_TABLE(table), inc_label, 0, 1, 4, 5,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(inc_label), 0, 0.5);
		inc_entry = gtk_entry_new();
		gtk_widget_show(inc_entry);
		gtk_table_attach(GTK_TABLE(table), inc_entry, 1, 2, 4, 5,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(inc_entry), inc_str);
		g_signal_connect(inc_entry, "changed",
			(GCallback)parameter_range_inc_changed, data);

		digits_label = gtk_label_new(_("Digits:"));
		gtk_widget_show(digits_label);
		gtk_table_attach(GTK_TABLE(table), digits_label, 0, 1, 5, 6,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(digits_label), 0, 0.5);
		digits_entry = gtk_entry_new();
		gtk_widget_show(digits_entry);
		gtk_table_attach(GTK_TABLE(table), digits_entry, 1, 2, 5, 6,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_entry_set_text(GTK_ENTRY(digits_entry), digits_str);
		g_signal_connect(digits_entry, "changed",
			(GCallback)parameter_range_digits_changed, data);

		break;
	} case GEOXML_PARAMETERTYPE_ENUM: {
		GtkWidget *		enum_option_edit;
		GtkWidget *		options_label;
		GtkWidget *		vbox;

		GeoXmlSequence *	option;

		data->specific.widget = parameter_widget_new_enum(data->parameter, TRUE);

		options_label = gtk_label_new (_("Options:"));
		gtk_widget_show(options_label);
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(vbox);
		gtk_box_pack_start(GTK_BOX(vbox), options_label, FALSE, FALSE, 0);
		gtk_table_attach(GTK_TABLE (table), vbox, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(GTK_FILL), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(options_label), 0, 0.5);

		geoxml_program_parameter_get_enum_option(GEOXML_PROGRAM_PARAMETER(data->parameter), &option, 0);
		enum_option_edit = enum_option_edit_new(GEOXML_ENUM_OPTION(option),
			GEOXML_PROGRAM_PARAMETER(data->parameter));
		gtk_widget_show(enum_option_edit);
		gtk_table_attach (GTK_TABLE (table), enum_option_edit, 1, 2, 1, 2,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		g_signal_connect(GTK_OBJECT(enum_option_edit), "changed",
			GTK_SIGNAL_FUNC(parameter_enum_options_changed), data);
		g_object_set(G_OBJECT(enum_option_edit), "user-data", default_widget_hbox, NULL);

		break;
	} default:
		data->specific.widget = NULL;
		return;
	}

	if (parameter_is_exclusive(data) == TRUE) {
		GtkToggleButton *	radio_button;

		g_object_get(G_OBJECT(data->label), "user-data", &radio_button, NULL);
		gtk_toggle_button_set_active(radio_button, TRUE);
	}

	default_widget = data->specific.widget->widget;
	gtk_widget_show(default_widget);
	parameter_widget_set_auto_submit_callback(data->specific.widget,
		(changed_callback)parameter_default_widget_changed, data);

	gtk_box_pack_start(GTK_BOX(default_widget_hbox), default_widget, TRUE, TRUE, 0);
	gtk_table_attach(GTK_TABLE(table), default_widget_hbox, 1, 2, 0, 1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
}

/*
 * Section: Private
 */

static void
parameter_create_ui_type_general(GtkWidget * table, struct parameter_data * data)
{
	GtkWidget *			specific_table;
	enum GEOXML_PARAMETERTYPE	type;

	type = geoxml_parameter_get_type(data->parameter);
	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		GtkWidget *		exclusive_label;
		GtkWidget *		exclusive_check_button;
		GtkWidget *		expanded_label;
		GtkWidget *		expanded_check_button;
		GtkWidget *		multiple_label;
		GtkWidget *		multiple_check_button;

		GeoXmlParameterGroup *	parameter_group;

		parameter_group = GEOXML_PARAMETER_GROUP(data->parameter);

		exclusive_label = gtk_label_new(_("Exclusive:"));
		gtk_widget_show(exclusive_label);
		gtk_table_attach(GTK_TABLE(table), exclusive_label, 0, 1, 0, 1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(exclusive_label), 0, 0.5);

		exclusive_check_button = gtk_check_button_new();
		data->specific.group.exclusive_check_button = exclusive_check_button;
		gtk_widget_show(exclusive_check_button);
		gtk_table_attach(GTK_TABLE(table), exclusive_check_button, 1, 2, 0, 1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(exclusive_check_button),
			geoxml_parameter_group_get_exclusive(parameter_group) != NULL);
		g_signal_connect(exclusive_check_button, "toggled",
			(GCallback)parameter_group_exclusive_changed, data);

		expanded_label = gtk_label_new(_("Expanded by default:"));
		gtk_widget_show(expanded_label);
		gtk_table_attach(GTK_TABLE(table), expanded_label, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(expanded_label), 0, 0.5);

		expanded_check_button = gtk_check_button_new();
		gtk_widget_show(expanded_check_button);
		gtk_table_attach(GTK_TABLE(table), expanded_check_button, 1, 2, 1, 2,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		g_signal_connect(expanded_check_button, "toggled",
			(GCallback)parameter_group_expanded_changed, data);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(expanded_check_button),
			geoxml_parameter_group_get_expand(parameter_group));

		multiple_label = gtk_label_new(_("Instanciable:"));
		gtk_widget_show(multiple_label);
		gtk_table_attach(GTK_TABLE(table), multiple_label, 0, 1, 2, 3,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(multiple_label), 0, 0.5);

		multiple_check_button = gtk_check_button_new();
		gtk_widget_show(multiple_check_button);
		gtk_table_attach(GTK_TABLE(table), multiple_check_button, 1, 2, 2, 3,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multiple_check_button),
			geoxml_parameter_group_get_can_instanciate(parameter_group));
		g_signal_connect(multiple_check_button, "toggled",
			(GCallback)parameter_group_multiple_changed, data);
	} else {
		GtkWidget *			keyword_label;
		GtkWidget *			keyword_entry;
		GeoXmlProgramParameter *	program_parameter;

		program_parameter = GEOXML_PROGRAM_PARAMETER(data->parameter);

		/*
 		 * Keyword
 		 */
		keyword_label = gtk_label_new(_("Keyword:"));
		gtk_widget_show(keyword_label);
		gtk_table_attach(GTK_TABLE(table), keyword_label, 0, 1, 0, 1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		gtk_misc_set_alignment(GTK_MISC(keyword_label), 0, 0.5);

		keyword_entry = gtk_entry_new();
		gtk_widget_show(keyword_entry);
		gtk_table_attach(GTK_TABLE(table), keyword_entry, 1, 2, 0, 1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
		/* read */
		gtk_entry_set_text(GTK_ENTRY(keyword_entry),
			geoxml_program_parameter_get_keyword(program_parameter));
		/* signal */
		g_signal_connect(keyword_entry, "changed",
			(GCallback)parameter_keyword_changed, data);
		g_object_set(G_OBJECT(keyword_entry), "user-data", data->label, NULL);

		if (type != GEOXML_PARAMETERTYPE_FLAG) {
			GtkWidget *	required_label;
			GtkWidget *	required_check_button;
			GtkWidget *	is_list_label;
			GtkWidget *	is_list_check_button;
			gboolean	is_list;

			required_label = gtk_label_new(_("Required:"));
			gtk_widget_show(required_label);
			gtk_table_attach(GTK_TABLE(table), required_label, 0, 1, 1, 2,
				(GtkAttachOptions)(GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			gtk_misc_set_alignment(GTK_MISC(required_label), 0, 0.5);

			required_check_button = gtk_check_button_new();
			gtk_widget_show(required_check_button);
			gtk_table_attach(GTK_TABLE(table), required_check_button, 1, 2, 1, 2,
				(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			g_signal_connect(required_check_button, "toggled",
				(GCallback)parameter_required_changed, data);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(required_check_button),
				geoxml_program_parameter_get_required(program_parameter));

			is_list_label = gtk_label_new(_("Is list?:"));
			gtk_widget_show(is_list_label);
			gtk_table_attach(GTK_TABLE(table), is_list_label, 0, 1, 2, 3,
				(GtkAttachOptions)(GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			gtk_misc_set_alignment(GTK_MISC(is_list_label), 0, 0.5);

			is_list_check_button = gtk_check_button_new();
			gtk_widget_show(is_list_check_button);
			gtk_table_attach(GTK_TABLE(table), is_list_check_button, 1, 2, 2, 3,
				(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions)(0), 0, 0);
			is_list = geoxml_program_parameter_get_is_list(program_parameter);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(is_list_check_button), is_list);
			g_signal_connect(is_list_check_button, "toggled",
				(GCallback)parameter_is_list_changed, data);
			g_object_set(G_OBJECT(is_list_check_button), "user-data", table, NULL);

			if (is_list == TRUE) {
				GtkWidget *	separator_label;
				GtkWidget *	separator_entry;

				separator_label = gtk_label_new(_("List separator:"));
				gtk_widget_show(separator_label);
				gtk_table_attach(GTK_TABLE(table), separator_label, 0, 1, 3, 4,
					(GtkAttachOptions)(GTK_FILL),
					(GtkAttachOptions)(0), 0, 0);
				gtk_misc_set_alignment(GTK_MISC(separator_label), 0, 0.5);

				separator_entry = gtk_entry_new();
				gtk_widget_show(separator_entry);
				gtk_table_attach(GTK_TABLE(table), separator_entry, 1, 2, 3, 4,
					(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions)(0), 0, 0);
				gtk_entry_set_text(GTK_ENTRY(separator_entry),
					geoxml_program_parameter_get_list_separator(program_parameter));
				g_signal_connect(separator_entry, "changed",
					(GCallback)parameter_separator_changed, data);
			}
		}
	}

	if (data->parameters_data->is_group == TRUE) {
		GeoXmlParameterGroup *	parameter_group;
		GtkObject *		radio_button;

		g_object_get(G_OBJECT(data->label), "user-data", &radio_button, NULL);
		parameter_group = GEOXML_PARAMETER_GROUP(
			((struct group_parameters_data *)data->parameters_data)->parameter->parameter);
		g_object_set(radio_button,
			"visible", geoxml_parameter_group_get_exclusive(parameter_group) != NULL,
			NULL);
	}

	specific_table = gtk_table_new(6, 2, FALSE);
	data->specific_table = specific_table;
	gtk_widget_show(specific_table);
	gtk_table_attach(GTK_TABLE(table), specific_table, 0, 2, 4, 5,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_table_set_row_spacings(GTK_TABLE(specific_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(specific_table), 5);

	parameter_create_ui_type_specific(specific_table, data);
}

static void
parameter_data_free(GtkObject * expander, struct parameter_data * data)
{
	g_free(data);
}

static void
parameter_up(GtkButton * button, struct parameter_data * data)
{
	GtkWidget *	vbox;
	GList *		parameters_frames;
	GList *		this;
	GList *		up;

	vbox = gtk_widget_get_parent(data->frame);
	parameters_frames = gtk_container_get_children(GTK_CONTAINER(vbox));
	this = g_list_find(parameters_frames, data->frame);
	up = g_list_previous(this);
	if (up != NULL) {
		gtk_box_reorder_child(GTK_BOX(vbox), up->data, g_list_position(parameters_frames, this));
		geoxml_sequence_move_up(GEOXML_SEQUENCE(data->parameter));
	}

	g_list_free(parameters_frames);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_down(GtkButton * button, struct parameter_data * data)
{
	GtkWidget *	vbox;
	GList *		parameters_frames;
	GList *		this;
	GList *		down;

	vbox = gtk_widget_get_parent(data->frame);
	parameters_frames = gtk_container_get_children(GTK_CONTAINER(vbox));
	this = g_list_find(parameters_frames, data->frame);
	down = g_list_next(this);
	if (down != NULL) {
		gtk_box_reorder_child(GTK_BOX(vbox), down->data, g_list_position(parameters_frames, this));
		geoxml_sequence_move_down(GEOXML_SEQUENCE(data->parameter));
	}

	g_list_free(parameters_frames);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_required_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	geoxml_program_parameter_set_required(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_is_list_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	GtkWidget *	table;

	g_object_get(G_OBJECT(toggle_button), "user-data", &table, NULL);

	geoxml_program_parameter_set_be_list(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	/* rebuild ui */
	gtk_container_foreach(GTK_CONTAINER(table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_general(table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_separator_changed(GtkEntry * entry, struct parameter_data * data)
{
	geoxml_program_parameter_set_list_separator(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));

	/* rebuild specific ui */
	gtk_container_foreach(GTK_CONTAINER(data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(data->specific_table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_label_changed(GtkEntry * entry, struct parameter_data * data)
{
	geoxml_parameter_set_label(data->parameter, gtk_entry_get_text(GTK_ENTRY(entry)));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_keyword_changed(GtkEntry * entry, struct parameter_data * data)
{
	GtkWidget *	parameter_label;

	g_object_get(G_OBJECT(entry), "user-data", &parameter_label, NULL);
	geoxml_program_parameter_set_keyword(GEOXML_PROGRAM_PARAMETER(data->parameter),
		gtk_entry_get_text(GTK_ENTRY(entry)));

	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_default_widget_changed(struct parameter_widget * widget, struct parameter_data * data)
{
	parameter_uilabel_update(data);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_file_type_changed(GtkComboBox * combo, struct parameter_data * data)
{
	gboolean	is_directory;

	is_directory = gtk_combo_box_get_active(combo) == 0 ? FALSE : TRUE;

	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(data->specific.widget->value_widget), is_directory);
	geoxml_program_parameter_set_file_be_directory(GEOXML_PROGRAM_PARAMETER(data->parameter), is_directory);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_min_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		min, max;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->specific.widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);
	gtk_spin_button_get_range(spin_button, &min, &max);

	min_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	min = atof(min_str);

	gtk_spin_button_set_range(spin_button, min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_max_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		min, max;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->specific.widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);
	gtk_spin_button_get_range(spin_button, &min, &max);

	max_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	max = atof(max_str);

	gtk_spin_button_set_range(spin_button, min, max);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_inc_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		inc;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->specific.widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	inc_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	inc = atof(inc_str);

	gtk_spin_button_set_increments(spin_button, inc, 0);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_range_digits_changed(GtkEntry * entry, struct parameter_data * data)
{
	gchar *		min_str;
	gchar *		max_str;
	gchar *		inc_str;
	gchar *		digits_str;
	gdouble		digits;
	GtkSpinButton *	spin_button;

	spin_button = GTK_SPIN_BUTTON(data->specific.widget->value_widget);
	geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		&min_str, &max_str, &inc_str, &digits_str);

	digits_str = (gchar*)gtk_entry_get_text(GTK_ENTRY(entry));
	digits = atof(digits_str);

	gtk_spin_button_set_digits(spin_button, digits);
	geoxml_program_parameter_set_range_properties(GEOXML_PROGRAM_PARAMETER(data->parameter),
		min_str, max_str, inc_str, digits_str);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_enum_options_changed(EnumOptionEdit * enum_option_edit, struct parameter_data * data)
{
	GtkWidget *	default_widget_hbox;

	g_object_get(G_OBJECT(enum_option_edit), "user-data", &default_widget_hbox, NULL);

	gtk_widget_destroy(data->specific.widget->widget);
	data->specific.widget = parameter_widget_new_enum(data->parameter, TRUE);
	parameter_widget_set_auto_submit_callback(data->specific.widget,
		(changed_callback)parameter_default_widget_changed, data);
	gtk_widget_show(data->specific.widget->widget);
	gtk_box_pack_start(GTK_BOX(default_widget_hbox), data->specific.widget->widget, TRUE, TRUE, 0);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
parameter_is_exclusive(struct parameter_data * data)
{
	if (data->parameters_data->is_group == FALSE)
		return FALSE;

	GeoXmlParameterGroup *	parameter_group;

	parameter_group = GEOXML_PARAMETER_GROUP(
		((struct group_parameters_data *)data->parameters_data)->parameter->parameter);

	return geoxml_parameter_group_get_exclusive(parameter_group) == data->parameter;
}

static void
parameter_change_exclusive(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	GeoXmlParameterGroup *	parameter_group;

	parameter_group = GEOXML_PARAMETER_GROUP(
		((struct group_parameters_data *)data->parameters_data)->parameter->parameter);
	geoxml_parameter_group_set_exclusive(parameter_group, data->parameter);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_group_exclusive_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	if (gtk_toggle_button_get_active(toggle_button) == TRUE)
		geoxml_parameter_group_set_exclusive(GEOXML_PARAMETER_GROUP(data->parameter),
			GEOXML_PARAMETER(geoxml_parameters_get_first_parameter(
				geoxml_parameter_group_get_parameters(GEOXML_PARAMETER_GROUP(data->parameter)))));
	else
		geoxml_parameter_group_set_exclusive(GEOXML_PARAMETER_GROUP(data->parameter), NULL);
	((struct group_parameters_data *)data->parameters_data)->radio_group = NULL;

	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(data->specific_table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_group_expanded_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	geoxml_parameter_group_set_expand(GEOXML_PARAMETER_GROUP(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static void
parameter_group_multiple_changed(GtkToggleButton * toggle_button, struct parameter_data * data)
{
	geoxml_parameter_group_set_can_instanciate(GEOXML_PARAMETER_GROUP(data->parameter),
		gtk_toggle_button_get_active(toggle_button));

	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(data->specific_table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

static gboolean
parameter_group_instances_changed(GtkSpinButton * spin_button, struct parameter_data * data)
{
	gint	i, instanciate;

	instanciate = gtk_spin_button_get_value(spin_button) -
	geoxml_parameter_group_get_instances(GEOXML_PARAMETER_GROUP(data->parameter));
	if (instanciate == 0)
		return FALSE;

	if (instanciate > 0)
		for (i = 0; i < instanciate; ++i)
			geoxml_parameter_group_instanciate(GEOXML_PARAMETER_GROUP(data->parameter));
	else {
		GeoXmlParameter *	exclusive;

		exclusive = geoxml_parameter_group_get_exclusive(GEOXML_PARAMETER_GROUP(data->parameter));

		for (i = instanciate; i < 0; ++i)
			geoxml_parameter_group_deinstanciate(GEOXML_PARAMETER_GROUP(data->parameter));

		/* the exclusive parameter was deleted? */
		if (exclusive != geoxml_parameter_group_get_exclusive(GEOXML_PARAMETER_GROUP(data->parameter)))
			group_parameters_reset_exclusive(data->specific.group.parameters_data);
	}

	/* rebuild parameters' widgets */
	gtk_container_foreach(GTK_CONTAINER(data->specific_table), (GtkCallback)gtk_widget_destroy, NULL);
	parameter_create_ui_type_specific(data->specific_table, data);

	menu_saved_status_set(MENU_STATUS_UNSAVED);

	return FALSE;
}
