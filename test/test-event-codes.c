/*
 * Copyright © 2013 David Herrmann <dh.herrmann@gmail.com>
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
#include <libevdev/libevdev-int.h>
#include "test-common.h"

START_TEST(test_type_codes)
{
	ck_assert(libevdev_event_type_from_name("EV_SYN", -1) == EV_SYN);
	ck_assert(libevdev_event_type_from_name("EV_KEY", -1) == EV_KEY);
	ck_assert(libevdev_event_type_from_name("EV_REL", -1) == EV_REL);
	ck_assert(libevdev_event_type_from_name("EV_ABS", -1) == EV_ABS);
	ck_assert(libevdev_event_type_from_name("EV_MSC", -1) == EV_MSC);
	ck_assert(libevdev_event_type_from_name("EV_SND", -1) == EV_SND);
	ck_assert(libevdev_event_type_from_name("EV_SW", -1) == EV_SW);
	ck_assert(libevdev_event_type_from_name("EV_LED", -1) == EV_LED);
	ck_assert(libevdev_event_type_from_name("EV_REP", -1) == EV_REP);
	ck_assert(libevdev_event_type_from_name("EV_FF", -1) == EV_FF);
	ck_assert(libevdev_event_type_from_name("EV_FF_STATUS", -1) == EV_FF_STATUS);
	ck_assert(libevdev_event_type_from_name("EV_MAX", -1) == EV_MAX);

	ck_assert(libevdev_event_type_from_name("EV_SYNTAX", 6) == EV_SYN);
	ck_assert(libevdev_event_type_from_name("EV_REPTILE", 6) == EV_REP);
}
END_TEST

START_TEST(test_type_invalid)
{
	ck_assert(libevdev_event_type_from_name("EV_Syn", -1) == -1);
	ck_assert(libevdev_event_type_from_name("ev_SYN", -1) == -1);
	ck_assert(libevdev_event_type_from_name("SYN", -1) == -1);

	ck_assert(libevdev_event_type_from_name("EV_SYN", 5) == -1);
	ck_assert(libevdev_event_type_from_name("EV_SYNTAX", -1) == -1);
	ck_assert(libevdev_event_type_from_name("EV_REPTILE", 7) == -1);
}
END_TEST

START_TEST(test_key_codes)
{
	ck_assert(libevdev_event_code_from_name(EV_SYN, "SYN_REPORT", -1) == SYN_REPORT);
	ck_assert(libevdev_event_code_from_name(EV_ABS, "ABS_X", -1) == ABS_X);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "BTN_A", -1) == BTN_A);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "KEY_A", -1) == KEY_A);
	ck_assert(libevdev_event_code_from_name(EV_REL, "REL_X", -1) == REL_X);
	ck_assert(libevdev_event_code_from_name(EV_ABS, "ABS_YXZ", 5) == ABS_Y);
	ck_assert(libevdev_event_code_from_name(EV_MSC, "MSC_RAW", -1) == MSC_RAW);
	ck_assert(libevdev_event_code_from_name(EV_LED, "LED_KANA", -1) == LED_KANA);
	ck_assert(libevdev_event_code_from_name(EV_SND, "SND_BELL", -1) == SND_BELL);
	ck_assert(libevdev_event_code_from_name(EV_REP, "REP_DELAY", -1) == REP_DELAY);
	ck_assert(libevdev_event_code_from_name(EV_SYN, "SYN_DROPPED", -1) == SYN_DROPPED);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "KEY_RESERVED", -1) == KEY_RESERVED);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "BTN_0", -1) == BTN_0);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "KEY_0", -1) == KEY_0);
	ck_assert(libevdev_event_code_from_name(EV_FF, "FF_GAIN", -1) == FF_GAIN);
	ck_assert(libevdev_event_code_from_name(EV_FF_STATUS, "FF_STATUS_MAX", -1) == FF_STATUS_MAX);
	ck_assert(libevdev_event_code_from_name(EV_SW, "SW_MAX", -1) == SW_MAX);
}
END_TEST

START_TEST(test_key_invalid)
{
	ck_assert(libevdev_event_code_from_name(EV_MAX, "MAX_FAKE", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_CNT, "CNT_FAKE", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_PWR, "PWR_SOMETHING", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_ABS, "EV_ABS", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_ABS, "ABS_X", 4) == -1);
	ck_assert(libevdev_event_code_from_name(EV_ABS, "ABS_XY", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "BTN_GAMEPAD", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "BUS_PCI", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_FF_STATUS, "FF_STATUS", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_FF_STATUS, "FF_STATUS_", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_FF, "FF_STATUS", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_FF, "FF_STATUS_", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_KEY, "ID_BUS", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_SND, "SND_CNT", -1) == -1);
	ck_assert(libevdev_event_code_from_name(EV_SW, "SW_CNT", -1) == -1);
}
END_TEST

Suite *
event_code_suite(void)
{
	Suite *s = suite_create("Event codes");

	TCase *tc = tcase_create("type tests");
	tcase_add_test(tc, test_type_codes);
	tcase_add_test(tc, test_type_invalid);
	suite_add_tcase(s, tc);

	tc = tcase_create("key tests");
	tcase_add_test(tc, test_key_codes);
	tcase_add_test(tc, test_key_invalid);
	suite_add_tcase(s, tc);

	return s;
}
