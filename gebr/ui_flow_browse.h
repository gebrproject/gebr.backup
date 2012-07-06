/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/**
 * \file ui_flow_browse.c Responsible for UI for browsing a line's flows.
 */

#ifndef __UI_FLOW_BROWSE_H
#define __UI_FLOW_BROWSE_H

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>
#include "libgebr/comm/gebr-comm-process.h"

G_BEGIN_DECLS

/*
 * Store fields
 */
enum {
	FB_TITLE = 0,
	FB_FILENAME,
	FB_LINE_FLOW_POINTER,
	FB_XMLPOINTER,
	FB_LAST_QUEUES,
	FB_N_COLUMN
};

/**
 */
typedef struct {
	GtkWidget *widget;
	GtkListStore *store;
	GtkWidget *view;

	GtkWidget *info_window;
	GtkWidget *warn_window;

	GtkWidget *revisions_button;

	GtkWidget *rev_main;
	GtkWidget *revpage_main;
	GtkWidget *revpage_warn;
	gboolean update_graph;
	GebrCommProcess *graph_process;

	struct ui_flow_browse_info {
		GtkBuilder *builder_flow;

		GtkWidget *title;
		GtkWidget *description;

		GtkWidget *rev_num;

		GtkWidget *created_label;
		GtkWidget *created;
		GtkWidget *modified_label;
		GtkWidget *modified;

		GtkWidget *lastrun;

		GtkWidget *input_label;
		GtkWidget *input;
		GtkWidget *output_label;
		GtkWidget *output;
		GtkWidget *error_label;
		GtkWidget *error;

		GtkWidget *help_view;
		GtkWidget *help_edit;
		GtkWidget *author;
	} info;

	GtkWidget *nice_button_high;
	GtkWidget *nice_button_low;
	GtkWidget *speed_button;
	GtkWidget *speed_slider;
	GtkWidget *ruler;
} GebrUiFlowBrowse;

/**
 * Assembly the flow browse page.
 * Return:
 * The structure containing relevant data.
 */
GebrUiFlowBrowse *flow_browse_setup_ui();

/**
 * Update information shown about the selected flow
 */
void flow_browse_info_update(void);

/**
 * Set to _iter_ the current selected flow
 */
gboolean flow_browse_get_selected(GtkTreeIter * iter, gboolean warn_unselected);

/**
 */
void flow_browse_reload_selected(void);

/**
 * Select flow at _iter_
 */
void flow_browse_select_iter(GtkTreeIter * iter);

/**
 * Turn multiple selection into single.
 */
void flow_browse_single_selection(void);

void flow_browse_show_help(void);

void flow_browse_edit_help(void);

void gebr_flow_browse_hide(GebrUiFlowBrowse *self);

void gebr_flow_browse_show(GebrUiFlowBrowse *self);

G_END_DECLS
#endif				//__UI_FLOW_BROWSE_H
