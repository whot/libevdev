/*
 * Copyright © 2014 Red Hat, Inc.
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
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <sys/signalfd.h>

#include "libevdev.h"

struct counter {
	unsigned int nevents;

	bool has_abs,
	     has_rel,
	     has_key;

	unsigned int ev[EV_CNT];
	unsigned int abs[ABS_CNT];
	unsigned int rel[REL_CNT];
	unsigned int key[KEY_CNT];

	unsigned int abs_per_ev[ABS_CNT];
	unsigned int rel_per_ev[REL_CNT];
	unsigned int key_per_ev[KEY_CNT];

	/* for this event: */
	unsigned int abscnt;
	unsigned int relcnt;
	unsigned int keycnt;
};

static void
handle_event(struct counter *counter, const struct input_event *ev)
{
	switch (ev->type) {
		case EV_SYN:
			counter->nevents++;
			if (counter->abscnt)
				counter->ev[EV_ABS]++;
			if (counter->relcnt)
				counter->ev[EV_REL]++;
			if (counter->keycnt)
				counter->ev[EV_KEY]++;

			counter->abs_per_ev[counter->abscnt]++;
			counter->rel_per_ev[counter->relcnt]++;
			counter->key_per_ev[counter->keycnt]++;
			counter->abscnt = 0;
			counter->relcnt = 0;
			counter->keycnt = 0;
			printf("\rEvents received: %5d", counter->nevents);
			break;
		case EV_ABS:
			counter->has_abs = true;
			counter->abscnt++;
			counter->abs[ev->code]++;
			break;
		case EV_REL:
			counter->has_rel = true;
			counter->relcnt++;
			counter->rel[ev->code]++;
			break;
		case EV_KEY:
			counter->has_key = true;
			counter->keycnt++;
			counter->key[ev->code]++;
			break;
	}
}

static void
print_event_stat(const struct counter *counter, int type, int code)
{
	const unsigned int *data = NULL;

	switch (type) {
		case EV_ABS: data = counter->abs; break;
		case EV_REL: data = counter->rel; break;
		case EV_KEY: data = counter->key; break;
		default:
			     return;
	}
	if (data[code] == 0)
		return;

	printf("\t\t%-18s %4d (%.1f%% of type, %.1f%% of total)\n",
	       libevdev_event_code_get_name(type, code),
	       data[code],
	       100.0 * data[code]/counter->ev[type],
	       100.0 * data[code]/counter->nevents);
}

static void
print_per_event_stat(const struct counter *counter, int type)
{
	const unsigned int *data = NULL;
	int i, max;

	switch (type) {
		case EV_ABS:
			data = counter->abs_per_ev;
			break;
		case EV_REL:
			data = counter->rel_per_ev;
			break;
		case EV_KEY:
			data = counter->key_per_ev;
			break;
		default:
			     return;
	}

	printf("\tEvents with/without %s data: %d/%d (%.1f%%/%.1f%%)\n",
	       libevdev_event_type_get_name(type),
	       data[0],
	       counter->ev[type],
	       100.0 * data[0]/counter->nevents,
	       100.0 * counter->ev[type]/counter->nevents);

	max = libevdev_event_type_get_max(type);
	for (i = 1; i < max; i++) {
		if (data[i] == 0)
			continue;

		printf("\t %d %s events per SYN_REPORT: %4d (%.1f%% of type, %.1f%% of total)\n",
		       i,
		       libevdev_event_type_get_name(type),
		       data[i],
		       100.0 * data[i]/counter->ev[type],
		       100.0 * data[i]/counter->nevents);
	}
}

static void
print_stats(struct counter *counter, struct libevdev *dev)
{
	int i;

	printf("Axis/key statistics:\n");

	if (counter->has_abs) {
		printf("\tEV_ABS:\n");

		for (i = 0; i < ABS_CNT; i++)
			print_event_stat(counter, EV_ABS, i);
        }

	if (counter->has_rel) {
		printf("\tEV_REL:\n");

		for (i = 0; i < REL_CNT; i++)
			print_event_stat(counter, EV_REL, i);
        }

	if (counter->has_key) {
		printf("\tEV_KEY:\n");

		for (i = 0; i < KEY_CNT; i++)
			print_event_stat(counter, EV_KEY, i);
        }

	printf("Per event type statistics:\n");
	if (counter->has_abs)
		print_per_event_stat(counter, EV_ABS);
	if (counter->has_rel)
		print_per_event_stat(counter, EV_REL);
	if (counter->has_key)
		print_per_event_stat(counter, EV_KEY);
}

static int
mainloop(struct counter *counter, struct libevdev *dev)
{
	int rc = -1;
	sigset_t mask;
	struct pollfd fds[2];

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	fds[0].fd = signalfd(-1, &mask, SFD_NONBLOCK);
	if (fds[0].fd == -1) {
		fprintf(stderr, "Failed to set up signal handler\n");
		return -1;
	}

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
		fprintf(stderr, "Failed to block signals\n");
		goto out;
	}
	fds[0].events = POLLIN;
	fds[1].fd = libevdev_get_fd(dev);
	fds[1].events = POLLIN;

	while (poll(fds, 2, -1) != -1) {
		int rc;
		struct input_event ev;

		if (fds[0].revents)
			break;

		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
			fprintf(stderr, "SYN_DROPPED received. event count unreliable\n");
			while (rc == LIBEVDEV_READ_STATUS_SYNC) {
				handle_event(counter, &ev);
				rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
			}
		} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS)
			handle_event(counter, &ev);
		else {
			fprintf(stderr, "Failed to read events\n");
			goto out;
		}
	}

	printf("\r                                ");
	printf("\rEvents received: %5d\n", counter->nevents);

	rc = 0;
out:
	close(fds[0].fd);
	return rc;
}

int
main(int argc, char **argv)
{
	struct libevdev *dev = NULL;
	const char *file;
	int fd;
	int rc = 1;
	struct counter counter = {0};

	if (argc < 2)
		goto out;

	file = argv[1];
	fd = open(file, O_RDONLY);
	if (fd < 0) {
		perror("Failed to open device");
		goto out;
	}

	rc = libevdev_new_from_fd(fd, &dev);
	if (rc < 0) {
		fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
		goto out;
	}

	printf("Input device ID: bus %#x vendor %#x product %#x\n",
			libevdev_get_id_bustype(dev),
			libevdev_get_id_vendor(dev),
			libevdev_get_id_product(dev));
	printf("Input device name: \"%s\"\n", libevdev_get_name(dev));

	if (mainloop(&counter, dev) != 0)
		goto out;

	print_stats(&counter, dev);

	rc = 0;
out:
	libevdev_free(dev);

	return rc;
}
