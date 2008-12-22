/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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

//remove round warning
#define _ISOC99_SOURCE
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "parameter.h"
#include "support.h"

#define DOUBLE_MAX +999999999
#define DOUBLE_MIN -999999999

/*
 * Section: Private
 * Internal stuff
 */

static GString *
__parameter_widget_get_widget_value(struct parameter_widget * parameter_widget, gboolean check_list)
{
	GString *			value;
	enum GEOXML_PARAMETERTYPE	type;

	value = g_string_new(NULL);

	if (check_list && geoxml_program_parameter_get_is_list(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter)) == TRUE) {
		g_string_assign(value, gtk_entry_get_text(GTK_ENTRY(parameter_widget->list_value_widget)));
		return value;
	}

	type = geoxml_parameter_get_type(parameter_widget->parameter);
	while (type == GEOXML_PARAMETERTYPE_REFERENCE)
		type = geoxml_parameter_get_type(geoxml_parameter_get_referencee(parameter_widget->parameter));
	switch (type) {
	case GEOXML_PARAMETERTYPE_FLOAT:
	case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_STRING:
		g_string_assign(value, gtk_entry_get_text(GTK_ENTRY(parameter_widget->value_widget)));
		break;
	case GEOXML_PARAMETERTYPE_RANGE: {
		guint	digits;
		digits = gtk_spin_button_get_digits(GTK_SPIN_BUTTON(parameter_widget->value_widget));
		if (digits == 0)
			g_string_printf(value, "%d", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(parameter_widget->value_widget)));
		else {
			GString *	mask;

			mask = g_string_new(NULL);
			g_string_printf(mask, "%%.%if", digits);
			g_string_printf(value, mask->str,
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(parameter_widget->value_widget)));

			g_string_free(mask, TRUE);
		}
		break;
	} case GEOXML_PARAMETERTYPE_FILE:
		g_string_assign(value, gtk_file_entry_get_path(GTK_FILE_ENTRY(parameter_widget->value_widget)));
		break;
	case GEOXML_PARAMETERTYPE_ENUM: {
		gint	index;

		index = gtk_combo_box_get_active(GTK_COMBO_BOX(parameter_widget->value_widget));
		if (index == -1)
			g_string_assign(value, "");
		else {
			GeoXmlSequence *	enum_option;

			/* minus one to skip the first empty value */
			geoxml_program_parameter_get_enum_option(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
				&enum_option, index-1);
			g_string_assign(value, geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(enum_option)));
		}

		break;
	} case GEOXML_PARAMETERTYPE_FLAG:
		g_string_assign(value,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget)) == TRUE
				? "on" : "off");
		break;
	default:
		break;
	}

	return value;
}

static void
parameter_widget_report_change(struct parameter_widget * parameter_widget)
{
	if (parameter_widget->callback != NULL)
		parameter_widget->callback(parameter_widget, parameter_widget->user_data);
}

/*
 * Function: parameter_list_value_widget_changed
 * Update XML when user is manually editing the list
 */
static void
parameter_list_value_widget_changed(GtkEntry * entry, struct parameter_widget * parameter_widget)
{
	GeoXmlSequence *	property_value;

	geoxml_program_parameter_set_parse_list_value(
		GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
		parameter_widget->use_default_value,
		gtk_entry_get_text(entry));

	geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
		parameter_widget->use_default_value, &property_value, 0);
	g_object_set(G_OBJECT(parameter_widget->value_sequence_edit), "value-sequence", property_value, NULL);

	parameter_widget_report_change(parameter_widget);
}

/*
 * Function: parameter_widget_sync_non_list
 * Syncronize input widget value with the parameter
 */
static void
parameter_widget_sync_non_list(struct parameter_widget * parameter_widget)
{
	GString *	value;

	value = __parameter_widget_get_widget_value(parameter_widget, FALSE);
	geoxml_program_parameter_set_first_value(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
		parameter_widget->use_default_value, value->str);

	g_string_free(value, TRUE);
}

static void
parameter_widget_changed(GtkWidget * widget, struct parameter_widget * parameter_widget)
{
	parameter_widget_sync_non_list(parameter_widget);
	parameter_widget_report_change(parameter_widget);
}

static gboolean
parameter_widget_range_changed(GtkSpinButton * spinbutton, struct parameter_widget * parameter_widget)
{
	parameter_widget_sync_non_list(parameter_widget);
	parameter_widget_report_change(parameter_widget);
	return FALSE;
}

