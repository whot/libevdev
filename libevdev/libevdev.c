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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libevdev.h"
#include "libevdev-int.h"

#define MAXEVENTS 64

static inline int
bit_is_set(const unsigned long *array, int bit)
{
    return !!(array[bit / LONG_BITS] & (1LL << (bit % LONG_BITS)));
}

static inline void
set_bit(unsigned long *array, int bit)
{
    array[bit / LONG_BITS] |= (1LL << (bit % LONG_BITS));
}

static inline void
clear_bit(unsigned long *array, int bit)
{
    array[bit / LONG_BITS] &= ~(1LL << (bit % LONG_BITS));
}

static inline void
set_bit_state(unsigned long *array, int bit, int state)
{
	if (state)
		set_bit(array, bit);
	else
		clear_bit(array, bit);
}

static unsigned int
type_to_mask_const(const struct libevdev *dev, unsigned int type, const unsigned long **mask)
{
	unsigned int max;

	switch(type) {
		case EV_ABS:
			*mask = dev->abs_bits;
			max = ABS_MAX;
			break;
		case EV_REL:
			*mask = dev->rel_bits;
			max = REL_MAX;
			break;
		case EV_KEY:
			*mask = dev->key_bits;
			max = KEY_MAX;
			break;
		case EV_LED:
			*mask = dev->led_bits;
			max = LED_MAX;
			break;
		default:
		     return 0;
	}

	return max;
}

static unsigned int
type_to_mask(struct libevdev *dev, unsigned int type, unsigned long **mask)
{
	unsigned int max;

	switch(type) {
		case EV_ABS:
			*mask = dev->abs_bits;
			max = ABS_MAX;
			break;
		case EV_REL:
			*mask = dev->rel_bits;
			max = REL_MAX;
			break;
		case EV_KEY:
			*mask = dev->key_bits;
			max = KEY_MAX;
			break;
		case EV_LED:
			*mask = dev->led_bits;
			max = LED_MAX;
			break;
		default:
		     return 0;
	}

	return max;
}

static int
init_event_queue(struct libevdev *dev)
{
	/* FIXME: count the number of axes, keys, etc. to get a better idea at how many events per
	   EV_SYN we could possibly get. Then multiply that by the actual buffer size we care about */

	const int QUEUE_SIZE = 256;

	dev->queue = calloc(QUEUE_SIZE, sizeof(struct input_event));
	if (!dev->queue)
		return -ENOSPC;

	dev->queue_size = QUEUE_SIZE;
	dev->queue_next = 0;

	return 0;
}

struct libevdev*
libevdev_new(int fd)
{
	struct libevdev *dev;

	dev = calloc(1, sizeof(*dev));
	dev->num_slots = -1;

	if (fd >= 0)
		libevdev_set_fd(dev, fd);
	dev->fd = fd;

	return dev;
}

void
libevdev_free(struct libevdev *dev)
{
	free(dev);
}

int
libevdev_change_fd(struct libevdev *dev, int fd)
{
	if (dev->fd == -1)
		return -1;
	dev->fd = fd;
	return 0;
}

int
libevdev_set_fd(struct libevdev* dev, int fd)
{
	int rc;
	int i;

	if (dev->fd == -1) {
		libevdev_callback_proc cb, scb;
		void *userdata;

		/* these may be set before set_fd */
		cb = dev->callback;
		scb = dev->sync_callback;
		userdata = dev->userdata;

		memset(dev, 0, sizeof(*dev));

		dev->fd = -1;
		dev->callback = cb;
		dev->sync_callback = scb;
		dev->userdata = userdata;
	}

	rc = ioctl(fd, EVIOCGBIT(0, sizeof(dev->bits)), dev->bits);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGNAME(sizeof(dev->name) - 1), dev->name);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGID, &dev->ids);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGPROP(sizeof(dev->props)), dev->props);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGBIT(EV_REL, sizeof(dev->rel_bits)), dev->rel_bits);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(dev->abs_bits)), dev->abs_bits);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGBIT(EV_LED, sizeof(dev->led_bits)), dev->led_bits);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(dev->key_bits)), dev->key_bits);
	if (rc < 0)
		goto out;

	for (i = ABS_X; i <= ABS_MAX; i++) {
		if (bit_is_set(dev->abs_bits, i)) {
			struct input_absinfo abs_info;
			rc = ioctl(fd, EVIOCGABS(i), &dev->abs_info[i]);
			if (rc < 0)
				goto out;

			if (i == ABS_MT_SLOT)
				dev->num_slots = abs_info.maximum + 1; /* FIXME: non-zero min? */

		}
	}

	rc = init_event_queue(dev);
	if (rc < 0)
		return -rc;

	/* not copying key state because we won't know when we'll start to
	 * use this fd and key's are likely to change state by then.
	 * Same with the valuators, really, but they may not change.
	 */

	dev->fd = fd;

