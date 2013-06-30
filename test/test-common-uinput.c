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

#define _GNU_SOURCE
#include <config.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <linux/uinput.h>
#include <sys/inotify.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-int.h>
#include <libevdev/libevdev-util.h>

#include "test-common-uinput.h"

#define SYS_INPUT_DIR "/sys/class/input"
#define DEV_INPUT_DIR "/dev/input/"

struct uinput_device
{
	struct libevdev d; /* lazy, it has all the accessors */
	char *devnode; /* path after creation */
	int dev_fd; /* open fd to the devnode */
};

struct uinput_device*
uinput_device_new(const char *name)
{
	struct uinput_device *dev;

	dev = calloc(1, sizeof(*dev));
	if (!dev)
		return NULL;

	dev->d.fd = -1;
	dev->dev_fd = -1;

	if (name)
		dev->d.name = strdup(name);

	return dev;
}

int
uinput_device_new_with_events_v(struct uinput_device **d, const char *name, const struct input_id *id, va_list args)
{
	int rc;
	struct uinput_device *dev;

	dev = uinput_device_new(name);
	if (!dev)
		return -ENOSPC;
	if (id != DEFAULT_IDS)
		uinput_device_set_ids(dev, id);

	rc = uinput_device_set_event_bits_v(dev, args);

	if (rc == 0)
		rc = uinput_device_create(dev);

	if (rc != 0) {
		uinput_device_free(dev);
		dev = NULL;
	} else
		*d = dev;

	return rc;
}

int
uinput_device_new_with_events(struct uinput_device **d, const char *name, const struct input_id *id, ...)
{
	int rc;
	va_list args;

	va_start(args, id);
	rc = uinput_device_new_with_events_v(d, name, id, args);
	va_end(args);

	return rc;
}

void
uinput_device_free(struct uinput_device *dev)
{
	if (!dev)
		return;

	if (dev->d.fd != -1) {
		ioctl(dev->d.fd, UI_DEV_DESTROY, NULL);
		close(dev->d.fd);
	}
	if (dev->dev_fd != -1)
		close(dev->dev_fd);
	free(dev->d.name);
	free(dev->devnode);
	free(dev);
}

int
uinput_device_get_fd(const struct uinput_device *dev)
{
	return dev->dev_fd;
}


static char*
wait_for_inotify(int fd)
{
	char *devnode = NULL;
	int found = 0;
	char buf[1024];
	size_t bufidx = 0;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;

	while (!found && poll(&pfd, 1, 2000) > 0) {
		struct inotify_event *e;
		ssize_t r;

		r = read(fd, buf + bufidx, sizeof(buf) - bufidx);
		if (r == -1 && errno != EAGAIN)
			return NULL;

		bufidx += r;

		e = (struct inotify_event*)buf;

		while (bufidx > sizeof(*e) && bufidx >= sizeof(*e) + e->len) {
			if (strncmp(e->name, "event", 5) == 0) {
				asprintf(&devnode, "%s%s", DEV_INPUT_DIR, e->name);
				found = 1;
				break;
			}

			/* this packet didn't contain what we're looking for */
			int len = sizeof(*e) + e->len;
			memmove(buf, buf + len, bufidx - len);
			bufidx -= len;
		}
	}

	return devnode;
}

static int
inotify_setup()
{
	int ifd = inotify_init1(IN_NONBLOCK);
	if (ifd == -1 || inotify_add_watch(ifd, DEV_INPUT_DIR, IN_CREATE) == -1) {
		if (ifd != -1)
			close(ifd);
		ifd = -1;
	}

	return ifd;
}

