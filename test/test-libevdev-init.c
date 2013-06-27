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
#include <libevdev/libevdev.h>

#include <check.h>
#include <errno.h>
#include <unistd.h>

#include "test-common-uinput.h"

START_TEST(test_new_device)
{
	struct libevdev *dev;

	dev = libevdev_new();
	ck_assert(dev != NULL);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_free_device)
{
	libevdev_free(NULL);
}
END_TEST

START_TEST(test_init_from_invalid_fd)
{
	int rc;
	struct libevdev *dev = NULL;

	rc = libevdev_new_from_fd(-1, &dev);

	ck_assert(dev == NULL);
	ck_assert_int_eq(rc, -EBADF);

	rc = libevdev_new_from_fd(STDIN_FILENO, &dev);
	ck_assert(dev == NULL);
	ck_assert_int_eq(rc, -ENOTTY);
}
END_TEST

START_TEST(test_init_and_change_fd)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	dev = libevdev_new();
	ck_assert(dev != NULL);
	ck_assert_int_eq(libevdev_set_fd(dev, -1), -EBADF);
	ck_assert_int_eq(libevdev_change_fd(dev, -1), -1);

	rc = uinput_device_new_with_events(&uidev,
					   "test device", DEFAULT_IDS,
					   EV_SYN, SYN_REPORT,
					   EV_REL, REL_X,
					   EV_REL, REL_Y,
					   EV_REL, REL_WHEEL,
					   EV_KEY, BTN_LEFT,
					   EV_KEY, BTN_MIDDLE,
					   EV_KEY, BTN_RIGHT,
					   -1);
	ck_assert_msg(rc == 0, "Failed to create uinput device: %s", strerror(-rc));
	ck_assert_int_eq(libevdev_set_fd(dev, uinput_device_get_fd(uidev)), 0);
	ck_assert_int_eq(libevdev_set_fd(dev, 0), -EBADF);

	ck_assert_int_eq(libevdev_get_fd(dev), uinput_device_get_fd(uidev));

	ck_assert_int_eq(libevdev_change_fd(dev, 0), 0);
	ck_assert_int_eq(libevdev_get_fd(dev), 0);

	uinput_device_free(uidev);
}
END_TEST

static void logfunc(const char *f, va_list args) {}

START_TEST(test_log_init)
{
	struct libevdev *dev = NULL;

	libevdev_set_log_handler(NULL, logfunc);
	libevdev_set_log_handler(NULL, NULL);

	dev = libevdev_new();
	ck_assert(dev != NULL);
	libevdev_set_log_handler(dev, logfunc);
	libevdev_free(dev);

	dev = libevdev_new();
	ck_assert(dev != NULL);
	libevdev_set_log_handler(dev, NULL);
	libevdev_set_log_handler(dev, logfunc);
	libevdev_free(dev);
	/* well, we didn't crash. can't test this otherwise */

}
END_TEST

START_TEST(test_device_init)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = uinput_device_new_with_events(&uidev,
					   "test device", DEFAULT_IDS,
					   EV_SYN, SYN_REPORT,
					   EV_REL, REL_X,
					   EV_REL, REL_Y,
					   EV_REL, REL_WHEEL,
					   EV_KEY, BTN_LEFT,
					   EV_KEY, BTN_MIDDLE,
					   EV_KEY, BTN_RIGHT,
					   -1);
	ck_assert_msg(rc == 0, "Failed to create uinput device: %s", strerror(-rc));

	dev = libevdev_new();
	ck_assert(dev != NULL);
	rc = libevdev_set_fd(dev, uinput_device_get_fd(uidev));
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));;

	uinput_device_free(uidev);
}
END_TEST

START_TEST(test_device_init_from_fd)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = uinput_device_new_with_events(&uidev,
					   "test device", DEFAULT_IDS,
					   EV_SYN, SYN_REPORT,
					   EV_REL, REL_X,
					   EV_REL, REL_Y,
					   EV_REL, REL_WHEEL,
					   EV_KEY, BTN_LEFT,
					   EV_KEY, BTN_MIDDLE,
					   EV_KEY, BTN_RIGHT,
					   -1);
	ck_assert_msg(rc == 0, "Failed to create uinput device: %s", strerror(-rc));

	rc = libevdev_new_from_fd(uinput_device_get_fd(uidev), &dev);
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));;

	uinput_device_free(uidev);
}
END_TEST

START_TEST(test_device_grab)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = uinput_device_new_with_events(&uidev,
					   "test device", DEFAULT_IDS,
					   EV_SYN, SYN_REPORT,
					   EV_REL, REL_X,
					   EV_REL, REL_Y,
					   EV_REL, REL_WHEEL,
					   EV_KEY, BTN_LEFT,
					   EV_KEY, BTN_MIDDLE,
					   EV_KEY, BTN_RIGHT,
					   -1);
	ck_assert_msg(rc == 0, "Failed to create uinput device: %s", strerror(-rc));

	rc = libevdev_new_from_fd(uinput_device_get_fd(uidev), &dev);
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));;

	rc = libevdev_grab(dev, 0);
	ck_assert_int_eq(rc, -EINVAL);
	rc = libevdev_grab(dev, 1);
	ck_assert_int_eq(rc, -EINVAL);

	rc = libevdev_grab(dev, LIBEVDEV_UNGRAB);
	ck_assert_int_eq(rc, 0);
	rc = libevdev_grab(dev, LIBEVDEV_GRAB);
	ck_assert_int_eq(rc, 0);
	rc = libevdev_grab(dev, LIBEVDEV_GRAB);
	ck_assert_int_eq(rc, 0);
	rc = libevdev_grab(dev, LIBEVDEV_UNGRAB);
	ck_assert_int_eq(rc, 0);

	uinput_device_free(uidev);
}
END_TEST

Suite *
libevdev_init_test(void)
{
	Suite *s = suite_create("libevdev init tests");

	TCase *tc = tcase_create("device init");
	tcase_add_test(tc, test_new_device);
	tcase_add_test(tc, test_free_device);
	tcase_add_test(tc, test_init_from_invalid_fd);
	tcase_add_test(tc, test_init_and_change_fd);
	suite_add_tcase(s, tc);

	tc = tcase_create("log init");
	tcase_add_test(tc, test_log_init);
	suite_add_tcase(s, tc);

	tc = tcase_create("device fd init");
	tcase_add_test(tc, test_device_init);
	tcase_add_test(tc, test_device_init_from_fd);
	suite_add_tcase(s, tc);

	tc = tcase_create("device grab");
	tcase_add_test(tc, test_device_grab);
	suite_add_tcase(s, tc);

	return s;
}
