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

#ifndef LIBEVDEV_H
#define LIBEVDEV_H

#include <linux/input.h>
#include <stdarg.h>

/**
 * @mainpage
 *
 * **libevdev** is a library for handling evdev kernel devices. It abstracts
 * the ioctls through type-safe interfaces and provides functions to change
 * the appearance of the device.
 *
 * Development of libevdev is discussed on
 * [input-tools@lists.freedesktop.org](http://lists.freedesktop.org/mailman/listinfo/input-tools)
 * Please submit patches, questions or general comments there.
 *
 * Handling events and SYN_DROPPED
 * ===============================
 *
 * libevdev provides an interface for handling events, including most notably
 * SYN_DROPPED events. SYN_DROPPED events are sent by the kernel when the
 * process does not read events fast enough and the kernel is forced to drop
 * some events. This causes the device to get out of sync with the process'
 * view of it. libevdev handles this by telling the caller that a SYN_DROPPED
 * has been received and that the state of the device is different to what is
 * to be expected. It then provides the delta between the previous state and
 * the actual state of the device as a set of events. See
 * libevdev_next_event() for more information.
 *
 * Signal safety
 * =============
 *
 * libevdev is signal-safe for the majority of its operations. Check the API
 * documentation to make sure, unless explicitly stated a call is <b>not</b>
 * signal safe.
 *
 * Device handling
 * ===============
 *
 * libevdev does not attempt duplicate detection. Initializing two libevdev
 * devices for the same fd is valid and behaves the same as for two different
 * devices.
 *
 * libevdev does not handle the file descriptors directly, it merely uses
 * them. The caller is responsible for opening the file descriptors, setting
 * them to O_NONBLOCK and handling permissions.
 *
 * Where does libevdev sit?
 * ========================
 *
 * libevdev is essentially a `read(2)` on steroids for `/dev/input/eventX
 * devices. It sits below the process that handles input events, in between
 * the kernel and that process. In the simplest case, e.g. an evtest-like tool
 * the stack would look like this:
 *
 *      kernel → libevdev → evtest
 *
 * For X.Org input modules, the stack would look like this:
 *
 *      kernel → libevdev → xf86-input-evdev → X server → X client
 *
 * For Weston/Wayland, the stack would look like this:
 *
 *      kernel → libevdev → Weston → Wayland client
 *
 * libevdev does **not** have knowledge of X clients or Wayland clients, it is
 * too low in the stack.
 *
 * Example
 * =======
 * Below is a simple example that shows how libevdev could be used. This example
 * opens a device, checks for relative axes and a left mouse button and if it
 * finds them monitors the device to print the event.
 *
 * @code
 *      struct libevdev *dev = NULL;
 *      int fd;
 *      int rc = 1;
 *
 *      fd = open("/dev/input/event0", O_RDONLY|O_NONBLOCK);
 *      rc = libevdev_new_from_fd(fd, &dev);
 *      if (rc < 0) {
 *              fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
 *              exit(1);
 *      }
 *      printf("Input device name: \"%s\"\n", libevdev_get_name(dev));
 *      printf("Input device ID: bus %#x vendor %#x product %#x\n",
 *             libevdev_get_bustype(dev),
 *             libevdev_get_vendor_id(dev),
 *             libevdev_get_product_id(dev));
 *      if (!libevdev_has_event_type(dev, EV_REL) ||
 *          !libevdev_has_event_code(dev, EV_KEY, BTN_LEFT)) {
 *              printf("This device does not look like a mouse\n");
 *              exit(1);
 *      }
 *
 *      do {
 *              struct input_event ev;
 *              rc = libevdev_next_event(dev, LIBEVDEV_READ_NORMAL, &ev);
 *              if (rc == 0)
 *                      printf("Event: %s %s %d\n",
 *                             libevdev_get_event_type_name(ev.type),
 *                             libevdev_get_event_code_name(ev.type, ev.code),
 *                             ev.value);
 *      } while (rc == 1 || rc == 0 || rc == -EAGAIN);
 * @endcode
 *
 * A more complete example is available with the libevdev-events tool here:
 * http://cgit.freedesktop.org/libevdev/tree/tools/libevdev-events.c
 *
 * libevdev internal test suite
 * ============================
 *
 * libevdev's internal test suite uses the
 * [Check unit testing framework](http://check.sourceforge.net/). Tests are
 * divided into test suites and test cases. Most tests create a uinput device,
 * so you'll need to run as root.
 *
 * To run a specific suite only:
 *
 *     export CK_RUN_SUITE="suite name"
 *
 * To run a specific test case only:
 *
 *     export CK_RUN_TEST="test case name"
 *
 * To get a list of all suites or tests:
 *
 *     git grep "suite_create"
 *     git grep "tcase_create"
 *
 * By default, check forks, making debugging harder. Run gdb as below to avoid
 * forking.
 *
 *     sudo CK_FORK=no CK_RUN_TEST="test case name" gdb ./test/test-libevdev
 *
 * A special target `make gcov-report.txt` exists that runs gcov and leaves a
 * `libevdev.c.gcov` file. Check that for test coverage.
 *
 * `make check` is hooked up to run the test and gcov (again, needs root).
 *
 * The test suite creates a lot of devices, very quickly. Add the following
 * xorg.conf.d snippet to avoid the devices being added as X devices (at the
 * time of writing, mutter can't handle these devices and exits after getting
 * a BadDevice error).
 *
 *     $ cat /etc/X11/xorg.conf.d/99-ignore-libevdev-devices.conf
 *     Section "InputClass"
 *             Identifier "Ignore libevdev test devices"
 *             MatchProduct "libevdev test device"
 *             Option "Ignore" "on"
 *     EndSection
 *
 * License information
 * ===================
 * libevdev is licensed under the
 * [X11 license](http://cgit.freedesktop.org/libevdev/tree/COPYING).
 *
 * Reporting bugs
 * ==============
 * Please report bugs in the freedesktop.org bugzilla under the libevdev product:
 * https://bugs.freedesktop.org/enter_bug.cgi?product=libevdev
 */

