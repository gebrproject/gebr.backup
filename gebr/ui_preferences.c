/*   GeBR - An environment for seismic processing.
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

/**
 * \file ui_preferences.c Preferences dialog and config related stuff. Assembly preferences dialog and changes
 * configuration file according to user's change.
 *
 * \ingroup gebr
 */

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <locale.h>
#include <libgebr/gui/gebr-gui-enhanced-entry.h>

#include "ui_preferences.h"
#include "gebr.h"
#include "gebr-version.h"

typedef enum {
	WIZARD_STATUS_WITHOUT_PREFERENCES,
	WIZARD_STATUS_WITHOUT_MAESTRO,
	WIZARD_STATUS_WITHOUT_DAEMON,
	WIZARD_STATUS_WITHOUT_GVFS,
	WIZARD_STATUS_COMPLETE
} WizardStatus;

#define DEFAULT_SERVERS_ENTRY_TEXT _("Type hostname[.domain] or address")

/*
 * Prototypes
 */

static void set_status_for_maestro(GebrMaestroController *self,
                                   GebrMaestroServer     *maestro,
                                   struct ui_preferences *up,
                                   GebrCommServerState state);

static WizardStatus get_wizard_status(struct ui_preferences *up);

static void on_preferences_destroy(GtkWidget *window,
                                   struct ui_preferences *up);

static void on_assistant_destroy(GtkWidget *window,
                                 struct ui_preferences *up);

static void on_assistant_prepare(GtkAssistant *assistant,
				 GtkWidget *current_page,
				 struct ui_preferences *up);

static void validate_entry(GtkEntry *entry,
                           gboolean error,
                           const gchar *err_text,
                           const gchar *clean_text);

static void
save_preferences_configuration(struct ui_preferences *up)
{
	gchar *tmp;

	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(up->usermenus));

	g_string_assign(gebr.config.username, gtk_entry_get_text(GTK_ENTRY(up->username)));
	g_string_assign(gebr.config.email, gtk_entry_get_text(GTK_ENTRY(up->email)));

	if (g_strcmp0(gebr.config.usermenus->str, tmp) != 0) {
		gebr.update_usermenus = TRUE;
		g_string_assign(gebr.config.usermenus, tmp);
		gebr_config_apply();
	}

	gebr_config_save(FALSE);

	g_free(tmp);
}

static void
on_assistant_cancel(GtkAssistant *assistant,
		    struct ui_preferences *up)
{
	gint curr_page = gtk_assistant_get_current_page(assistant);
	up->prev_page = curr_page;

	if (up->first_run) {
		if (curr_page == CANCEL_PAGE) {
			if (get_wizard_status(up) == WIZARD_STATUS_COMPLETE)
				gtk_widget_destroy(GTK_WIDGET(assistant));
			else {
				on_assistant_destroy(GTK_WIDGET(assistant), up);
				gebr_quit(FALSE);
			}
		} else{
			up->cancel_assistant = TRUE;
			gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), CANCEL_PAGE);
		}
	} else {
		up->cancel_assistant = FALSE;
		gtk_widget_destroy(GTK_WIDGET(assistant));
	}
	up->tried_to_mount_gvfs = FALSE;
}

static void
on_assistant_back_button(GtkButton *button,
                         struct ui_preferences *up)
{
	gtk_assistant_set_current_page(GTK_ASSISTANT(up->dialog), up->prev_page);
}

static void
on_assistant_close(GtkAssistant *assistant,
                   struct ui_preferences *up)
{
	gint page = gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));
	if (page == CANCEL_PAGE) {
		gboolean wizard_status = get_wizard_status(up);
		if (wizard_status == WIZARD_STATUS_COMPLETE || wizard_status == WIZARD_STATUS_WITHOUT_GVFS) {
			g_signal_handlers_disconnect_by_func(up->back_button, on_assistant_back_button, up);
			gtk_assistant_remove_action_widget(assistant, up->back_button);
			save_preferences_configuration(up);
			gtk_widget_destroy(up->dialog);
		} else {
			on_assistant_destroy(GTK_WIDGET(assistant), up);
			gebr_quit(FALSE);
		}
	}
	else if (page == GVFS_PAGE) {
		up->prev_page = GVFS_PAGE;
		up->cancel_assistant = TRUE;
		gtk_assistant_set_current_page(assistant, CANCEL_PAGE);
	}
	up->tried_to_mount_gvfs = FALSE;
}

static void
on_maestro_state_changed(GebrMaestroController *self,
                         GebrMaestroServer     *maestro,
                         struct ui_preferences *up)
{
	GebrCommServerState state = gebr_maestro_server_get_state(maestro);

	gebr_maestro_server_set_wizard_setup(maestro, TRUE);

	if (state != SERVER_STATE_LOGGED && state != SERVER_STATE_DISCONNECTED)
		return;

	set_status_for_maestro(self, maestro, up, state);
}

