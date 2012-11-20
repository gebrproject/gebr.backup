/*   GeBR - An environment for seismic processing.
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

#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "job.h"
#include "gebr.h"
#include "ui_job_control.h"

struct job *job_find(GString * address, GString * jid)
{
	struct job *job = NULL;

	gboolean job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		struct job *i;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), iter, JC_STRUCT, &i, JC_IS_JOB, &is_job,
				   -1);
		if (!is_job)
			return FALSE;
		if (!strcmp(i->server->comm->address->str, address->str) && !strcmp(i->jid->str, jid->str)) {
			job = i;
			return TRUE;	
		}
		return FALSE;
	}

	gebr_gui_gtk_tree_model_foreach_recursive(GTK_TREE_MODEL(gebr.ui_job_control->store),
						  (GtkTreeModelForeachFunc)job_find_foreach_func, NULL); 

	return job;
}

static GtkTreeIter job_add_jc_queue_iter(struct job * job)
{
	GtkTreeIter queue_jc_iter;

	server_queue_find_at_job_control(job->server, job->queue->str, &queue_jc_iter);
	gtk_tree_store_set(gebr.ui_job_control->store, &queue_jc_iter, JC_STRUCT, job, -1);

	return queue_jc_iter;
}

struct job *job_add(struct server *server, GString * jid,
		    GString * _status, GString * title,
		    GString * start_date, GString * finish_date, GString * hostname, GString * issues, GString *
		    cmd_line, GString * output, GString * queue, GString * moab_jid)
{
	GtkTreeIter iter;
	GtkTreeIter queue_jc_iter;

	struct job *job;
	enum JobStatus status;
	gchar local_hostname[100];

	gethostname(local_hostname, 100);
	status = job_translate_status(_status);
	job = g_new(struct job, 1);
	job->status = status; 
	job->server = server;
	job->title = g_string_new(title->str);
	job->jid = g_string_new(jid->str);
	job->start_date = g_string_new(start_date->str);
	job->finish_date = g_string_new(finish_date == NULL ? "" : finish_date->str);
	job->hostname = g_string_new(hostname == NULL ? local_hostname : hostname->str);
	job->issues = g_string_new(issues->str);
	job->cmd_line = g_string_new(cmd_line->str);
	job->output = g_string_new(NULL);
	job->queue = g_string_new(queue->str); 
	job->moab_jid = g_string_new(moab_jid->str);

	GtkTreeIter queue_iter;
	gboolean queue_exists = server_queue_find(job->server, queue->str, &queue_iter);
	if (server->type == GEBR_COMM_SERVER_TYPE_REGULAR) {

		if (queue->str[0] == 'j') {
			/* If the queue name prefix is 'j', it is an internal queue, with only one job
			 * running on it. The user did not give a name to it and does not know it exists. */

			if (queue_exists && job->status != JOB_STATUS_RUNNING) { 
				/* If the job is not running anymore, then it is not an option to start a queue.
				 * Thus, it should not be in the combobox model. */

				gtk_list_store_remove(server->queues_model, &queue_iter);
				gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), 0);
			} else if (job->status == JOB_STATUS_RUNNING) {
				GString *string = g_string_new(NULL);

				g_string_printf(string, _("After '%s'"), title->str);

				if (!queue_exists)
					/* If the queue does not exist yet, then the job is a new option for enqueuing,
					 * and should be appended to the combobox model. */
					gtk_list_store_append(server->queues_model, &queue_iter);
				
				gtk_list_store_set(server->queues_model, &queue_iter, 0, string->str, 1, queue->str, 2, job, -1);

				g_string_free(string, TRUE);
			}
		} else if (queue_exists) { /* The queue name prefix is 'q' (it has already been named by the user). */
			GString *string = g_string_new(NULL);
			gchar *queue_title = job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR 
				? queue->str+1 /* jump q identifier */ : queue->str;

			if (job->status != JOB_STATUS_RUNNING && job->status != JOB_STATUS_QUEUED)
				g_string_printf(string, _("At '%s'"), queue_title);
			else
				g_string_printf(string, _("After '%s' at '%s'"), title->str, queue_title);

			gtk_list_store_set(server->queues_model, &queue_iter, 0, string->str, 1, queue->str, 2,
					   job, -1);

			g_string_free(string, TRUE);
		}
	}

	queue_jc_iter = job_add_jc_queue_iter(job);
	/* append to the store and select it */
	gtk_tree_store_append(gebr.ui_job_control->store, &iter, &queue_jc_iter); 
	gtk_tree_store_set(gebr.ui_job_control->store, &iter,
			   JC_SERVER_ADDRESS, job->server->comm->address->str,
			   JC_QUEUE_NAME, job->queue->str,
			   JC_TITLE, job->title->str,
			   JC_STRUCT, job,
			   JC_IS_JOB, TRUE,
			   -1);
	job->iter = iter;
	job_append_output(job, output);
	job_set_active(job);

	return job;
}