/**
 * @defgroup init Initialization and setup
 *
 * Initialization, initial setup and file descriptor handling.
 * These functions are the main entry points for users of libevdev, usually a
 * caller will use this series of calls:
 *
 * @code
 * struct libevdev *dev;
 * int err;
 *
 * dev = libevdev_new();
 * if (!dev)
 *         return ENOSPC;
 *
 * err = libevdev_set_fd(dev, fd);
 * if (err < 0) {
 *         printf("Failed (errno %d): %s\n", -err, strerror(-err));
 *
 * libevdev_free(dev);
 * @endcode
 *
 * libevdev_set_fd() is the central call and initializes the internal structs
 * for the device at the given fd. libevdev functions will fail if called
 * before libevdev_set_fd() unless documented otherwise.
 */

/**
 * @defgroup bits Querying device capabilities
 *
 * Abstraction functions to handle device capabilities, specificially
 * device propeties such as the name of the device and the bits
 * representing the events suppported by this device.
 *
 * The logical state returned may lag behind the physical state of the device.
 * libevdev queries the device state on libevdev_set_fd() and then relies on
 * the caller to parse events through libevdev_next_fd(). If a caller does not
 * use libevdev_next_fd(), libevdev will not update the internal state of the
 * device and thus returns outdated values.
 */

/**
 * @defgroup mt Multi-touch related functions
 * Functions for querying multi-touch-related capabilities. MT devices
 * following the kernel protocol B (using ABS_MT_SLOT) provide multiple touch
 * points through so-called slots on the same axis. The slots are enumerated,
 * a client reading from the device will first get an ABS_MT_SLOT event, then
 * the values of axes changed in this slot. Multiple slots may be provided in
 * before an EV_SYN event.
 *
 * As with @ref bits, the logical state of the device as seen by the library
 * depends on the caller using libevdev_next_event().
 */

/**
 * @defgroup kernel Modifying the appearance or capabilities of the device
 *
 * Modifying the set of events reported by this device. By default, the
 * libevdev device mirrors the kernel device, enabling only those bits
 * exported by the kernel. This set of functions enable or disable bits as
 * seen from the caller.
 *
 * Enabling an event type or code does not affect event reporting - a
 * software-enabled event will not be generated by the physical hardware.
 * Disabling an event will prevent libevdev from routing such events to the
 * caller. Enabling and disabling event types and codes is at the library
 * level and thus only affects the caller.
 *
 * If an event type or code is enabled at kernel-level, future users of this
 * device will see this event enabled. Currently there is no option of
 * disabling an event type or code at kernel-level.
 */

/**
 * @defgroup misc Miscellaneous helper functions
 *
 * Functions for printing or querying event ranges. The list of names is
 * compiled into libevdev and will not change when the kernel changes. Adding
 * or removing names requires a re-compilation of libevdev. Likewise, the max
 * for each event type is compiled in and does not check the underlying
 * kernel.
 */