static void
set_status_for_maestro(GebrMaestroController *self,
                       GebrMaestroServer     *maestro,
                       struct ui_preferences *up,
                       GebrCommServerState state)
{
	GtkComboBox *combo = GTK_COMBO_BOX(gtk_builder_get_object(up->builder, "maestro_combo"));
	GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_chooser"));
	GtkWidget *main_status = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_status"));
	GObject *status_img = gtk_builder_get_object(up->builder, "status_img");
	GObject *status_label = gtk_builder_get_object(up->builder, "status_label");
	GObject *status_title = gtk_builder_get_object(up->builder, "status_title");
	GtkWidget *maestro_status_label = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_status_label"));

	gtk_widget_show(main_status);
	gtk_widget_hide(maestro_status_label);

	gchar *summary_txt;

	const gchar *address = gebr_maestro_server_get_address(maestro);

	if (state == SERVER_STATE_LOGGED) {
		gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_YES, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(status_label), _("Success!"));
		gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_maestro, TRUE);

		summary_txt = g_markup_printf_escaped(_("<span size='large'>Successfully connected to maestro <b>%s</b>!</span>"),
		                                      address);

		gtk_label_set_markup(GTK_LABEL(status_title), summary_txt);
		g_free(summary_txt);

		const gchar *label = gebr_maestro_server_get_nfs_label(maestro);
		const gchar *addr = gebr_maestro_server_get_address(maestro);

		gchar *auto_label = g_strdup_printf("%s (%s)", label, addr);
		gtk_entry_set_text(up->maestro_entry, auto_label);

		gebr_config_maestro_save();
	}
	else {
		const gchar *type, *msg;
		gebr_maestro_server_get_error(maestro, &type, &msg);

		if (!g_strcmp0(type, "error:none")) {
			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_DIALOG);

			gtk_label_set_markup(GTK_LABEL(status_label), _("Connecting..."));

			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_maestro, FALSE);

			summary_txt = g_markup_printf_escaped(_("<span size='large'>Connecting to maestro <b>%s</b>!</span>"),
			                                      address);

			gtk_label_set_markup(GTK_LABEL(status_title), summary_txt);
			g_free(summary_txt);
		} else if (!g_strcmp0(type, "error:stop")) {
			gchar *title = g_markup_printf_escaped(_("<span size='large'>Maestro <b>%s</b> disconnected!</span>!"), address);
			gtk_label_set_markup(GTK_LABEL(status_title), title);
			g_free(title);

			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_DIALOG);
			gchar *txt = g_markup_printf_escaped(_("Stopped!"));
			gtk_label_set_markup(GTK_LABEL(status_label), txt);
			g_free(txt);

			gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_maestro, FALSE);
		} else {
			gchar *title = g_markup_printf_escaped(_("<span size='large'>Could not connect to maestro <b>%s</b></span>!"), address);
			gtk_label_set_markup(GTK_LABEL(status_title), title);
			g_free(title);

			gchar *message;
			if (g_strrstr(msg, "Host key"))
				message = g_strdup_printf("%sThis machine appears to be untrusted, contact the system administrator "
							  "to resolve that problem, or connect to another machine.",
						          msg);
			else
				message = g_strdup(msg);

			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
			gchar *txt = g_markup_printf_escaped(_("The connection reported the following error:\n<i>%s</i>"), message);
			gtk_label_set_markup(GTK_LABEL(status_label), txt);
			g_free(txt);
			g_free(message);

			gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_maestro, FALSE);
		}
	}

	gebr_maestro_controller_update_chooser_model(maestro, NULL, combo);
}

static void
on_daemons_changed(GebrMaestroServer *maestro,
                   struct ui_preferences *up)
{
	gboolean has_connected_servers = gebr_maestro_server_has_servers(maestro, TRUE);
	gboolean maestro_has_servers = gebr_maestro_server_has_servers(maestro, FALSE);

	GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers"));
	GtkWidget *servers_view = GTK_WIDGET(gtk_builder_get_object(up->builder, "servers_view"));
	GtkWidget *connect_all = GTK_WIDGET(gtk_builder_get_object(up->builder, "hbox8"));

	if (maestro_has_servers) {
		gtk_widget_show(servers_view);
		gtk_widget_show(connect_all);

		if (has_connected_servers)
			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_servers, TRUE);
		else
			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_servers, FALSE);

	} else {
		gtk_widget_hide(servers_view);
		gtk_widget_hide(connect_all);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_servers, FALSE);
	}
}

static void
on_connect_all_server_clicked(GtkButton *button,
                              struct ui_preferences *up)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	gebr_maestro_server_connect_on_daemons(maestro);
}

static void
on_add_server_clicked(GtkButton *button,
		      struct ui_preferences *up)
{
	const gchar *addr = gebr_gui_enhanced_entry_get_text(GEBR_GUI_ENHANCED_ENTRY(up->server_entry));
	if (!*addr)
		return;

	if (!g_strcmp0(addr, DEFAULT_SERVERS_ENTRY_TEXT))
		return;

	gebr_maestro_controller_server_list_add(gebr.maestro_controller, addr);

	gebr_gui_enhanced_entry_set_text(GEBR_GUI_ENHANCED_ENTRY(up->server_entry), "");
}

static void
on_entry_server_activate(GtkEntry *entry,
                         struct ui_preferences *up)
{
	const gchar *entry_text = gtk_entry_get_text(entry);
	gboolean error = !gebr_verify_address_without_username(entry_text);
	if (error)
		return;

	GObject *server_add = gtk_builder_get_object(up->builder, "server_add");
	on_add_server_clicked(NULL, up);
	gtk_widget_grab_focus(GTK_WIDGET(server_add));
}

static gboolean
server_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
                        GtkTreeIter * iter, GtkTreeViewColumn * column, GebrMaestroController *mc)
{
	if (gtk_tree_view_get_column(tree_view, 0) == column) {
		GebrDaemonServer *daemon;

		GtkTreeModel *model = gebr_maestro_controller_get_servers_model(mc);
		gtk_tree_model_get(model, iter, 0, &daemon, -1);

		if (!daemon)
			return FALSE;

		const gchar *error = gebr_daemon_server_get_error(daemon);

		if (!error || !*error)
			return FALSE;

		gtk_tooltip_set_text(tooltip, error);
		return TRUE;
	}
	return FALSE;
}

