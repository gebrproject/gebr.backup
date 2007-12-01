/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include "program.h"
#include "gebrme.h"
#include "support.h"
#include "parameter.h"
#include "interface.h"
#include "help.h"

void
program_create_ui(GeoXmlProgram * program, gboolean hidden)
{
	GtkWidget *			program_expander;
	GtkWidget *			program_label_widget;
	GtkWidget *			program_label;
	GtkWidget *			program_vbox;
	GtkWidget *			io_hbox;
	GtkWidget *			io_label;
	GtkWidget *			io_stdin_checkbutton;
	GtkWidget *			io_stdout_checkbutton;
	GtkWidget *			io_stderr_checkbutton;
	GtkWidget *			info_expander;
	GtkWidget *			info_table;
	GtkWidget *			title_label;
	GtkWidget *			title_entry;
	GtkWidget *			binary_label;
	GtkWidget *			binary_entry;
	GtkWidget *			desc_label;
	GtkWidget *			desc_entry;
	GtkWidget *			help_hbox;
	GtkWidget *			help_label;
	GtkWidget *			help_view_button;
	GtkWidget *			help_edit_button;
	GtkWidget *			parameters_expander;
	GtkWidget *			parameters_label_widget;
	GtkWidget *			parameters_label;
	GtkWidget *			parameters_vbox;
	GtkWidget *			depth_hbox;
	GtkWidget *			widget;
	gchar *				program_title_str;
	GeoXmlProgramParameter *	parameter;

	program_expander = gtk_expander_new("");
	gtk_box_pack_start(GTK_BOX(gebrme.programs_vbox), program_expander, FALSE, TRUE, 0);
	gtk_expander_set_expanded (GTK_EXPANDER (program_expander), hidden);
	gtk_widget_show(program_expander);
	depth_hbox = create_depth(program_expander);

	program_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(program_expander), program_label_widget);
	gtk_widget_show(program_label_widget);
	program_label = gtk_label_new("");
	gtk_widget_show(program_label);
	gtk_box_pack_start(GTK_BOX(program_label_widget), program_label, FALSE, TRUE, 0);

	widget = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(program_label_widget), widget, FALSE, TRUE, 5);
	g_signal_connect ((gpointer) widget, "clicked",
		GTK_SIGNAL_FUNC (program_up),
		program);
	g_object_set(G_OBJECT(widget), "user-data", program_expander, NULL);

	widget = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(program_label_widget), widget, FALSE, TRUE, 5);
	g_signal_connect ((gpointer) widget, "clicked",
		GTK_SIGNAL_FUNC (program_down),
		program);
	g_object_set(G_OBJECT(widget), "user-data", program_expander, NULL);

	widget = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(program_label_widget), widget, FALSE, TRUE, 5);
	g_signal_connect ((gpointer) widget, "clicked",
		GTK_SIGNAL_FUNC (program_remove),
		program);
	g_object_set(G_OBJECT(widget), "user-data", program_expander, NULL);

	program_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(depth_hbox), program_vbox, TRUE, TRUE, 0);
	gtk_widget_show(program_vbox);

	io_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(io_hbox);
	gtk_box_pack_start(GTK_BOX(program_vbox), io_hbox, FALSE, TRUE, 0);

	io_label = gtk_label_new(_("IO:"));
	gtk_widget_show(io_label);
	gtk_box_pack_start(GTK_BOX(io_hbox), io_label, FALSE, TRUE, 0);

	io_stdin_checkbutton = gtk_check_button_new_with_label(_("input"));
	gtk_widget_show(io_stdin_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_hbox), io_stdin_checkbutton, FALSE, TRUE, 30);
	g_signal_connect ((gpointer) io_stdin_checkbutton, "clicked",
		GTK_SIGNAL_FUNC (program_stdin_changed),
		program);
	g_object_set(G_OBJECT(io_stdin_checkbutton), "user-data", program_expander, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdin_checkbutton), geoxml_program_get_stdin(program));

	io_stdout_checkbutton = gtk_check_button_new_with_label(_("output"));
	gtk_widget_show(io_stdout_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_hbox), io_stdout_checkbutton, FALSE, TRUE, 5);
	g_signal_connect ((gpointer) io_stdout_checkbutton, "clicked",
		GTK_SIGNAL_FUNC (program_stdout_changed),
		program);
	g_object_set(G_OBJECT(io_stdout_checkbutton), "user-data", program_expander, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdout_checkbutton), geoxml_program_get_stdout(program));

	io_stderr_checkbutton = gtk_check_button_new_with_label(_("error"));
	gtk_widget_show(io_stderr_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_hbox), io_stderr_checkbutton, FALSE, TRUE, 5);
	g_signal_connect ((gpointer) io_stderr_checkbutton, "clicked",
		GTK_SIGNAL_FUNC (program_stderr_changed),
		program);
	g_object_set(G_OBJECT(io_stderr_checkbutton), "user-data", program_expander, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stderr_checkbutton), geoxml_program_get_stderr(program));

	info_expander = gtk_expander_new(_("Information"));
	gtk_expander_set_expanded (GTK_EXPANDER (info_expander), hidden);
	gtk_box_pack_start(GTK_BOX(program_vbox), info_expander, FALSE, TRUE, 0);
	gtk_widget_show(info_expander);
	depth_hbox = create_depth(info_expander);

	info_table = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (info_table);
	gtk_box_pack_start(GTK_BOX(depth_hbox), info_table, TRUE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (info_table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (info_table), 5);

	title_label = gtk_label_new (_("Title:"));
	gtk_widget_show (title_label);
	gtk_table_attach (GTK_TABLE (info_table), title_label, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (title_label), 0, 0.5);

	title_entry = gtk_entry_new ();
	gtk_widget_show (title_entry);
	gtk_table_attach (GTK_TABLE (info_table), title_entry, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	g_signal_connect ((gpointer) title_entry, "changed",
		G_CALLBACK (program_info_title_changed),
		program);
	g_object_set(G_OBJECT(title_entry), "user-data", program_label, NULL);
	program_title_str = (gchar*)geoxml_program_get_title(program);
	if (!strlen(program_title_str))
		program_title_str = _("New program");
	gtk_entry_set_text(GTK_ENTRY(title_entry), program_title_str);
	gtk_label_set_text(GTK_LABEL(program_label), program_title_str);

	binary_label = gtk_label_new (_("Binary:"));
	gtk_widget_show (binary_label);
	gtk_table_attach (GTK_TABLE (info_table), binary_label, 0, 1, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (binary_label), 0, 0.5);

	binary_entry = gtk_entry_new ();
	gtk_widget_show (binary_entry);
	gtk_table_attach (GTK_TABLE (info_table), binary_entry, 1, 2, 1, 2,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	g_signal_connect ((gpointer) binary_entry, "changed",
		G_CALLBACK (program_info_binary_changed),
		program);
	gtk_entry_set_text(GTK_ENTRY(binary_entry), geoxml_program_get_binary(program));

	desc_label = gtk_label_new (_("Description:"));
	gtk_widget_show (desc_label);
	gtk_table_attach (GTK_TABLE (info_table), desc_label, 0, 1, 2, 3,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (desc_label), 0, 0.5);

	desc_entry = gtk_entry_new ();
	gtk_widget_show (desc_entry);
	gtk_table_attach (GTK_TABLE (info_table), desc_entry, 1, 2, 2, 3,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	g_signal_connect ((gpointer) desc_entry, "changed",
		G_CALLBACK (program_info_desc_changed),
		program);
	gtk_entry_set_text(GTK_ENTRY(desc_entry), geoxml_program_get_description(program));

	help_label = gtk_label_new (_("Help"));
	gtk_widget_show (help_label);
	gtk_table_attach (GTK_TABLE (info_table), help_label, 0, 1, 3, 4,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (help_label), 0, 0.5);

	help_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(help_hbox);
	gtk_table_attach (GTK_TABLE (info_table), help_hbox, 1, 2, 3, 4,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);
	help_view_button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
	gtk_widget_show (help_view_button);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_view_button, FALSE, FALSE, 0);
	g_signal_connect ((gpointer) help_view_button, "clicked",
			GTK_SIGNAL_FUNC (program_info_help_view),
			program);
	help_edit_button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_widget_show (help_edit_button);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_edit_button, FALSE, FALSE, 5);
	g_signal_connect ((gpointer) help_edit_button, "clicked",
			GTK_SIGNAL_FUNC (program_info_help_edit),
			program);

	parameters_expander = gtk_expander_new("");
	gtk_expander_set_expanded (GTK_EXPANDER (parameters_expander), hidden);
	gtk_box_pack_start(GTK_BOX(program_vbox), parameters_expander, FALSE, TRUE, 0);
	gtk_widget_show(parameters_expander);
	depth_hbox = create_depth(parameters_expander);

	parameters_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(depth_hbox), parameters_vbox, TRUE, TRUE, 0);
	gtk_widget_show(parameters_vbox);

	parameters_label_widget = gtk_hbox_new(FALSE, 0);
	gtk_expander_set_label_widget(GTK_EXPANDER(parameters_expander), parameters_label_widget);
	gtk_widget_show(parameters_label_widget);
	gtk_expander_hacked_define(parameters_expander, parameters_label_widget);
	parameters_label = gtk_label_new(_("Parameters"));
	gtk_widget_show(parameters_label);
	gtk_box_pack_start(GTK_BOX(parameters_label_widget), parameters_label, FALSE, TRUE, 0);
	widget = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(parameters_label_widget), widget, FALSE, TRUE, 5);
	g_signal_connect ((gpointer) widget, "clicked",
		GTK_SIGNAL_FUNC (parameter_add),
		program);
	g_object_set(G_OBJECT(widget), "user-data", parameters_vbox, NULL);

	parameter = geoxml_program_get_first_parameter(program);
	while (parameter != NULL) {
		gtk_box_pack_start(GTK_BOX(parameters_vbox), parameter_create_ui(parameter, hidden), FALSE, TRUE, 0);
		geoxml_program_parameter_next(&parameter);
	}
}

