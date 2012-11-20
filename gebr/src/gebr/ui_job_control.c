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

#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gui/utils.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "ui_job_control.h"
#include "gebr.h"
#include "job.h"

/*
 * Prototypes
 */

static void job_control_on_cursor_changed(void);

static void on_text_view_populate_popup(GtkTextView * textview, GtkMenu * menu);

/*
 * Public functions.
 */

struct ui_job_control *job_control_setup_ui(void)
{
	struct ui_job_control *ui_job_control;

	GtkWidget *hpanel;
	GtkWidget *vbox;
	GtkWidget *scrolled_window;
	GtkWidget *frame;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *text_view;
	GtkTextIter iter_end;

	/* alloc */
	ui_job_control = g_new(struct ui_job_control, 1);

	hpanel = gtk_hpaned_new();
	ui_job_control->widget = hpanel;

	/*
	 * Left side
	 */
	frame = gtk_frame_new(_("Jobs"));
	gtk_paned_pack1(GTK_PANED(hpanel), frame, FALSE, FALSE);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	ui_job_control->store = gtk_tree_store_new(JC_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN, G_TYPE_STRING,
						   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ui_job_control->store), JC_SERVER_ADDRESS,
					     GTK_SORT_ASCENDING);
	ui_job_control->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_job_control->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_job_control->view), FALSE);
	g_signal_connect(GTK_OBJECT(ui_job_control->view), "cursor-changed", G_CALLBACK(job_control_on_cursor_changed),
			 NULL);

	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_job_control->view), col);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", JC_ICON);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", JC_TITLE);

	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_job_control->view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 180, 30);

	/*
	 * Right side
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(hpanel), vbox, TRUE, TRUE);

	ui_job_control->label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), ui_job_control->label, FALSE, TRUE, 0);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_box_pack_end(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	ui_job_control->text_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_end_iter(ui_job_control->text_buffer, &iter_end);
	gtk_text_buffer_create_mark(ui_job_control->text_buffer, "end", &iter_end, FALSE);
	text_view = gtk_text_view_new_with_buffer(ui_job_control->text_buffer);
	g_signal_connect(text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), ui_job_control);
	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	{
		PangoFontDescription *font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(text_view, font);

		pango_font_description_free(font);
	}
	ui_job_control->text_view = text_view;
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

	return ui_job_control;
}

void job_control_save(void)
{
	GtkTreeIter iter;
	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;

	gchar *path;
	FILE *fp;

	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *text;

	struct job *job;

	if (!job_control_get_selected(&iter, JobControlJobQueueSelection))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, JC_STRUCT, &job, -1);

	/* run file chooser */
	chooser_dialog = gebr_gui_save_dialog_new(_("Choose filename to save"), GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), ".txt");

	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Text (*.txt)"));
	gtk_file_filter_add_pattern(filefilter, "*.txt");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	path = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!path)
		return;

	/* save to file */
	fp = fopen(path, "w");
	if (fp == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not write file."));
		goto out;
	}
	gtk_text_buffer_get_start_iter(gebr.ui_job_control->text_buffer, &start_iter);
	gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &end_iter);
	text = gtk_text_buffer_get_text(gebr.ui_job_control->text_buffer, &start_iter, &end_iter, FALSE);
	fputs(text, fp);
	fclose(fp);

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Saved job information at '%s'."), path);

	g_free(text);
 out:	g_free(path);
}

void job_control_cancel(void)
{
	GtkTreeIter iter;

	struct job *job;

	if (!job_control_get_selected(&iter, JobControlJobSelection))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, JC_STRUCT, &job, -1);

	if (job->status != JOB_STATUS_RUNNING && job->status != JOB_STATUS_QUEUED) {
		gebr_message(GEBR_LOG_WARNING, TRUE, FALSE, _("Job is not running."));
		return;
	}
	if (gebr_comm_server_is_logged(job->server->comm) == FALSE) {
		gebr_message(GEBR_LOG_WARNING, TRUE, FALSE, _("You are not connected to job's server."));
		return;
	}
	if (gebr_gui_confirm_action_dialog
	    (_("Terminate job"), _("Are you sure you want to terminate job '%s'?"), job->title->str) == FALSE)
		return;

	gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking server to terminate job."));
	if (gebr_comm_server_is_local(job->server->comm) == FALSE)
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server '%s' to terminate job '%s'."),
			     job->server->comm->address->str, job->title->str);
	else
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking local server to terminate job '%s'."),
			     job->title->str);

	gebr_comm_protocol_send_data(job->server->comm->protocol, job->server->comm->stream_socket,
				     gebr_comm_protocol_defs.end_def, 1, job->jid->str);
}

void job_control_close(void)
{
	GtkTreeIter iter;

	struct job *job;

	if (!job_control_get_selected(&iter, JobControlJobSelection))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, JC_STRUCT, &job, -1);

	if (gebr_gui_confirm_action_dialog
	    (_("Clear job "), _("Are you sure you want to clear job '%s'?"), job->title->str) == FALSE)
		return;

	job_close(job, FALSE, TRUE);
}

void job_control_clear(gboolean force)
{
	if (!force && !gebr_gui_confirm_action_dialog(_("Clear all jobs"),
						      _("Are you sure you want to clear all jobs from all servers?")))
		return;

	gboolean job_control_clear_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		struct job *job;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), iter, JC_STRUCT, &job, JC_IS_JOB,
				   &is_job, -1);
		if (!is_job)
			return FALSE;
		job_close(job, force, FALSE);

		return FALSE;
	}
	gebr_gui_gtk_tree_model_foreach_recursive(GTK_TREE_MODEL(gebr.ui_job_control->store),
						  (GtkTreeModelForeachFunc)job_control_clear_foreach, NULL); 
}

