/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
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

/*
 * File: ui_log.c
 * Responsible for UI for viewing the log file
 */

#include <string.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/date.h>

#include "gebr-maestro-controller.h"
#include "ui_log.h"
#include "gebr.h"

/*
 * Prototypes
 */

/*
 * Section: Public
 * Public functions.
 */

/*
 * Section: Private
 * Private functions.
 */

static gboolean
on_maestro_press_event(GtkWidget      *widget,
                       GdkEventButton *event)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED)
		preferences_setup_ui(FALSE, TRUE, FALSE, MAESTRO_PAGE);
	else if (!gebr_maestro_server_has_servers(maestro, TRUE))
		preferences_setup_ui(FALSE, TRUE, FALSE, SERVERS_PAGE);
	else
		return FALSE;

	return TRUE;
}

static gboolean
on_remote_browse_press_event(GtkWidget      *widget,
                             GdkEventButton *event)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (on_maestro_press_event(widget, event))
		return TRUE;

	gchar *prefix = gebr_maestro_server_get_browse_prefix(maestro);
 	if (!prefix) {
		preferences_setup_ui(FALSE, TRUE, FALSE, GVFS_PAGE);
		g_free(prefix);
 	}

	return TRUE;
}

/*
 * Function: log_setup_ui
 * Assembly the job control page.
 *
 * Return:
 * The structure containing relevant data.
 *
 */
struct ui_log *log_setup_ui(void)
{
	struct ui_log *ui_log;

	GtkWidget *scrolled_win;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	ui_log = g_new(struct ui_log, 1);

	ui_log->box = gtk_hbox_new(FALSE, 10);
	ui_log->widget = gtk_expander_new("");

	/*
	 * Store/View
	 */
	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), 180, 130);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(ui_log->widget), scrolled_win);

	ui_log->store = gtk_list_store_new(GEBR_LOG_N_COLUMN, GDK_TYPE_PIXBUF,	/* Icon         */
					   G_TYPE_STRING,	/* Date         */
					   G_TYPE_STRING);	/* struct job   */
	ui_log->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_log->store));
	gtk_container_add(GTK_CONTAINER(scrolled_win), ui_log->view);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_log->view), FALSE);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_log->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", GEBR_LOG_TYPE_ICON);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Date"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_log->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", GEBR_LOG_DATE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Message"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_log->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", GEBR_LOG_MESSAGE);

	/*
	 * Maestro / Remote Browse
	 */
	GtkWidget *internal_box = gtk_hbox_new(FALSE, 5);

	GtkWidget *event_maestro = gtk_event_box_new();
	ui_log->maestro_icon = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(event_maestro), ui_log->maestro_icon);
	g_signal_connect(event_maestro, "button-press-event",
	                 G_CALLBACK(on_maestro_press_event), NULL);
	gtk_box_pack_start(GTK_BOX(internal_box), event_maestro, FALSE, FALSE, 5);

	GtkWidget *event_browse = gtk_event_box_new();
	ui_log->remote_browse = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(event_browse), ui_log->remote_browse);
	g_signal_connect(event_browse, "button-press-event",
	                 G_CALLBACK(on_remote_browse_press_event), NULL);
	gtk_box_pack_start(GTK_BOX(internal_box), event_browse, FALSE, FALSE, 5);

	gtk_image_set_from_stock(GTK_IMAGE(ui_log->maestro_icon), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_set_tooltip_text(ui_log->maestro_icon, _("Disconnected"));

	gtk_widget_set_tooltip_text(ui_log->remote_browse, _("Remote browsing disabled"));
	gtk_image_set_from_stock(GTK_IMAGE(ui_log->remote_browse), "folder-warning", GTK_ICON_SIZE_LARGE_TOOLBAR);
	/*
	 * Pack Maestro / Remote Browse with Log
	 */
	gtk_box_pack_start(GTK_BOX(ui_log->box), ui_log->widget, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(ui_log->box), internal_box, FALSE, FALSE, 5);

	return ui_log;
}

/*
 * Section: Private
 * Private functions.
 */

void log_set_message(struct ui_log *ui_log, const gchar * message)
{
	gchar *msga;
	gchar *ptr;
	GString *msgb;

	msga = g_strndup(message, 80);
	msga = g_strdelimit(g_strchug(msga), "\n", ' ');

	ptr = msga;
	while (*ptr != '\0') {
		if (g_ascii_iscntrl(*ptr))
			*ptr = ' ';
		ptr++;
	}

	msgb = g_string_new(msga);
	if (strlen(message) > 80)
		g_string_append(msgb, "...");
	gtk_expander_set_label(GTK_EXPANDER(ui_log->widget), msgb->str);

	g_free(msga);
	g_string_free(msgb, TRUE);
}