static GtkTreeView *
create_view_for_servers(struct ui_preferences *up)
{
	GtkWidget *view = GTK_WIDGET(gtk_builder_get_object(up->builder, "servers_view"));

	if (gtk_tree_view_get_model(GTK_TREE_VIEW(view)))
		return GTK_TREE_VIEW(view);

	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(view),
	                                            (GebrGuiGtkTreeViewTooltipCallback) server_tooltip_callback,
	                                            gebr.maestro_controller);

	// Server Column
	GtkCellRenderer *renderer ;
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_min_width(col, 100);

#if GTK_CHECK_VERSION(2,20,0)
	renderer = gtk_cell_renderer_spinner_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_maestro_controller_daemon_server_progress_func,
	                                        NULL, NULL);
#endif

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_maestro_controller_daemon_server_status_func,
	                                        NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_maestro_controller_daemon_server_address_func,
	                                        GINT_TO_POINTER(FALSE), NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	// End of Server Column

	return GTK_TREE_VIEW(view);
}

static void
on_maestro_info_button_clicked (GtkButton *button, gpointer pointer)
{
	const gchar *section = "additional_features_maestro_servers_configuration";
	gchar *error;

	gebr_gui_help_button_clicked(section, &error);

	if (error) {
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, error);
		g_free(error);
	}
}

static WizardStatus
get_wizard_status(struct ui_preferences *up)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED) {
		if (gebr_maestro_server_has_servers(maestro, TRUE)) {
			if (gebr_maestro_server_has_connected_daemon(maestro))
				return WIZARD_STATUS_COMPLETE;
			else
				return WIZARD_STATUS_WITHOUT_GVFS;
		} else {
			return WIZARD_STATUS_WITHOUT_DAEMON;
		}
	} else {
		const gchar *email = gtk_entry_get_text(GTK_ENTRY(up->email));
		if (gebr_validate_check_is_email(email))
			return WIZARD_STATUS_WITHOUT_MAESTRO;
		else
			return WIZARD_STATUS_WITHOUT_PREFERENCES;
	}

}

static void
on_connect_maestro_clicked(GtkButton *button,
                           struct ui_preferences *up)
{
	GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_chooser"));

	gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), main_maestro, GTK_ASSISTANT_PAGE_PROGRESS);

	if (up->maestro_addr)
		g_free(up->maestro_addr);

	const gchar *entry_text = gtk_entry_get_text(up->maestro_entry);
	if (g_strrstr(entry_text, "("))
		return;

	const gchar *address = gebr_apply_pattern_on_address(entry_text);
	up->maestro_addr = g_strdup(address);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, up->maestro_addr);

	if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED || g_strcmp0(up->maestro_addr, gebr_maestro_server_get_address(maestro)))
		gebr_maestro_controller_connect(gebr.maestro_controller, up->maestro_addr);
	else
		set_status_for_maestro(gebr.maestro_controller, maestro, up, gebr_maestro_server_get_state(maestro));
}

static gboolean
on_focus_in_event(GtkWidget *entry,
                  GdkEventFocus *event,
                  GebrMaestroController *self)
{
	if (!entry)
		return TRUE;

	gtk_entry_set_text(GTK_ENTRY(entry), "");

	return FALSE;
}

static void
on_connect_maestro_activate(GtkEntry *entry,
                            struct ui_preferences *up)
{
	on_connect_maestro_clicked(NULL, up);
}

static void
on_changed_validate_email(GtkWidget     *widget,
                          struct ui_preferences *up)
{
	GtkWidget *page_preferences = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_preferences"));
	const gchar *email = gtk_entry_get_text(GTK_ENTRY(up->email));

	gboolean error = FALSE;

	error = !gebr_validate_check_is_email(email);


	validate_entry(GTK_ENTRY(up->email), error, _("Invalid email"), _("Your email address"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), page_preferences, !error);
}

static void
set_status_for_mount(GebrMaestroServer *maestro,
                     GebrMountStatus status,
                     struct ui_preferences *up)
{
	static gboolean first_time = TRUE;

	GtkWidget *mount_gvfs = GTK_WIDGET(gtk_builder_get_object(up->builder, "mount_gvfs"));
	GObject *status_img = gtk_builder_get_object(up->builder, "mount_img");
	GObject *status_label = gtk_builder_get_object(up->builder, "mount_label");
	GtkWidget *button = GTK_WIDGET(gtk_builder_get_object(up->builder, "mount_button"));

	if (status == STATUS_MOUNT_OK) {
		first_time = FALSE;
		gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_YES, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(status_label), _("Remote browsing is enabled!"));
		gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), mount_gvfs, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), mount_gvfs, !first_time);
	}
	else if (status == STATUS_MOUNT_NOK) {
		if (!up->tried_to_mount_gvfs) {
			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
			gtk_label_set_text(GTK_LABEL(status_label), _("Remote browsing is disabled!"));
			up->tried_to_mount_gvfs = TRUE;
		}
		else {
			gtk_label_set_text(GTK_LABEL(status_label), _("Failed to enable remote browsing!"));
			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		}
		gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), mount_gvfs, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_widget_set_sensitive(button, TRUE);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), mount_gvfs, !first_time);
	}
	else if (status == STATUS_MOUNT_PROGRESS) {
		first_time = FALSE;
		gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(status_label), _("Connecting..."));
		gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), mount_gvfs, GTK_ASSISTANT_PAGE_PROGRESS);
	}
}