void job_free(struct job *job)
{
	if (gtk_tree_store_remove(gebr.ui_job_control->store, &job->iter))
		gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_job_control->view), &job->iter);
	g_string_free(job->title, TRUE);
	g_string_free(job->jid, TRUE);
	g_string_free(job->hostname, TRUE);
	g_string_free(job->start_date, TRUE);
	g_string_free(job->finish_date, TRUE);
	g_string_free(job->cmd_line, TRUE);
	g_string_free(job->issues, TRUE);
	g_string_free(job->output, TRUE);
	g_string_free(job->queue, TRUE);
	g_string_free(job->moab_jid, TRUE);
	g_free(job);
}

void job_delete(struct job *job)
{
	job_free(job);
}

void job_close(struct job *job, gboolean force, gboolean verbose)
{
	/* Checking if passed job pointer is valid */
	if (job == NULL)
		return;

	if (force) {
		job_delete(job);
		return;
	}
	/* NOTE: changes here must reflect changes in job_clear at gebrd */
	if (job->status == JOB_STATUS_RUNNING || job->status == JOB_STATUS_QUEUED) {
		if (verbose) {
			if (job->status == JOB_STATUS_RUNNING)
				gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Can't close running job '%s'"), job->title->str);
			else if (job->status == JOB_STATUS_QUEUED)
				gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Can't close queued job '%s'"), job->title->str);
		}
		return;
	}

	if (gebr_comm_server_is_logged(job->server->comm))
		gebr_comm_protocol_send_data(job->server->comm->protocol, job->server->comm->stream_socket,
					     gebr_comm_protocol_defs.clr_def, 1, job->jid->str);

	job_delete(job);
}

void job_set_active(struct job *job)
{
 	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_job_control->view), &job->iter);
}

gboolean job_is_active(struct job *job)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));

	return gtk_tree_selection_iter_is_selected(selection, &job->iter);
}

void job_append_output(struct job *job, GString * output)
{
	GtkTextIter iter;
	GString *text;
	GtkTextMark *mark;

	if (!output->len)
		return;
	if (!job->output->len) {
		g_string_printf(job->output, "\n%s\n%s", _("Output:"), output->str);
		text = job->output;
	} else {
		g_string_append(job->output, output->str);
		text = output;
	}
	if (job_is_active(job) == TRUE) {
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &iter);
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, text->str, text->len);
		if (gebr.config.job_log_auto_scroll) {
			mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "end");
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
		}
	}
}

void job_update(struct job *job)
{
	if (job_is_active(job) == FALSE)
		return;
	job_set_active(job);
}

void job_update_label(struct job *job)
{
	GString *label;

	/* initialization */
	label = g_string_new(NULL);

	g_string_printf(label, "job at %s: %s", job->hostname->str, gebr_localized_date(job->start_date->str));
	if (job->status != JOB_STATUS_RUNNING) {
		g_string_append(label, " - ");
		g_string_append(label, gebr_localized_date(job->finish_date->str));
	}
	gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), label->str);

	/* free */
	g_string_free(label, TRUE);
}

enum JobStatus job_translate_status(GString * status)
{
	enum JobStatus translated_status;

	if (!strcmp(status->str, "unknown"))
		translated_status = JOB_STATUS_UNKNOWN;
	else if (!strcmp(status->str, "queued"))
		translated_status = JOB_STATUS_QUEUED;
	else if (!strcmp(status->str, "failed"))
		translated_status = JOB_STATUS_FAILED;
	else if (!strcmp(status->str, "running"))
		translated_status = JOB_STATUS_RUNNING;
	else if (!strcmp(status->str, "finished"))
		translated_status = JOB_STATUS_FINISHED;
	else if (!strcmp(status->str, "canceled"))
		translated_status = JOB_STATUS_CANCELED;
	else if (!strcmp(status->str, "requeued"))
		translated_status = JOB_STATUS_REQUEUED;
	else if (!strcmp(status->str, "issued"))
		translated_status = JOB_STATUS_ISSUED;
	else
		translated_status = JOB_STATUS_UNKNOWN;

	return translated_status;
}