/*
 * Function: parameter_widget_weak_ref
 * Free struct before widget deletion
 */
static void
parameter_widget_weak_ref(struct parameter_widget * parameter_widget, GtkWidget * widget)
{
	if (parameter_widget->value_sequence_edit)
		g_object_unref(G_OBJECT(parameter_widget->value_sequence_edit));
	g_free(parameter_widget);
}

static void
parameter_list_value_widget_changed(GtkEntry * entry, struct parameter_widget * parameter_widget);

/*
 * Function: parameter_list_value_widget_update
 * Update from the XML
 */
static void
parameter_list_value_widget_update(struct parameter_widget * parameter_widget)
{
	GString *	value;

	value = geoxml_program_parameter_get_string_value(
		GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
		parameter_widget->use_default_value);

	g_signal_handlers_block_matched(G_OBJECT(parameter_widget->list_value_widget),
		G_SIGNAL_MATCH_FUNC,
		0, 0, NULL,
		(GCallback)parameter_list_value_widget_changed,
		NULL);
	gtk_entry_set_text(GTK_ENTRY(parameter_widget->list_value_widget), value->str);
	g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->list_value_widget),
		G_SIGNAL_MATCH_FUNC,
		0, 0, NULL,
		(GCallback)parameter_list_value_widget_changed,
		NULL);

	g_string_free(value, TRUE);
}

static void
on_sequence_edit_changed(GtkSequenceEdit * sequence_edit, struct parameter_widget * parameter_widget);
static void
parameter_widget_changed(GtkWidget * widget, struct parameter_widget * parameter_widget);

/*
 * Function: on_edit_list_toggled
 * Take action to start/finish editing list of parameter's values
 */
static void
on_edit_list_toggled(GtkToggleButton * toggle_button, struct parameter_widget * parameter_widget)
{
	gboolean		toggled;

	g_signal_handlers_block_matched(G_OBJECT(parameter_widget->value_sequence_edit),
		G_SIGNAL_MATCH_FUNC,
		0, 0, NULL,
		(GCallback)parameter_widget_changed,
		NULL);

	toggled = gtk_toggle_button_get_active(toggle_button);
	if (toggled == TRUE) {
		g_signal_handlers_block_matched(G_OBJECT(parameter_widget->list_value_widget),
			G_SIGNAL_MATCH_FUNC,
			0, 0, NULL,
			(GCallback)parameter_list_value_widget_changed,
			NULL);
		g_signal_handlers_block_matched(G_OBJECT(parameter_widget->value_sequence_edit),
			G_SIGNAL_MATCH_FUNC,
			0, 0, NULL,
			(GCallback)on_sequence_edit_changed,
			NULL);
		value_sequence_edit_load(parameter_widget->value_sequence_edit);
		g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->list_value_widget),
			G_SIGNAL_MATCH_FUNC,
			0, 0, NULL,
			(GCallback)parameter_list_value_widget_changed,
			NULL);
		g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->value_sequence_edit),
			G_SIGNAL_MATCH_FUNC,
			0, 0, NULL,
			(GCallback)on_sequence_edit_changed,
			NULL);

		gtk_box_pack_start(GTK_BOX(parameter_widget->widget),
			GTK_WIDGET(parameter_widget->value_sequence_edit), TRUE, TRUE, 0);
	} else {
		gtk_container_remove(GTK_CONTAINER(parameter_widget->widget),
			GTK_WIDGET(parameter_widget->value_sequence_edit));
	}

	gtk_widget_set_sensitive(parameter_widget->list_value_widget, !toggled);
	g_signal_handlers_unblock_matched(G_OBJECT(parameter_widget->value_sequence_edit),
		G_SIGNAL_MATCH_FUNC,
		0, 0, NULL,
		(GCallback)parameter_widget_changed,
		NULL);
}

static void
on_sequence_edit_add_request(ValueSequenceEdit * value_sequence_edit, struct parameter_widget * parameter_widget)
{
	GString *		value;
	GeoXmlValueSequence *	value_sequence;

	value = __parameter_widget_get_widget_value(parameter_widget, FALSE);
	value_sequence = GEOXML_VALUE_SEQUENCE(geoxml_program_parameter_append_value(
		GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter), parameter_widget->use_default_value));
	geoxml_value_sequence_set(value_sequence, value->str);
	value_sequence_edit_add(value_sequence_edit, value_sequence);

	g_string_free(value, TRUE);
}

static void
on_sequence_edit_changed(GtkSequenceEdit * sequence_edit, struct parameter_widget * parameter_widget)
{
	parameter_list_value_widget_update(parameter_widget);
}

