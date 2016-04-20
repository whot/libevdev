/*
 * Copyright © 2014 Red Hat, Inc.
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
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/input.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-util.h>
#include <libevdev/libevdev-uinput.h>
#include "test-common.h"

START_TEST(test_revoke)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2;
	int rc, fd;
	struct input_event ev1, ev2;
	int dev_fd;

	test_create_device(&uidev, &dev,
			   EV_SYN, SYN_REPORT,
			   EV_REL, REL_X,
			   EV_REL, REL_Y,
			   EV_REL, REL_WHEEL,
			   EV_KEY, BTN_LEFT,
			   EV_KEY, BTN_MIDDLE,
			   EV_KEY, BTN_RIGHT,
			   -1);

	fd = open(uinput_device_get_devnode(uidev), O_RDONLY|O_NONBLOCK);
	ck_assert_int_gt(fd, -1);
	rc = libevdev_new_from_fd(fd, &dev2);
	ck_assert_msg(rc == 0, "Failed to create second device: %s", strerror(-rc));

	uinput_device_event(uidev, EV_REL, REL_X, 1);
	uinput_device_event(uidev, EV_SYN, SYN_REPORT, 0);

	rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev1);
	ck_assert_int_eq(rc, LIBEVDEV_READ_STATUS_SUCCESS);

	rc = libevdev_next_event(dev2, LIBEVDEV_READ_FLAG_NORMAL, &ev2);
	ck_assert_int_eq(rc, LIBEVDEV_READ_STATUS_SUCCESS);

	ck_assert_int_eq(ev1.type, ev2.type);
	ck_assert_int_eq(ev1.code, ev2.code);
	ck_assert_int_eq(ev1.value, ev2.value);

	/* revoke first device, expect it closed, second device still open */
	dev_fd = libevdev_get_fd(dev);
	ck_assert_int_ge(dev_fd, 0);
	rc = ioctl(dev_fd, EVIOCREVOKE, NULL);
	if (rc == -1 && errno == EINVAL) {
		fprintf(stderr, "WARNING: skipping EVIOCREVOKE test, not suported by current kernel\n");
		goto out;
	}
	ck_assert_msg(rc == 0, "Failed to revoke device: %s", strerror(errno));

	rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev1);
	ck_assert_int_eq(rc, -ENODEV);

	rc = libevdev_next_event(dev2, LIBEVDEV_READ_FLAG_NORMAL, &ev2);
	ck_assert_int_eq(rc, LIBEVDEV_READ_STATUS_SUCCESS);

out:
	uinput_device_free(uidev);
	libevdev_free(dev);
	libevdev_free(dev2);
	close(fd);
}
END_TEST

START_TEST(test_revoke_invalid)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;
	int dev_fd;

	test_create_device(&uidev, &dev,
			   EV_SYN, SYN_REPORT,
			   EV_REL, REL_X,
			   EV_REL, REL_Y,
			   EV_REL, REL_WHEEL,
			   EV_KEY, BTN_LEFT,
			   EV_KEY, BTN_MIDDLE,
			   EV_KEY, BTN_RIGHT,
			   -1);

	dev_fd = libevdev_get_fd(dev);
	ck_assert_int_ge(dev_fd, 0);
	/* ioctl requires 0 as value */
	rc = ioctl(dev_fd, EVIOCREVOKE, 1);
	ck_assert_int_eq(rc, -1);
	ck_assert_int_eq(errno, EINVAL);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_revoke_fail_after)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2 = NULL;
	int rc, fd;

	test_create_device(&uidev, &dev,
			   EV_SYN, SYN_REPORT,
			   EV_REL, REL_X,
			   EV_REL, REL_Y,
			   EV_REL, REL_WHEEL,
			   EV_KEY, BTN_LEFT,
			   EV_KEY, BTN_MIDDLE,
			   EV_KEY, BTN_RIGHT,
			   -1);

	fd = open(uinput_device_get_devnode(uidev), O_RDONLY|O_NONBLOCK);
	ck_assert_int_gt(fd, -1);

	rc = ioctl(fd, EVIOCREVOKE, NULL);
	if (rc == -1 && errno == EINVAL) {
		fprintf(stderr, "WARNING: skipping EVIOCREVOKE test, not suported by current kernel\n");
		goto out;
	}
	ck_assert_msg(rc == 0, "Failed to revoke device: %s", strerror(errno));

	rc = libevdev_new_from_fd(fd, &dev2);
	ck_assert_int_eq(rc, -ENODEV);

out:
	uinput_device_free(uidev);
	libevdev_free(dev);
	close(fd);
}
END_TEST

