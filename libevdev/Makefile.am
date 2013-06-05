lib_LTLIBRARIES=libevdev.la

AM_CPPFLAGS = $(GCOV_CFLAGS)

libevdev_la_SOURCES = \
                   libevdev.h \
                   libevdev-int.h \
                   libevdev-util.h \
                   libevdev.c

libevdev_la_LDFLAGS = -version-info $(LIBEVDEV_LT_VERSION) -export-symbols-regex '^libevdev_' $(GCOV_LDFLAGS)

libevdevincludedir = $(includedir)/libevdev-1.0/libevdev
libevdevinclude_HEADERS = libevdev.h

event-names.h: Makefile make-event-names.py
	        $(srcdir)/make-event-names.py --output=c > $@

EXTRA_DIST = make-event-names.py
CLEANFILES = event-names.h
BUILT_SOURCES = event-names.h