static void
on_mount_status_changed(GebrMaestroServer *maestro,
                        GebrMountStatus status,
                        struct ui_preferences *up)
{
	set_status_for_mount(maestro, status, up);

	gebr_log_update_maestro_info(gebr.ui_log, maestro);
}

static void
on_mount_gvfs_clicked(GtkButton *button,
                      struct ui_preferences *up)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	GtkTreeIter iter;
	GtkTreeModel *model = gebr_maestro_server_get_model(maestro, FALSE, NULL);

	g_signal_connect(maestro, "gvfs-mount", G_CALLBACK(on_mount_status_changed), up);

	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		GebrDaemonServer *daemon;

		gtk_tree_model_get(model, &iter, 0, &daemon, -1);

		if (gebr_daemon_server_get_state(daemon) != SERVER_STATE_LOGGED) {
			valid = gtk_tree_model_iter_next(model, &iter);
			continue;
		}

		gebr_maestro_server_mount_gvfs(maestro, gebr_daemon_server_get_address(daemon));
		break;
	}
}

static void
on_assistant_prepare(GtkAssistant *assistant,
		     GtkWidget *current_page,
		     struct ui_preferences *up)
{
	gint page = gtk_assistant_get_current_page(assistant);

	GtkTreeIter iter;
	GtkWidget *maestro_info_button = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_info_button"));

	gtk_widget_hide(up->back_button);

