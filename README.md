libevdev - wrapper library for evdev input devices
==================================================

libevdev is a wrapper library for evdev devices. it moves the common
tasks when dealing with evdev devices into a library and provides a library
interface to the callers, thus avoiding erroneous ioctls, etc.

git@gitlab.freedesktop.org:libevdev/libevdev.git
https://gitlab.freedesktop.org/libevdev/libevdev.git

The eventual goal is that libevdev wraps all ioctls available to evdev
devices, thus making direct access unnecessary.

Go here for the API documentation:
http://www.freedesktop.org/software/libevdev/doc/latest/

File bugs in the freedesktop.org GitLab instance:
https://gitlab.freedesktop.org/libevdev/libevdev/issues/

Patches, questions and general comments should be submitted to the input-tools@lists.freedesktop.org
mailing list:
http://lists.freedesktop.org/mailman/listinfo/input-tools