out:
	return rc ? -errno : 0;
}

int
libevdev_get_fd(const struct libevdev* dev)
{
	return dev->fd;
}

int
libevdev_set_callbacks(struct libevdev *dev,
		       libevdev_callback_proc callback,
		       libevdev_callback_proc sync_callback,
		       void *userdata)
{
	dev->callback = callback;
	dev->sync_callback = sync_callback;
	dev->userdata = userdata;

	return 0;
}

static inline void
init_event(struct input_event *ev, int type, int code, int value)
{
	ev->time.tv_sec = 0; /* FIXME: blah! */
	ev->time.tv_usec = 0; /* FIXME: blah! */
	ev->type = type;
	ev->code = code;
	ev->value = value;
}

static int
sync_key_state(struct libevdev *dev)
{
	int rc;
	int i;
	unsigned long keystate[NLONGS(KEY_MAX)];
	struct input_event ev;

	rc = ioctl(dev->fd, EVIOCGKEY(sizeof(keystate)), keystate);
	if (rc < 0)
		goto out;

	for (i = 0; i < KEY_MAX; i++) {
		int old, new;
		old = bit_is_set(dev->key_values, i);
		new = bit_is_set(keystate, i);
		if (old ^ new) {
			init_event(&ev, EV_KEY, i, new ? 1 : 0);
			dev->sync_callback(dev, &ev, dev->userdata);
		}
		set_bit_state(dev->key_values, i, new);
	}

	rc = 0;
out:
	return rc ? -errno : 0;
}

static int
sync_abs_state(struct libevdev *dev)
{
	int rc;
	int i;
	struct input_event ev;

	for (i = ABS_X; i <= ABS_MAX; i++) {
		if (i >= ABS_MT_MIN && i <= ABS_MT_MAX)
			continue;

		if (bit_is_set(dev->abs_bits, i)) {
			struct input_absinfo abs_info;
			rc = ioctl(dev->fd, EVIOCGABS(i), &abs_info);
			if (rc < 0)
				goto out;

			if (dev->abs_info[i].value != abs_info.value) {
				init_event(&ev, EV_ABS, i, abs_info.value);
				dev->sync_callback(dev, &ev, dev->userdata);
				dev->abs_info[i].value = abs_info.value;
			}
		}
	}

	rc = 0;
out:
	return rc ? -errno : 0;
}

static int
sync_mt_state(struct libevdev *dev)
{
	int rc;
	int i;
	struct mt_state {
		int code;
		int val[MAX_SLOTS];
	} mt_state[ABS_MT_CNT];
	struct input_event ev;

	for (i = ABS_MT_MIN; i < ABS_MT_MAX; i++) {
		int idx;
		if (i == ABS_MT_SLOT)
			continue;

		idx = i - ABS_MT_MIN;
		mt_state[idx].code = i;
		rc = ioctl(dev->fd, EVIOCGMTSLOTS(sizeof(struct mt_state)), &mt_state[idx]);
		if (rc < 0)
			goto out;
	}

	for (i = 0; i < dev->num_slots; i++) {
		int j;
		init_event(&ev, EV_ABS, ABS_MT_SLOT, i);
		dev->sync_callback(dev, &ev, dev->userdata);
		for (j = ABS_MT_MIN; j < ABS_MT_MAX; j++) {
			int jdx = j - ABS_MT_MIN;

			if (dev->mt_slot_vals[i][jdx] != mt_state[jdx].val[i]) {
				init_event(&ev, EV_ABS, j, mt_state[jdx].val[i]);
				dev->sync_callback(dev, &ev, dev->userdata);
				dev->mt_slot_vals[i][jdx] = mt_state[jdx].val[i];
			}
		}
	}

	rc = 0;
out:
	return rc ? -errno : 0;
}

static int
sync_state(struct libevdev *dev, unsigned int flags)
{
	int rc = 0;
	struct input_event ev;

	if (libevdev_has_event_type(dev, EV_KEY))
		rc = sync_key_state(dev); /* FIXME: handle ER_SINGLE */
	if (rc == 0 && libevdev_has_event_type(dev, EV_ABS))
		rc = sync_abs_state(dev);
	if (rc == 0 && libevdev_has_event_code(dev, EV_ABS, ABS_MT_SLOT))
		rc = sync_mt_state(dev);

	init_event(&ev, EV_SYN, SYN_REPORT, 0);
	dev->sync_callback(dev, &ev, dev->userdata);

	dev->need_sync = 0;

	return rc;
}

