/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <linux/input.h>

#include "libevdev.h"
#include "event-names.h"

void
print_code_bits(struct libevdev *dev, unsigned int type, unsigned int max)
{
	unsigned int i;
	for (i = 0; i <= max; i++) {
		if (libevdev_has_event_code(dev, type, i))
			printf("%s: %s\n",
					event_get_type_name(type),
					event_get_code_name(type, i));
	}
}

int print_event(struct input_event *ev)
{
	if (ev->type == EV_SYN)
		printf("Event: time %ld.%06ld, ++++++++++++++++++++ %s +++++++++++++++\n",
				ev->time.tv_sec,
				ev->time.tv_usec,
				event_get_type_name(ev->type));
	else
		printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
			ev->time.tv_sec,
			ev->time.tv_usec,
			ev->type,
			event_get_type_name(ev->type),
			ev->code,
			event_get_code_name(ev->type, ev->code),
			ev->value);
	return 0;
}

int print_sync_event(struct input_event *ev)
{
	printf("SYNC: ");
	print_event(ev);
	return 0;
}

int
main(int argc, char **argv)
{
	struct libevdev *dev = NULL;
	const char *file;
	int fd;
	int rc = 1;

	if (argc < 2)
		goto out;

	file = argv[1];
	fd = open(file, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		perror("Failed to open device");
		goto out;
	}

	dev = libevdev_new(fd);
	if (!dev) {
		fprintf(stderr, "Failed to init libevdev\n");
		goto out;
	}

	do {
		struct input_event ev;
		rc = libevdev_next_event(dev, 0, &ev);
		if (rc == 1) {
			printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
			while (rc == 1) {
				print_sync_event(&ev);
				rc = libevdev_next_event(dev, LIBEVDEV_READ_SYNC, &ev);
			}
			printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
		} else if (rc == 0)
			print_event(&ev);
	} while (rc == 1 || rc == 0 || rc == -EAGAIN);

	if (rc != 0 && rc != -EAGAIN)
		fprintf(stderr, "Failed to handle events: %s\n", strerror(rc));

	rc = 0;
out:
	libevdev_free(dev);

	return rc;
}