void job_status_show(struct job *job)
{
	GdkPixbuf *pixbuf;
	
	switch (job->status) {
	case JOB_STATUS_RUNNING:
		pixbuf = gebr.pixmaps.stock_execute;
		break;
	case JOB_STATUS_QUEUED:
		pixbuf = gebr.pixmaps.chronometer;
		break;
	case JOB_STATUS_FINISHED:
		pixbuf = gebr.pixmaps.stock_apply;
		break;
	case JOB_STATUS_FAILED:
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_ISSUED:
		pixbuf = gebr.pixmaps.stock_cancel;
		break;
	default:
		pixbuf = NULL;
		return;
	}
	gtk_tree_store_set(gebr.ui_job_control->store, &job->iter, JC_ICON, pixbuf, -1);

	if (job_is_active(job) == FALSE) 
		return;

	job_update_label(job);
	if (gebr.config.job_log_auto_scroll) {
		GtkTextMark *mark;

		mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "end");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
	}
}

void job_status_update(struct job *job, enum JobStatus status, const gchar *parameter)
{
	if (status == JOB_STATUS_REQUEUED) {
		/* 'parameter' is the new name for the job's queue. */
		g_string_assign(job->queue, parameter);

		GtkTreeIter iter;
		GtkTreeIter parent = job_add_jc_queue_iter(job);
		gtk_tree_store_append(gebr.ui_job_control->store, &iter, &parent); 
		gtk_tree_store_set(gebr.ui_job_control->store, &iter,
				   JC_SERVER_ADDRESS, job->server->comm->address->str,
				   JC_QUEUE_NAME, job->queue->str,
				   JC_TITLE, job->title->str,
				   JC_STRUCT, job,
				   JC_IS_JOB, TRUE,
				   -1);

		gboolean was_selected = job_is_active(job);
		gtk_tree_store_remove(gebr.ui_job_control->store, &job->iter);
		job->iter = iter;
		job_status_show(job);

		if (was_selected)
			job_set_active(job);

		return;
	} else
		job->status = status;
	
	job_status_show(job);

	if (job->status == JOB_STATUS_ISSUED){
		GtkTextIter iter;
		g_string_append(job->issues , parameter);
		gtk_text_buffer_get_iter_at_mark(gebr.ui_job_control->text_buffer, &iter, gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "issue"));
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, parameter, g_utf8_strlen(parameter, -1));
		return;
	}

	if ((job->status == JOB_STATUS_FINISHED) || job->status == JOB_STATUS_CANCELED) {
		/* Update combobox model. */
		GtkTreeIter queue_iter;
		gboolean queue_exists = server_queue_find(job->server, job->queue->str, &queue_iter);
		struct job *last_job;
		
		if (queue_exists) {
			gtk_tree_model_get(GTK_TREE_MODEL(job->server->queues_model), &queue_iter, 2, &last_job, -1);

		       	if (job->queue->str[0] == 'j') {
				gtk_list_store_remove(job->server->queues_model, &queue_iter);
				gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), 0);
			} else if (job->queue->str[0] == 'q' && job == last_job)  {
				GString *string = g_string_new(NULL);

				g_string_printf(string, _("At '%s'"), job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR
						? job->queue->str+1 /* jump q identifier */ : job->queue->str);
				gtk_list_store_set(job->server->queues_model, &queue_iter, 0, string->str, 1,
						   job->queue->str, -1);

				g_string_free(string, TRUE);
			}
		} else {
			GString *string = g_string_new(NULL);

			g_string_printf(string, _("At '%s'"), job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR
					? job->queue->str+1 /* jump q identifier */ : job->queue->str);
			gtk_list_store_append(job->server->queues_model, &queue_iter);
			gtk_list_store_set(job->server->queues_model, &queue_iter, 0, string->str, -1);

			g_string_free(string, TRUE);
		}
		
		/* Fill 'finish date' if job is active (selected for viewing). */
		g_string_assign(job->finish_date, parameter);

		if (job_is_active(job) == FALSE) 
			return;

		GString *finish_date;
		GtkTextIter iter;
		GtkTreeIter tree_iter;
		
		if (!job_control_get_selected(&tree_iter, JobControlDontWarnUnselection))
			return;

		if (!gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(gebr.ui_job_control->store), &(job->iter), &tree_iter))
			return;
		finish_date = g_string_new(NULL);

		if (job->status == JOB_STATUS_FINISHED)
			g_string_printf(finish_date, "\n%s %s", _("Finish date:"), gebr_localized_date(job->finish_date->str));
		if (job->status == JOB_STATUS_CANCELED)
			g_string_printf(finish_date, "\n%s %s", _("Cancel date:"), gebr_localized_date(job->finish_date->str));

		job_update_label(job);
		
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &iter);
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, finish_date->str, finish_date->len);

		g_string_free(finish_date, TRUE);
	} 
}