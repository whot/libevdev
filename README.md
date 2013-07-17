libevdev - wrapper library for evdev input devices
==================================================

libevdev is a wrapper library for evdev devices. it moves the common
tasks when dealing with evdev devices into a library and provides a library
interface to the callers, thus avoiding erroneous ioctls, etc.

git://git.freedesktop.org/git/libevdev
http://cgit.freedesktop.org/libevdev/

**libevdev is currently in early stages of development. Use at your own risk**

The eventual goal is that libevdev wraps all ioctls available to evdev
devices, thus making direct access unnecessary.

Go here for the API documentation:
http://whot.github.io/libevdev/

File bugs in the freedesktop.org bugzilla:
https://bugs.freedesktop.org/enter_bug.cgi?product=libevdev