/*
 * Function: parameter_widget_enum_set_widget_value
 *
 */
static void
parameter_widget_enum_set_widget_value(struct parameter_widget * parameter_widget, const gchar * value)
{
	GeoXmlSequence *	option;
	int			i;

	geoxml_program_parameter_get_enum_option(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter), &option, 0);
	for (i = 0; option != NULL; ++i, geoxml_sequence_next(&option))
		if (strcmp(value, geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(option))) == 0) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(parameter_widget->value_widget), i+1);
			return;
		}

	gtk_combo_box_set_active(GTK_COMBO_BOX(parameter_widget->value_widget), 0);
}

/*
 * Function: validate_int
 * Validate an int parameter
 */
static void
validate_int(GtkEntry * entry)
{
	gchar		number[15];
	gchar *		valuestr;
	gdouble		value;

	valuestr = (gchar*)gtk_entry_get_text(entry);
	if (strlen(valuestr) == 0)
		return;

	value = g_ascii_strtod(valuestr, NULL);
	gtk_entry_set_text(entry, g_ascii_dtostr(number, 15, round(value)));
}

/*
 * Function: validate_float
 * Validate a float parameter
 */
static void
validate_float(GtkEntry * entry)
{
	gchar *		valuestr;
	gchar *		last;
	GString *	value;

	valuestr = (gchar*)gtk_entry_get_text(entry);
	if (strlen(valuestr) == 0)
		return;

	/* initialization */
	value = g_string_new(NULL);

	g_ascii_strtod(valuestr, &last);
	g_string_assign(value, valuestr);
	g_string_truncate(value, strlen(valuestr) - strlen(last));
	gtk_entry_set_text(entry, value->str);

	/* frees */
	g_string_free(value, TRUE);
}

/*
 * Function: validate_on_leaving
 * Call a validation function
 */
static gboolean
validate_on_leaving(GtkWidget * widget, GdkEventFocus * event, void (*function)(GtkEntry*))
{
	function(GTK_ENTRY(widget));

	return FALSE;
}

/*
 * Function: parameter_widget_configure
 * Create UI
 */
