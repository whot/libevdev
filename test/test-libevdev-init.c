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
#include <unistd.h>

#include "test-common.h"

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
					   TEST_DEVICE_NAME, DEFAULT_IDS,
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
	libevdev_free(dev);
}
END_TEST


static int log_fn_called = 0;
static char *logdata = "test";
static void logfunc(enum libevdev_log_priority priority,
		    void *data,
		    const char *file, int line, const char *func,
		    const char *f, va_list args) {
	ck_assert_int_eq(strcmp(logdata, data), 0);
	log_fn_called++;
}

START_TEST(test_log_init)
{
	struct libevdev *dev = NULL;

	libevdev_set_log_function(logfunc, NULL);
	libevdev_set_log_function(NULL, NULL);

	dev = libevdev_new();
	ck_assert(dev != NULL);

	libevdev_set_log_function(logfunc, logdata);
	libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, NULL);

	libevdev_set_log_function(NULL, NULL);
	libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, NULL);

	libevdev_set_log_function(logfunc, logdata);
	libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, NULL);

	/* libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, NULL) should
	   trigger a log message. We called it three times, but only twice
	   with the logfunc set, thus, ensure we only called the logfunc
	   twice */
	ck_assert_int_eq(log_fn_called, 2);

	libevdev_free(dev);
}
END_TEST

static char *logdata_1 = "foo";
static char *logdata_2 = "bar";
static int log_data_fn_called = 0;
static void logfunc_data(enum libevdev_log_priority priority,
			 void *data,
			 const char *file, int line, const char *func,
			 const char *f, va_list args) {
	switch(log_data_fn_called) {
		case 0: ck_assert(data == logdata_1); break;
		case 1: ck_assert(data == logdata_2); break;
		case 2: ck_assert(data == NULL); break;
		default:
			ck_abort();
	}
	log_data_fn_called++;
}

START_TEST(test_log_data)
{
	struct libevdev *dev = NULL;

	dev = libevdev_new();
	ck_assert(dev != NULL);

	libevdev_set_log_function(logfunc_data, logdata_1);
	libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, NULL);

	libevdev_set_log_function(logfunc_data, logdata_2);
	libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, NULL);

	libevdev_set_log_function(logfunc_data, NULL);
	libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, NULL);

	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_init)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = uinput_device_new_with_events(&uidev,
					   TEST_DEVICE_NAME, DEFAULT_IDS,
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
	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_init_from_fd)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = uinput_device_new_with_events(&uidev,
					   TEST_DEVICE_NAME, DEFAULT_IDS,
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
	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_grab)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = test_create_device(&uidev, &dev,
				EV_SYN, SYN_REPORT,
				EV_REL, REL_X,
				EV_REL, REL_Y,
				EV_REL, REL_WHEEL,
				EV_KEY, BTN_LEFT,
				EV_KEY, BTN_MIDDLE,
				EV_KEY, BTN_RIGHT,
				-1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

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
	libevdev_free(dev);
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
	tcase_add_test(tc, test_log_data);
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