void
program_add(void)
{
	GeoXmlProgram *	program;

	program = geoxml_flow_new_program(gebrme.current);
	/* default settings */
	geoxml_program_set_stdin(program, TRUE);
	geoxml_program_set_stdout(program, TRUE);
	geoxml_program_set_stderr(program, TRUE);

	program_create_ui(program, TRUE);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
program_remove(GtkButton * button, GeoXmlProgram * program)
{
	GtkWidget *	program_expander;

	g_object_get(G_OBJECT(button), "user-data", &program_expander, NULL);
	gtk_widget_destroy(program_expander);
	geoxml_flow_remove_program(gebrme.current, program);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
program_up(GtkButton * button, GeoXmlProgram * program)
{
	GtkWidget *	program_expander;
	GList *		programs_expanders;
	GList *		this;
	GList *		up;

	g_object_get(G_OBJECT(button), "user-data", &program_expander, NULL);

	programs_expanders = gtk_container_get_children(GTK_CONTAINER(gebrme.programs_vbox));
	this = g_list_find(programs_expanders, program_expander);
	up = g_list_previous(this);
	if (up != NULL) {
		gtk_box_reorder_child(GTK_BOX(gebrme.programs_vbox), up->data, g_list_position(programs_expanders, this));
		geoxml_flow_move_program_up(gebrme.current, program);
	}

	g_list_free(programs_expanders);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
program_down(GtkButton * button, GeoXmlProgram * program)
{
	GtkWidget *	program_expander;
	GList *		programs_expanders;
	GList *		this;
	GList *		down;

	g_object_get(G_OBJECT(button), "user-data", &program_expander, NULL);

	programs_expanders = gtk_container_get_children(GTK_CONTAINER(gebrme.programs_vbox));
	this = g_list_find(programs_expanders, program_expander);
	down = g_list_next(this);
	if (down != NULL) {
		gtk_box_reorder_child(GTK_BOX(gebrme.programs_vbox), down->data, g_list_position(programs_expanders, this));
		geoxml_flow_move_program_down(gebrme.current, program);
	}

	g_list_free(programs_expanders);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
program_stdin_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program)
{
	geoxml_program_set_stdin(program, gtk_toggle_button_get_active(togglebutton));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
program_stdout_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program)
{
	geoxml_program_set_stdout(program, gtk_toggle_button_get_active(togglebutton));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
program_stderr_changed(GtkToggleButton *togglebutton, GeoXmlProgram * program)
{
	geoxml_program_set_stderr(program, gtk_toggle_button_get_active(togglebutton));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

gboolean
program_info_title_changed(GtkEntry * entry, gpointer user_data)
{
	GtkWidget *	program_label;

	g_object_get(G_OBJECT(entry), "user-data", &program_label, NULL);
	geoxml_program_set_title((GeoXmlProgram*)user_data, gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_label_set_text(GTK_LABEL(program_label), gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

gboolean
program_info_binary_changed(GtkEntry * entry, gpointer user_data)
{
	geoxml_program_set_binary((GeoXmlProgram*)user_data, gtk_entry_get_text(GTK_ENTRY(entry)));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

gboolean
program_info_desc_changed(GtkEntry * entry, gpointer user_data)
{
	geoxml_program_set_description((GeoXmlProgram*)user_data, gtk_entry_get_text(GTK_ENTRY(entry)));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

void
program_info_help_view(GtkButton * button, GeoXmlProgram * program)
{
	help_show(geoxml_program_get_help(program));
}

void
program_info_help_edit(GtkButton * button, GeoXmlProgram * program)
{
	GString *	help;

	help = help_edit(geoxml_program_get_help(program));
	geoxml_program_set_help(program, help->str);
	g_string_free(help, TRUE);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
