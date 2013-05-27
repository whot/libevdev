libevemu - wrapper library for evdev input devices
==================================================

libevdevdev is a wrapper library for evdev devices. it moves the common
tasks when dealing with evdev devices into a library and provides a library
interface to the callers, thus avoiding erroneous ioctls, etc.

http://github.com/whot/libevdev

**libevdev is currently in early stages of development. Use at your own risk**

Device capabilities
-------------------
libevdev provides interfaces to query a device's capabilities.  These
interfaces are type-safe (as opposed to the ioctl bits) and protect against
invalid codes, etc.

SYN_DROPPED handling
--------------------
SYN_DROPPED is sent by the kernel if userspace cannot keep up with the
reporting rate of the device. Once the kernel's buffer is full, it will
issue a SYN_DROPPED event signalling dropped event. The userspace process
must re-sync the device.

libevdevdev semi-transparently handles SYN_DROPPED events, providing an
interface to the caller to sync up device state without having to manually
compare bitfields. Instead, libevdev sends the 'missing' events to the
caller, allowing it to use the same event processing paths as it would
otherwise.

Changing devices
----------------
libevdev provides interfaces to **modify** the kernel device.