void job_control_stop(void)
{
	GtkTreeIter iter;

	struct job *job;

	if (!job_control_get_selected(&iter, JobControlJobSelection))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, JC_STRUCT, &job, -1);

	if (job->status != JOB_STATUS_RUNNING) {
		gebr_message(GEBR_LOG_WARNING, TRUE, FALSE, _("Job is not running."));
		return;
	}
	if (gebr_comm_server_is_logged(job->server->comm) == FALSE) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("You are not connected to job's server."));
		return;
	}
	if (gebr_gui_confirm_action_dialog(_("Kill job"), _("Are you sure you want to kill job '%s'?"), job->title->str)
	    == FALSE)
		return;

	gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking server to kill job."));
	if (gebr_comm_server_is_local(job->server->comm) == FALSE) 
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server '%s' to kill job '%s'."),
			     job->server->comm->address->str, job->title->str);
	else 
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking local server to kill job '%s'."), job->title->str);

	gebr_comm_protocol_send_data(job->server->comm->protocol, job->server->comm->stream_socket,
				     gebr_comm_protocol_defs.kil_def, 1, job->jid->str);
}

gboolean job_control_get_selected(GtkTreeIter * iter, enum JobControlSelectionType check_type)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_job_control->view), iter)) {
		switch (check_type) {
		case JobControlDontWarnUnselection:
			break;
		case JobControlJobQueueSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Nothing selected."));
			break;
		case JobControlJobSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No job selected."));
			break;
		case JobControlQueueSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No queue selected."));
			break;
		default:
			break;
		}
		return FALSE;
	}

	gboolean is_job;
	GtkTreePath *path;
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_job_control->store), iter);
	is_job = gtk_tree_path_get_depth(path) == 2 ? TRUE : FALSE;
	gtk_tree_path_free(path);
	if (check_type == JobControlJobSelection && !is_job) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Please select a job."));
		return FALSE;
	}
	if (check_type == JobControlQueueSelection && is_job) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Please select a queue."));
		return FALSE;
	}

	return TRUE;
}

/**
 * \internal
 */
static void job_control_on_cursor_changed(void)
{
	GtkTreeIter iter;
	struct job *job;
	gboolean is_job;
 	GString *info, *queue_info;
  
	if (!job_control_get_selected(&iter, JobControlDontWarnUnselection)) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), "");
		gtk_text_buffer_set_text(gebr.ui_job_control->text_buffer, "", 0);
		return;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, JC_STRUCT, &job, JC_IS_JOB,
			   &is_job, -1);
	if (!is_job) {
		gtk_text_buffer_set_text(gebr.ui_job_control->text_buffer, "", 0);
		gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), "");
		return;
	}
	
	info = g_string_new(NULL);
 	/*
 	 * Fill job information
 	 */
 	/* who and where */
	queue_info = g_string_new(NULL); 
	if (job->queue->str[0] == 'j') {
		g_string_assign(queue_info, "unqueued");
	}
	else if (job->queue->str[0] == 'q') {
		g_string_printf(queue_info, "on %s", job->queue->str+1);
	}
 	g_string_append_printf(info, _("Job executed at %s (%s) by %s.\n"),
 			       job->server->comm->protocol->hostname->str, queue_info->str, job->hostname->str);
	g_string_free(queue_info, TRUE);

 	/* start date */
 	g_string_append_printf(info, "%s %s\n", _("Start date:"), gebr_localized_date(job->start_date->str));
 	/* issues */
 	if (job->issues->len)
 		g_string_append_printf(info, "\n%s\n%s", _("Issues:"), job->issues->str); /* command line */
 	if (job->cmd_line->len)
 		g_string_append_printf(info, "\n%s\n%s\n", _("Command line:"), job->cmd_line->str);
 
 	/* job id */
 	if (job->server->type == GEBR_COMM_SERVER_TYPE_MOAB && job->moab_jid->len)
 		g_string_append_printf(info, "\n%s\n%s\n", _("Moab Job ID:"), job->moab_jid->str);
 
 	/* output */
 	if (job->output->len)
 		g_string_append(info, job->output->str);
 	/* finish date */
 	if (job->finish_date->len)
		if (job->status == JOB_STATUS_FINISHED)
			g_string_append_printf(info, "\n%s %s", _("Finish date:"), gebr_localized_date(job->finish_date->str));
		if (job->status == JOB_STATUS_CANCELED)
			g_string_append_printf(info, "\n%s %s", _("Cancel date:"), gebr_localized_date(job->finish_date->str));
 	/* to view */
 	gtk_text_buffer_set_text(gebr.ui_job_control->text_buffer, info->str, info->len);

	job_status_show(job);
 
 	/* frees */
 	g_string_free(info, TRUE);
}

/**
 * \internal
 */
static void wordwrap_toggled(GtkCheckMenuItem * check_menu_item, GtkTextView * text_view)
{
	gebr.config.job_log_word_wrap = gtk_check_menu_item_get_active(check_menu_item);
	g_object_set(G_OBJECT(text_view), "wrap-mode",
		     gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
}

/**
 * \internal
 */
static void autoscroll_toggled(GtkCheckMenuItem * check_menu_item)
{
	gebr.config.job_log_auto_scroll = gtk_check_menu_item_get_active(check_menu_item);
}

/**
 * \internal
 */
static void on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_label(_("Word-wrap"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(wordwrap_toggled), text_view);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_word_wrap);

	menu_item = gtk_check_menu_item_new_with_label(_("Auto-scroll"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(autoscroll_toggled), NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_auto_scroll);
}