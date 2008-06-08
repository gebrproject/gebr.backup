/*   DeBR - GeBR Designer
 *   Copyright(C) 2007-2008 GeBR core team(http://debr.sourceforge.net)
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

#include <gui/pixmaps.h>

#include <gui/utils.h>
#include <gui/valuesequenceedit.h>

#include "interface.h"
#include "debr.h"
#include "callbacks.h"
#include "support.h"
#include "menu.h"
#include "program.h"

/*
 * File: interface.c
 * Interface creation: mainwindow, actions and callbacks.
 * See also <callbacks.c>
 */

/*
 * Prototypes
 */



/*
 * Section: Public
 */

/*
 * Function: debr_create_window
 * Create GeBR's main window
 */
void
debr_create_window(void)
{
	GtkWidget *		menu_bar;
	GtkWidget *		notebook;
	GtkWidget *		statusbar;

	GtkWidget *		vbox;
	GtkWidget *		menu;
	GtkWidget *		menu_item;
	GtkWidget *		child_menu_item;
	GtkWidget *		toolbar;

	/*
	 * Main window and its vbox contents
	 */
	gtk_window_set_default_icon(pixmaps_gebr_icon_16x16());
	debr.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(debr.window), "DêBR");
	gtk_widget_set_size_request(debr.window, 500, 300);
	debr.about = about_setup_ui("DêBR", _("Flow designer for GêBR"));

	g_signal_connect(debr.window, "delete_event",
		GTK_SIGNAL_FUNC(debr_quit), NULL);
	g_signal_connect(debr.window, "show",
		GTK_SIGNAL_FUNC(debr_init), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(debr.window), vbox);

	menu_bar = gtk_menu_bar_new();
	gtk_widget_show(menu_bar);
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	gtk_widget_show(notebook);

	debr.statusbar = statusbar = gtk_statusbar_new();
	gtk_widget_show(statusbar);
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

	/*
	 * Accelerator group
	 */
	debr.accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(debr.window), debr.accel_group);

	/*
	 * Actions: Menu
	 */
	/* new */
	debr.actions.menu.new = gtk_action_new("menu_new",
		NULL, NULL, GTK_STOCK_NEW);
	g_signal_connect(debr.actions.menu.new, "activate",
		(GCallback)on_menu_new_activate, NULL);
	/* open */
	debr.actions.menu.open = gtk_action_new("menu_open",
		NULL, NULL, GTK_STOCK_OPEN);
	g_signal_connect(debr.actions.menu.open, "activate",
		(GCallback)on_menu_open_activate, NULL);
	/* save */
	debr.actions.menu.save = gtk_action_new("menu_save",
		NULL, NULL, GTK_STOCK_SAVE);
	g_signal_connect(debr.actions.menu.save, "activate",
		(GCallback)on_menu_save_activate, NULL);
	/* save_as */
	debr.actions.menu.save_as = gtk_action_new("menu_save_as",
		NULL, NULL, GTK_STOCK_SAVE_AS);
	g_signal_connect(debr.actions.menu.save_as, "activate",
		(GCallback)on_menu_save_as_activate, NULL);
	/* revert */
	debr.actions.menu.revert = gtk_action_new("menu_revert",
		NULL, NULL, GTK_STOCK_REVERT_TO_SAVED);
	g_signal_connect(debr.actions.menu.revert, "activate",
		(GCallback)on_menu_revert_activate, NULL);
	/* delete */
	debr.actions.menu.delete = gtk_action_new("menu_delete",
		NULL, NULL, GTK_STOCK_DELETE);
	g_signal_connect(debr.actions.menu.delete, "activate",
		(GCallback)on_menu_delete_activate, NULL);
	/* close */
	debr.actions.menu.close = gtk_action_new("menu_close",
		NULL, NULL, GTK_STOCK_CLOSE);
	g_signal_connect(debr.actions.menu.close, "activate",
		(GCallback)on_menu_close_activate, NULL);

	/*
	 * Actions: Program
	 */
	/* new */
	debr.actions.program.new = gtk_action_new("program_new",
		NULL, NULL, GTK_STOCK_NEW);
	g_signal_connect(debr.actions.program.new, "activate",
		(GCallback)on_program_new_activate, NULL);
	/* delete */
	debr.actions.program.delete = gtk_action_new("program_delete",
		NULL, NULL, GTK_STOCK_DELETE);
	g_signal_connect(debr.actions.program.delete, "activate",
		(GCallback)on_program_delete_activate, NULL);

	/*
	 * Actions: Parameter
	 */
	/* new */
	debr.actions.parameter.new = gtk_action_new("parameter_new",
		NULL, NULL, GTK_STOCK_NEW);
	g_signal_connect(debr.actions.parameter.new, "activate",
		(GCallback)on_parameter_new_activate, NULL);
	/* delete */
	debr.actions.parameter.delete = gtk_action_new("parameter_delete",
		NULL, NULL, GTK_STOCK_DELETE);
	g_signal_connect(debr.actions.parameter.delete, "activate",
		(GCallback)on_parameter_delete_activate, NULL);
	/* up */
	debr.actions.parameter.up = gtk_action_new("parameter_up",
		NULL, NULL, GTK_STOCK_GO_UP);
	g_signal_connect(debr.actions.parameter.up, "activate",
		(GCallback)on_parameter_up_activate, NULL);
	/* down */
	debr.actions.parameter.down = gtk_action_new("parameter_down",
		NULL, NULL, GTK_STOCK_GO_DOWN);
	g_signal_connect(debr.actions.parameter.down, "activate",
		(GCallback)on_parameter_down_activate, NULL);
	/* duplicate */
	debr.actions.parameter.duplicate = gtk_action_new("parameter_duplicate",
		_("Duplicate"), NULL, GTK_STOCK_COPY);
	g_signal_connect(debr.actions.parameter.duplicate, "activate",
		(GCallback)on_parameter_duplicate_activate, NULL);
	/* change type */
	debr.actions.parameter.change_type = gtk_action_new("parameter_change_type",
		_("Change type"), NULL, GTK_STOCK_CONVERT);
	g_signal_connect(debr.actions.parameter.change_type, "activate",
		(GCallback)on_parameter_change_type_activate, NULL);

	/*
	 * Menu: Menu
	 */
	menu_item = gtk_menu_item_new_with_mnemonic(_("_Menu"));
	gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.menu.new));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.menu.open));

	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
	
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.menu.save));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.menu.save_as));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.menu.revert));

	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.menu.delete));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.menu.close));

	/*
	 * Menu: Program
	 */
	menu_item = gtk_menu_item_new_with_mnemonic(_("Program"));
	gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.program.new));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.program.delete));

