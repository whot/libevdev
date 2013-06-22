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

#ifndef libevdev_H
#define libevdev_H

#include <config.h>
#include <linux/input.h>
#include <stdarg.h>

/**
 * @mainpage
 *
 * libevdev is a library for handling evdev kernel devices. It abstracts
 * the ioctls through type-safe interfaces and provides functions to change
 * the appearance of the device.
 *
 * libevdev provides an interface for handling events, including most notably
 * SYN_DROPPED events.
 *
 * libevdev is signal-safe for the majority of its operations. Check the API
 * documentation to make sure, unless explicitly stated a call is <b>not</b>
 * signal safe.
 *
 * libevdev does not attempt duplicate detection. Initializing two libevdev
 * devices for the same fd is valid and behaves the same as for two different
 * devices.
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
 *    return ENOSPC;

 * err = libevdev_set_fd(dev, fd);
 * if (err < 0) {
 *	printf("Failed (errno %d): %s\n", -err, strerror(-err));
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
 */

/**
 * @defgroup mt Multi-touch related functions
 * Functions for querying multi-touch-related capabilities.
 */

/**
 * @defgroup kernel Modifying the appearance or capabilities of the device
 *
 * Modifying the set of events reported by this device.
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
 * @ingroup init
 *
 * Opaque struct representing an evdev device.
 */
struct libevdev;

