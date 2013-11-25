if BUILD_TESTS
noinst_PROGRAMS = test-libevdev test-compile-pedantic

TESTS = $(noinst_PROGRAMS)

libevdev_sources = $(top_srcdir)/libevdev/libevdev.c \
		   $(top_srcdir)/libevdev/libevdev.h \
		   $(top_srcdir)/libevdev/libevdev-names.c \
		   $(top_srcdir)/libevdev/libevdev-uinput.h \
		   $(top_srcdir)/libevdev/libevdev-uinput.c \
		   $(top_srcdir)/libevdev/libevdev-uinput-int.h \
		   $(top_srcdir)/libevdev/libevdev-util.h \
		   $(top_srcdir)/libevdev/libevdev-int.h
common_sources = $(libevdev_sources) \
		 test-common-uinput.c \
		 test-common-uinput.h \
		 test-common.c \
		 test-common.h

# include builddir for event-names.h
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_builddir)/libevdev $(CHECK_CFLAGS) $(GCOV_CFLAGS)

test_libevdev_SOURCES = \
			test-main.c \
			test-event-names.c \
			test-event-codes.c \
			test-libevdev-init.c \
			test-libevdev-has-event.c \
			test-int-queue.c \
			test-libevdev-events.c \
			test-uinput.c \
			$(common_sources)

test_libevdev_LDADD =  $(CHECK_LIBS) $(GCOV_LDFLAGS)

test_compile_pedantic_SOURCES = test-compile-pedantic.c
test_compile_pedantic_CFLAGS = $(AM_CPPFLAGS) -pedantic -Werror -std=c90

if GCOV_ENABLED

CLEANFILES = gcov-report.txt

gcov-clean:
	@rm -f *.gcov

gcov-report.txt: gcov-clean check-TESTS
	$(AM_V_GEN)(rm -rf $@; \
		echo "========== coverage report ========" >> $@; \
		for file in `find $(top_srcdir)/libevdev -name "*.c" -printf "%P\n"`; do \
			gcov $$file > /dev/null; \
			if test -f $$file.gcov; then \
				total=`grep -v " -:" $$file.gcov | wc -l`; \
				missing=`grep "#####" $$file.gcov | wc -l`; \
				hit=$$((total - missing)); \
				echo -e "$$file: total lines: $$total not tested: $$missing ($$((($$hit * 100)/$$total))%)"; \
			fi \
		done >> $@; \
		echo "========== =============== ========" >> $@; \
	)

gcov: gcov-report.txt
	@cat gcov-report.txt

check-local: gcov

else

gcov-report.txt:
	@true

gcov:
	@true

gcov-clean:
	@true

endif

.PHONY: gcov gcov-clean gcov-report.txt

clean-local: gcov-clean
	rm -f *.gcno *.gcda

endif