// 	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
	
	/*
	 * Menu: Parameter
	 */

	menu_item = gtk_menu_item_new_with_mnemonic(_("Parameter"));
	gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.parameter.new));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.parameter.delete));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(debr.actions.parameter.change_type));

	/*
	 * Menu: Configure
	 */

	menu_item = gtk_menu_item_new_with_mnemonic(_("_Configure"));
	gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, debr.accel_group);
	gtk_container_add(GTK_CONTAINER(menu), child_menu_item);
	g_signal_connect(child_menu_item, "activate",
		(GCallback)on_configure_preferences_activate, NULL);

	/*
	 * Menu: Help
	 */

	menu_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_item));
	gtk_container_add(GTK_CONTAINER(menu_bar), menu_item);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	child_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, debr.accel_group);
	gtk_container_add(GTK_CONTAINER(menu), child_menu_item);
	g_signal_connect(child_menu_item, "activate",
		(GCallback)on_help_about_activate, NULL);

	gtk_widget_show_all(menu_bar);

	/*
	 * Notebook page: Menu
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Menu")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.menu.new)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.menu.open)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.menu.save)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.menu.save_as)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.menu.revert)), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.menu.delete)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.menu.close)), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	menu_setup_ui();
	gtk_box_pack_start(GTK_BOX(vbox), debr.ui_menu.widget, TRUE, TRUE, 0);

	/*
	 * Notebook page: Program
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Program")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.program.new)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.program.delete)), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	program_setup_ui();
	gtk_box_pack_start(GTK_BOX(vbox), debr.ui_program.widget, TRUE, TRUE, 0);

	/*
	 * Notebook page: Parameter
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Parameter")));

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.parameter.new)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.parameter.delete)), -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),
		GTK_TOOL_ITEM(gtk_action_create_tool_item(debr.actions.parameter.change_type)), -1);

	gtk_widget_show_all(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	parameter_setup_ui();
	gtk_box_pack_start(GTK_BOX(vbox), debr.ui_parameter.widget, TRUE, TRUE, 0);

}
