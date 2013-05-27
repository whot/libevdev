noinst_PROGRAMS = libevdev-print libevdev-events

AM_CPPFLAGS = -I$(top_srcdir)/libevdev
libevdev_ldadd = $(top_builddir)/libevdev/libevdev.la

libevdev_print_SOURCES = libevdev-print.c event-names.h
libevdev_print_LDADD = $(libevdev_ldadd)

libevdev_events_SOURCES = libevdev-events.c event-names.h
libevdev_events_LDADD = $(libevdev_ldadd)