	if (page == CANCEL_PAGE) {
		if (!up->cancel_assistant) {
			gtk_assistant_set_current_page(assistant, (up->prev_page > INITIAL_PAGE) ? --up->prev_page : INITIAL_PAGE);
			return;
		}

		up->cancel_assistant = FALSE;
		up->tried_to_mount_gvfs = FALSE;
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

		gtk_widget_show(up->back_button);

		GtkLabel *review_orientations_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_orientation"));
		GtkImage *review_img = GTK_IMAGE(gtk_builder_get_object(up->builder, "review_img"));

		GtkLabel *review_pref_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_pref_label"));
		GtkLabel *review_maestro_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_maestro_label"));
		GtkLabel *review_servers_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_servers_label"));
		GtkLabel *review_gvfs_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_gvfs_label"));
		GtkImage *review_pref_img = GTK_IMAGE(gtk_builder_get_object(up->builder, "review_pref_img"));
		GtkImage *review_maestro_img = GTK_IMAGE(gtk_builder_get_object(up->builder, "review_maestro_img"));
		GtkImage *review_servers_img = GTK_IMAGE(gtk_builder_get_object(up->builder, "review_servers_img"));
		GtkImage *review_gvfs_img = GTK_IMAGE(gtk_builder_get_object(up->builder, "review_gvfs_img"));

		WizardStatus wizard_status = get_wizard_status(up);
		if (wizard_status == WIZARD_STATUS_COMPLETE || wizard_status == WIZARD_STATUS_WITHOUT_GVFS) {
			GtkTreeModel *model_servers = gebr_maestro_server_get_model(maestro, FALSE, NULL);
			gboolean active = gtk_tree_model_get_iter_first(model_servers, &iter);
			gint counter = 0;

			while (active) {
				GebrDaemonServer *daemon;
				gtk_tree_model_get(model_servers, &iter, 0, &daemon, -1);
				if (gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED)
					counter++;
				active = gtk_tree_model_iter_next(model_servers, &iter);
			}

			if (counter > 0) {
				gchar *num_servers;
				num_servers = g_markup_printf_escaped(_("<i><b>%d</b> connected.</i>"), counter);
				gtk_label_set_markup(review_servers_label, num_servers);
				g_free(num_servers);
			} else {
				gtk_label_set_markup(review_servers_label, _("<i>None connected.</i>"));
			}

			gtk_label_set_markup(review_maestro_label, _("<i>Connected.</i>"));

			gtk_label_set_markup(review_orientations_label, _("GêBR is ready."));
			gtk_label_set_markup(review_pref_label, _("<i>Done.</i>"));
			gtk_image_set_from_stock(GTK_IMAGE(review_pref_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
			gtk_image_set_from_stock(GTK_IMAGE(review_maestro_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
			gtk_image_set_from_stock(GTK_IMAGE(review_servers_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
			gtk_image_set_from_stock(GTK_IMAGE(review_img), GTK_STOCK_YES, GTK_ICON_SIZE_DIALOG);
			if (wizard_status == WIZARD_STATUS_WITHOUT_GVFS) {
				gtk_label_set_markup(review_gvfs_label, _("<i>Disabled.</i>"));
				gtk_image_set_from_stock(GTK_IMAGE(review_gvfs_img), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU);
			} else {
				gtk_label_set_markup(review_gvfs_label, _("<i>Enabled.</i>"));
				gtk_image_set_from_stock(GTK_IMAGE(review_gvfs_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
			}
		} else {
			gtk_label_set_markup(review_orientations_label, _("GêBR is unable to proceed without this configuration."));
			gtk_image_set_from_stock(GTK_IMAGE(review_img), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
			if (wizard_status == WIZARD_STATUS_WITHOUT_MAESTRO) {
				gtk_label_set_markup(review_pref_label, _("<i>Done.</i>"));
				gtk_image_set_from_stock(GTK_IMAGE(review_pref_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
				gtk_image_set_from_stock(GTK_IMAGE(review_maestro_img), GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
				gtk_label_set_markup(review_maestro_label, _("<i>Not connected.</i>"));
			} else if (wizard_status == WIZARD_STATUS_WITHOUT_DAEMON){
				gtk_label_set_markup(review_pref_label, _("<i>Done.</i>"));
				gtk_image_set_from_stock(GTK_IMAGE(review_pref_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
				gtk_label_set_markup(review_maestro_label, _("<i>Connected.</i>"));
				gtk_image_set_from_stock(GTK_IMAGE(review_maestro_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
				gtk_label_set_markup(review_servers_label, _("<i>None connected.</i>"));
				gtk_image_set_from_stock(GTK_IMAGE(review_servers_img), GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
			} else if (wizard_status == WIZARD_STATUS_WITHOUT_PREFERENCES){
				gtk_label_set_markup(review_pref_label, _("<i>Not configured</i>"));
				gtk_image_set_from_stock(GTK_IMAGE(review_pref_img), GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
			}
		}
	}
	else if (page == INITIAL_PAGE) {
		if (up->insert_preferences) {
			on_changed_validate_email(up->email, up);
			g_signal_connect(up->email, "changed", G_CALLBACK(on_changed_validate_email), up);
		}
	}
	else if (page == MAESTRO_INFO_PAGE) {
		g_signal_connect(GTK_BUTTON(maestro_info_button), "clicked", G_CALLBACK(on_maestro_info_button_clicked), NULL);
	}
	else if (page == MAESTRO_PAGE) {
		GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_chooser"));
		GtkWidget *connections_info = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_connection"));
		GtkWidget *main_status = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_status"));
		GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_status_label"));

		GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(up->builder, "connect_button"));

		g_signal_connect(connect_button, "clicked", G_CALLBACK(on_connect_maestro_clicked), up);
		g_signal_connect(up->maestro_entry, "activate", G_CALLBACK(on_connect_maestro_activate), up);
		g_signal_connect(up->maestro_entry, "focus-in-event", G_CALLBACK(on_focus_in_event), up);
		g_signal_connect(gebr.maestro_controller, "maestro-state-changed", G_CALLBACK(on_maestro_state_changed), up);

		gtk_widget_show(connections_info);

		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
		if (maestro) {
			GebrCommServerState state = gebr_maestro_server_get_state(maestro);
			if (state == SERVER_STATE_LOGGED || SERVER_STATE_DISCONNECTED) {
				gtk_widget_hide(status_label);
				gtk_widget_show(main_status);
				set_status_for_maestro(gebr.maestro_controller, maestro, up, state);
			}
			if (state == SERVER_STATE_LOGGED)
				gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_maestro, TRUE);
			else
				gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_maestro, FALSE);
		} else {
			gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_maestro, FALSE);
			gtk_widget_show(status_label);
			gtk_widget_hide(main_status);
		}

		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_maestro, _("Maestro"));
	}
	else if (page == SERVERS_PAGE) {
		GtkTreeView *view = create_view_for_servers(up);
		if (view) {
			GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers"));
			GObject *server_add = gtk_builder_get_object(up->builder, "server_add");
			GObject *server_all = gtk_builder_get_object(up->builder, "server_all");

			GtkWidget *main_servers_label = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers_label"));
			GtkWidget *connect_all = GTK_WIDGET(gtk_builder_get_object(up->builder, "hbox8"));
			GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

			g_signal_connect(maestro, "daemons-changed", G_CALLBACK(on_daemons_changed), up);

			gchar *main_servers_text = g_markup_printf_escaped(_("Maestro <b>%s</b> needs at least one <i>connected node</i> "
                                                                             "to run processing flows. You must either connect to a new "
                                                                             "one or connect your already registered nodes.\n"),
									   gebr_maestro_server_get_address(maestro));

			gtk_label_set_markup(GTK_LABEL(main_servers_label), main_servers_text);
			g_free(main_servers_text);

			gebr_maestro_controller_update_daemon_model(maestro, gebr.maestro_controller);
			GtkTreeModel *model = gebr_maestro_controller_get_servers_model(gebr.maestro_controller);
			gtk_tree_view_set_model(view, model);

			gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view),
			                                          (GebrGuiGtkPopupCallback) gebr_maestro_controller_server_popup_menu,
			                                          gebr.maestro_controller);

			GtkTreeIter it;
			GtkTreeModel *store = gebr_maestro_server_get_model(maestro, FALSE, NULL);
			if (!gtk_tree_model_get_iter_first(store, &it)) {
				gtk_widget_hide(GTK_WIDGET(view));
				gtk_widget_hide(connect_all);
				gchar **split = g_strsplit(gebr_maestro_server_get_address(maestro), "@", -1);
				if (split[1])
					gtk_entry_set_text(GTK_ENTRY(up->server_entry), split[1]);
				else
					gtk_entry_set_text(GTK_ENTRY(up->server_entry), split[0]);
				g_strfreev(split);
			} else {
				gtk_widget_show(GTK_WIDGET(view));
				gtk_widget_show(connect_all);
			}

			WizardStatus wizard_status = get_wizard_status(up);
			if (wizard_status == WIZARD_STATUS_COMPLETE || wizard_status == WIZARD_STATUS_WITHOUT_GVFS)
				gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_servers, TRUE);

			if (!g_strcmp0(gebr_gui_enhanced_entry_get_text(GEBR_GUI_ENHANCED_ENTRY(up->server_entry)), ""))
				gtk_widget_grab_focus(GTK_WIDGET(view));
			else
				gtk_widget_grab_focus(up->server_entry);

			g_signal_connect(GTK_BUTTON(server_all), "clicked", G_CALLBACK(on_connect_all_server_clicked), up);
			g_signal_connect(GTK_BUTTON(server_add), "clicked", G_CALLBACK(on_add_server_clicked), up);
			g_signal_connect(GTK_ENTRY(up->server_entry), "activate", G_CALLBACK(on_entry_server_activate), up);
		}
	}
	else if (page == GVFS_PAGE) {
		GtkButton *button = GTK_BUTTON(gtk_builder_get_object(up->builder, "mount_button"));

		g_signal_connect(button, "clicked", G_CALLBACK(on_mount_gvfs_clicked), up);

		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
		if (gebr_maestro_server_has_connected_daemon(maestro))
			set_status_for_mount(maestro, STATUS_MOUNT_OK, up);
		else
			set_status_for_mount(maestro, STATUS_MOUNT_NOK, up);
	}
}

static void
on_assistant_preferences_apply(GtkAssistant *assistant,
                               struct ui_preferences *up)
{
	save_preferences_configuration(up);
}

static void
on_assistant_preferences_close(GtkAssistant *assistant,
                               struct ui_preferences *up)
{
	gtk_widget_destroy(GTK_WIDGET(assistant));
}

static void
on_preferences_destroy(GtkWidget *window,
                       struct ui_preferences *up)
{
	g_free(up);
}

static void
on_assistant_destroy(GtkWidget *window,
                     struct ui_preferences *up)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	if (maestro) {
		gebr_maestro_server_set_wizard_setup(maestro, FALSE);
		g_signal_handlers_disconnect_by_func(maestro, on_daemons_changed, up);
		g_signal_handlers_disconnect_by_func(maestro, on_mount_status_changed, up);
		g_signal_handlers_disconnect_by_func(gebr.maestro_controller, on_maestro_state_changed, up);
	}

	g_free(up);
}

static void
set_preferences_page(GtkBuilder *builder,
                     struct ui_preferences *ui_preferences)
{
	/*
	 * User name
	 */
	ui_preferences->username = GTK_WIDGET(gtk_builder_get_object(builder, "entry_name"));
	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->username), TRUE);
	gebr_gui_gtk_widget_set_tooltip(ui_preferences->username, _("You should know your name"));

	/* read config */
	if (strlen(gebr.config.username->str))
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->username), gebr.config.username->str);
	else
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->username), g_get_real_name());

	/*
	 * User email
	 */
	ui_preferences->email = GTK_WIDGET(gtk_builder_get_object(builder, "entry_email"));
	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->email), TRUE);
	gebr_gui_gtk_widget_set_tooltip(ui_preferences->email, _("Your email address"));

	/* read config */
	if (strlen(gebr.config.email->str))
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), gebr.config.email->str);
	else
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), g_get_user_name());

	/*
	 * User's menus directory
	 */
	GtkWidget *eventbox = GTK_WIDGET(gtk_builder_get_object(builder, "menus_box"));
	ui_preferences->usermenus = gtk_file_chooser_button_new(_("User's menu directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences->usermenus);
	gebr_gui_gtk_widget_set_tooltip(eventbox, _("Path to look for user's private menus"));

	/* read config */
	if (gebr.config.usermenus->len > 0)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ui_preferences->usermenus),
		                                    gebr.config.usermenus->str);
}