START_TEST(test_mask)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2 = NULL;
	int rc, fd;
	unsigned long m[NLONGS(KEY_CNT)] = {0};
	struct input_mask mask = {
		mask.type = EV_REL,
		mask.codes_size = sizeof(m),
		mask.codes_ptr = (uint64_t)m,
	};
	int code;

	test_create_device(&uidev, &dev,
			   EV_SYN, SYN_REPORT,
			   EV_REL, REL_X,
			   EV_REL, REL_Y,
			   EV_REL, REL_WHEEL,
			   EV_KEY, BTN_LEFT,
			   EV_KEY, BTN_MIDDLE,
			   EV_KEY, BTN_RIGHT,
			   -1);
	fd = open(uinput_device_get_devnode(uidev), O_RDONLY|O_NONBLOCK);
	ck_assert_int_gt(fd, -1);

	rc = ioctl(fd, EVIOCSMASK, NULL);
	if (rc == -1 && errno == EINVAL) {
		fprintf(stderr, "WARNING: skipping EVIOCSMASK test, not suported by current kernel\n");
		goto out;
	}

	rc = libevdev_new_from_fd(fd, &dev2);
	ck_assert_int_eq(rc, 0);

	rc = ioctl(fd, EVIOCGMASK, &mask);
	ck_assert_int_eq(rc, 0);

	/* By default all bits are set */
	for (code = 0; code < REL_CNT; code++)
		ck_assert(bit_is_set(m, code));

	clear_bit(m, REL_Y);

	rc = ioctl(fd, EVIOCSMASK, &mask);
	ck_assert_int_eq(rc, 0);

	memset(m, 0, sizeof(m));
	rc = ioctl(fd, EVIOCGMASK, &mask);
	ck_assert_int_eq(rc, 0);

	ck_assert(!bit_is_set(m, REL_Y));
	for (code = 0; code < REL_CNT; code++) {
		if (code == REL_Y)
			continue;
		ck_assert(bit_is_set(m, code));
	}

out:
	uinput_device_free(uidev);
	libevdev_free(dev);
	close(fd);
}
END_TEST

START_TEST(test_mask_events)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2 = NULL;
	int rc, fd;
	unsigned long m[NLONGS(KEY_CNT)] = {-1};
	struct input_mask mask = {
		mask.type = EV_REL,
		mask.codes_size = sizeof(m),
		mask.codes_ptr = (uint64_t)m,
	};
	struct input_event ev;

	test_create_device(&uidev, &dev,
			   EV_SYN, SYN_REPORT,
			   EV_REL, REL_X,
			   EV_REL, REL_Y,
			   EV_REL, REL_WHEEL,
			   EV_KEY, BTN_LEFT,
			   EV_KEY, BTN_MIDDLE,
			   EV_KEY, BTN_RIGHT,
			   -1);
	fd = open(uinput_device_get_devnode(uidev), O_RDONLY|O_NONBLOCK);
	ck_assert_int_gt(fd, -1);

	rc = ioctl(fd, EVIOCSMASK, NULL);
	if (rc == -1 && errno == EINVAL) {
		fprintf(stderr, "WARNING: skipping EVIOCSMASK test, not suported by current kernel\n");
		goto out;
	}

	rc = libevdev_new_from_fd(fd, &dev2);
	ck_assert_int_eq(rc, 0);

	clear_bit(m, REL_Y);
	rc = ioctl(fd, EVIOCSMASK, &mask);
	ck_assert_int_eq(rc, 0);

	/* we disabled the kernel, not the libevdev device */
	ck_assert(libevdev_has_event_code(dev2, EV_REL, REL_Y));

	uinput_device_event(uidev, EV_REL, REL_X, 1);
	uinput_device_event(uidev, EV_REL, REL_Y, 1);
	uinput_device_event(uidev, EV_SYN, SYN_REPORT, 0);

	rc = libevdev_next_event(dev2, LIBEVDEV_READ_FLAG_NORMAL, &ev);
	ck_assert_int_eq(rc, LIBEVDEV_READ_STATUS_SUCCESS);
	ck_assert(libevdev_event_is_code(&ev, EV_REL, REL_X));

	rc = libevdev_next_event(dev2, LIBEVDEV_READ_FLAG_NORMAL, &ev);
	ck_assert_int_eq(rc, LIBEVDEV_READ_STATUS_SUCCESS);
	ck_assert(libevdev_event_is_code(&ev, EV_SYN, SYN_REPORT));

out:
	uinput_device_free(uidev);
	libevdev_free(dev);
	close(fd);
}
END_TEST

