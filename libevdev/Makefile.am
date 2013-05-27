lib_LTLIBRARIES=libevdev.la

libevdev_la_SOURCES = \
                   libevdev.h \
                   libevdev-int.h \
                   libevdev.c

libevdev_la_LDFLAGS = -version-info $(LIBEVDEV_LT_VERSION) -export-symbols-regex '^libevdev_'

libevdevincludedir = $(includedir)/libevdev-1.0/libevdev
libevdevinclude_HEADERS = libevdev.h
