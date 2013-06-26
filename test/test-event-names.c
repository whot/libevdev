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

START_TEST(test_limits)
{
	ck_assert(libevdev_get_event_type_name(EV_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_ABS, ABS_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_REL, REL_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_KEY, KEY_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_LED, LED_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_SW, SW_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_MSC, MSC_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_SND, SND_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_REP, REP_MAX + 1) == NULL);
	ck_assert(libevdev_get_event_code_name(EV_FF, FF_MAX + 1) == NULL);
}
END_TEST

START_TEST(test_syn_max)
{
	ck_assert_msg(libevdev_get_event_code_name(EV_SYN, 4) == NULL,
			"If this test fails, update SYN_MAX and implement new functionality");
}
END_TEST



START_TEST(test_type_name)
{
	ck_assert_str_eq(libevdev_get_event_type_name(EV_SYN), "EV_SYN");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_REL), "EV_REL");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_ABS), "EV_ABS");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_MSC), "EV_MSC");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_SW),  "EV_SW");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_LED), "EV_LED");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_SND), "EV_SND");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_REP), "EV_REP");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_FF),  "EV_FF");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_PWR), "EV_PWR");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_FF_STATUS), "EV_FF_STATUS");
	ck_assert_str_eq(libevdev_get_event_type_name(EV_MAX), "EV_MAX");
}
END_TEST

START_TEST(test_code_abs_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_ABS, ABS_X), "ABS_X");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_ABS, ABS_Y), "ABS_Y");

	ck_assert_str_eq(libevdev_get_event_code_name(EV_ABS, ABS_MT_SLOT), "ABS_MT_SLOT");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_ABS, ABS_MISC), "ABS_MISC");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_ABS, ABS_MAX), "ABS_MAX");

	ck_assert(libevdev_get_event_code_name(EV_ABS, ABS_MAX - 1) == NULL);

}
END_TEST

START_TEST(test_code_rel_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_REL, REL_X), "REL_X");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_REL, REL_Y), "REL_Y");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_REL, REL_MISC), "REL_MISC");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_REL, REL_MAX), "REL_MAX");

	ck_assert(libevdev_get_event_code_name(EV_REL, REL_MAX - 1) == NULL);

}
END_TEST

START_TEST(test_code_key_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_RESERVED), "KEY_RESERVED");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_ESC), "KEY_ESC");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_1), "KEY_1");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_2), "KEY_2");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_UNKNOWN), "KEY_UNKNOWN");

	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_0), "BTN_0");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_LEFT), "BTN_LEFT");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_TRIGGER), "BTN_TRIGGER");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_A), "BTN_A");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_TOOL_PEN), "BTN_TOOL_PEN");

	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_TOUCHPAD_TOGGLE), "KEY_TOUCHPAD_TOGGLE");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_TRIGGER_HAPPY), "BTN_TRIGGER_HAPPY1");

	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_MAX), "KEY_MAX");
	ck_assert(libevdev_get_event_code_name(EV_KEY, KEY_MAX - 1) == NULL);

	/* special cases that resolve to something else */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_HANGUEL), "KEY_HANGEUL");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, KEY_SCREENLOCK), "KEY_COFFEE");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_MISC), "BTN_0");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_MOUSE), "BTN_LEFT");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_JOYSTICK), "BTN_TRIGGER");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_GAMEPAD), "BTN_A");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_DIGI), "BTN_TOOL_PEN");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_WHEEL), "BTN_GEAR_DOWN");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_KEY, BTN_TRIGGER_HAPPY), "BTN_TRIGGER_HAPPY1");

}
END_TEST

START_TEST(test_code_led_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_LED, LED_NUML), "LED_NUML");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_LED, LED_KANA), "LED_KANA");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_LED, LED_MAX), "LED_MAX");

	ck_assert(libevdev_get_event_code_name(EV_LED, LED_MAX - 1) == NULL);

}
END_TEST

START_TEST(test_code_snd_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SND, SND_CLICK), "SND_CLICK");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SND, SND_TONE), "SND_TONE");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SND, SND_MAX), "SND_MAX");

	ck_assert(libevdev_get_event_code_name(EV_SND, SND_MAX - 1) == NULL);

}
END_TEST

