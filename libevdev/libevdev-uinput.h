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

#ifndef libevdev_uinput_H
#define libevdev_uinput_H

#include <libevdev/libevdev.h>

struct libevdev_uinput;

/**
 * @defgroup uinput uinput device creation
 *
 * Creation of uinput devices based on existing libevdev devices. These functions
 * help to create uinput devices that emulate libevdev devices. In the simplest
 * form it serves to duplicate an existing device:
 *
 * @code
 * int err;
 * int new_fd;
 * struct libevdev *dev, new_dev;
 * struct libevdev_uinput *uidev;
 *
 * const char *syspath;
 *
 * err = libevdev_new_from_fd(&dev, fd);
 * if (err != 0)
 *	return err;
 *
 * uifd = open("/dev/uinput", O_RDWR);
 * if (uidev < 0)
 *	return -errno;
 *
 * err = libevdev_uinput_create_from_device(dev, uifd, &uidev);
 * if (err != 0)
 *	return err;
 *
 * syspath = libevdev_uinput_get_syspath(uidev);
 *
 * ... see libevdev_uinput_get_syspath() for information on
 *     how to get the devnode ...
 *
 * new_fd = open(devnode, O_RDONLY);
 * if (new_fd < 0)
 *	return -errno;
 *
 * err = libevdev_new_from_fd(&new_dev, new_fd);
 * if (err != 0)
 *	return err;
 *
 * ... do something ...
 *
 * libevdev_free(new_dev);
 * libevdev_uinput_destroy(uidev);
 *
 * @endcode
 *
 * Alternatively, a device can be constructed from scratch:
 *
 * @code
 * int err;
 * struct libevdev *dev;
 * struct libevdev_uinput *uidev;
 *
 * dev = libevdev_new();
 * libevdev_set_name(dev, "test device");
 * libevdev_enable_event_type(dev, EV_REL);
 * libevdev_enable_event_code(dev, EV_REL, REL_X);
 * libevdev_enable_event_code(dev, EV_REL, REL_Y);
 * libevdev_enable_event_type(dev, EV_KEY);
 * libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT);
 * libevdev_enable_event_code(dev, EV_KEY, BTN_MIDDLE);
 * libevdev_enable_event_code(dev, EV_KEY, BTN_RIGHT);
 *
 * uifd = open("/dev/uinput", O_RDWR);
 * if (uidev < 0)
 *	return -errno;
 *
 * err = libevdev_uinput_create_from_device(dev, uifd, &uidev);
 * if (err != 0)
 *	return err;
 *
 * ... do something ...
 *
 * libevdev_free(new_dev);
 * libevdev_uinput_destroy(uidev);
 *
 * @endcode
 */

/**
 * @ingroup uinput
 *
 * Create a uinput device based on the libevdev device given. The uinput device
 * will be an exact copy of the libevdev device, minus the bits that uinput doesn't
 * allow to be set.
 *
 * The device's lifetime is tied to the uinput file descriptor, closing it will
 * destroy the uinput device. You should call libevdev_uinput_destroy() before
 * closing the file descriptor.
 *
 * You don't need to keep the file descriptor variable, libevdev_uinput_get_fd()
 * will return the same number for a future call to libevdev_uinput_destroy().
 *
 * @note Due to limitations in the uinput kernel module, EV_REP repeat/period values
 * will default to the kernel defaults, not to the ones set in the source device.
 *
 * @param dev The device to duplicate
 * @param uinput_fd A file descriptor to /dev/uinput, opened with the required
 * permissions to create a device.
 * @param[out] uinput_dev A newly created libevdev device.
 *
 * @return 0 on success or a negative errno on failure. On failure, the value of
 * uinput_dev is undefined.
 *
 * @see libevdev_uinput_destroy
 */
int libevdev_uinput_create_from_device(const struct libevdev *dev, int uinput_fd, struct libevdev_uinput **uinput_dev);

/**
 * @ingroup uinput
 *
 * Destroy a previously created uinput device and free associated memory.
 * uinput_dev is invalid after calling this function.
 *
 * @param uinput_dev A previously created uinput device.
 *
 * @return 0 on success or a negative errno on failure
 */
void libevdev_uinput_destroy(struct libevdev_uinput *uinput_dev);

/**
 * @ingroup uinput
 *
 * Return the file descriptor for the uinput device.
 *
 * @param uinput_dev A previously created uinput device.
 *
 * @return The file descriptor used to create this device
 */
int libevdev_uinput_get_fd(struct libevdev_uinput *uinput_dev);

/**
 * @ingroup uinput
 *
 * This function may return NULL. As of 3.11, the uinput kernel device does not
 * provide a way to get the syspath directly through uinput so libevdev must guess.
 * In some cases libevdev is unable to derive the syspath. If the running kernel
 * supports the UI_GET_SYSPATH ioctl, the syspath is retrieved through that and will
 * not be NULL.
 *
 * To obtain a /dev/input/eventX device node from this path use
 *
 * @code
 * const char *syspath = libevdev_uinput_get_syspath(uidev);
 * DIR *dir;
 * struct dirent *dp;
 * struct udev *udev;
 * struct udev_device *udev_device;
 *
 * udev = udev_new();
 * dir = opendir(syspath);
 * while ((dp = readdir(dir))) {
 *      char *path;
 *
 *      if (strncmp(dp->d_name, "event", 5) != 0)
 *          continue;
 *
 *      asprintf(&path, "%s/%s", syspath, dp->d_name);
 *      udev_device = udev_device_new_from_syspath(udev, path);
 *      printf("device node: %s\n", udev_device_get_devnode(udev_device));
 *      udev_device_unref(udev_device);
 *      free(path);
 * }
 *
 * closedir(dir);
 * udev_unref(udev);
 * @endcode
 *
 *
 * @param uinput_dev A previously created uinput device.
 * @return the syspath for this device, including preceding /sys.
 */
const char*libevdev_uinput_get_syspath(struct libevdev_uinput *uinput_dev);

#endif /* libevdev_uinput_H */