void gebr_log_add_message_to_list(struct ui_log *ui_log, GebrLogMessage *message)
{
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	GString *markuped_date;

	/* date */
	markuped_date = g_string_new(NULL);
	g_string_printf(markuped_date, "<small>%s</small>", gebr_localized_date(gebr_log_message_get_date(message)));

	/* icon type */
	// FIXME: Use stock icons here, please
	switch (gebr_log_message_get_type(message)) {
	case GEBR_LOG_START:
		pixbuf = gebr.pixmaps.stock_go_forward;
		break;
	case GEBR_LOG_END:
		pixbuf = gebr.pixmaps.stock_go_back;
		break;
	case GEBR_LOG_INFO:
		pixbuf = gebr.pixmaps.stock_info;
		break;
	case GEBR_LOG_ERROR:
		pixbuf = gebr.pixmaps.stock_cancel;
		break;
	case GEBR_LOG_WARNING:
		pixbuf = gebr.pixmaps.stock_warning;
		break;
	default:
		return;
	}

	/* add */
	gtk_list_store_append(ui_log->store, &iter);
	gtk_list_store_set(ui_log->store, &iter,
			   GEBR_LOG_TYPE_ICON, pixbuf,
			   GEBR_LOG_DATE, markuped_date->str, GEBR_LOG_MESSAGE, gebr_log_message_get_message(message), -1);
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(ui_log->view), &iter);

	/* frees */
	g_string_free(markuped_date, TRUE);
}

static void
on_maestro_error(GebrMaestroServer *maestro,
		const gchar *addr,
		const gchar *error_type,
		const gchar *error_msg,
		struct ui_log *ui_log)
{
	if (g_strcmp0(error_type, "error:none") != 0) {
		gchar *msg = g_strdup(error_msg);

		gtk_image_set_from_stock(GTK_IMAGE(ui_log->maestro_icon), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_LARGE_TOOLBAR);

		gchar *text = g_strdup_printf(_("Error on %s:\n%s"),addr, g_strstrip(msg));
		gtk_widget_set_tooltip_text(ui_log->maestro_icon, text);
		g_free(text);

		gtk_image_set_from_stock(GTK_IMAGE(ui_log->remote_browse), "folder-warning", GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_set_tooltip_text(ui_log->remote_browse, _("Remote browsing disabled"));

		g_free(msg);
	}
}

static void
on_gvfs_mount(GebrMaestroServer *maestro,
              GebrMountStatus status,
              struct ui_log *ui_log)
{
	if (status == STATUS_MOUNT_OK) {
		gtk_image_set_from_stock(GTK_IMAGE(ui_log->remote_browse), "folder-ok", GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_set_tooltip_text(ui_log->remote_browse, _("Remote browsing enabled"));
	} else if (status == STATUS_MOUNT_NOK) {
		gtk_image_set_from_stock(GTK_IMAGE(ui_log->remote_browse), "folder-warning", GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_widget_set_tooltip_text(ui_log->remote_browse, _("Remote browsing disabled"));
	}
}

static void
on_state_change(GebrMaestroServer *maestro,
                struct ui_log *ui_log)
{
	const gchar *addr = gebr_maestro_server_get_address(maestro);

	if (gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED) {
		if (gebr_maestro_server_has_servers(maestro, TRUE)) {
			gtk_image_set_from_stock(GTK_IMAGE(ui_log->maestro_icon), GTK_STOCK_CONNECT, GTK_ICON_SIZE_LARGE_TOOLBAR);

			gchar *text = g_markup_printf_escaped(_("Maestro <b>%s</b> connected"), addr);
			gtk_widget_set_tooltip_markup(ui_log->maestro_icon, text);
			g_free(text);
		} else {
			gtk_image_set_from_stock(GTK_IMAGE(ui_log->maestro_icon), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_LARGE_TOOLBAR);
			gchar *text = g_markup_printf_escaped(_("Maestro <b>%s</b> without connected nodes"), addr);
			gtk_widget_set_tooltip_markup(ui_log->maestro_icon, text);
			g_free(text);
		}
	} else {
		const gchar *error_msg;
		const gchar *error_type;
		gebr_maestro_server_get_error(maestro, &error_type, &error_msg);
		if (g_strcmp0(error_type, "error:none") != 0) {
			gchar *msg = g_strdup(error_msg);

			gtk_image_set_from_stock(GTK_IMAGE(ui_log->maestro_icon), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_LARGE_TOOLBAR);
			gchar *text = g_markup_printf_escaped(_("Error on <b>%s</b>:\n%s"),addr, g_strstrip(msg));
			gtk_widget_set_tooltip_markup(ui_log->maestro_icon, text);

			g_free(text);
			g_free(msg);
		} else {
			gtk_image_set_from_stock(GTK_IMAGE(ui_log->maestro_icon), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_LARGE_TOOLBAR);
			gchar *text = g_markup_printf_escaped(_("Maestro <b>%s</b> disconnected"), addr);
			gtk_widget_set_tooltip_markup(ui_log->maestro_icon, text);
			g_free(text);
		}
	}
}

void
gebr_log_update_maestro_info(struct ui_log *ui_log,
                             GebrMaestroServer *maestro)
{
	on_state_change(maestro, ui_log);
}

void
gebr_log_update_maestro_info_signal(struct ui_log *ui_log,
                                    GebrMaestroServer *maestro)
{
	g_signal_connect(maestro, "state-change",
	                 G_CALLBACK(on_state_change), ui_log);

	g_signal_connect(maestro, "maestro-error",
	                 G_CALLBACK(on_maestro_error), ui_log);

	g_signal_connect(maestro, "gvfs-mount",
		                 G_CALLBACK(on_gvfs_mount), ui_log);
}