START_TEST(test_code_msc_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_MSC, MSC_SERIAL), "MSC_SERIAL");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_MSC, MSC_RAW), "MSC_RAW");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_MSC, MSC_TIMESTAMP), "MSC_TIMESTAMP");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_MSC, MSC_MAX), "MSC_MAX");

	ck_assert(libevdev_get_event_code_name(EV_MSC, MSC_MAX - 1) == NULL);

}
END_TEST

START_TEST(test_code_sw_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SW, SW_LID), "SW_LID");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SW, SW_RFKILL_ALL), "SW_RFKILL_ALL");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SW, SW_LINEIN_INSERT), "SW_LINEIN_INSERT");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SW, SW_MAX), "SW_MAX");

	ck_assert(libevdev_get_event_code_name(EV_SW, SW_MAX - 1) == NULL);

}
END_TEST

START_TEST(test_code_ff_name)
{
	/* pick out a few only */
	ck_assert_str_eq(libevdev_get_event_code_name(EV_FF, FF_STATUS_STOPPED), "FF_STATUS_STOPPED");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_FF, FF_FRICTION), "FF_FRICTION");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_FF, FF_CUSTOM), "FF_CUSTOM");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_FF, FF_MAX), "FF_MAX");

	ck_assert(libevdev_get_event_code_name(EV_FF, FF_MAX - 1) == NULL);

}
END_TEST

START_TEST(test_code_syn_name)
{
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SYN, SYN_REPORT), "SYN_REPORT");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SYN, SYN_CONFIG), "SYN_CONFIG");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SYN, SYN_MT_REPORT), "SYN_MT_REPORT");
	ck_assert_str_eq(libevdev_get_event_code_name(EV_SYN, SYN_DROPPED), "SYN_DROPPED");

	/* there is no SYN_MAX */
}
END_TEST

START_TEST(test_prop_name)
{
	ck_assert_str_eq(libevdev_get_input_prop_name(INPUT_PROP_POINTER), "INPUT_PROP_POINTER");
	ck_assert_str_eq(libevdev_get_input_prop_name(INPUT_PROP_DIRECT), "INPUT_PROP_DIRECT");
	ck_assert_str_eq(libevdev_get_input_prop_name(INPUT_PROP_BUTTONPAD), "INPUT_PROP_BUTTONPAD");
	ck_assert_str_eq(libevdev_get_input_prop_name(INPUT_PROP_SEMI_MT), "INPUT_PROP_SEMI_MT");
	ck_assert_str_eq(libevdev_get_input_prop_name(INPUT_PROP_MAX), "INPUT_PROP_MAX");

	ck_assert(libevdev_get_input_prop_name(INPUT_PROP_MAX - 1) == NULL);
}
END_TEST

Suite *
event_name_suite(void)
{
	Suite *s = suite_create("Event names");

	TCase *tc_limits = tcase_create("type limits");
	tcase_add_test(tc_limits, test_limits);
	tcase_add_test(tc_limits, test_syn_max);
	suite_add_tcase(s, tc_limits);

	TCase *tc_tname = tcase_create("type names");
	tcase_add_test(tc_tname, test_type_name);
	suite_add_tcase(s, tc_tname);

	TCase *tc_cname = tcase_create("code names");
	tcase_add_test(tc_tname, test_code_abs_name);
	tcase_add_test(tc_tname, test_code_rel_name);
	tcase_add_test(tc_tname, test_code_key_name);
	tcase_add_test(tc_tname, test_code_led_name);
	tcase_add_test(tc_tname, test_code_snd_name);
	tcase_add_test(tc_tname, test_code_msc_name);
	tcase_add_test(tc_tname, test_code_sw_name);
	tcase_add_test(tc_tname, test_code_ff_name);
	tcase_add_test(tc_tname, test_code_syn_name);
	suite_add_tcase(s, tc_cname);

	TCase *tc_pname = tcase_create("prop names");
	tcase_add_test(tc_pname, test_prop_name);
	suite_add_tcase(s, tc_pname);

	return s;
}

