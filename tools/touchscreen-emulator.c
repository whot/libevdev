/*
 * Copyright © 2016 Red Hat, Inc.
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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "libevdev/libevdev.h"
#include "libevdev/libevdev-util.h"

#define streq(a_, b_) (strcmp(a_, b_) == 0)

typedef int percent_t;

enum slot_state {
	SS_NONE,
	SS_BEGIN,
	SS_UPDATE,
	SS_END,
};

struct emulator {
	const char *source_path;
	const char *dest_path;

	int source_fd;
	int dest_fd;

	struct libevdev *source;
	struct libevdev *dest;

	/* offset on each edge, 0.05 means move 5% in from min */
	double scale;

	/* Track the slot state so we don't forward events from slots
	 * already active when we started up or ones higher than
	 * dest->nslots
	 */
	struct slot {
		enum slot_state state;
	} *slots;
	size_t nslots;
	size_t dest_nslots;
	bool at_least_one_slot_is_active;
};

static void
usage(void)
{
	printf("Usage: %s --dest /dev/input/eventX --source /dev/input/eventY\n",
	       program_invocation_short_name);
	printf("Arguments:\n"
	       " --dest   ... write events into the device provided\n"
	       " --source ... the touchpad to read events from\n"
	       "\n"
	       "Optional arguments:\n"
	       " --scale X ... reduce the touchpad's surface area to the center X%%\n");
}

enum options {
	OPT_HELP,
	OPT_SOURCE,
	OPT_DEST,
	OPT_SCALE,
};

static void
parse_args(struct emulator *context, int argc, char **argv)
{
	static struct option opts[] = {
		{ "help", no_argument, 0, OPT_DEST },
		{ "dest", required_argument, 0, OPT_DEST },
		{ "source", required_argument, 0, OPT_SOURCE },
		{ "scale", required_argument, 0, OPT_SCALE },
		{ 0, 0, 0, 0 }
	};
	percent_t scale;

	while (1) {
		int option_index;
		int c;

		c = getopt_long(argc, argv, "h", opts, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
			case OPT_HELP:
				usage();
				exit(0);
				break;
			case OPT_SOURCE:
				context->source_path = optarg;
				break;
			case OPT_DEST:
				context->dest_path = optarg;
				break;
			case OPT_SCALE:
				scale = atoi(optarg);
				if (scale <= 0 || scale > 100)
					goto error;

				context->scale = (1.0 - scale/100.0)/2.0;
				break;
			default:
				goto error;
		}
	}

	if (optind < argc)
		goto error;

	if (context->source_path == NULL ||
	    context->dest_path == NULL)
		goto error;

	if (streq(context->source_path, context->dest_path))
		goto error;

	return;

error:
	usage();
	exit(1);
}

static bool
source_is_touchpad(struct libevdev *source)
{
	if (!libevdev_has_event_code(source, EV_ABS, ABS_MT_SLOT) ||
	    libevdev_get_num_slots(source) <= 0)
		return false;

	if (!libevdev_has_event_code(source, EV_KEY, BTN_TOOL_FINGER))
		return false;

	/* no tablets */
	if (libevdev_has_event_code(source, EV_KEY, BTN_TOOL_PEN))
		return false;

	/* note: we don't check for INPUT_PROP_DIRECT so we can emulate one
	 * touchscreen from another for testing */

	return true;
}

static bool
dest_is_touchscreen(struct libevdev *source)
{
	if (!libevdev_has_event_code(source, EV_ABS, ABS_MT_SLOT) ||
	    libevdev_get_num_slots(source) <= 0)
		return false;

	/* no tablets */
	if (libevdev_has_event_code(source, EV_KEY, BTN_TOOL_PEN))
		return false;

	/* don't check for INPUT_PROP_DIRECT to allow for touchpad
	   forwarding */

	return true;
}

static bool
init(struct emulator *context)
{
	int fd, rc;
	struct slot *slots;

	fd = open(context->source_path, O_RDONLY);
	if (fd < 0) {
		perror("Failed to open device");
		return false;
	}
	context->source_fd = fd;

	rc = libevdev_new_from_fd(fd, &context->source);
	if (rc != 0) {
		errno = -rc;
		perror("Failed to init device");
		return false;
	}

	fd = open(context->dest_path, O_RDWR);
	if (fd < 0) {
		perror("Failed to open device");
		return false;
	}
	context->dest_fd = fd;
	rc = libevdev_new_from_fd(fd, &context->dest);
	if (rc != 0) {
		errno = -rc;
		perror("Failed to init device");
		return false;
	}

	if (!source_is_touchpad(context->source)) {
		fprintf(stderr, "Invalid source device\n");
		return false;
	}

	if (!dest_is_touchscreen(context->dest)) {
		fprintf(stderr, "Invalid destination device\n");
		return false;
	}

	printf("Forwarding '%s' to '%s'\n",
	       libevdev_get_name(context->source),
	       libevdev_get_name(context->dest));

	context->nslots = libevdev_get_num_slots(context->source);
	slots = calloc(context->nslots, sizeof(*slots));
	context->slots = slots;

	context->dest_nslots = libevdev_get_num_slots(context->dest);
	if (context->dest_nslots < context->nslots) {
		printf("Dest only has %zd slots, anything above will be filtered\n",
		       context->dest_nslots);
	}

	for (unsigned int code = ABS_X; code <= ABS_MAX; code++) {
		static int once = 0;
		bool src_avl, dst_avl;

		src_avl = libevdev_has_event_code(context->source,
						  EV_ABS, code);
		dst_avl = libevdev_has_event_code(context->dest,
						  EV_ABS, code);
		if (src_avl == dst_avl)
			continue;

		if (once == 0) {
			once = 1;
			printf("Mismatching capabilities: \n");
			printf("Capabilities not present on the source device cannot be emulated on the destination\n");
			printf("%23s  Source  Dest\n", "Axis");
		}

		printf("%23s    %s      %s\n",
		       libevdev_event_code_get_name(EV_ABS, code),
		       src_avl ? "✓" : "✗",
		       dst_avl ? "✓" : "✗");
	}

	return true;
}

