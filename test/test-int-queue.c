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

#include <limits.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-int.h>

#include <check.h>

START_TEST(test_queue_alloc)
{
	struct libevdev dev;
	int rc;

	rc = queue_alloc(&dev, 0);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = queue_alloc(&dev, ULONG_MAX);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = queue_alloc(&dev, 100);
	ck_assert_int_eq(rc, 0);

	ck_assert_int_eq(dev.queue_size, 100);
	ck_assert_int_eq(dev.queue_next, 0);

	queue_free(&dev);
	ck_assert_int_eq(dev.queue_size, 0);
	ck_assert_int_eq(dev.queue_next, 0);

}
END_TEST

START_TEST(test_queue_sizes)
{
	struct libevdev dev = {0};

	queue_alloc(&dev, 0);
	ck_assert_int_eq(queue_num_elements(&dev), 0);
	ck_assert_int_eq(queue_num_free_elements(&dev), 0);
	ck_assert_int_eq(queue_size(&dev), 0);

	queue_alloc(&dev, 100);
	ck_assert_int_eq(queue_num_elements(&dev), 0);
	ck_assert_int_eq(queue_num_free_elements(&dev), 100);
	ck_assert_int_eq(queue_size(&dev), 100);

	queue_free(&dev);

	ck_assert_int_eq(queue_num_elements(&dev), 0);
	ck_assert_int_eq(queue_num_free_elements(&dev), 0);
	ck_assert_int_eq(queue_size(&dev), 0);
}
END_TEST

START_TEST(test_queue_push)
{
	struct libevdev dev = {0};
	struct input_event *ev;

	queue_alloc(&dev, 0);
	ev = queue_push(&dev);
	ck_assert(ev == NULL);

	queue_alloc(&dev, 2);
	ev = queue_push(&dev);
	ck_assert(ev == dev.queue);
	ck_assert_int_eq(queue_num_elements(&dev), 1);
	ck_assert_int_eq(queue_num_free_elements(&dev), 1);

	ev = queue_push(&dev);
	ck_assert(ev == dev.queue + 1);

	ev = queue_push(&dev);
	ck_assert(ev == NULL);

	queue_free(&dev);
	ev = queue_push(&dev);
	ck_assert(ev == NULL);

}
END_TEST

START_TEST(test_queue_pop)
{
	struct libevdev dev = {0};
	struct input_event ev, *e, tmp;
	int rc;

	queue_alloc(&dev, 0);
	rc = queue_pop(&dev, &ev);
	ck_assert_int_eq(rc, 1);

	queue_alloc(&dev, 2);
	e = queue_push(&dev);
	memset(e, 0xab, sizeof(*e));
	ck_assert_int_eq(queue_num_elements(&dev), 1);
	ck_assert_int_eq(queue_num_free_elements(&dev), 1);

	rc = queue_pop(&dev, &ev);
	ck_assert_int_eq(rc, 0);
	memset(&tmp, 0xab, sizeof(tmp));
	rc = memcmp(&tmp, &ev, sizeof(tmp));
	ck_assert_int_eq(rc, 0);

	ck_assert_int_eq(queue_num_elements(&dev), 0);
	ck_assert_int_eq(queue_num_free_elements(&dev), 2);

	rc = queue_pop(&dev, &ev);
	ck_assert_int_eq(rc, 1);

	queue_free(&dev);
}
END_TEST

Suite *
queue_suite(void)
{
	Suite *s = suite_create("Event queue");

	TCase *tc = tcase_create("Queue allocation");
	tcase_add_test(tc, test_queue_alloc);
	tcase_add_test(tc, test_queue_sizes);
	suite_add_tcase(s, tc);

	tc= tcase_create("Queue push/pop");
	tcase_add_test(tc, test_queue_push);
	tcase_add_test(tc, test_queue_pop);
	suite_add_tcase(s, tc);

	return s;
}

int main(int argc, char **argv)
{
	int failed;
	Suite *s = queue_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return failed;
}
