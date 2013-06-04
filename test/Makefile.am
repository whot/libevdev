noinst_PROGRAMS = test-event-names

TESTS = $(noinst_PROGRAMS)

AM_CPPFLAGS = -I$(top_srcdir)/libevdev $(CHECK_CFLAGS)
libevdev_ldadd = $(top_builddir)/libevdev/libevdev.la

test_event_names_SOURCES = test-event-names.c 
test_event_names_LDADD = $(libevdev_ldadd) $(CHECK_LIBS)