START_TEST(test_mask_out_of_range)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2 = NULL;
	int rc, fd;
	const unsigned int fake_max = REL_CNT + 13;
	unsigned long m[NLONGS(KEY_CNT)] = {0};
	struct input_mask mask = {
		mask.type = EV_REL,
		mask.codes_size = sizeof(m),
		mask.codes_ptr = (uint64_t)m,
	};
	int code;

	test_create_device(&uidev, &dev,
			   EV_SYN, SYN_REPORT,
			   EV_REL, REL_X,
			   EV_REL, REL_Y,
			   EV_REL, REL_WHEEL,
			   EV_KEY, BTN_LEFT,
			   EV_KEY, BTN_MIDDLE,
			   EV_KEY, BTN_RIGHT,
			   -1);
	fd = open(uinput_device_get_devnode(uidev), O_RDONLY|O_NONBLOCK);
	ck_assert_int_gt(fd, -1);

	rc = ioctl(fd, EVIOCSMASK, NULL);
	if (rc == -1 && errno == EINVAL) {
		fprintf(stderr, "WARNING: skipping EVIOCSMASK test, not suported by current kernel\n");
		goto out;
	}

	rc = libevdev_new_from_fd(fd, &dev2);
	ck_assert_int_eq(rc, 0);

	rc = ioctl(fd, EVIOCGMASK, &mask);
	ck_assert_int_eq(rc, 0);

	/* By default all known bits are set */
	for (code = 0; code < fake_max; code++)
		ck_assert(bit_is_set(m, code));

	clear_bit(m, REL_Y);

	rc = ioctl(fd, EVIOCSMASK, &mask);
	ck_assert_int_eq(rc, 0);

	memset(m, 0, sizeof(m));
	rc = ioctl(fd, EVIOCGMASK, &mask);
	ck_assert_int_eq(rc, 0);

	ck_assert(!bit_is_set(m, REL_Y));
	for (code = 0; code < fake_max; code++) {
		if (code == REL_Y)
			continue;
		ck_assert(bit_is_set(m, code));
	}

out:
	uinput_device_free(uidev);
	libevdev_free(dev);
	close(fd);
}
END_TEST

START_TEST(test_mask_out_of_high_range)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2 = NULL;
	int rc, fd;
	const unsigned int fake_max = REL_CNT + 64; /* runs into the next long */
	unsigned long m[NLONGS(KEY_CNT)] = {-1};
	struct input_mask mask = {
		mask.type = EV_REL,
		mask.codes_size = sizeof(m),
		mask.codes_ptr = (uint64_t)m,
	};
	int code;
	_Static_assert(KEY_CNT >= REL_CNT + 64, "KEY_CNT is too low");

	test_create_device(&uidev, &dev,
			   EV_SYN, SYN_REPORT,
			   EV_REL, REL_X,
			   EV_REL, REL_Y,
			   EV_REL, REL_WHEEL,
			   EV_KEY, BTN_LEFT,
			   EV_KEY, BTN_MIDDLE,
			   EV_KEY, BTN_RIGHT,
			   -1);
	fd = open(uinput_device_get_devnode(uidev), O_RDONLY|O_NONBLOCK);
	ck_assert_int_gt(fd, -1);

	rc = ioctl(fd, EVIOCSMASK, NULL);
	if (rc == -1 && errno == EINVAL) {
		fprintf(stderr, "WARNING: skipping EVIOCSMASK test, not suported by current kernel\n");
		goto out;
	}

	rc = libevdev_new_from_fd(fd, &dev2);
	ck_assert_int_eq(rc, 0);

	rc = ioctl(fd, EVIOCGMASK, &mask);
	ck_assert_int_eq(rc, 0);

#if 0
	/* By default all known bits are set */
	for (code = 0; code < fake_max; code++)
		ck_assert(bit_is_set(m, code));

	clear_bit(m, REL_Y);

	rc = ioctl(fd, EVIOCSMASK, &mask);
	ck_assert_int_eq(rc, 0);

	memset(m, 0, sizeof(m));
	rc = ioctl(fd, EVIOCGMASK, &mask);
	ck_assert_int_eq(rc, 0);
#endif

	rc = ioctl(fd, EVIOCSMASK, &mask);
	ck_assert_int_eq(rc, 0);
	rc = ioctl(fd, EVIOCGMASK, &mask);
	ck_assert_int_eq(rc, 0);
	//ck_assert(!bit_is_set(m, REL_Y));
	for (code = 0; code < fake_max; code++) {
		if (code == REL_Y)
			continue;
		printf("code: %d\n", code);
		ck_assert(bit_is_set(m, code));
	}

out:
	uinput_device_free(uidev);
	libevdev_free(dev);
	close(fd);
}
END_TEST
int main(int argc, char **argv)
{
	SRunner *sr;
	Suite *s;
	TCase *tc;
	int failed;

	s = suite_create("kernel tests");

	tc = tcase_create("EVIOCREVOKE");
	tcase_add_test(tc, test_revoke);
	tcase_add_test(tc, test_revoke_invalid);
	tcase_add_test(tc, test_revoke_fail_after);
	suite_add_tcase(s, tc);

	tc = tcase_create("EVIOCSMASK");
	tcase_add_test(tc, test_mask);
	tcase_add_test(tc, test_mask_events);
	tcase_add_test(tc, test_mask_out_of_range);
	tcase_add_test(tc, test_mask_out_of_high_range);
	suite_add_tcase(s, tc);

	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);

	failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return failed;
}
