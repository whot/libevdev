
# find all libevdev_*.3 pages
# for this to work, the man pages must be created before the man target, so
# we hook into all-local (see below)
dist_man3_MANS = $(shell find . -name "libevdev_*.3" -printf "%P\n")

man_src = \
	libevdev_enable_event_type.txt	\
	libevdev_get_abs_min.txt	\
	libevdev_get_name.txt		\
	libevdev_get_num_slots.txt	\
	libevdev_get_phys.txt		\
	libevdev_grab.txt		\
	libevdev_has_event_type.txt	\
	libevdev_new.txt		\
	libevdev_next_event.txt		\
	libevdev_set_fd.txt		\
	libevdev_set_log_handler.txt

if HAVE_ASCIIDOC

.txt.3:
	a2x --doctype manpage --format manpage $<

all-local: $(man_src) $(man_src:.txt=.3)

SUFFIXES = .3 .txt

endif

EXTRA_DIST = $(man_src)

clean-local:
	rm -f *.3
