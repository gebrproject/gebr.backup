include $(top_srcdir)/Makefile.decl

noinst_PROGRAMS = $(TEST_PROGS)

AM_CFLAGS = $(COMMON_CFLAGS)

AM_CPPFLAGS =			\
	$(GLIB_CFLAGS)		\
	$(GDOME2_CFLAGS)	\
	$(GEBR_CFLAGS)		\
	$(GEBR_GEOXML_CFLAGS)	\
	$(GEBR_JSON_CFLAGS)	\
	$(GEBR_COMM_CFLAGS)	\
	@DEBUG_CFLAGS@ 		\
	-DTEST_DIR='"$(srcdir)"'\
	-I$(srcdir)/..		\
	$(NULL)

AM_LDFLAGS =			\
	$(GLIB_LIBS)		\
	$(GDOME2_LIBS)		\
	$(GEBR_LIBS)		\
	$(GEBR_GEOXML_LIBS)	\
	$(GEBR_JSON_LIBS)	\
	$(GEBR_COMM_LIBS)	\
	$(NULL)

TEST_PROGS += test-sysinfo
test_sysinfo_SOURCES = test-sysinfo.c
test_sysinfo_LDADD = ../libgebrd.la

EXTRA_DIST = cpuinfo meminfo
