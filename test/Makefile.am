noinst_PROGRAMS = test-event-names test-libevdev-init test-libevdev-has-event

TESTS = $(noinst_PROGRAMS)

common_sources = test-common-uinput.c test-common-uinput.h

AM_CPPFLAGS = -I$(top_srcdir)/libevdev $(CHECK_CFLAGS)
libevdev_ldadd = $(top_builddir)/libevdev/libevdev.la

test_event_names_SOURCES = test-event-names.c $(common_sources)
test_event_names_LDADD = $(libevdev_ldadd) $(CHECK_LIBS)

test_libevdev_init_SOURCES = test-libevdev-init.c $(common_sources)
test_libevdev_init_LDADD = $(libevdev_ldadd) $(CHECK_LIBS)

test_libevdev_has_event_SOURCES = test-libevdev-has-event.c $(common_sources)
test_libevdev_has_event_LDADD = $(libevdev_ldadd) $(CHECK_LIBS)
