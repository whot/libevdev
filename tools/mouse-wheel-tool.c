/*
 * Copyright © 2014 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libevdev/libevdev.h>
#include <sys/signalfd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

static int
usage(void) {
	printf("Usage: %s /dev/input/event0\n", program_invocation_short_name);
	printf("\n");
	printf("This tool counts scroll wheel events from the kernel.\n");
	return 1;
}

static int
print_current_values(int wheel_counts)
{
	static int progress = 0;
	char status = 0;

	switch (progress) {
		case 0: status = '|'; break;
		case 1: status = '/'; break;
		case 2: status = '-'; break;
		case 3: status = '\\'; break;
		default:
			status = '?';
			break;
	}

	progress = (progress + 1) % 4;
printf("\rWheel steps counted: %8d	%c",
	        wheel_counts, status);

	return 0;
}

static unsigned int
mainloop(struct libevdev *dev) {
	struct pollfd fds[2];
	sigset_t mask;
	int wheel_counts = 0;

	fds[0].fd = libevdev_get_fd(dev);
	fds[0].events = POLLIN;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	fds[1].fd = signalfd(-1, &mask, SFD_NONBLOCK);
	fds[1].events = POLLIN;

	sigprocmask(SIG_BLOCK, &mask, NULL);

	while (poll(fds, 2, -1)) {
		struct input_event ev;
		int rc;

		if (fds[1].revents)
			break;

		do {
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
			if (rc == LIBEVDEV_READ_STATUS_SYNC) {
				fprintf(stderr, "Error: cannot keep up\n");
				return 1;
			} else if (rc != -EAGAIN && rc < 0) {
				fprintf(stderr, "Error: %s\n", strerror(-rc));
				return 1;
			} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
				if (libevdev_event_is_code(&ev,
							   EV_REL,
							   REL_WHEEL)) {
					wheel_counts += ev.value;
					print_current_values(abs(wheel_counts));
				}
			}
		} while (rc != -EAGAIN);
	}

	return abs(wheel_counts);
}

static inline const char*
bustype(int bustype)
{
	const char *bus;

	switch(bustype) {
		case BUS_PCI: bus = "pci"; break;
		case BUS_ISAPNP: bus = "isapnp"; break;
		case BUS_USB: bus = "usb"; break;
		case BUS_HIL: bus = "hil"; break;
		case BUS_BLUETOOTH: bus = "bluetooth"; break;
		case BUS_VIRTUAL: bus = "virtual"; break;
		default: bus = "unknown bus type"; break;
	}

	return bus;
}

int
main (int argc, char **argv) {
	int rc;
	int fd;
	const char *path;
	struct libevdev *dev;
	unsigned int wheel_count = 0;

	if (argc < 2)
		return usage();

	path = argv[1];
	if (path[0] == '-')
		return usage();

	fd = open(path, O_RDONLY|O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Error opening the device: %s\n", strerror(errno));
		return 1;
	}

	rc = libevdev_new_from_fd(fd, &dev);
	if (rc != 0) {
		fprintf(stderr, "Error fetching the device info: %s\n", strerror(-rc));
		return 1;
	}

	if (libevdev_grab(dev, LIBEVDEV_GRAB) != 0) {
		fprintf(stderr, "Error: cannot grab the device, something else is grabbing it.\n");
		fprintf(stderr, "Use 'fuser -v %s' to find processes with an open fd\n", path);
		return 1;
	}
	libevdev_grab(dev, LIBEVDEV_UNGRAB);

	if (!libevdev_has_event_code(dev, EV_REL, REL_WHEEL)) {
		fprintf(stderr, "Error: this device doesn't have a wheel.\n");
		return 1;
	}

	printf("Mouse %s on %s\n", libevdev_get_name(dev), path);
	printf("Rotate the wheel by one full rotation. Ctrl+C to exit.\n");
	setbuf(stdout, NULL);

	wheel_count = mainloop(dev);

	printf("\n");

	printf("Entry for hwdb match (replace XXX with the resolution in DPI):\n"
	       "mouse:%s:v%04xp%04x:name:%s:\n"
	       " MOUSE_WHEEL_STOPS=%d\n",
	       bustype(libevdev_get_id_bustype(dev)),
	       libevdev_get_id_vendor(dev),
	       libevdev_get_id_product(dev),
	       libevdev_get_name(dev),
	       wheel_count);

	libevdev_free(dev);
	close(fd);

	return 0;
}