static void
write_event(const struct emulator *context,
	    const struct input_event *event)
{
	write(context->dest_fd, event, sizeof(*event));
}

static void
forward_key(const struct emulator *context,
	    const struct input_event *event)
{
	switch (event->code) {
		case BTN_TOUCH:
		case BTN_TOOL_FINGER:
		case BTN_TOOL_DOUBLETAP:
		case BTN_TOOL_TRIPLETAP:
		case BTN_TOOL_QUADTAP:
		case BTN_TOOL_QUINTTAP:
			write_event(context, event);
			break;
		default:
			break;
	}
}

static bool
scale(const struct emulator *context,
      const struct input_event *event,
      int *value)
{
	const struct input_absinfo *src, *dst;
	int val = event->value;
	double srange, drange;
	double factor;
	int smin, smax;

	src = libevdev_get_abs_info(context->source, event->code);
	dst = libevdev_get_abs_info(context->dest, event->code);
	if (!dst)
		return false;

	smin = src->minimum;
	smax = src->maximum;
	srange = smax - smin;

	/* Scale into custom range */
	if (context->scale != 0.0) {
		smin = smin + context->scale * srange;
		smax = smax - context->scale * srange;
		srange = smax - smin;
		val = max(min(smax, val), smin);
	}

	drange = dst->maximum - dst->minimum;
	factor = drange/srange;

	*value = (val - smin) * factor + dst->minimum;
	return true;
}

static void
forward_abs(struct emulator *context,
	    const struct input_event *event)
{
	struct input_event ev = *event;
	int value = event->value;
	bool forward = true;
	static int slot_index = -1;
	struct slot *slot;

	if (slot_index == -1)
		slot_index = libevdev_get_current_slot(context->source);

	slot = &context->slots[slot_index];

	switch (event->code) {
		case ABS_X:
		case ABS_Y:
			if (!context->at_least_one_slot_is_active)
				forward = false;
			break;
		case ABS_MT_POSITION_X:
		case ABS_MT_POSITION_Y:
		case ABS_MT_PRESSURE:
		case ABS_PRESSURE:
			if (slot->state != SS_NONE)
				forward = scale(context, event, &value);
			break;
		case ABS_MT_SLOT:
			slot_index = event->value;
			break;
		case ABS_MT_TRACKING_ID:
			if ((size_t)slot_index >= context->dest_nslots) {
				forward = false;
				break;
			}
			if (event->value != -1) {
				context->at_least_one_slot_is_active = true;
				slot->state = SS_BEGIN;
			} else {
				slot->state = SS_END;
			}
			break;
		default:
			return;
	}

	if (!forward)
		return;

	ev.value = value;
	write_event(context, &ev);
}

static void
update_slots(struct emulator *context)
{
	context->at_least_one_slot_is_active = false;

	for (size_t i = 0; i < context->nslots; i++) {
		struct slot *s = &context->slots[i];

		if (s->state == SS_BEGIN)
			s->state = SS_UPDATE;
		else if (s->state == SS_END)
			s->state = SS_NONE;

		if (s->state != SS_NONE)
			context->at_least_one_slot_is_active = true;
	}
}

static void
forward_event(struct emulator *context,
	      const struct input_event *event)
{
	switch (event->type) {
		case EV_KEY:
			forward_key(context, event);
			break;
		case EV_ABS:
			forward_abs(context, event);
			break;
		case EV_SYN:
			update_slots(context);
			write_event(context, event);
			break;
		default:
			break;
	}
}

static void
mainloop(struct emulator *context)
{
	int rc;

	libevdev_grab(context->source, LIBEVDEV_GRAB);

	do {
		struct input_event ev;
		rc = libevdev_next_event(context->source,
					 LIBEVDEV_READ_FLAG_NORMAL|LIBEVDEV_READ_FLAG_BLOCKING, &ev);
		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
			printf("SYN_DROPPED received, giving up\n");
			return;
		} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS)
			forward_event(context, &ev);
	} while (rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
}

int
main(int argc, char **argv)
{
	struct emulator context = {0};

	context.source_fd = -1;
	context.dest_fd = -1;

	parse_args(&context, argc, argv);
	if (!init(&context))
		goto out;

	mainloop(&context);

out:
	close(context.source_fd);
	close(context.dest_fd);
	libevdev_free(context.source);
	libevdev_free(context.dest);
	free(context.slots);

	return 0;
}
