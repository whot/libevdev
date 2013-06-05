noinst_PROGRAMS = test-event-names test-libevdev-init test-libevdev-has-event

TESTS = $(noinst_PROGRAMS)

libevdev_sources = $(top_srcdir)/libevdev/libevdev.c \
		   $(top_srcdir)/libevdev/libevdev.h \
		   $(top_srcdir)/libevdev/libevdev-util.h \
		   $(top_srcdir)/libevdev/libevdev-int.h
common_sources = $(libevdev_sources) test-common-uinput.c test-common-uinput.h

AM_CPPFLAGS = -I$(top_srcdir)/libevdev $(CHECK_CFLAGS)

test_event_names_SOURCES = test-event-names.c $(common_sources)
test_event_names_LDADD = $(CHECK_LIBS)

test_libevdev_init_SOURCES = test-libevdev-init.c $(common_sources)
test_libevdev_init_LDADD = $(CHECK_LIBS)

test_libevdev_has_event_SOURCES = test-libevdev-has-event.c $(common_sources)
test_libevdev_has_event_LDADD = $(CHECK_LIBS)
