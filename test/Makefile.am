noinst_PROGRAMS = test-event-names test-libevdev-init test-libevdev-has-event test-int-queue

TESTS = $(noinst_PROGRAMS)

libevdev_sources = $(top_srcdir)/libevdev/libevdev.c \
		   $(top_srcdir)/libevdev/libevdev.h \
		   $(top_srcdir)/libevdev/libevdev-util.h \
		   $(top_srcdir)/libevdev/libevdev-int.h
common_sources = $(libevdev_sources) test-common-uinput.c test-common-uinput.h

# include builddir for event-names.h
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)/libevdev $(CHECK_CFLAGS) $(GCOV_CFLAGS)



test_event_names_SOURCES = test-event-names.c $(common_sources)
test_event_names_LDADD =  $(CHECK_LIBS) $(GCOV_LDFLAGS)

test_libevdev_init_SOURCES = test-libevdev-init.c $(common_sources)
test_libevdev_init_LDADD = $(CHECK_LIBS) $(GCOV_LDFLAGS)

test_libevdev_has_event_SOURCES = test-libevdev-has-event.c $(common_sources)
test_libevdev_has_event_LDADD = $(CHECK_LIBS) $(GCOV_LDFLAGS)

test_int_queue_SOURCES = test-int-queue.c $(common_sources)
test_int_queue_LDADD = $(CHECK_LIBS) $(GCOV_LDFLAGS)

if GCOV_ENABLED
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