/**
 * @defgroup events Event handling
 *
 * Functions to handle events and fetch the current state of the event. Generally,
 * libevdev updates its internal state as the event is processed and forwarded
 * to the caller. Thus, the libevdev state of the device should always be identical
 * to the caller's state. It may however lag behind the actual state of the device.
 */

/**
 * @ingroup init
 *
 * Opaque struct representing an evdev device.
 */
struct libevdev;

enum EvdevReadFlags {
	LIBEVDEV_READ_SYNC		= 1, /**< Process data in sync mode */
	LIBEVDEV_READ_NORMAL		= 2, /**< Process data in normal mode */
	LIBEVDEV_FORCE_SYNC		= 4, /**< Pretend the next event is a SYN_DROPPED. There is
					          no reason to ever use this except for
						  automated tests, so don't. */
	LIBEVDEV_READ_BLOCKING		= 8, /**< The fd is not in O_NONBLOCK and a read may block */
};

/**
 * @ingroup init
 *
 * Initialize a new libevdev device. This function only allocates the
 * required memory and initializes the struct to sane default values.
 * To actually hook up the device to a kernel device, use
 * libevdev_set_fd().
 *
 * Memory allocated through libevdev_new() must be released by the
 * caller with libevdev_free().
 *
 * @see libevdev_set_fd
 * @see libevdev_free
 */
struct libevdev* libevdev_new(void);

/**
 * @ingroup init
 *
 * Initialize a new libevdev device from the given fd.
 *
 * This is a shortcut for
 *
 * @code
 * int err;
 * struct libevdev *dev = libevdev_new();
 * err = libevdev_set_fd(dev, fd);
 * @endcode
 *
 * @param fd A file descriptor to the device in O_RDWR or O_RDONLY mode.
 * @param[out] dev The newly initialized evdev device.
 *
 * @return On success, zero is returned and dev is set to the newly
 * allocated struct. On failure, a negative errno is returned and the value
 * of dev is undefined.
 *
 * @see libevdev_free
 */
int libevdev_new_from_fd(int fd, struct libevdev **dev);

/**
 * @ingroup init
 *
 * Clean up and free the libevdev struct. After completion, the <code>struct
 * libevdev</code> is invalid and must not be used.
 *
 * @param dev The evdev device
 *
 * @note This function may be called before libevdev_set_fd().
 */
void libevdev_free(struct libevdev *dev);

/**
 * Logging function called by library-internal logging.
 * This function is expected to treat it's input like printf would.
 *
 * @param format printf-style format string
 * @param args List of arguments
 *
 * @see libevdev_set_log_handler
 */
typedef void (*libevdev_log_func_t)(const char *format, va_list args);

/**
 * Set a printf-style logging handler for library-internal logging. The default
 * logging function is a noop.
 *
 * @param dev The evdev device
 * @param logfunc The logging function for this device. If NULL, the current
 * logging function is unset.
 *
 * @note This function may be called before libevdev_set_fd().
 */
void libevdev_set_log_handler(struct libevdev *dev, libevdev_log_func_t logfunc);


enum EvdevGrabModes {
	LIBEVDEV_GRAB = 3,
	LIBEVDEV_UNGRAB = 4,
};

/**
 * Grab or ungrab the device through a kernel EVIOCGRAB. This prevents other
 * clients (including kernel-internal ones such as rfkill) from receiving
 * events from this device.
 *
 * This is generally a bad idea. Don't do this.
 *
 * Grabbing an already grabbed device, or ungrabbing an ungrabbed device is
 * a noop and always succeeds.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param grab If true, grab the device. Otherwise ungrab the device.
 *
 * @return 0 if the device was successfull grabbed or ungrabbed, or a
 * negative errno in case of failure.
 */
int libevdev_grab(struct libevdev *dev, int grab);

/**
 * @ingroup init
 *
 * Set the fd for this struct and initialize internal data.
 * The fd must be in O_RDONLY or O_RDWR mode.
 *
 * This function may only be called once per device. If the device changed and
 * you need to re-read a device, use libevdev_free() and libevdev_new(). If
 * you need to change the fd after closing and re-opening the same device, use
 * libevdev_change_fd().
 *
 * Unless otherwise specified, libevdev function behavior is undefined until
 * a successfull call to libevdev_set_fd().
 *
 * @param dev The evdev device
 * @param fd The file descriptor for the device
 *
 * @return 0 on success, or a negative error code on failure
 *
 * @see libevdev_change_fd
 * @see libevdev_new
 * @see libevdev_free
 */