int
uinput_device_create(struct uinput_device* d)
{
	int i;
	struct uinput_user_dev dev;
	int rc;
	int fd;
	int ifd = -1; /* inotify fd */

	fd = open("/dev/uinput", O_RDWR);
	if (fd < 0)
		goto error;

	d->d.fd = fd;

	memset(&dev, 0, sizeof(dev));
	if (d->d.name)
		strncpy(dev.name, d->d.name, UINPUT_MAX_NAME_SIZE - 1);
	dev.id = d->d.ids;

	for (i = 0; i < EV_MAX; i++) {
		int j;
		int max;
		int uinput_bit;
		const unsigned long *mask;

		if (!bit_is_set(d->d.bits, i))
			continue;

		rc = ioctl(fd, UI_SET_EVBIT, i);
		if (rc == -1)
			goto error;

		max = type_to_mask_const(&d->d, i, &mask);
		if (max == -1)
			continue;

		switch(i) {
			case EV_KEY: uinput_bit = UI_SET_KEYBIT; break;
			case EV_REL: uinput_bit = UI_SET_RELBIT; break;
			case EV_ABS: uinput_bit = UI_SET_ABSBIT; break;
			case EV_MSC: uinput_bit = UI_SET_MSCBIT; break;
			case EV_LED: uinput_bit = UI_SET_LEDBIT; break;
			case EV_SND: uinput_bit = UI_SET_SNDBIT; break;
			case EV_FF: uinput_bit = UI_SET_FFBIT; break;
			case EV_SW: uinput_bit = UI_SET_SWBIT; break;
			default:
				    errno = EINVAL;
				    goto error;
		}


		for (j = 0; j < max; j++) {
			if (!bit_is_set(mask, j))
				continue;

			rc = ioctl(fd, uinput_bit, j);
			if (rc == -1)
				goto error;
		}

	}

	rc = write(fd, &dev, sizeof(dev));
	if (rc < 0)
		goto error;
	else if (rc < sizeof(dev)) {
		errno = EINVAL;
		goto error;
	}

	ifd = inotify_setup();

	rc = ioctl(fd, UI_DEV_CREATE, NULL);
	if (rc == -1)
		goto error;

	d->devnode = wait_for_inotify(ifd);
	if (d->devnode == NULL)
		goto error;

	d->dev_fd = open(d->devnode, O_RDWR);
	if (d->dev_fd == -1)
		goto error;

	return 0;

error:
	if (ifd != -1)
		close(ifd);
	if (d->d.fd != -1)
		close(fd);
	return -errno;

}

int uinput_device_set_name(struct uinput_device *dev, const char *name)
{
	if (dev->d.name)
		free(dev->d.name);
	if (name)
		dev->d.name = strdup(name);
	return 0;
}

int uinput_device_set_ids(struct uinput_device *dev, const struct input_id *ids)
{
	dev->d.ids = *ids;
	return 0;
}

int
uinput_device_set_bit(struct uinput_device* dev, unsigned int bit)
{
	if (!dev)
		return -EINVAL;

	if (bit > EV_MAX)
		return -EINVAL;

	set_bit(dev->d.bits, bit);
	return 0;
}

int
uinput_device_set_event_bit(struct uinput_device* dev, unsigned int type, unsigned int code)
{
	int max;
	unsigned long *mask;

	if (uinput_device_set_bit(dev, type) < 0)
		return -EINVAL;

	if (type == EV_SYN)
		return 0;

	max = type_to_mask(&dev->d, type, &mask);
	if (max == -1 || code > max)
		return -EINVAL;

	set_bit(mask, code);
	return 0;
}

int
uinput_device_set_event_bits_v(struct uinput_device *dev, va_list args)
{
	int type, code;
	int rc = 0;

	do {
		type = va_arg(args, int);
		if (type == -1)
			break;
		code = va_arg(args, int);
		if (code == -1)
			break;
		rc = uinput_device_set_event_bit(dev, type, code);
	} while (rc == 0);

	return rc;
}

int
uinput_device_set_event_bits(struct uinput_device *dev, ...)
{
	int rc;
	va_list args;
	va_start(args, dev);
	rc = uinput_device_set_event_bits_v(dev, args);
	va_end(args);

	return rc;
}

int
uinput_device_set_abs_bit(struct uinput_device* dev, unsigned int code, const struct input_absinfo *absinfo)
{
	if (uinput_device_set_event_bit(dev, EV_ABS, code) < 0)
		return -EINVAL;

	dev->d.abs_info[code] = *absinfo;
	return 0;
}

int
uinput_device_event(const struct uinput_device *dev, unsigned int type, unsigned int code, int value)
{
	int max;
	int rc;
	const unsigned long *mask;
	struct input_event ev;

	if (type > EV_MAX)
		return -EINVAL;

	if (type != EV_SYN) {
		max = type_to_mask_const(&dev->d, type, &mask);
		if (max == -1 || code > max)
			return -EINVAL;
	}

	ev.type = type;
	ev.code = code;
	ev.value = value;
	ev.time.tv_sec = 0;
	ev.time.tv_usec = 0;

	rc = write(dev->d.fd, &ev, sizeof(ev));

	return (rc == -1) ? -errno : 0;
}

int uinput_device_event_multiple_v(const struct uinput_device* dev, va_list args)
{
	int type, code, value;
	int rc = 0;

	do {
		type = va_arg(args, int);
		if (type == -1)
			break;
		code = va_arg(args, int);
		if (code == -1)
			break;
		value = va_arg(args, int);
		rc = uinput_device_event(dev, type, code, value);
	} while (rc == 0);

	return rc;
}

int uinput_device_event_multiple(const struct uinput_device* dev, ...)
{
	int rc;
	va_list args;
	va_start(args, dev);
	rc = uinput_device_event_multiple_v(dev, args);
	va_end(args);
	return rc;
}
