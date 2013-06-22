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
#include <stdarg.h>
#include <linux/uinput.h>

#include "libevdev.h"
#include "libevdev-int.h"
#include "libevdev-util.h"
#include "event-names.h"

#define MAXEVENTS 64

static int
init_event_queue(struct libevdev *dev)
{
	/* FIXME: count the number of axes, keys, etc. to get a better idea at how many events per
	   EV_SYN we could possibly get. Then multiply that by the actual buffer size we care about */

	const int QUEUE_SIZE = 256;

	return queue_alloc(dev, QUEUE_SIZE);
}

static void
_libevdev_log(struct libevdev *dev, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	dev->log(format, args);
	va_end(args);
}

static void
libevdev_noop_log_func(const char *format, va_list args)
{
}

struct libevdev*
libevdev_new(void)
{
	struct libevdev *dev;

	dev = calloc(1, sizeof(*dev));
	if (!dev)
		return NULL;
	dev->fd = -1;
	dev->num_slots = -1;
	dev->current_slot = -1;
	dev->log = libevdev_noop_log_func;

	return dev;
}

int
libevdev_new_from_fd(int fd, struct libevdev **dev)
{
	struct libevdev *d;
	int rc;

	d = libevdev_new();
	if (!d)
		return -ENOSPC;

	rc = libevdev_set_fd(d, fd);
	if (rc < 0)
		libevdev_free(d);
	else
		*dev = d;
	return rc;
}

void
libevdev_free(struct libevdev *dev)
{
	if (!dev)
		return;

	free(dev->name);
	free(dev->phys);
	free(dev->uniq);
	queue_free(dev);
	free(dev);
}