static void
on_combo_set_text(GtkCellLayout   *cell_layout,
                  GtkCellRenderer *cell,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer 	   data)
{
	gchar *description;

	gtk_tree_model_get(tree_model, iter,
	                   MAESTRO__DEFAULT_DESCRIPTION, &description,
	                   -1);

	gchar *text = g_markup_printf_escaped("<i>%s</i>", description);

	g_object_set(cell, "markup", text, NULL);

	g_free(text);
	g_free(description);
}

static void
validate_entry(GtkEntry *entry,
	       gboolean error,
	       const gchar *err_text,
	       const gchar *clean_text)
{
	if (!error) {
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		gtk_entry_set_icon_tooltip_text(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		gtk_widget_set_tooltip_text(GTK_WIDGET(entry), clean_text);
	} else {
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
		gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, err_text);
		gtk_widget_set_tooltip_text(GTK_WIDGET(entry), err_text);
	}
}

static void
on_server_entry_changed(GtkWidget *entry,
                        struct ui_preferences *up)
{
	const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
	GtkWidget *server_add = GTK_WIDGET(gtk_builder_get_object(up->builder, "server_add"));

	gboolean error = !gebr_verify_address_without_username(entry_text);

	gtk_widget_set_sensitive(server_add, !error);
	if (!g_strcmp0(entry_text, DEFAULT_SERVERS_ENTRY_TEXT))
		return;

	if (*entry_text) {
		validate_entry(GTK_ENTRY(up->server_entry), error,
		               _("GêBR supports the formats hostname or ip address."),
		               DEFAULT_SERVERS_ENTRY_TEXT);
	} else {
		gtk_entry_set_icon_from_stock(GTK_ENTRY(up->server_entry), GTK_ENTRY_ICON_SECONDARY, NULL);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(up->server_entry), GTK_ENTRY_ICON_SECONDARY, NULL);
		gtk_widget_set_tooltip_text(GTK_WIDGET(entry), DEFAULT_SERVERS_ENTRY_TEXT);
	}
}

static void
set_servers_page(GtkBuilder *builder,
                 struct ui_preferences *up)
{
	GObject *server_box = gtk_builder_get_object(up->builder, "add_server_box");

