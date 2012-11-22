# WEBOOB plugin

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src $(GRISBI_CFLAGS) $(LIBWEBOOB_CFLAGS)

if HAVE_WEBOOB

if ENABLE_STATIC
noinst_LTLIBRARIES = weboob.la
else
plugin_LTLIBRARIES = weboob.la
endif
weboob_la_SOURCES = weboob.c weboob.h
weboob_la_LDFLAGS = -module -avoid-version
weboob_la_LIBADD = $(LIBWEBOOB_LIBS)

endif

CLEANFILES = \
	*~

MAINTAINERCLEANFILES = \
	Makefile.in