int libevdev_set_fd(struct libevdev* dev, int fd);

/**
 * @ingroup init
 *
 * Change the fd for this device, without re-reading the actual device. If the fd
 * changes after initializing the device, for example after a VT-switch in the
 * X.org X server, this function updates the internal fd to the newly opened.
 * No check is made that new fd points to the same device. If the device has
 * changed, libevdev's behavior is undefined.
 *
 * The fd may be open in O_RDONLY or O_RDWR.
 *
 * It is an error to call this function before calling libevdev_set_fd().
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param fd The new fd
 *
 * @return 0 on success, or -1 on failure.
 *
 * @see libevdev_set_fd
 */
int libevdev_change_fd(struct libevdev* dev, int fd);

/**
 * @param dev The evdev device
 *
 * @return The previously set fd, or -1 if none had been set previously.
 * @note This function may be called before libevdev_set_fd().
 */
int libevdev_get_fd(const struct libevdev* dev);

/**
 * @ingroup events
 *
 * Get the next event from the device. This function operates in two different
 * modes: normal mode or sync mode.
 *
 * In normal mode, this function returns 0 and returns the event in the
 * parameter ev. If no events are available at this time, it returns -EAGAIN
 * and ev is undefined.
 *
 * If a SYN_DROPPED is read from the device, this function returns 1. The
 * caller should now call this function with the LIBEVDEV_READ_SYNC flag set,
 * to get the set of events that make up the device state delta. This function
 * returns 1 for each event part of that delta, until it returns -EAGAIN once
 * all events have been synced.
 *
 * If a device needs to be synced by the caller but the caller does not call
 * with the LIBEVDEV_READ_SYNC flag set, all events from the diff are dropped
 * and event processing continues as normal.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param flags Set of flags to determine behaviour. If LIBEVDEV_READ_NORMAL
 * is set, the next event is read in normal mode. If LIBEVDEV_READ_SYNC is
 * set, the next event is read in sync mode.
 * @param ev On success, set to the current event.
 * @return On failure, a negative errno is returned.
 * @retval 0 One or more events where read of the device
 * @retval -EAGAIN No events are currently available on the device
 * @retval 1 A SYN_DROPPED event was received, or a synced event was
 * returned.
 *
 * @note This function is signal-safe.
 */
int libevdev_next_event(struct libevdev *dev, unsigned int flags, struct input_event *ev);

/**
 * @ingroup events
 *
 * Check if there are events waiting for us. This does not read an event off
 * the fd and may not access the fd at all. If there are events queued
 * internally this function will return non-zero. If the internal queue is empty,
 * this function will poll the file descriptor for data.
 *
 * This is a convenience function for simple processes, most complex programs
 * are expected to use select(2) or poll(2) on the file descriptor. The kernel
 * guarantees that if data is available, it is a multiple of sizeof(struct
 * input_event), and thus calling libevdev_next_event() when select(2) or
 * poll(2) return is safe. You do not need libevdev_has_event_pending() if
 * you're using select(2) or poll(2).
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @return On failure, a negative errno is returned.
 * @retval 0 No event is currently available
 * @retval 1 One or more events are available on the fd
 *
 * @note This function is signal-safe.
 */