static void
parameter_widget_configure(struct parameter_widget * parameter_widget)
{
	enum GEOXML_PARAMETERTYPE	type;

	type = geoxml_parameter_get_type(parameter_widget->parameter);
	while (type == GEOXML_PARAMETERTYPE_REFERENCE)
		type = geoxml_parameter_get_type(geoxml_parameter_get_referencee(parameter_widget->parameter));
	switch (type) {
	case GEOXML_PARAMETERTYPE_FLOAT: {
		GtkWidget *			entry;

		parameter_widget->value_widget = entry = gtk_entry_new();
		gtk_widget_set_size_request(entry, 90, 30);

		g_signal_connect(parameter_widget->value_widget, "changed",
			(GCallback)parameter_widget_changed, parameter_widget);
		g_signal_connect(entry, "activate",
			GTK_SIGNAL_FUNC(validate_float), NULL);
		g_signal_connect(entry, "focus-out-event",
			GTK_SIGNAL_FUNC(validate_on_leaving), &validate_float);

		break;
	} case GEOXML_PARAMETERTYPE_INT: {
		GtkWidget *			entry;

		parameter_widget->value_widget = entry = gtk_entry_new();
		gtk_widget_set_size_request(entry, 90, 30);

		g_signal_connect(parameter_widget->value_widget, "changed",
			(GCallback)parameter_widget_changed, parameter_widget);
		g_signal_connect(entry, "activate",
			GTK_SIGNAL_FUNC(validate_int), NULL);
		g_signal_connect(entry, "focus-out-event",
			GTK_SIGNAL_FUNC(validate_on_leaving), &validate_int);

		break;
	} case GEOXML_PARAMETERTYPE_STRING: {
		parameter_widget->value_widget = gtk_entry_new();
		gtk_widget_set_size_request(parameter_widget->value_widget, 140, 30);

		g_signal_connect(parameter_widget->value_widget, "changed",
			(GCallback)parameter_widget_changed, parameter_widget);
	
		break;
	} case GEOXML_PARAMETERTYPE_RANGE: {
		GtkWidget *			spin;

		gchar *				min_str;
		gchar *				max_str;
		gchar *				inc_str;
		gchar *				digits_str;
		double				min, max, inc, digits;

		geoxml_program_parameter_get_range_properties(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
			&min_str, &max_str, &inc_str, &digits_str);
		min = !strlen(min_str) ? DOUBLE_MIN : atof(min_str);
		max = !strlen(max_str) ? DOUBLE_MAX : atof(max_str);
		inc = !strlen(inc_str) ? 1.0 : atof(inc_str);
		digits = !strlen(digits_str) ? 0 : atof(digits_str);

		parameter_widget->value_widget = spin = gtk_spin_button_new_with_range(min, max, inc);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), atoi(digits_str));
		gtk_widget_set_size_request(spin, 90, 30);

		g_signal_connect(parameter_widget->value_widget, "output",
			(GCallback)parameter_widget_range_changed, parameter_widget);

		break;
	} case GEOXML_PARAMETERTYPE_FILE: {
		GtkWidget *		file_entry;

		/* file entry */
		parameter_widget->value_widget =
			file_entry = gtk_file_entry_new((GtkFileEntryCustomize)parameter_widget->data);
		gtk_widget_set_size_request(file_entry, 220, 30);

		gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry),
			geoxml_program_parameter_get_file_be_directory(
				GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter)));
		gtk_file_entry_set_do_overwrite_confirmation(GTK_FILE_ENTRY(file_entry), FALSE);

		g_signal_connect(parameter_widget->value_widget, "path-changed",
			(GCallback)parameter_widget_changed, parameter_widget);

		break;
	} case GEOXML_PARAMETERTYPE_ENUM: {
		GtkWidget *		combo_box;
		GeoXmlSequence *	sequence;

		parameter_widget->value_widget = combo_box = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), "");
		geoxml_program_parameter_get_enum_option(
			GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter), &sequence, 0);
		while (sequence != NULL) {
			const gchar *		text;

			text = strlen(geoxml_enum_option_get_label(GEOXML_ENUM_OPTION(sequence)))
				? geoxml_enum_option_get_label(GEOXML_ENUM_OPTION(sequence))
				: geoxml_enum_option_get_value(GEOXML_ENUM_OPTION(sequence));
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), text);

			geoxml_sequence_next(&sequence);
		}

		g_signal_connect(parameter_widget->value_widget, "changed",
			(GCallback)parameter_widget_changed, parameter_widget);

		break;
	} case GEOXML_PARAMETERTYPE_FLAG: {
		parameter_widget->value_widget = gtk_check_button_new();

		g_signal_connect(parameter_widget->value_widget, "toggled",
			(GCallback)parameter_widget_changed, parameter_widget);

		break;
	} default:
		return;
	}

	if (geoxml_program_parameter_get_is_list(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter)) == TRUE) {
		GtkWidget *		vbox;
		GtkWidget *		hbox;
		GtkWidget *		button;
		GtkWidget *		sequence_edit;

		GeoXmlSequence *	property_value;

		geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
			parameter_widget->use_default_value, &property_value, 0);

		vbox = gtk_vbox_new(FALSE, 10);
		hbox = gtk_hbox_new(FALSE, 10);
		gtk_widget_show(hbox);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

		parameter_widget->list_value_widget = gtk_entry_new();
		gtk_widget_show(parameter_widget->list_value_widget);
		gtk_box_pack_start(GTK_BOX(hbox), parameter_widget->list_value_widget, TRUE, TRUE, 0);
		g_signal_connect(parameter_widget->list_value_widget, "changed",
			(GCallback)parameter_list_value_widget_changed, parameter_widget);

		button = gtk_toggle_button_new_with_label(_("Edit list"));
		gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
		g_signal_connect(button, "toggled",
			GTK_SIGNAL_FUNC(on_edit_list_toggled), parameter_widget);

		gtk_widget_show(parameter_widget->value_widget);
		sequence_edit = value_sequence_edit_new_with_sequence(parameter_widget->value_widget,
			GEOXML_VALUE_SEQUENCE(property_value));
		gtk_widget_show(sequence_edit);
		parameter_widget->value_sequence_edit = VALUE_SEQUENCE_EDIT(sequence_edit);
		g_object_set(G_OBJECT(sequence_edit), "minimum-one", TRUE, NULL);
		g_signal_connect(sequence_edit, "add-request",
			GTK_SIGNAL_FUNC(on_sequence_edit_add_request), parameter_widget);
		g_signal_connect(sequence_edit, "changed",
			GTK_SIGNAL_FUNC(on_sequence_edit_changed), parameter_widget);
		/* for not being destroyed when removed from vbox */
		g_object_ref(G_OBJECT(sequence_edit));

		parameter_widget->widget = vbox;
	} else {
		parameter_widget->widget = parameter_widget->value_widget;
		parameter_widget->value_sequence_edit = NULL;
	}

	parameter_widget_update(parameter_widget);
	parameter_widget_set_auto_submit_callback(parameter_widget,
		parameter_widget->callback, parameter_widget->user_data);

	/* delete struct */
	g_object_weak_ref(G_OBJECT(parameter_widget->widget), (GWeakNotify)parameter_widget_weak_ref, parameter_widget);
}

