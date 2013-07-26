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
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <libevdev/libevdev-uinput.h>

#include "test-common.h"
#define UINPUT_NODE "/dev/uinput"

START_TEST(test_uinput_create_device)
{
	struct libevdev *dev;
	struct libevdev_uinput *uidev;
	int fd;
	int rc;

	dev = libevdev_new();
	ck_assert(dev != NULL);
	libevdev_set_name(dev, TEST_DEVICE_NAME);
	libevdev_enable_event_type(dev, EV_SYN);
	libevdev_enable_event_type(dev, EV_REL);
	libevdev_enable_event_code(dev, EV_REL, REL_X, NULL);
	libevdev_enable_event_code(dev, EV_REL, REL_Y, NULL);

	fd = open(UINPUT_NODE, O_RDWR);
	ck_assert_int_gt(fd, -1);

	rc = libevdev_uinput_create_from_device(dev, fd, &uidev);
	ck_assert_int_eq(rc, 0);
	ck_assert(uidev != NULL);

	ck_assert_int_eq(libevdev_uinput_get_fd(uidev), fd);

	libevdev_free(dev);
	libevdev_uinput_destroy(uidev);
	close(fd);
}
END_TEST

START_TEST(test_uinput_check_syspath_time)
{
	struct libevdev *dev;
	struct libevdev_uinput *uidev, *uidev2;
	const char *syspath, *syspath2;
	int fd, fd2;
	int rc;

	dev = libevdev_new();
	ck_assert(dev != NULL);
	libevdev_set_name(dev, TEST_DEVICE_NAME);
	libevdev_enable_event_type(dev, EV_SYN);
	libevdev_enable_event_type(dev, EV_REL);
	libevdev_enable_event_code(dev, EV_REL, REL_X, NULL);
	libevdev_enable_event_code(dev, EV_REL, REL_Y, NULL);

	fd = open(UINPUT_NODE, O_RDWR);
	ck_assert_int_gt(fd, -1);
	fd2 = open(UINPUT_NODE, O_RDWR);
	ck_assert_int_gt(fd2, -1);

	rc = libevdev_uinput_create_from_device(dev, fd, &uidev);
	ck_assert_int_eq(rc, 0);

	/* sleep for 1.5 seconds so syspath gives us something useful */
	usleep(1500000);

	/* create a second one to stress the syspath filtering code */
	rc = libevdev_uinput_create_from_device(dev, fd2, &uidev2);
	ck_assert_int_eq(rc, 0);

	syspath = libevdev_uinput_get_syspath(uidev);
	ck_assert(syspath != NULL);

	/* get syspath twice returns same pointer */
	syspath2 = libevdev_uinput_get_syspath(uidev);
	ck_assert(syspath == syspath2);

	/* second dev has different syspath */
	syspath2 = libevdev_uinput_get_syspath(uidev2);
	ck_assert(strcmp(syspath, syspath2) != 0);

	libevdev_free(dev);
	libevdev_uinput_destroy(uidev);
	libevdev_uinput_destroy(uidev2);

	close(fd);
	close(fd2);
}
END_TEST

START_TEST(test_uinput_check_syspath_name)
{
	struct libevdev *dev;
	struct libevdev_uinput *uidev, *uidev2;
	const char *syspath, *syspath2;
	int fd, fd2;
	int rc;

	dev = libevdev_new();
	ck_assert(dev != NULL);
	libevdev_set_name(dev, TEST_DEVICE_NAME);
	libevdev_enable_event_type(dev, EV_SYN);
	libevdev_enable_event_type(dev, EV_REL);
	libevdev_enable_event_code(dev, EV_REL, REL_X, NULL);
	libevdev_enable_event_code(dev, EV_REL, REL_Y, NULL);

	fd = open(UINPUT_NODE, O_RDWR);
	ck_assert_int_gt(fd, -1);
	fd2 = open(UINPUT_NODE, O_RDWR);
	ck_assert_int_gt(fd2, -1);

	rc = libevdev_uinput_create_from_device(dev, fd, &uidev);
	ck_assert_int_eq(rc, 0);

	/* create a second one to stress the syspath filtering code */
	libevdev_set_name(dev, TEST_DEVICE_NAME " 2");
	rc = libevdev_uinput_create_from_device(dev, fd2, &uidev2);
	ck_assert_int_eq(rc, 0);

	syspath = libevdev_uinput_get_syspath(uidev);
	ck_assert(syspath != NULL);

	/* get syspath twice returns same pointer */
	syspath2 = libevdev_uinput_get_syspath(uidev);
	ck_assert(syspath == syspath2);

	/* second dev has different syspath */
	syspath2 = libevdev_uinput_get_syspath(uidev2);
	ck_assert(strcmp(syspath, syspath2) != 0);

	libevdev_free(dev);
	libevdev_uinput_destroy(uidev);
	libevdev_uinput_destroy(uidev2);

	close(fd);
	close(fd2);
}
END_TEST

Suite *
uinput_suite(void)
{
	Suite *s = suite_create("libevdev uinput device tests");

	TCase *tc = tcase_create("device creation");
	tcase_add_test(tc, test_uinput_create_device);
	tcase_add_test(tc, test_uinput_check_syspath_time);
	tcase_add_test(tc, test_uinput_check_syspath_name);
	suite_add_tcase(s, tc);

	return s;
}