int libevdev_has_event_pending(struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The device name as read off the kernel device. The name is never
 * NULL but it may be the empty string.
 *
 * @note This function is signal-safe.
 */
const char* libevdev_get_name(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param name The new, non-NULL, name to assign to this device.
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
void libevdev_set_name(struct libevdev *dev, const char *name);

/**
 * @ingroup bits
 *
 * Virtual devices such as uinput devices have no phys location.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The physical location of this device, or NULL if there is none
 *
 * @note This function is signal safe.
 */
const char * libevdev_get_phys(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param phys The new, non-NULL, phys to assign to this device.
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
void libevdev_set_phys(struct libevdev *dev, const char *phys);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The unique identifier for this device, or NULL if there is none
 *
 * @note This function is signal safe.
 */
const char * libevdev_get_uniq(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param uniq The new, non-NULL, uniq to assign to this device.
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
void libevdev_set_uniq(struct libevdev *dev, const char *uniq);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The device's product ID
 *
 * @note This function is signal-safe.
 */
int libevdev_get_id_product(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param product_id The product ID to assign to this device
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
void libevdev_set_id_product(struct libevdev *dev, int product_id);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The device's vendor ID
 *
 * @note This function is signal-safe.
 */
int libevdev_get_id_vendor(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param vendor_id The vendor ID to assign to this device
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
void libevdev_set_id_vendor(struct libevdev *dev, int vendor_id);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The device's bus type
 *
 * @note This function is signal-safe.
 */
int libevdev_get_id_bustype(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param bustype The bustype to assign to this device
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
void libevdev_set_id_bustype(struct libevdev *dev, int bustype);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The device's firmware version
 *
 * @note This function is signal-safe.
 */
int libevdev_get_id_version(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param version The version to assign to this device
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
void libevdev_set_id_version(struct libevdev *dev, int version);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The driver version for this device
 *
 * @note This function is signal-safe.
 */
int libevdev_get_driver_version(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param prop The input property to query for, one of INPUT_PROP_...
 *
 * @return 1 if the device provides this input property, or 0 otherwise.
 *
 * @note This function is signal-safe
 */
int libevdev_has_property(const struct libevdev *dev, unsigned int prop);

/**
 * @ingroup kernel
 *
 * @param dev The evdev device
 * @param prop The input property to enable, one of INPUT_PROP_...
 *
 * @return 0 on success or -1 on failure
 *
 * @note This function may be called before libevdev_set_fd(). A call to
 * libevdev_set_fd() will overwrite any previously set value.
 */
int libevdev_enable_property(struct libevdev *dev, unsigned int prop);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type to query for, one of EV_SYN, EV_REL, etc.
 *
 * @return 1 if the device supports this event type, or 0 otherwise.
 *
 * @note This function is signal-safe.
 */
int libevdev_has_event_type(const struct libevdev *dev, unsigned int type);

/**
 * @ingroup bits
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type for the code to query (EV_SYN, EV_REL, etc.)
 * @param code The event code to query for, one of ABS_X, REL_X, etc.
 *
 * @return 1 if the device supports this event type and code, or 0 otherwise.
 *
 * @note This function is signal-safe.
 */
int libevdev_has_event_code(const struct libevdev *dev, unsigned int type, unsigned int code);

/**
 * @ingroup bits
 *
 * Get the minimum axis value for the given axis, as advertised by the kernel.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param code The EV_ABS event code to query for, one of ABS_X, ABS_Y, etc.
 *
 * @return axis minimum for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_minimum(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * Get the maximum axis value for the given axis, as advertised by the kernel.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param code The EV_ABS event code to query for, one of ABS_X, ABS_Y, etc.
 *
 * @return axis maximum for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_maximum(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * Get the axis fuzz for the given axis, as advertised by the kernel.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param code The EV_ABS event code to query for, one of ABS_X, ABS_Y, etc.
 *
 * @return axis fuzz for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_fuzz(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * Get the axis flat for the given axis, as advertised by the kernel.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param code The EV_ABS event code to query for, one of ABS_X, ABS_Y, etc.
 *
 * @return axis flat for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_flat(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * Get the axis resolution for the given axis, as advertised by the kernel.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param code The EV_ABS event code to query for, one of ABS_X, ABS_Y, etc.
 *
 * @return axis resolution for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_resolution(const struct libevdev *dev, unsigned int code);

/**
 * @ingroup bits
 *
 * Get the axis info for the given axis, as advertised by the kernel.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param code The EV_ABS event code to query for, one of ABS_X, ABS_Y, etc.
 *
 * @return The input_absinfo for the given code, or NULL if the device does
 * not support this event code.
 */
const struct input_absinfo* libevdev_get_abs_info(const struct libevdev *dev, unsigned int code);

/**
 * @ingroup bits
 *
 * Behaviour of this function is undefined if the device does not provide
 * the event.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type for the code to query (EV_SYN, EV_REL, etc.)
 * @param code The event code to query for, one of ABS_X, REL_X, etc.
 *
 * @return The current value of the event.
 *
 * @note This function is signal-safe.
 * @note The value for ABS_MT_ events is undefined, use
 * libevdev_get_slot_value() instead
 *
 * @see libevdev_get_slot_value
 */
int libevdev_get_event_value(const struct libevdev *dev, unsigned int type, unsigned int code);

/**
 * @ingroup bits
 *
 * Fetch the current value of the event type. This is a shortcut for
 *
 * @code
 *   if (libevdev_has_event_type(dev, t) && libevdev_has_event_code(dev, t, c))
 *       val = libevdev_get_event_value(dev, t, c);
 * @endcode
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type for the code to query (EV_SYN, EV_REL, etc.)
 * @param code The event code to query for, one of ABS_X, REL_X, etc.
 * @param[out] value The current value of this axis returned.
 *
 * @return If the device supports this event type and code, the return value is
 * non-zero and value is set to the current value of this axis. Otherwise,
 * zero is returned and value is unmodified.
 *
 * @note This function is signal-safe.
 * @note The value for ABS_MT_ events is undefined, use
 * libevdev_fetch_slot_value() instead
 *
 * @see libevdev_fetch_slot_value
 */
int libevdev_fetch_event_value(const struct libevdev *dev, unsigned int type, unsigned int code, int *value);

/**
 * @ingroup mt
 *
 * Return the current value of the code for the given slot.
 *
 * The return value is undefined for a slot exceeding the available slots on
 * the device, for a code that is not in the permitted ABS_MT range or for a
 * device that does not have slots.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param slot The numerical slot number, must be smaller than the total number
 * of slots on this device
 * @param code The event code to query for, one of ABS_MT_POSITION_X, etc.
 *
 * @note This function is signal-safe.
 * @note The value for events other than ABS_MT_ is undefined, use
 * libevdev_fetch_value() instead
 *
 * @see libevdev_get_value
 */
int libevdev_get_slot_value(const struct libevdev *dev, unsigned int slot, unsigned int code);

/**
 * @ingroup mt
 *
 * Fetch the current value of the code for the given slot. This is a shortcut for
 *
 * @code
 *   if (libevdev_has_event_type(dev, EV_ABS) &&
 *       libevdev_has_event_code(dev, EV_ABS, c) &&
 *       slot < device->number_of_slots)
 *       val = libevdev_get_slot_value(dev, slot, c);
 * @endcode
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param slot The numerical slot number, must be smaller than the total number
 * of slots on this * device
 * @param[out] value The current value of this axis returned.
 *
 * @param code The event code to query for, one of ABS_MT_POSITION_X, etc.
 * @return If the device supports this event code, the return value is
 * non-zero and value is set to the current value of this axis. Otherwise, or
 * if the event code is not an ABS_MT_* event code, zero is returned and value
 * is unmodified.
 *
 * @note This function is signal-safe.
 */
int libevdev_fetch_slot_value(const struct libevdev *dev, unsigned int slot, unsigned int code, int *value);

/**
 * @ingroup mt
 *
 * Get the number of slots supported by this device.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return The number of slots supported, or -1 if the device does not provide
 * any slots
 *
 * @note A device may provide ABS_MT_SLOT but a total number of 0 slots. Hence
 * the return value of -1 for "device does not provide slots at all"
 */
int libevdev_get_num_slots(const struct libevdev *dev);

/**
 * @ingroup mt
 *
 * Get the currently active slot. This may differ from the value
 * an ioctl may return at this time as events may have been read off the fd
 * since changing the slot value but those events are still in the buffer
 * waiting to be processed. The returned value is the value a caller would
 * see if it were to process events manually one-by-one.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 *
 * @return the currently active slot (logically)
 */
int libevdev_get_current_slot(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * Change the minimum for the given EV_ABS event code, if the code exists.
 * This function has no effect if libevdev_has_event_code() returns false for
 * this code.
 */
void libevdev_set_abs_minimum(struct libevdev *dev, unsigned int code, int min);

/**
 * @ingroup kernel
 *
 * Change the maximum for the given EV_ABS event code, if the code exists.
 * This function has no effect if libevdev_has_event_code() returns false for
 * this code.
 */
void libevdev_set_abs_maximum(struct libevdev *dev, unsigned int code, int max);

/**
 * @ingroup kernel
 *
 * Change the fuzz for the given EV_ABS event code, if the code exists.
 * This function has no effect if libevdev_has_event_code() returns false for
 * this code.
 */
void libevdev_set_abs_fuzz(struct libevdev *dev, unsigned int code, int fuzz);

/**
 * @ingroup kernel
 *
 * Change the flat for the given EV_ABS event code, if the code exists.
 * This function has no effect if libevdev_has_event_code() returns false for
 * this code.
 */
void libevdev_set_abs_flat(struct libevdev *dev, unsigned int code, int flat);

/**
 * @ingroup kernel
 *
 * Change the resolution for the given EV_ABS event code, if the code exists.
 * This function has no effect if libevdev_has_event_code() returns false for
 * this code.
 */
void libevdev_set_abs_resolution(struct libevdev *dev, unsigned int code, int resolution);

/**
 * @ingroup kernel
 *
 * Change the abs info for the given EV_ABS event code, if the code exists.
 * This function has no effect if libevdev_has_event_code() returns false for
 * this code.
 */
void libevdev_set_abs_info(struct libevdev *dev, unsigned int code, const struct input_absinfo *abs);

/**
 * @ingroup kernel
 *
 * Forcibly enable an event type on this device, even if the underlying
 * device does not support it. While this cannot make the device actually
 * report such events, it will now return true for libevdev_has_event_type().
 *
 * This is a local modification only affecting only this representation of
 * this device.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type to enable (EV_ABS, EV_KEY, ...)
 *
 * @return 0 on success or -1 otherwise
 *
 * @see libevdev_has_event_type
 */
int libevdev_enable_event_type(struct libevdev *dev, unsigned int type);

/**
 * @ingroup kernel
 *
 * Forcibly disable an event type on this device, even if the underlying
 * device provides it. This effectively mutes the respective set of
 * events. libevdev will filter any events matching this type and none will
 * reach the caller. libevdev_has_event_type() will return false for this
 * type.
 *
 * In most cases, a caller likely only wants to disable a single code, not
 * the whole type. Use libevdev_disable_event_code() for that.
 *
 * Disabling EV_SYN will not work. Don't shoot yourself in the foot.
 * It hurts.
 *
 * This is a local modification only affecting only this representation of
 * this device.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type to disable (EV_ABS, EV_KEY, ...)
 *
 * @return 0 on success or -1 otherwise
 *
 * @see libevdev_has_event_type
 * @see libevdev_disable_event_type
 */
int libevdev_disable_event_type(struct libevdev *dev, unsigned int type);

/**
 * @ingroup kernel
 *
 * Forcibly enable an event type on this device, even if the underlying
 * device does not support it. While this cannot make the device actually
 * report such events, it will now return true for libevdev_has_event_code().
 *
 * The last argument depends on the type and code:
 * - If type is EV_ABS, data must be a pointer to a struct input_absinfo
 * containing the data for this axis.
 * - For all other types, the argument must be NULL.
 *
 * This function calls libevdev_enable_event_type() if necessary.
 *
 * This is a local modification only affecting only this representation of
 * this device.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type to enable (EV_ABS, EV_KEY, ...)
 * @param code The event code to enable (ABS_X, REL_X, etc.)
 * @param data If type is EV_ABS, data points to a struct input_absinfo. If type is EV_REP, data
 * points to an integer. Otherwise, data must be NULL.
 *
 * @return 0 on success or -1 otherwise
 *
 * @see libevdev_enable_event_type
 */
int libevdev_enable_event_code(struct libevdev *dev, unsigned int type, unsigned int code, const void *data);

/**
 * @ingroup kernel
 *
 * Forcibly disable an event code on this device, even if the underlying
 * device provides it. This effectively mutes the respective set of
 * events. libevdev will filter any events matching this type and code and
 * none will reach the caller. libevdev_has_event_code() will return false for
 * this code.
 *
 * Disabling all event codes for a given type will not disable the event
 * type. Use libevdev_disable_event_type() for that.
 *
 * This is a local modification only affecting only this representation of
 * this device.
 *
 * Disabling EV_SYN will not work. Don't shoot yourself in the foot.
 * It hurts.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param type The event type to disable (EV_ABS, EV_KEY, ...)
 * @param code The event code to disable (ABS_X, REL_X, etc.)
 *
 * @return 0 on success or -1 otherwise
 *
 * @see libevdev_has_event_code
 * @see libevdev_disable_event_type
 */
int libevdev_disable_event_code(struct libevdev *dev, unsigned int type, unsigned int code);

/**
 * @ingroup kernel
 *
 * Set the device's EV_ABS axis to the value defined in the abs
 * parameter. This will be written to the kernel.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param code The EV_ABS event code to modify, one of ABS_X, ABS_Y, etc.
 * @param abs Axis info to set the kernel axis to
 *
 * @return zero on success, or a negative errno on failure
 *
 * @see libevdev_enable_event_code
 */
int libevdev_kernel_set_abs_info(struct libevdev *dev, unsigned int code, const struct input_absinfo *abs);

/**
 * @ingroup misc
 *
 * Helper function to check if an event is of a specific type. This is
 * virtually the same as:
 *
 *      ev->type == type
 *
 * with the exception that some sanity checks are performed to ensure type
 * is valid.
 *
 * @param ev The input event to check
 * @param type Input event type to compare the event against (EV_REL, EV_ABS,
 * etc.)
 *
 * @return 1 if the event type matches the given type, 0 otherwise (or if
 * type is invalid)
 */
int libevdev_is_event_type(const struct input_event *ev, unsigned int type);

/**
 * @ingroup misc
 *
 * Helper function to check if an event is of a specific type and code. This
 * is virtually the same as:
 *
 *      ev->type == type && ev->code == code
 *
 * with the exception that some sanity checks are performed to ensure type and
 * code are valid.
 *
 * @param ev The input event to check
 * @param type Input event type to compare the event against (EV_REL, EV_ABS,
 * etc.)
 * @param code Input event code to compare the event against (ABS_X, REL_X,
 * etc.)
 *
 * @return 1 if the event type matches the given type and code, 0 otherwise
 * (or if type/code are invalid)
 */
int libevdev_is_event_code(const struct input_event *ev, unsigned int type, unsigned int code);

/**
 * @ingroup misc
 *
 * @param type The event type to return the name for.
 *
 * @return The name of the given event type (e.g. EV_ABS) or NULL for an
 * invalid type
 *
 * @note The list of names is compiled into libevdev. If the kernel adds new
 * defines for new properties libevdev will not automatically pick these up.
 */
const char * libevdev_get_event_type_name(unsigned int type);
/**
 * @ingroup misc
 *
 * @param type The event type for the code to query (EV_SYN, EV_REL, etc.)
 * @param code The event code to return the name for (e.g. ABS_X)
 *
 * @return The name of the given event code (e.g. ABS_X) or NULL for an
 * invalid type or code
 *
 * @note The list of names is compiled into libevdev. If the kernel adds new
 * defines for new properties libevdev will not automatically pick these up.
 */
const char * libevdev_get_event_code_name(unsigned int type, unsigned int code);

/**
 * @ingroup misc
 *
 * @param prop The input prop to return the name for (e.g. INPUT_PROP_BUTTONPAD)
 *
 * @return The name of the given input prop (e.g. INPUT_PROP_BUTTONPAD) or NULL for an
 * invalid property
 *
 * @note The list of names is compiled into libevdev. If the kernel adds new
 * defines for new properties libevdev will not automatically pick these up.
 * @note On older kernels input properties may not be defined and
 * libevdev_get_input_prop_name() will always return NULL
 */
const char* libevdev_get_property_name(unsigned int prop);

/**
 * @ingroup misc
 *
 * @param type The event type to return the maximum for (EV_ABS, EV_REL, etc.). No max is defined for
 * EV_SYN.
 *
 * @return The max value defined for the given event type, e.g. ABS_MAX for a type of EV_ABS, or -1
 * for an invalid type.
 *
 * @note The max value is compiled into libevdev. If the kernel changes the
 * max value, libevdev will not automatically pick these up.
 */
int libevdev_get_event_type_max(unsigned int type);

/**
 * @ingroup bits
 *
 * Get the repeat delay and repeat period values for this device.
 *
 * @param dev The evdev device, already initialized with libevdev_set_fd()
 * @param delay If not null, set to the repeat delay value
 * @param period If not null, set to the repeat period value
 *
 * @return 0 on success, -1 if this device does not have repeat settings.
 *
 * @note This function is signal-safe
 */
int libevdev_get_repeat(struct libevdev *dev, int *delay, int *period);


/********* DEPRECATED SECTION *********/
#if defined(__GNUC__) && __GNUC__ >= 4
#define LIBEVDEV_DEPRECATED __attribute__ ((deprecated))
#else
#define LIBEVDEV_DEPRECATED
#endif

/* replacement: libevdev_get_abs_minimum */
int libevdev_get_abs_min(const struct libevdev *dev, unsigned int code) LIBEVDEV_DEPRECATED;
/* replacement: libevdev_get_abs_maximum */
int libevdev_get_abs_max(const struct libevdev *dev, unsigned int code) LIBEVDEV_DEPRECATED;

/* replacement: libevdev_get_property_name */
const char* libevdev_get_input_prop_name(unsigned int prop) LIBEVDEV_DEPRECATED;

int libevdev_get_product_id(const struct libevdev *dev) LIBEVDEV_DEPRECATED;
int libevdev_get_vendor_id(const struct libevdev *dev) LIBEVDEV_DEPRECATED;
int libevdev_get_bustype(const struct libevdev *dev) LIBEVDEV_DEPRECATED;
int libevdev_get_version(const struct libevdev *dev) LIBEVDEV_DEPRECATED;

/* replacement: libevdev_kernel_set_abs_info */
int libevdev_kernel_set_abs_value(struct libevdev *dev, unsigned int code, const struct input_absinfo *abs) LIBEVDEV_DEPRECATED;


/**************************************/
#endif /* LIBEVDEV_H */
