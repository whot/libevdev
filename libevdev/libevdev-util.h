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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <config.h>
#include "libevdev-int.h"

static inline int
bit_is_set(const unsigned long *array, int bit)
{
    return !!(array[bit / LONG_BITS] & (1LL << (bit % LONG_BITS)));
}

static inline void
set_bit(unsigned long *array, int bit)
{
    array[bit / LONG_BITS] |= (1LL << (bit % LONG_BITS));
}

static inline void
clear_bit(unsigned long *array, int bit)
{
    array[bit / LONG_BITS] &= ~(1LL << (bit % LONG_BITS));
}

static inline void
set_bit_state(unsigned long *array, int bit, int state)
{
	if (state)
		set_bit(array, bit);
	else
		clear_bit(array, bit);
}

static unsigned int
type_to_mask_const(const struct libevdev *dev, unsigned int type, const unsigned long **mask)
{
	unsigned int max;

	switch(type) {
		case EV_ABS:
			*mask = dev->abs_bits;
			max = ABS_MAX;
			break;
		case EV_REL:
			*mask = dev->rel_bits;
			max = REL_MAX;
			break;
		case EV_KEY:
			*mask = dev->key_bits;
			max = KEY_MAX;
			break;
		case EV_LED:
			*mask = dev->led_bits;
			max = LED_MAX;
			break;
		default:
		     return 0;
	}

	return max;
}

static unsigned int
type_to_mask(struct libevdev *dev, unsigned int type, unsigned long **mask)
{
	unsigned int max;

	switch(type) {
		case EV_ABS:
			*mask = dev->abs_bits;
			max = ABS_MAX;
			break;
		case EV_REL:
			*mask = dev->rel_bits;
			max = REL_MAX;
			break;
		case EV_KEY:
			*mask = dev->key_bits;
			max = KEY_MAX;
			break;
		case EV_LED:
			*mask = dev->led_bits;
			max = LED_MAX;
			break;
		default:
		     return 0;
	}

	return max;
}

#endif