	GtkWidget *server_entry = gebr_gui_enhanced_entry_new_with_empty_text(DEFAULT_SERVERS_ENTRY_TEXT);
	gtk_box_pack_start(GTK_BOX(server_box), server_entry, TRUE, TRUE, 0);
	gtk_widget_show_all(server_entry);

	up->server_entry = server_entry;

	g_signal_connect(GTK_ENTRY(server_entry), "changed", G_CALLBACK(on_server_entry_changed), up);
}

static void
set_maestro_chooser_page(GtkBuilder *builder,
                         struct ui_preferences *up)
{
	GtkComboBox *combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "maestro_combo"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	up->maestro_entry = entry;

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo), renderer, on_combo_set_text, NULL, NULL);

	GtkListStore *model = gtk_list_store_new(MAESTRO_DEFAULT_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	gebr_maestro_controller_create_chooser_model(model, maestro);

	g_signal_connect(combo, "changed", G_CALLBACK(gebr_maestro_controller_on_maestro_combo_changed), gebr.maestro_controller);

	gtk_combo_box_set_model(combo, GTK_TREE_MODEL(model));
	gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(combo), MAESTRO_DEFAULT_LABEL);
	gtk_combo_box_set_active(combo, 0);
}

static void
on_preferences_button_clicked (GtkButton *button, gpointer pointer)
{
	const gchar *section = "actions_preferences";
	gchar *error;

	gebr_gui_help_button_clicked(section, &error);

	if (error) {
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, error);
		g_free(error);
	}
}

