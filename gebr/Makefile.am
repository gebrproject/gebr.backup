# GeBR - An environment for seismic processing.
# Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
include $(top_srcdir)/Makefile.decl

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
EXTRADIR =

SUBDIRS = . tests po data

bin_PROGRAMS = gebr

touch:
	if [ -d $(top_srcdir)/.hg ]; \
	then \
	  touch $(srcdir)/main.c -r $(top_srcdir)/.hg; \
	fi
cmdline.c: touch

#TODO: remove cmdline.{c,h} in future versions (remove old config format support)
noinst_LTLIBRARIES = libgebr.la
libgebr_la_SOURCES =		\
	callbacks.c		\
	callbacks.h		\
	cmdline.c		\
	cmdline.h		\
	document.c		\
	document.h		\
	flow.c			\
	flow.h			\
	gebr-connectable.c	\
	gebr-connectable.h	\
	gebr-daemon-server.c	\
	gebr-daemon-server.h	\
	gebr-dictionary.c	\
	gebr-dictionary.h	\
	gebr-flow-edition.c	\
	gebr-flow-edition.h	\
	gebr-gettext.h		\
	gebr-help-edit-widget.c	\
	gebr-help-edit-widget.h	\
	gebr-job-control.c	\
	gebr-job-control.h	\
	gebr-job.c		\
	gebr-job.h		\
	gebr-maestro-controller.c	\
	gebr-maestro-controller.h	\
	gebr-maestro-server.c	\
	gebr-maestro-server.h	\
	gebr-marshal.c		\
	gebr-marshal.h		\
	gebr-menu-view.c	\
	gebr-menu-view.h	\
	gebr-report.c		\
	gebr-report.h		\
	gebr.c			\
	gebr.h			\
	interface.c		\
	interface.h		\
	line.c			\
	line.h			\
	menu.c			\
	menu.h			\
	project.c		\
	project.h		\
	ui_document.c		\
	ui_document.h		\
	ui_flow.c		\
	ui_flow.h		\
	ui_flows_io.c		\
	ui_flows_io.h		\
	ui_flow_program.c	\
	ui_flow_program.h	\
	ui_flow_execution.c	\
	ui_flow_execution.h	\
	ui_flow_browse.c	\
	ui_flow_browse.h	\
	ui_help.c		\
	ui_help.h		\
	ui_log.c		\
	ui_log.h		\
	ui_parameters.c		\
	ui_parameters.h		\
	ui_paths.c		\
	ui_paths.h		\
	ui_preferences.c	\
	ui_preferences.h	\
	ui_project_line.c	\
	ui_project_line.h	\
	$(NULL)

libgebr_la_LIBADD =		\
	$(GUI_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(GEBR_GUI_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	$(NULL)

# glib-genmarshal rules
glib_marshal_list = gebr-marshal.list
glib_marshal_prefix = gebr_cclosure_marshal
include $(top_srcdir)/Makefile.am.marshal

gebr_SOURCES = main.c

AM_CPPFLAGS =			\
	$(GUI_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_COMM_CFLAGS)	\
	$(GEBR_GUI_CFLAGS)	\
	$(GEBR_JSON_CFLAGS)	\
	@DEBUG_CFLAGS@		\
	-DNANOVERSION=\"$(NANOVERSION)\" \
	$(NULL)

AM_CFLAGS = $(COMMON_CFLAGS)

gebr_LDADD =			\
	$(GUI_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(GEBR_GUI_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	libgebr.la		\
	$(NULL)

EXTRA_DIST += gebr-marshal.list

.PHONY: touch
