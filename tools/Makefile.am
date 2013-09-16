noinst_PROGRAMS = libevdev-events

AM_CPPFLAGS = $(GCC_CFLAGS) -I$(top_srcdir)/libevdev
libevdev_ldadd = $(top_builddir)/libevdev/libevdev.la

libevdev_events_SOURCES = libevdev-events.c
libevdev_events_LDADD = $(libevdev_ldadd)