static void
assistant_set_initial_labels(GtkBuilder *builder)
{
	GtkLabel *review_orientations_label = GTK_LABEL(gtk_builder_get_object(builder, "review_orientation"));
	GtkImage *review_img = GTK_IMAGE(gtk_builder_get_object(builder, "review_img"));

	GtkLabel *review_pref_label = GTK_LABEL(gtk_builder_get_object(builder, "review_pref_label"));
	GtkImage *review_pref_img = GTK_IMAGE(gtk_builder_get_object(builder, "review_pref_img"));
	GtkImage *review_servers_img = GTK_IMAGE(gtk_builder_get_object(builder, "review_servers_img"));
	GtkLabel *review_servers_label = GTK_LABEL(gtk_builder_get_object(builder, "review_servers_label"));
	GtkLabel *review_maestro_label = GTK_LABEL(gtk_builder_get_object(builder, "review_maestro_label"));
	GtkImage *review_maestro_img = GTK_IMAGE(gtk_builder_get_object(builder, "review_maestro_img"));
	GtkImage *review_gvfs_img = GTK_IMAGE(gtk_builder_get_object(builder, "review_gvfs_img"));
	GtkLabel *review_gvfs_label = GTK_LABEL(gtk_builder_get_object(builder, "review_gvfs_label"));

	gtk_label_set_markup(review_orientations_label, _("GêBR is unable to proceed without this configuration."));
	gtk_image_set_from_stock(GTK_IMAGE(review_img), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_label_set_markup(review_pref_label, _("<i>Done.</i>"));
	gtk_image_set_from_stock(GTK_IMAGE(review_pref_img), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
	gtk_image_set_from_stock(GTK_IMAGE(review_maestro_img), GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
	gtk_label_set_markup(review_maestro_label, _("<i>Not connected.</i>"));
	gtk_image_set_from_stock(GTK_IMAGE(review_servers_img), GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
	gtk_label_set_markup(review_servers_label, _("<i>None connected.</i>"));
	gtk_image_set_from_stock(GTK_IMAGE(review_gvfs_img), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU);
	gtk_label_set_markup(review_gvfs_label, _("<i>Disabled.</i>"));
}

/**
 * Assembly preference window.
 *
 * Parameters:
 * @first_run: %TRUE if want to run preferences wizard for the first time
 * @wizard_run: %TRUE to run connections wizard, or %FALSE to create only preferences
 * @insert_preferences: %TRUE to include preferences tab on connections wizard
 * @page: A @GebrUiPreferencesPage with one of the options pages, or -1 to use default
 *
 * \return The structure containing relevant data. It will be automatically freed when the dialog closes.
 */
struct ui_preferences *
preferences_setup_ui(gboolean first_run,
                     gboolean wizard_run,
                     gboolean insert_preferences,
                     GebrUiPreferencesPage page)
{
	struct ui_preferences *ui_preferences;

	ui_preferences = g_new(struct ui_preferences, 1);
	ui_preferences->first_run = first_run;
	ui_preferences->cancel_assistant = FALSE;
	ui_preferences->maestro_addr = NULL;
	ui_preferences->insert_preferences = insert_preferences;
	ui_preferences->tried_to_mount_gvfs = FALSE;

	/* Load pages from Glade */
	GtkBuilder *builder = gtk_builder_new();
	const gchar *glade_file = GEBR_GLADE_DIR "/gebr-connections-settings.glade";
	gtk_builder_add_from_file(builder, glade_file, NULL);

	ui_preferences->builder = builder;
	assistant_set_initial_labels(builder);

	GtkWidget *page_intro  = GTK_WIDGET(gtk_builder_get_object(builder, "intro"));
	GtkWidget *page_preferences = GTK_WIDGET(gtk_builder_get_object(builder, "main_preferences"));
	GtkWidget *page_minfo = GTK_WIDGET(gtk_builder_get_object(builder, "maestro_info"));
	GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(builder, "maestro_chooser"));
	GtkWidget *page_review = GTK_WIDGET(gtk_builder_get_object(builder, "review"));
	GtkWidget *servers_info = GTK_WIDGET(gtk_builder_get_object(builder, "servers_info"));
	GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(builder, "main_servers"));
	GtkWidget *mount_gvfs = GTK_WIDGET(gtk_builder_get_object(builder, "mount_gvfs"));

	GtkWidget *assistant = gtk_assistant_new();
	gtk_window_set_position(GTK_WINDOW(assistant), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_transient_for(GTK_WINDOW(assistant), gebr_maestro_controller_get_window(gebr.maestro_controller));

	g_signal_connect(assistant, "cancel", G_CALLBACK(on_assistant_cancel), ui_preferences);

	/* Create Wizard if the first_run of GeBR */
	if (first_run || wizard_run) {
		gtk_window_set_title(GTK_WINDOW(assistant), _("Configuring GêBR"));

		g_signal_connect(assistant, "destroy", G_CALLBACK(on_assistant_destroy), ui_preferences);
		g_signal_connect(assistant, "close", G_CALLBACK(on_assistant_close), ui_preferences);
		g_signal_connect(assistant, "prepare", G_CALLBACK(on_assistant_prepare), ui_preferences);

		// CANCEL_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_review);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_review, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_review, GTK_ASSISTANT_PAGE_SUMMARY);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_review, _("Status"));

		if (insert_preferences) {
			// PREFERENCES_PAGE
			gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_preferences);
			gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_preferences, GTK_ASSISTANT_PAGE_INTRO);
			gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_preferences, _("Preferences"));

		} else {
			// INTRO PAGE
			gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_intro);
			gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_intro, TRUE);
			gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_intro, GTK_ASSISTANT_PAGE_INTRO);
			gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_intro, _("Welcome"));
			GtkWidget *intro_label_version  = GTK_WIDGET(gtk_builder_get_object(builder, "intro_label_version"));
			gchar *intro_label_version_text = g_strdup_printf(_("It's the first time you use GêBR %s."), gebr_version());
			gtk_label_set_markup(GTK_LABEL(intro_label_version), intro_label_version_text);
			g_free(intro_label_version_text);
		}

		// MAESTRO_INFO_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_minfo);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_minfo, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_minfo, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_minfo, _("Maestro"));

		// MAESTRO_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), main_maestro);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_maestro, FALSE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_maestro, _("Maestro"));

		// SERVERS_INFO_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), servers_info);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), servers_info, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), servers_info, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), servers_info, _("Nodes"));

		// SERVERS_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), main_servers);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_servers, FALSE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_servers, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_servers, _("Nodes"));

		// GVFS_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), mount_gvfs);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), mount_gvfs, FALSE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), mount_gvfs, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), mount_gvfs, _("Remote Browsing"));

		ui_preferences->prev_page = INITIAL_PAGE;
		ui_preferences->back_button = gtk_button_new_with_mnemonic("_Back");
		gtk_assistant_add_action_widget(GTK_ASSISTANT(assistant), ui_preferences->back_button);
		g_signal_connect(ui_preferences->back_button, "clicked", G_CALLBACK(on_assistant_back_button), ui_preferences);

		ui_preferences->dialog = assistant;

		/* Set Preferences Page */
		set_preferences_page(builder, ui_preferences);

		/* Set Maestro Chooser Page */
		set_maestro_chooser_page(builder, ui_preferences);

		/* Set Servers Page */
		set_servers_page(builder, ui_preferences);

		/* finally... */
		gtk_widget_show_all(ui_preferences->dialog);

		if (page != -1) {
			GtkWidget *curr_page = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), page);
			gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), page);
			gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), curr_page, GTK_ASSISTANT_PAGE_INTRO);
		} else {
			if (!first_run && wizard_run && !insert_preferences) {
				gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), MAESTRO_INFO_PAGE);
				gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_minfo, GTK_ASSISTANT_PAGE_INTRO);
			} else {
				gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), INITIAL_PAGE);
			}
		}
	}
	else {
		gtk_window_set_title(GTK_WINDOW(assistant), _("Preferences"));

		g_signal_connect(assistant, "destroy", G_CALLBACK(on_preferences_destroy), ui_preferences);
		g_signal_connect(assistant, "apply", G_CALLBACK(on_assistant_preferences_apply), ui_preferences);
		g_signal_connect(assistant, "close", G_CALLBACK(on_assistant_preferences_close), ui_preferences);

		// PREFERENCES_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_preferences);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_preferences, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_preferences, _("Preferences"));

		ui_preferences->dialog = assistant;

		/* Set Preferences Page */
		set_preferences_page(builder, ui_preferences);

		on_changed_validate_email(ui_preferences->email, ui_preferences);
		g_signal_connect(ui_preferences->email, "changed", G_CALLBACK(on_changed_validate_email), ui_preferences);


		GtkWidget *help_preferences_button = gtk_button_new_from_stock(GTK_STOCK_HELP);
		gtk_assistant_add_action_widget(GTK_ASSISTANT(assistant), help_preferences_button);
		g_signal_connect(GTK_BUTTON(help_preferences_button), "clicked", G_CALLBACK(on_preferences_button_clicked), NULL);
		gtk_widget_show(help_preferences_button);

		/* finally... */
		gtk_widget_show_all(ui_preferences->dialog);
	}

	gtk_window_set_modal(GTK_WINDOW(ui_preferences->dialog), TRUE);

	return ui_preferences;
}
