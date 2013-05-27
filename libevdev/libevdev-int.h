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

#ifndef libevdev_INT_H
#define libevdev_INT_H

#include <config.h>
#include "libevdev.h"

#define LONG_BITS (sizeof(long) * 8)
#define NLONGS(x) (((x) + LONG_BITS - 1) / LONG_BITS)
#define ARRAY_LENGTH(a) (sizeof(a) / (sizeof((a)[0])))
#define MAX_NAME 256
#define MAX_SLOTS 32
#define ABS_MT_MIN ABS_MT_SLOT
#define ABS_MT_MAX ABS_MT_TOOL_Y
#define ABS_MT_CNT (ABS_MT_MAX - ABS_MT_MIN + 1)

struct libevdev {
	int fd;
	libevdev_callback_proc callback;
	libevdev_callback_proc sync_callback;
	void *userdata;

	char name[MAX_NAME];
	struct input_id ids;
	unsigned long bits[NLONGS(EV_CNT)];
	unsigned long props[NLONGS(INPUT_PROP_CNT)];
	unsigned long key_bits[NLONGS(KEY_CNT)];
	unsigned long rel_bits[NLONGS(REL_CNT)];
	unsigned long abs_bits[NLONGS(ABS_CNT)];
	unsigned long led_bits[NLONGS(LED_CNT)];
	unsigned long key_values[NLONGS(KEY_CNT)];
	struct input_absinfo abs_info[ABS_CNT];
	unsigned int mt_slot_vals[MAX_SLOTS][ABS_MT_CNT];
	int num_slots; /**< valid slots in mt_slot_vals */

	int need_sync;
	int grabbed;

	struct input_event *queue;
	size_t queue_size; /**< size of queue in elements */
	size_t queue_next; /**< next event index */
};

#endif

