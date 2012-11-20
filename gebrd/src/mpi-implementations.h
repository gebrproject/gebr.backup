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

#ifndef __MPI_IMPLEMENTATIONS_H__
#define __MPI_IMPLEMENTATIONS_H__

#include "mpi-interface.h"

G_BEGIN_DECLS

typedef struct _GebrdOpenMpi GebrdOpenMpi;

struct _GebrdOpenMpi {
	GebrdMpiInterface parent;
	const GebrdMpiConfig * config;
};

GebrdMpiInterface * gebrd_open_mpi_new(const gchar * n_process, const GebrdMpiConfig * config);

G_END_DECLS

#endif /* __MPI_IMPLEMENTATIONS_H__ */