if HAVE_DOXYGEN
all-local: doxygen

doxygen: libevdev.doxygen
	doxygen $<

clean-local:
	rm -rf html

.PHONY: doxygen

doc_src= $(shell find . -name "html" -printf "%P\n")
EXTRA_DIST = $(doc_src) libevdev.doxygen libevdev.css
endif

dist_noinst_DATA = libevdev.doxygen.in
