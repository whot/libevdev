libevdev - wrapper library for evdev input devices
==================================================

libevdev is a wrapper library for evdev devices. it moves the common
tasks when dealing with evdev devices into a library and provides a library
interface to the callers, thus avoiding erroneous ioctls, etc.

http://github.com/whot/libevdev

**libevdev is currently in early stages of development. Use at your own risk**

The eventual goal is that libevdev wraps all ioctls available to evdev
devices, thus making direct access unnecessary.

ioctl wrappers
--------------
libevdev provides interfaces to query a device's capabilities, providing
type-safe interfaces to query and set a device's capabilities and state.

SYN_DROPPED handling
--------------------
SYN_DROPPED is sent by the kernel if userspace cannot keep up with the
reporting rate of the device. Once the kernel's buffer is full, it will
issue a SYN_DROPPED event signalling dropped event. The userspace process
must re-sync the device.

libevdev semi-transparently handles SYN_DROPPED events, providing an
interface to the caller to sync up device state without having to manually
compare bitfields. Instead, libevdev sends the 'missing' events to the
caller, allowing it to use the same event processing paths as it would
otherwise.