/*
 * Public functions
 */

/*
 * Function: parameter_widget_new
 * Create a new parameter widget
 */
struct parameter_widget *
parameter_widget_new(GeoXmlParameter * parameter, gboolean use_default_value, gpointer data)
{
	struct parameter_widget *	parameter_widget;

	parameter_widget = g_malloc(sizeof(struct parameter_widget));
	*parameter_widget = (struct parameter_widget) {
		.parameter = parameter,
		.use_default_value = use_default_value,
		.data = data,
		.callback = NULL,
		.user_data = NULL
	};

	parameter_widget_configure(parameter_widget);

	return parameter_widget;
}

/*
 * Function: parameter_widget_set_widget_value
 * Sets widgets value to _value_
 */
void
parameter_widget_set_widget_value(struct parameter_widget * parameter_widget, const gchar * value)
{
	if (geoxml_program_parameter_get_is_list(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter)) == TRUE) {
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->list_value_widget), value);
		return;
	}

	enum GEOXML_PARAMETERTYPE	type;

	type = geoxml_parameter_get_type(parameter_widget->parameter);
	while (type == GEOXML_PARAMETERTYPE_REFERENCE)
		type = geoxml_parameter_get_type(geoxml_parameter_get_referencee(parameter_widget->parameter));
	switch (type) {
	case GEOXML_PARAMETERTYPE_FLOAT:
	case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_STRING:
		gtk_entry_set_text(GTK_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEOXML_PARAMETERTYPE_RANGE: {
		gchar *	endptr;
		double	number_value;

		number_value = strtod(value, &endptr);
		if (endptr != value)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(parameter_widget->value_widget), number_value);
		break;
	} case GEOXML_PARAMETERTYPE_FILE:
		gtk_file_entry_set_path(GTK_FILE_ENTRY(parameter_widget->value_widget), value);
		break;
	case GEOXML_PARAMETERTYPE_ENUM:
		parameter_widget_enum_set_widget_value(parameter_widget, value);
		break;
	case GEOXML_PARAMETERTYPE_FLAG:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(parameter_widget->value_widget),
			!strcmp(value, "on"));
		break;
	default:
		break;
	}
}

/*
 * Function: parameter_widget_get_widget_value
 * Return the parameter's widget value
 */
GString *
parameter_widget_get_widget_value(struct parameter_widget * parameter_widget)
{
	return __parameter_widget_get_widget_value(parameter_widget, TRUE);
}

/*
 * Function: parameter_widget_set_auto_submit_callback
 *
 */
void
parameter_widget_set_auto_submit_callback(struct parameter_widget * parameter_widget,
	changed_callback callback, gpointer user_data)
{
	parameter_widget->callback = callback;
	parameter_widget->user_data = user_data;
}

/*
 * Function: parameter_widget_update
 * Update input widget value from the parameter's value
 */
void
parameter_widget_update(struct parameter_widget * parameter_widget)
{
	if (geoxml_program_parameter_get_is_list(GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter)) == TRUE)
		parameter_list_value_widget_update(parameter_widget);
	else
		parameter_widget_set_widget_value(parameter_widget,
			geoxml_program_parameter_get_first_value(
				GEOXML_PROGRAM_PARAMETER(parameter_widget->parameter),
				parameter_widget->use_default_value));
}

/*
 * Function: parameter_widget_update_list_separator
 * Update UI of list with the new separator
 */
void
parameter_widget_update_list_separator(struct parameter_widget * parameter_widget)
{
	parameter_list_value_widget_update(parameter_widget);
}

/*
 * Function: parameter_widget_reconfigure
 * Rebuild the UI
 */
void
parameter_widget_reconfigure(struct parameter_widget * parameter_widget)
{
	g_object_weak_unref(G_OBJECT(parameter_widget->widget),
		(GWeakNotify)parameter_widget_weak_ref, parameter_widget);
	gtk_widget_destroy(parameter_widget->widget);
	parameter_widget_configure(parameter_widget);
}