static int
update_key_state(struct libevdev *dev, const struct input_event *e)
{
	if (!libevdev_has_event_type(dev, EV_KEY))
		return 1;

	if (e->code > KEY_MAX)
		return 1;

	if (e->value == 0)
		clear_bit(dev->key_values, e->code);
	else
		set_bit(dev->key_values, e->code);

	return 0;
}

static int
update_abs_state(struct libevdev *dev, const struct input_event *e)
{
	if (!libevdev_has_event_type(dev, EV_ABS))
		return 1;

	if (e->code > ABS_MAX)
		return 1;

	dev->abs_info[e->code].value = e->value;

	return 0;
}

static int
update_state(struct libevdev *dev, const struct input_event *e)
{
	int rc = 0;

	switch(e->type) {
		case EV_SYN:
		case EV_REL:
			break;
		case EV_KEY:
			rc = update_key_state(dev, e);
			break;
		case EV_ABS:
			rc = update_abs_state(dev, e);
			break;
	}

	return rc;
}

static int
read_more_events(struct libevdev *dev)
{
	int free_elem;
	int len;

	free_elem = dev->queue_size - dev->queue_next - 1;
	if (free_elem <= 0)
		return 0;

	len = read(dev->fd, &dev->queue[dev->queue_next], free_elem * sizeof(struct input_event));
	if (len < 0) {
		if (errno != EAGAIN || dev->queue_next == 0)
			return -errno;
	} else if (len > 0 && len % sizeof(struct input_event) != 0)
		return -EINVAL;

	dev->queue_next += len/sizeof(struct input_event);

	return 0;
}

int libevdev_read_events(struct libevdev *dev,
			 unsigned int flags)
{
	int nevents, processed;
	int i;
	int rc = 0;

	if (dev->fd < 0)
		return -ENODEV;

	if (flags & ER_SYNC) {
		if (!dev->need_sync)
			return 0;
		return sync_state(dev, flags);
	}

	/* Always read in some more events. Best case this smoothes over a potential SYN_DROPPED,
	   worst case we don't read fast enough and end up with SYN_DROPPED anyway */
	rc = read_more_events(dev);
	if (rc < 0)
		goto out;

	nevents = dev->queue_next;
	for (processed = 0; processed < nevents; processed++) {
		struct input_event *e = &dev->queue[processed];

		update_state(dev, e);

		if (e->type == EV_SYN && e->code == SYN_DROPPED) {
			dev->need_sync = 0;
			rc = 1;
			break;
		} else if (libevdev_has_event_code(dev, e->type, e->code)) {
			if (dev->callback(dev, e, dev->userdata) != 0) {
				rc = -ECANCELED;
				break;
			}
		}
	}

	for (i = 0; i < nevents - processed; i++) {
		dev->queue[i] = dev->queue[i + processed];
	}

	dev->queue_next -= processed;
out:
	return rc;
}

const char *
libevdev_get_name(const struct libevdev *dev)
{
	return dev->name;
}

int libevdev_get_pid(const struct libevdev *dev)
{
	return dev->ids.product;
}

int libevdev_get_vid(const struct libevdev *dev)
{
	return dev->ids.vendor;
}

int libevdev_get_bustype(const struct libevdev *dev)
{
	return dev->ids.bustype;
}

int
libevdev_has_property(const struct libevdev *dev, unsigned int prop)
{
	return (prop <= INPUT_PROP_MAX) && bit_is_set(dev->props, prop);
}

int
libevdev_has_event_type(const struct libevdev *dev, unsigned int type)
{
	return (type <= EV_MAX) && bit_is_set(dev->bits, type);
}

int
libevdev_has_event_code(const struct libevdev *dev, unsigned int type, unsigned int code)
{
	const unsigned long *mask;
	unsigned int max;

	if (!libevdev_has_event_type(dev, type))
		return 0;

	if (type == EV_SYN)
		return 1;

	max = type_to_mask_const(dev, type, &mask);

	if (code > max)
		return 0;

	return bit_is_set(mask, code);
}

int
libevdev_get_event_value(const struct libevdev *dev, unsigned int type, unsigned int code)
{
	int value;

	if (!libevdev_has_event_type(dev, type) || !libevdev_has_event_code(dev, type, code))
		return 0;

	switch (type) {
		case EV_ABS: value = dev->abs_info[code].value; break;
		case EV_KEY: value = bit_is_set(dev->key_values, code); break;
		default:
			value = 0;
			break;
	}

	return value;
}

int
libevdev_fetch_event_value(const struct libevdev *dev, unsigned int type, unsigned int code, int *value)
{
	if (libevdev_has_event_type(dev, type) &&
	    libevdev_has_event_code(dev, type, code)) {
		*value = libevdev_get_event_value(dev, type, code);
		return 1;
	} else
		return 0;
}