void
libevdev_set_log_handler(struct libevdev *dev, libevdev_log_func_t logfunc)
{
	if (dev == NULL)
		return;

	dev->log = logfunc ? logfunc : libevdev_noop_log_func;
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
	char buf[256];

	if (dev->fd != -1)
		return -EBADF;

	rc = ioctl(fd, EVIOCGBIT(0, sizeof(dev->bits)), dev->bits);
	if (rc < 0)
		goto out;

	memset(buf, 0, sizeof(buf));
	rc = ioctl(fd, EVIOCGNAME(sizeof(buf) - 1), buf);
	if (rc < 0)
		goto out;

	dev->name = calloc(strlen(buf) + 1, sizeof(char));
	if (!dev->name) {
		errno = ENOSPC;
		goto out;
	}
	strcpy(dev->name, buf);

	memset(buf, 0, sizeof(buf));
	rc = ioctl(fd, EVIOCGPHYS(sizeof(buf) - 1), buf);
	if (rc < 0) {
		/* uinput has no phys */
		if (errno != ENOENT)
			goto out;
	} else {
		dev->phys = calloc(strlen(buf) + 1, sizeof(char));
		if (!dev->phys) {
			errno = ENOSPC;
			goto out;
		}
		strcpy(dev->phys, buf);
	}

	memset(buf, 0, sizeof(buf));
	rc = ioctl(fd, EVIOCGUNIQ(sizeof(buf) - 1), buf);
	if (rc < 0) {
		if (errno != ENOENT)
			goto out;
	} else  {
		dev->uniq = calloc(strlen(buf) + 1, sizeof(char));
		if (!dev->uniq) {
			errno = ENOSPC;
			goto out;
		}
		strcpy(dev->uniq, buf);
	}

	rc = ioctl(fd, EVIOCGID, &dev->ids);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGVERSION, &dev->driver_version);
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

	rc = ioctl(fd, EVIOCGBIT(EV_SW, sizeof(dev->sw_bits)), dev->sw_bits);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGBIT(EV_MSC, sizeof(dev->msc_bits)), dev->msc_bits);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGBIT(EV_FF, sizeof(dev->ff_bits)), dev->ff_bits);
	if (rc < 0)
		goto out;

	rc = ioctl(fd, EVIOCGBIT(EV_SND, sizeof(dev->snd_bits)), dev->snd_bits);
	if (rc < 0)
		goto out;

	/* rep is a special case, always set it to 1 for both values if EV_REP is set */
	if (bit_is_set(dev->bits, EV_REP)) {
		for (i = 0; i < REP_MAX; i++)
			set_bit(dev->rep_bits, i);
		rc = ioctl(fd, EVIOCGREP, dev->rep_values);
		if (rc < 0)
			goto out;
	}

	for (i = ABS_X; i <= ABS_MAX; i++) {
		if (bit_is_set(dev->abs_bits, i)) {
			struct input_absinfo abs_info;
			rc = ioctl(fd, EVIOCGABS(i), &abs_info);
			if (rc < 0)
				goto out;

			dev->abs_info[i] = abs_info;
			if (i == ABS_MT_SLOT) {
				dev->num_slots = abs_info.maximum + 1;
				dev->current_slot = abs_info.value;
			}

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

static inline void
init_event(struct libevdev *dev, struct input_event *ev, int type, int code, int value)
{
	ev->time = dev->last_event_time;
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

	rc = ioctl(dev->fd, EVIOCGKEY(sizeof(keystate)), keystate);
	if (rc < 0)
		goto out;

	for (i = 0; i < KEY_MAX; i++) {
		int old, new;
		old = bit_is_set(dev->key_values, i);
		new = bit_is_set(keystate, i);
		if (old ^ new) {
			struct input_event *ev = queue_push(dev);
			init_event(dev, ev, EV_KEY, i, new ? 1 : 0);
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

	for (i = ABS_X; i <= ABS_MAX; i++) {
		struct input_absinfo abs_info;

		if (i >= ABS_MT_MIN && i <= ABS_MT_MAX)
			continue;

		if (!bit_is_set(dev->abs_bits, i))
			continue;

		rc = ioctl(dev->fd, EVIOCGABS(i), &abs_info);
		if (rc < 0)
			goto out;

		if (dev->abs_info[i].value != abs_info.value) {
			struct input_event *ev = queue_push(dev);

			init_event(dev, ev, EV_ABS, i, abs_info.value);
			dev->abs_info[i].value = abs_info.value;
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
		struct input_event *ev;

		ev = queue_push(dev);
		init_event(dev, ev, EV_ABS, ABS_MT_SLOT, i);
		for (j = ABS_MT_MIN; j < ABS_MT_MAX; j++) {
			int jdx = j - ABS_MT_MIN;

			if (j == ABS_MT_SLOT)
				continue;

			if (dev->mt_slot_vals[i][jdx] == mt_state[jdx].val[i])
				continue;

			ev = queue_push(dev);
			init_event(dev, ev, EV_ABS, j, mt_state[jdx].val[i]);
			dev->mt_slot_vals[i][jdx] = mt_state[jdx].val[i];
		}
	}

	rc = 0;
out:
	return rc ? -errno : 0;
}

static int
sync_state(struct libevdev *dev)
{
	int i;
	int rc = 0;
	struct input_event *ev;

	/* FIXME: if we have events in the queue after the SYN_DROPPED (which was
	   queue[0]) we need to shift this backwards. Except that chances are that the
	   queue may be either full or too full to prepend all the events needed for
	   syncing.

	   so we search for the last sync event in the queue and drop everything before
	   including that event and rely on the kernel to tell us the right value for that
	   bitfield during the sync process.
	 */

	for (i = queue_num_elements(dev) - 1; i >= 0; i--) {
		struct input_event e;
		queue_peek(dev, i, &e);
		if (e.type == EV_SYN)
			break;
	}

	if (i > 0)
		queue_shift_multiple(dev, i + 1, NULL);

	if (libevdev_has_event_type(dev, EV_KEY))
		rc = sync_key_state(dev);
	if (rc == 0 && libevdev_has_event_type(dev, EV_ABS))
		rc = sync_abs_state(dev);
	if (rc == 0 && libevdev_has_event_code(dev, EV_ABS, ABS_MT_SLOT))
		rc = sync_mt_state(dev);

	ev = queue_push(dev);
	init_event(dev, ev, EV_SYN, SYN_REPORT, 0);

	dev->queue_nsync = queue_num_elements(dev);
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
update_mt_state(struct libevdev *dev, const struct input_event *e)
{
	if (e->code == ABS_MT_SLOT) {
		dev->current_slot = e->value;
		return 0;
	} else if (dev->current_slot == -1)
		return 1;

	dev->mt_slot_vals[dev->current_slot][e->code - ABS_MT_MIN] = e->value;

	return 0;
}

static int
update_abs_state(struct libevdev *dev, const struct input_event *e)
{
	if (!libevdev_has_event_type(dev, EV_ABS))
		return 1;

	if (e->code > ABS_MAX)
		return 1;

	if (e->code >= ABS_MT_MIN && e->code <= ABS_MT_MAX)
		return update_mt_state(dev, e);

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

	dev->last_event_time = e->time;

	return rc;
}

static int
read_more_events(struct libevdev *dev)
{
	int free_elem;
	int len;
	struct input_event *next;

	free_elem = queue_num_free_elements(dev);
	if (free_elem <= 0)
		return 0;

	next = queue_next_element(dev);
	len = read(dev->fd, next, free_elem * sizeof(struct input_event));
	if (len < 0) {
		return -errno;
	} else if (len > 0 && len % sizeof(struct input_event) != 0)
		return -EINVAL;
	else if (len > 0) {
		int nev = len/sizeof(struct input_event);
		queue_set_num_elements(dev, queue_num_elements(dev) + nev);
	}

	return 0;
}

int libevdev_next_event(struct libevdev *dev, unsigned int flags, struct input_event *ev)
{
	int rc = 0;

	if (dev->fd < 0)
		return -ENODEV;

	if (flags & LIBEVDEV_READ_SYNC) {
		if (!dev->need_sync && dev->queue_nsync == 0)
			return -EAGAIN;
		else if (dev->need_sync) {
			rc = sync_state(dev);
			if (rc != 0)
				return rc;
		}
	} else if (dev->need_sync) {
		/* FIXME: still need to call update_state for all events
		 * here, otherwise the library has the wrong view of the
		 * device too */
		queue_shift_multiple(dev, dev->queue_nsync, NULL);
	}

	/* FIXME: check for O_NONBLOCK and if not set, skip if we have an
	 * event in the queue from the previous read.
	 */

	/* Always read in some more events. Best case this smoothes over a potential SYN_DROPPED,
	   worst case we don't read fast enough and end up with SYN_DROPPED anyway */
	rc = read_more_events(dev);
	if (rc < 0 && rc != -EAGAIN)
		goto out;

	if (queue_shift(dev, ev) != 0)
		return -EAGAIN;

	update_state(dev, ev);

	rc = 0;
	if (ev->type == EV_SYN && ev->code == SYN_DROPPED) {
		dev->need_sync = 1;
		rc = 1;
	}

	if (flags & LIBEVDEV_READ_SYNC && dev->queue_nsync > 0) {
		dev->queue_nsync--;
		rc = 1;
	}

out:
	return rc;
}

const char *
libevdev_get_name(const struct libevdev *dev)
{
	return dev->name;
}

const char *
libevdev_get_phys(const struct libevdev *dev)
{
	return dev->phys;
}

const char *
libevdev_get_uniq(const struct libevdev *dev)
{
	return dev->uniq;
}

int libevdev_get_product_id(const struct libevdev *dev)
{
	return dev->ids.product;
}

int libevdev_get_vendor_id(const struct libevdev *dev)
{
	return dev->ids.vendor;
}

int libevdev_get_bustype(const struct libevdev *dev)
{
	return dev->ids.bustype;
}

int libevdev_get_version(const struct libevdev *dev)
{
	return dev->ids.version;
}

int libevdev_get_driver_version(const struct libevdev *dev)
{
	return dev->driver_version;
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
	int max;

	if (!libevdev_has_event_type(dev, type))
		return 0;

	if (type == EV_SYN)
		return 1;

	max = type_to_mask_const(dev, type, &mask);

	if (max == -1 || code > max)
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

int
libevdev_get_current_slot(const struct libevdev *dev)
{
	return dev->current_slot;
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

	return 0;
}

int
libevdev_disable_event_type(struct libevdev *dev, unsigned int type)
{
	if (type > EV_MAX)
		return 1;

	clear_bit(dev->bits, type);

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

	return 0;
}

int
libevdev_kernel_enable_event_type(struct libevdev *dev, unsigned int type)
{
	int rc;

	if (type > EV_MAX)
		return -1;

	rc = ioctl(dev->fd, UI_SET_EVBIT, type);
	if (rc != -1)
		libevdev_enable_event_type(dev, type);

	return (rc != -1) ? 0 : -errno;
}

int
libevdev_kernel_enable_event_code(struct libevdev *dev, unsigned int type, unsigned int code)
{
	int rc;
	int uinput_bit;
	int max;
	const unsigned long *mask;

	rc = libevdev_kernel_enable_event_type(dev, type);
	if (rc != 0)
		return rc;

	max = type_to_mask_const(dev, type, &mask);
	if (max == -1 || code > max)
		return -EINVAL;

	switch(type) {
		case EV_KEY: uinput_bit = UI_SET_KEYBIT; break;
		case EV_REL: uinput_bit = UI_SET_RELBIT; break;
		case EV_ABS: uinput_bit = UI_SET_ABSBIT; break;
		case EV_MSC: uinput_bit = UI_SET_MSCBIT; break;
		case EV_LED: uinput_bit = UI_SET_LEDBIT; break;
		case EV_SND: uinput_bit = UI_SET_SNDBIT; break;
		case EV_FF: uinput_bit = UI_SET_FFBIT; break;
		case EV_SW: uinput_bit = UI_SET_SWBIT; break;
	}

	rc = ioctl(dev->fd, uinput_bit, type);
	if (rc != -1)
		libevdev_enable_event_type(dev, type);

	return (rc != -1) ? 0 : -errno;
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

	if (grab != LIBEVDEV_GRAB && grab != LIBEVDEV_UNGRAB)
		return -EINVAL;

	if (grab == dev->grabbed)
		return 0;

	if (grab == LIBEVDEV_GRAB)
		rc = ioctl(dev->fd, EVIOCGRAB, (void *)1);
	else if (grab == LIBEVDEV_UNGRAB)
		rc = ioctl(dev->fd, EVIOCGRAB, (void *)0);

	if (rc == 0)
		dev->grabbed = grab;

	return rc < 0 ? -errno : 0;
}

const char*
libevdev_get_event_type_name(unsigned int type)
{
	if (type > EV_MAX)
		return NULL;

	return ev_map[type];
}

const char*
libevdev_get_event_code_name(unsigned int type, unsigned int code)
{
	if (type > EV_MAX)
		return NULL;

	if (code > ev_max[type])
		return NULL;

	return event_type_map[type][code];
}

const char*
libevdev_get_input_prop_name(unsigned int prop)
{
	if (prop > INPUT_PROP_MAX)
		return NULL;

	return input_prop_map[prop];
}

int
libevdev_get_event_type_max(unsigned int type)
{
	if (type > EV_MAX)
		return -1;

	return ev_max[type];
}

int
libevdev_get_repeat(struct libevdev *dev, int *delay, int *period)
{
	if (!libevdev_has_event_type(dev, EV_REP))
		return -1;

	if (delay != NULL)
		*delay = dev->rep_values[REP_DELAY];
	if (period != NULL)
		*period = dev->rep_values[REP_PERIOD];

	return 0;
}
