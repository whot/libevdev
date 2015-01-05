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

#define _GNU_SOURCE
#include <config.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <linux/input.h>

#include "libevdev.h"

static unsigned int changes; /* bitmask of changes */
static struct input_absinfo absinfo;
static int axis;
static int led;
static int led_state = -1;
const char *path;

static void
usage(void)
{
	printf("%s --abs <axis> [--min min] [--max max] [--res res] [--fuzz fuzz] [--flat flat] /dev/input/eventXYZ\n"
	       "\tChange the absinfo struct for the named axis\n"
	       "%s --led <led> --on|--off /dev/input/eventXYZ\n"
	       "\tEnable or disable the named LED\n",
	       program_invocation_short_name, program_invocation_short_name);
}

enum opts {
	OPT_ABS = 1 << 0,
	OPT_MIN = 1 << 1,
	OPT_MAX = 1 << 2,
	OPT_FUZZ = 1 << 3,
	OPT_FLAT = 1 << 4,
	OPT_RES = 1 << 5,
	OPT_LED = 1 << 6,
	OPT_ON = 1 << 7,
	OPT_OFF = 1 << 8,
	OPT_HELP = 1 << 9,
};

static int
parse_options(int argc, char **argv)
{
	int rc = 1;
	int c;
	int option_index = 0;
	static struct option opts[] = {
		{ "abs", 1, 0, OPT_ABS },
		{ "min", 1, 0, OPT_MIN },
		{ "max", 1, 0, OPT_MAX },
		{ "fuzz", 1, 0, OPT_FUZZ },
		{ "flat", 1, 0, OPT_FLAT },
		{ "res", 1, 0, OPT_RES },
		{ "led", 1, 0, OPT_LED },
		{ "on", 0, 0, OPT_ON },
		{ "off", 0, 0, OPT_OFF },
		{ "help", 0, 0, OPT_HELP },
		{ NULL, 0, 0, 0 },
	};

	if (argc < 2)
		goto error;

	while (1) {
		c = getopt_long(argc, argv, "h", opts, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
			case OPT_HELP:
				goto error;
			case OPT_ABS:
				if (changes & OPT_LED)
					goto error;

				axis = libevdev_event_code_from_name(EV_ABS,
								     optarg);
				if (axis == -1)
					goto error;
				break;
			case OPT_LED:
				if (changes & OPT_ABS)
					goto error;

				led = libevdev_event_code_from_name(EV_LED,
								    optarg);
				if (led == -1)
					goto error;
				break;
			case OPT_MIN:
				absinfo.minimum = atoi(optarg);
				break;
			case OPT_MAX:
				absinfo.maximum = atoi(optarg);
				break;
			case OPT_FUZZ:
				absinfo.fuzz = atoi(optarg);
				break;
			case OPT_FLAT:
				absinfo.flat = atoi(optarg);
				break;
			case OPT_RES:
				absinfo.resolution = atoi(optarg);
				break;
			case OPT_ON:
				if (led_state != -1)
					goto error;
				led_state = 1;
				break;
			case OPT_OFF:
				if (led_state != -1)
					goto error;
				led_state = 0;
				break;
			default:
				goto error;
		}
		changes |= c;
	}

	if (optind >= argc)
		goto error;
	path = argv[optind];

	rc = 0;
error:
	return rc;
}

static void
set_abs(struct libevdev *dev)
{
	int rc;
	struct input_absinfo abs;
	const struct input_absinfo *a;

	if ((a = libevdev_get_abs_info(dev, axis)) == NULL) {
		fprintf(stderr,
			"Device '%s' doesn't have axis %s\n",
			libevdev_get_name(dev),
			libevdev_event_code_get_name(EV_ABS, axis));
		return;
	}

	abs = *a;
	if (changes & OPT_MIN)
		abs.minimum = absinfo.minimum;
	if (changes & OPT_MAX)
		abs.maximum = absinfo.maximum;
	if (changes & OPT_FUZZ)
		abs.fuzz = absinfo.fuzz;
	if (changes & OPT_FLAT)
		abs.flat = absinfo.flat;
	if (changes & OPT_RES)
		abs.resolution = absinfo.resolution;

	rc = libevdev_kernel_set_abs_info(dev, axis, &abs);
	if (rc != 0)
		fprintf(stderr,
			"Failed to set absinfo %s: %s",
			libevdev_event_code_get_name(EV_ABS, axis),
			strerror(-rc));
}

static void
set_led(struct libevdev *dev)
{
	int rc;
	enum libevdev_led_value state =
		led_state ? LIBEVDEV_LED_ON : LIBEVDEV_LED_OFF;

	if (!libevdev_has_event_code(dev, EV_LED, led)) {
		fprintf(stderr,
			"Device '%s' doesn't have %s\n",
			libevdev_get_name(dev),
			libevdev_event_code_get_name(EV_LED, axis));
		return;
	}


	rc = libevdev_kernel_set_led_value(dev, led, state);
	if (rc != 0)
		fprintf(stderr,
			"Failed to set LED %s: %s",
			libevdev_event_code_get_name(EV_LED, led),
			strerror(-rc));
}

int
main(int argc, char **argv)
{
	struct libevdev *dev = NULL;
	int fd;
	int rc = 1;

	rc = parse_options(argc, argv);
	if (rc != 0 || !path) {
		usage();
		goto out;
	}

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("Failed to open device");
		goto out;
	}

	rc = libevdev_new_from_fd(fd, &dev);
	if (rc < 0) {
		fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
		goto out;
	}

	if (changes & OPT_ABS)
		set_abs(dev);
	else if (changes & OPT_LED)
		set_led(dev);
	else
		fprintf(stderr,
			"++?????++ Out of Cheese Error. Redo From Start.\n");

out:
	libevdev_free(dev);

	return rc;
}
