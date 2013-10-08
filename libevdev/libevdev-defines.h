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

/* @file backwards-compatible defines
 *
 * #defines needed to make libevdev build on older kernels where linux/input.h
 * is missing the required defines.
 *
 * This file is NOT for general defines, only add linux/input.h defines here!
 */

#ifndef LIBEVDEV_DEFINES_H
#define LIBEVDEV_DEFINES_H

#ifndef EVIOCSCLOCKID
#define EVIOCSCLOCKID           _IOW('E', 0xa0, int)
#endif

#ifndef EVIOCGMTSLOTS
#define EVIOCGMTSLOTS(len)      _IOC(_IOC_READ, 'E', 0x0a, len)
#endif

#ifndef EVIOCGPROP
#define EVIOCGPROP(len)         _IOC(_IOC_READ, 'E', 0x09, len)
#endif

#ifndef ABS_MT_TOOL_Y
#define ABS_MT_TOOL_Y 0x3d
#endif

#ifndef SYN_DROPPED
#define SYN_DROPPED 3
#endif

#ifndef INPUT_PROP_MAX
#define INPUT_PROP_MAX 0x1f
#define INPUT_PROP_CNT (INPUT_PROP_MAX + 1)
#endif

#ifndef REP_CNT
#define REP_CNT                 (REP_MAX+1)
#endif

#endif /* LIBEVDEV_DEFINES_H */

