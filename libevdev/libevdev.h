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

struct libevdev;

enum EvdevReadFlags {
    LIBEVDEV_READ_SYNC		= 1, /**< Process data in sync mode */
};


/**
 * Initialize a new libevdev device.
*
 * @see libevdev_set_fd
 */
struct libevdev* libevdev_new(void);

/**
 * Initialize a new libevdev device from the given fd.
 *
 * This is a shortcut for
 *
 * <pre>
 * struct libevdev *dev = libevdev_new();
 * libevdev_set_fd(dev, fd);
 * </pre>
 *
 * @param fd A file descriptor to the device in O_RDWR or O_RDONLY mode.
 *
 * @return On success, zero is returned and dev is set to the newly
 * allocated struct. On failure, a negative errno is returned and the value
 * of dev is undefined.
 */
int libevdev_new_from_fd(int fd, struct libevdev **dev);

/**
 * Clean up and free the libevdev struct.
 *
 * @note This function may be called before libevdev_set_fd.
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
 * Set the fd for this struct and initialize internal data.
 * The fd must be open for reading and ioctl.
 *
 * This function may only be called once per device. If you need to re-read
 * a device, use libevdev_free and libevdev_new. If you need to change the
 * fd, use libevdev_change_fd.
 *
 * Unless otherwise specified, libevdev function behavior is undefined until
 * a successfull call to libevdev_set_fd.
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
 * Change the fd for this device, without re-reading the actual device.
 *
 * It is an error to call this function before calling libevdev_set_fd.
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
 * @return The device name as read off the kernel device
 *
 * @note This function is signal-safe.
 */
const char* libevdev_get_name(const struct libevdev *dev);

/**
 * @return The device's product ID
 *
 * @note This function is signal-safe.
 */
int libevdev_get_pid(const struct libevdev *dev);
/**
 * @return The device's vendor ID
 *
 * @note This function is signal-safe.
 */
int libevdev_get_vid(const struct libevdev *dev);

/**
 * @return The device's bus type
 *
 * @note This function is signal-safe.
 */
int libevdev_get_bustype(const struct libevdev *dev);

/**
 * @return 1 if the device supports this event type, or 0 otherwise.
 *
 * @note This function is signal-safe
 */
int libevdev_has_property(const struct libevdev *dev, unsigned int prop);

/**
 * @return 1 if the device supports this event type, or 0 otherwise.
 *
 * @note This function is signal-safe.
 */
int libevdev_has_event_type(const struct libevdev *dev, unsigned int type);

/**
 * @return 1 if the device supports this event type, or 0 otherwise.
 *
 * @note This function is signal-safe.
 */
int libevdev_has_event_code(const struct libevdev *dev, unsigned int type, unsigned int code);

/**
 * @return axis minimum for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_min(const struct libevdev *dev, unsigned int code);
/**
 * @return axis maximum for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_max(const struct libevdev *dev, unsigned int code);
/**
 * @return axis fuzz for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_fuzz(const struct libevdev *dev, unsigned int code);
/**
 * @return axis flat for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_flat(const struct libevdev *dev, unsigned int code);
/**
 * @return axis resolution for the given axis or 0 if the axis is invalid
 */
int libevdev_get_abs_resolution(const struct libevdev *dev, unsigned int code);

/**
 * @return The input_absinfo for the given code, or NULL if the device does
 * not support this event code.
 */
const struct input_absinfo* libevdev_get_abs_info(const struct libevdev *dev, unsigned int code);

/**
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
 * Get the number of slots supported by this device.
 *
 * Note that the slot offset may be non-zero, use libevdev_get_abs_min() or
 * libevdev_get_abs_info() to get the minimum slot number.
 *
 * @return The number of slots supported, or -1
 */
int libevdev_get_num_slots(const struct libevdev *dev);

/**
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
 * Set the device's EV_ABS/<code> axis to the value defined in the abs
 * parameter. This will be written to the kernel.
 *
 * @return zero on success, or a negative errno on failure
 *
 * @see libevdev_enable_event_code
 */
int libevdev_kernel_set_abs_value(struct libevdev *dev, unsigned int code, const struct input_absinfo *abs);

#endif /* libevdev_H */