int
libevdev_get_slot_value(const struct libevdev *dev, unsigned int slot, unsigned int code)
{
	if (!libevdev_has_event_type(dev, EV_ABS) || !libevdev_has_event_code(dev, EV_ABS, code))
		return 0;

	if (slot >= dev->num_slots || slot >= MAX_SLOTS)
		return 0;

	if (code > ABS_MT_MAX || code < ABS_MT_MIN)
		return 0;

	return dev->mt_slot_vals[slot][code - ABS_MT_MIN];
}

int
libevdev_fetch_slot_value(const struct libevdev *dev, unsigned int slot, unsigned int code, int *value)
{
	if (libevdev_has_event_type(dev, EV_ABS) &&
	    libevdev_has_event_code(dev, EV_ABS, code) &&
	    slot < dev->num_slots && slot < MAX_SLOTS) {
		*value = libevdev_get_slot_value(dev, slot, code);
		return 1;
	} else
		return 0;
}

int
libevdev_get_num_slots(const struct libevdev *dev)
{
	return dev->num_slots;
}

const struct input_absinfo*
libevdev_get_abs_info(const struct libevdev *dev, unsigned int code)
{
	if (!libevdev_has_event_type(dev, EV_ABS) ||
	    !libevdev_has_event_code(dev, EV_ABS, code))
		return NULL;

	return &dev->abs_info[code];
}

int
libevdev_get_abs_min(const struct libevdev *dev, unsigned int code)
{
	const struct input_absinfo *absinfo = libevdev_get_abs_info(dev, code);

	return absinfo ? absinfo->minimum : 0;
}

int
libevdev_get_abs_max(const struct libevdev *dev, unsigned int code)
{
	const struct input_absinfo *absinfo = libevdev_get_abs_info(dev, code);

	return absinfo ? absinfo->maximum : 0;
}

int
libevdev_get_abs_fuzz(const struct libevdev *dev, unsigned int code)
{
	const struct input_absinfo *absinfo = libevdev_get_abs_info(dev, code);

	return absinfo ? absinfo->fuzz : 0;
}

int
libevdev_get_abs_flat(const struct libevdev *dev, unsigned int code)
{
	const struct input_absinfo *absinfo = libevdev_get_abs_info(dev, code);

	return absinfo ? absinfo->flat : 0;
}

int
libevdev_get_abs_resolution(const struct libevdev *dev, unsigned int code)
{
	const struct input_absinfo *absinfo = libevdev_get_abs_info(dev, code);

	return absinfo ? absinfo->resolution : 0;
}

int
libevdev_enable_event_type(struct libevdev *dev, unsigned int type)
{
	if (type > EV_MAX)
		return 1;

	set_bit(dev->bits, type);

	/* FIXME: pass through to kernel */

	return 0;
}

int
libevdev_disable_event_type(struct libevdev *dev, unsigned int type)
{
	if (type > EV_MAX)
		return 1;

	clear_bit(dev->bits, type);

	/* FIXME: pass through to kernel */

	return 0;
}

int
libevdev_enable_event_code(struct libevdev *dev, unsigned int type,
			   unsigned int code, const void *data)
{
	unsigned int max;
	unsigned long *mask;

	if (libevdev_enable_event_type(dev, type))
		return 1;

	max = type_to_mask(dev, type, &mask);

	if (code > max)
		return 1;

	set_bit(mask, code);

	if (type == EV_ABS) {
		const struct input_absinfo *abs = data;
		dev->abs_info[code] = *abs;
	}

	/* FIXME: pass through to kernel */

	return 0;
}

int
libevdev_disable_event_code(struct libevdev *dev, unsigned int type, unsigned int code)
{
	unsigned int max;
	unsigned long *mask;

	if (type > EV_MAX)
		return 1;

	max = type_to_mask(dev, type, &mask);

	if (code > max)
		return 1;

	clear_bit(mask, code);

	/* FIXME: pass through to kernel */

	return 0;
}

int
libevdev_kernel_set_abs_value(struct libevdev *dev, unsigned int code, const struct input_absinfo *abs)
{
	int rc;

	if (code > ABS_MAX)
		return -EINVAL;

	rc = ioctl(dev->fd, EVIOCSABS(code), *abs);
	if (rc < 0)
		rc = -errno;
	else
		rc = libevdev_enable_event_code(dev, EV_ABS, code, abs);

	return rc;
}

int
libevdev_grab(struct libevdev *dev, int grab)
{
	int rc = 0;

	if (grab && !dev->grabbed)
		rc = ioctl(dev->fd, EVIOCGRAB, (void *)1);
	else if (!grab && dev->grabbed)
		rc = ioctl(dev->fd, EVIOCGRAB, (void *)0);

	if (rc == 0)
		dev->grabbed = grab;

	return rc < 0 ? -errno : 0;
}
