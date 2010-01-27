/*   GeBR Daemon - Process and control execution of flows
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

#ifndef __GEBRD_QUEUES_H
#define __GEBRD_QUEUES_H

#include "job.h"

/**
 * Adds a job at the end of \p queue.
 */
void gebrd_queues_add_job_to(const gchar * queue, struct job * job);

/**
 * Removes a queue and all its jobs.
 */
void gebrd_queues_remove(const gchar * queue);

/**
 * Returns a string containing the name of all queues separated by commas.
 * You must <em>not</em> free internal list data, only the list itself,
 * with #g_list_free.
 */
gchar * gebrd_queues_get_names(void);

/**
 * Returns TRUE if the given queue is busy, FALSE if it's not or queue doesn't exists.
 */
gboolean gebrd_queues_is_queue_busy(const gchar * queue);

/**
 * Sets \p queue to busy.
 */
void gebrd_queues_set_queue_busy(const gchar * queue, gboolean busy);

/**
 * Runs the next job in \p queue.
 */
void gebrd_queues_step_queue(const gchar * queue);

/**
 * TRUE if \p queue have pending jobs, FALSE otherwise.
 */
gboolean gebrd_queues_has_next(const gchar * queue);

#endif /* __GEBRD_QUEUES_H */