enum EvdevReadFlags {
    LIBEVDEV_READ_SYNC		= 1, /**< Process data in sync mode */
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
 * <pre>
 * int err;
 * struct libevdev *dev = libevdev_new();
 * err = libevdev_set_fd(dev, fd);
 * </pre>
 *
 * @param fd A file descriptor to the device in O_RDWR or O_RDONLY mode.
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
 * Set a printf-style logging handler for library-internal logging.
 *
 * @note This function may be called before libevdev_set_fd.
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
 * @param fd The new fd
 *
 * @return 0 on success, or -1 on failure.
 *
 * @see libevdev_set_fd
 */
int libevdev_change_fd(struct libevdev* dev, int fd);

/**
 *
 * @return The previously set fd, or -1 if none had been set previously.
 * @note This function may be called before libevdev_set_fd.
 */
int libevdev_get_fd(const struct libevdev* dev);

/**
 * @ingroup events
 *
 * Get the next event from the device.
 *
 * In normal mode, this function returns 0 and returns the event in the
 * parameter ev. If no events are available at this time, it returns -EAGAIN
 * and ev is undefined.
 *
 * If a SYN_DROPPED is read from the device, this function returns 1. The
 * caller should now call this function with the ER_SYNC flag set, to get
 * the set of events that make up the device state diff. This function
 * returns 1 for each event part of that diff, until it returns -EAGAIN once
 * all events have been synced.
 *
 * If a device needs to be synced by the caller but the caller does not call
 * with the ER_SYNC flag set, all events from the diff are dropped and event
 * processing continues as normal.
 *
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
 * @ingroup bits
 *
 * @return The device name as read off the kernel device
 *
 * @note This function is signal-safe.
 */
const char* libevdev_get_name(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * Virtual devices such as uinput devices have no phys location.
 *
 * @return The physical location of this device, or NULL if there is none
 *
 * @note This function is signal safe.
 */
const char * libevdev_get_phys(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @return The unique identifier for this device, or NULL if there is none
 *
 * @note This function is signal safe.
 */
const char * libevdev_get_uniq(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @return The device's product ID
 *
 * @note This function is signal-safe.
 */
int libevdev_get_product_id(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @return The device's vendor ID
 *
 * @note This function is signal-safe.
 */
int libevdev_get_vendor_id(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @return The device's bus type
 *
 * @note This function is signal-safe.
 */
int libevdev_get_bustype(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @return The device's firmware version
 *
 * @note This function is signal-safe.
 */
int libevdev_get_version(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @return The driver version for this device
 *
 * @note This function is signal-safe.
 */
int libevdev_get_driver_version(const struct libevdev *dev);

/**
 * @ingroup bits
 *
 * @return 1 if the device supports this event type, or 0 otherwise.
 *
 * @note This function is signal-safe
 */
int libevdev_has_property(const struct libevdev *dev, unsigned int prop);

/**
 * @ingroup bits
 *
 * @return 1 if the device supports this event type, or 0 otherwise.
 *
 * @note This function is signal-safe.
 */
int libevdev_has_event_type(const struct libevdev *dev, unsigned int type);

/**
 * @ingroup bits
 *
 * @return 1 if the device supports this event type, or 0 otherwise.
 *
 * @note This function is signal-safe.
 */
int libevdev_has_event_code(const struct libevdev *dev, unsigned int type, unsigned int code);

/**
 * @ingroup bits
 *
 * @return axis minimum for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_min(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * @return axis maximum for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_max(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * @return axis fuzz for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_fuzz(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * @return axis flat for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_flat(const struct libevdev *dev, unsigned int code);
/**
 * @ingroup bits
 *
 * @return axis resolution for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_resolution(const struct libevdev *dev, unsigned int code);

/**
 * @ingroup bits
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
 * @return The current value of the event.
 *
 * @note This function is signal-safe.
 * @note The value for ABS_MT_ events is undefined, use
 * libevdev_get_slot_value instead
 *
 * @see libevdev_get_slot_value
 */
int libevdev_get_event_value(const struct libevdev *dev, unsigned int type, unsigned int code);

/**
 * @ingroup bits
 *
 * Fetch the current value of the event type. This is a shortcut for
 *
 * <pre>
 *   if (libevdev_has_event_type(dev, t) && libevdev_has_event_code(dev, t, c))
 *       val = libevdev_get_event_value(dev, t, c);
 * </pre>
 *
 * @return non-zero if the device supports this event code, or zero
 * otherwise. On return of zero, value is unmodified.
 *
 * @note This function is signal-safe.
 * @note The value for ABS_MT_ events is undefined, use
 * libevdev_fetch_slot_value instead
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
 * the device, or for a device that does not have slots.
 *
 * @note This function is signal-safe.
 * @note The value for events other than ABS_MT_ is undefined, use
 * libevdev_fetch_value instead
 *
 * @see libevdev_get_value
 */
int libevdev_get_slot_value(const struct libevdev *dev, unsigned int slot, unsigned int code);

/**
 * @ingroup mt
 *
 * Fetch the current value of the code for the given slot. This is a shortcut for
 *
 * <pre>
 *   if (libevdev_has_event_type(dev, EV_ABS) &&
 *       libevdev_has_event_code(dev, EV_ABS, c) &&
 *       slot < device->number_of_slots)
 *       val = libevdev_get_slot_value(dev, slot, c);
 * </pre>
 *
 * @return non-zero if the device supports this event code, or zero
 * otherwise. On return of zero, value is unmodified.
 *
 * @note This function is signal-safe.
 * @note The value for ABS_MT_ events is undefined, use
 * libevdev_fetch_slot_value instead
 *
 * @see libevdev_fetch_slot_value
 */
int libevdev_fetch_slot_value(const struct libevdev *dev, unsigned int slot, unsigned int code, int *value);

/**
 * @ingroup mt
 *
 * Get the number of slots supported by this device.
 *
 * Note that the slot offset may be non-zero, use libevdev_get_abs_min() or
 * libevdev_get_abs_info() to get the minimum slot number.
 *
 * @return The number of slots supported, or -1
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
 * @return the currently active slot (logically)
 */
int libevdev_get_current_slot(const struct libevdev *dev);

/**
 * @ingroup kernel
 *
 * Forcibly enable an event type on this device, even if the underlying
 * device does not support it. While this cannot make the device actually
 * report such events, it will now return true for libevdev_has_event_type.
 *
 * This is a local modification only affecting only this process and only
 * this device.
 *
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
 * device provides it, effectively muting all keys or axes. libevdev will
 * filter any events matching this type and none will reach the caller.
 * libevdev_has_event_type will return false for this type.
 *
 * In most cases, a caller likely only wants to disable a single code, not
 * the whole type. Use libevdev_disable_event_code for that.
 *
 * This is a local modification only affecting only this process and only
 * this device.
 *
 * @param type The event type to enable (EV_ABS, EV_KEY, ...)
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
 * report such events, it will now return true for libevdev_has_event_code.
 *
 * The last argument depends on the type and code:
 * - If type is EV_ABS, the vararg must be a pointer to a struct input_absinfo
 * containing the data for this axis.
 * - For all other types, the argument is ignored.
 *
 * This function calls libevdev_enable_event_type if necessary.
 *
 * This is a local modification only affecting only this process and only
 * this device.
 *
 * @param type The event type to enable (EV_ABS, EV_KEY, ...)
 * @param code The event code to enable (ABS_X, REL_X, etc.)
 * @param data Axis/key data, depending on type and code
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
 * device provides it, effectively muting this key or axis. libevdev will
 * filter any events matching this type and code and none will reach the
 * caller.
 * libevdev_has_event_code will return false for this code combination.
 *
 * Disabling all event codes for a given type will not disable the event
 * type. Use libevdev_disable_event_type for that.
 *
 * This is a local modification only affecting only this process and only
 * this device.
 *
 * @param type The event type to enable (EV_ABS, EV_KEY, ...)
 * @param code The event code to enable (ABS_X, REL_X, etc.)
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
 * Forcibly enable an event type on this device, even if the underlying
 * device does not support it. While this cannot make the device actually
 * report such events, it will now return true for libevdev_has_event_code.
 *
 * This will be written to the kernel.
 *
 * This cannot be undone, the kernel only allows to enable axes, not disable
 * them.
 *
 * This function calls libevdev_kernel_enable_event_type if necessary.
 *
 * @param type The event type to enable (EV_ABS, EV_KEY, ...)
 * @param code The event code to enable (ABS_X, REL_X, etc.)
 */
int libevdev_kernel_enable_event_type(struct libevdev *dev, unsigned int type);

/**
 * @ingroup kernel
 *
 * Forcibly enable an event code on this device, even if the underlying
 * device does not support it. While this cannot make the device actually
 * report such events, it will now return true for libevdev_has_event_code.
 *
 * This will be written to the kernel.
 *
 * This cannot be undone, the kernel only allows to enable axes, not disable
 * them.
 *
 * This function calls libevdev_kernel_enable_event_type if necessary.
 *
 * @param type The event type to enable (EV_ABS, EV_KEY, ...)
 * @param code The event code to enable (ABS_X, REL_X, etc.)
 */
int libevdev_kernel_enable_event_code(struct libevdev *dev, unsigned int type, unsigned int code);

/**
 * @ingroup kernel
 *
 * Set the device's EV_ABS axis to the value defined in the abs
 * parameter. This will be written to the kernel.
 *
 * @return zero on success, or a negative errno on failure
 *
 * @see libevdev_enable_event_code
 */
int libevdev_kernel_set_abs_value(struct libevdev *dev, unsigned int code, const struct input_absinfo *abs);

/**
 * @ingroup misc
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
 * @return The name of the given input prop (e.g. INPUT_PROP_BUTTONPAD) or NULL for an
 * invalid property
 *
 * @note The list of names is compiled into libevdev. If the kernel adds new
 * defines for new properties libevdev will not automatically pick these up.
 * @note On older kernels input properties may not be defined and
 * libevdev_get_input_prop_name will always return NULL
 */
const char * libevdev_get_input_prop_name(unsigned int prop);

/**
 * @ingroup misc
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
 * @param delay If not null, set to the repeat delay value
 * @param period If not null, set to the repeat period value
 *
 * @return 0 on success, -1 if this device does not have repeat settings.
 */
int libevdev_get_repeat(struct libevdev *dev, int *delay, int *period);

#endif /* libevdev_H */
