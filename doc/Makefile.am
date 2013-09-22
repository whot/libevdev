if HAVE_DOXYGEN
all-local: doxygen

doxygen: libevdev.doxygen
	doxygen $<

clean-local:
	rm -rf html

.PHONY: doxygen

doc_src= $(shell find html -type f -printf "html/%P\n")
EXTRA_DIST = $(doc_src) libevdev.doxygen libevdev.css
endif

dist_noinst_DATA = libevdev.doxygen.in

# make sure doc was built before running dist
dist-hook:
	@test -f $(distdir)/html/index.html || (\
		echo "******************************************************" && \
		echo "Couldn't find documentation files, refusing make dist." && \
		echo "Install doxygen to build documentation for tarball." && \
		echo "******************************************************" && \
		test )
