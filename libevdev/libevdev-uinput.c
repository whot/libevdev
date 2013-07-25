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
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <linux/uinput.h>

#include "libevdev.h"
#include "libevdev-int.h"
#include "libevdev-uinput.h"
#include "libevdev-uinput-int.h"
#include "libevdev-util.h"

#define SYS_INPUT_DIR "/sys/devices/virtual/input/"

static struct libevdev_uinput *
_libevdev_uinput_new(const char *name)
{
	struct libevdev_uinput *uinput_dev;

	uinput_dev = calloc(1, sizeof(struct libevdev_uinput));
	if (uinput_dev) {
		uinput_dev->name = strdup(name);
	}

	return uinput_dev;
}

int
libevdev_uinput_create_from_device(const struct libevdev *dev, int fd, struct libevdev_uinput** uinput_dev)
{
	int rc;
	int type, prop;
	struct uinput_user_dev uidev;
	struct libevdev_uinput *new_device;

	new_device = _libevdev_uinput_new(libevdev_get_name(dev));
	if (!new_device)
		return -ENOMEM;

	memset(&uidev, 0, sizeof(uidev));

	strncpy(uidev.name, libevdev_get_name(dev), UINPUT_MAX_NAME_SIZE - 1);
	uidev.id = dev->ids;

	for (type = 0; type < EV_MAX; type++) {
		int code;
		int max;
		int uinput_bit;
		const unsigned long *mask;

		if (!libevdev_has_event_type(dev, type))
			continue;

		rc = ioctl(fd, UI_SET_EVBIT, type);
		if (rc == -1)
			goto error;

		/* uinput doesn't allow EV_REP codes to be set. */
		if (type == EV_REP)
			continue;

		max = type_to_mask_const(dev, type, &mask);
		if (max == -1)
			continue;

		switch(type) {
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

		for (code = 0; code < max; code++) {
			if (!libevdev_has_event_code(dev, type, code))
				continue;

			rc = ioctl(fd, uinput_bit, code);
			if (rc == -1)
				goto error;

			if (type == EV_ABS) {
				const struct input_absinfo *abs = libevdev_get_abs_info(dev, code);
				uidev.absmin[code] = abs->minimum;
				uidev.absmax[code] = abs->maximum;
				uidev.absfuzz[code] = abs->fuzz;
				uidev.absflat[code] = abs->flat;
				/* uinput has no resolution in the device struct, this needs
				 * to be fixed in the kernel */
			}
		}

	}

	for (prop = 0; prop < INPUT_PROP_MAX; prop++) {
		if (!libevdev_has_property(dev, prop))
			continue;

		rc = ioctl(fd, UI_SET_PROPBIT, prop);
		if (rc == -1)
			goto error;
	}

	rc = write(fd, &uidev, sizeof(uidev));
	if (rc < 0)
		goto error;
	else if (rc < sizeof(uidev)) {
		errno = EINVAL;
		goto error;
	}

	/* ctime notes before/after ioctl to help us filter out devices
	   when traversing /sys/devices/virtual/input inl
	   libevdev_uinput_get_syspath.
	 */
	new_device->ctime[0] = time(NULL);

	rc = ioctl(fd, UI_DEV_CREATE, NULL);
	if (rc == -1)
		goto error;

	new_device->ctime[1] = time(NULL);
	new_device->fd = fd;

	*uinput_dev = new_device;

	return 0;

error:
	libevdev_uinput_destroy(new_device);

	return -errno;
}

void libevdev_uinput_destroy(struct libevdev_uinput *uinput_dev)
{
	ioctl(uinput_dev->fd, UI_DEV_DESTROY, NULL);
	free(uinput_dev->name);
	free(uinput_dev);
}

int libevdev_uinput_get_fd(struct libevdev_uinput *uinput_dev)
{
	return uinput_dev->fd;
}

const char* libevdev_uinput_get_syspath(struct libevdev_uinput *uinput_dev)
{
	DIR *dir;
	struct dirent *dev;

	if (uinput_dev->syspath != NULL)
		return uinput_dev->syspath;

	/* FIXME: use new ioctl() here once kernel supports it */

	dir = opendir(SYS_INPUT_DIR);
	if (!dir)
		return NULL;

	while((dev = readdir(dir))) {
		struct stat st;
		int fd;
		char path[sizeof(SYS_INPUT_DIR) + 64];
		char buf[256];
		int len;

		if (strncmp("input", dev->d_name, 5) != 0)
			continue;

		strcpy(path, SYS_INPUT_DIR);
		strcat(path, dev->d_name);

		if (stat(path, &st) == -1)
			continue;

		/* created before/after UI_DEV_CREATE */
		if (st.st_ctime < uinput_dev->ctime[0] ||
		    st.st_ctime > uinput_dev->ctime[1])
			continue;

		/* created after UI_DEV_CREATE */
		strcat(path, "/name");
		fd = open(path, O_RDONLY);
		if (fd < 0)
			continue;

		len = read(fd, buf, sizeof(buf) - 1);
		if (len > 1) {
			buf[len - 1] = '\0'; /* file contains \n */
			if (strcmp(buf, uinput_dev->name) == 0) {
				strcpy(path, SYS_INPUT_DIR);
				strcat(path, dev->d_name);
				uinput_dev->syspath = strdup(path);
				break;
			}
		}

		close(fd);
	}

	closedir(dir);

	return uinput_dev->syspath;
}
