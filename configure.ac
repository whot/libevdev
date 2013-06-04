AC_PREREQ([2.64])

AC_INIT([libevdev],
        [0.1],
        [],
        [libevdev],
        [])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

# Before making a release, the LIBEVDEV_LT_VERSION string should be
# modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
LIBEVDEV_LT_VERSION=1:0:0
AC_SUBST(LIBEVDEV_LT_VERSION)


AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

PKG_PROG_PKG_CONFIG()

if test "x$GCC" = "xyes"; then
	GCC_CFLAGS="-Wall -Wextra -Wno-unused-parameter -g -Wstrict-prototypes -Wmissing-prototypes -fvisibility=hidden"
fi
AC_SUBST(GCC_CFLAGS)

AC_PATH_PROG(ASCIIDOC, [a2x])
if test "x$ASCIIDOC" = "x"; then
	AC_MSG_WARN([asciidoc not found - unable to build man pages])
	have_asciidoc=no
else
	have_asciidoc=yes
fi
AM_CONDITIONAL([HAVE_ASCIIDOC], [test "$have_asciidoc" = yes])


AC_CONFIG_FILES([Makefile
		 libevdev/Makefile
		 man/Makefile
		 test/Makefile
		 libevdev.pc])
AC_OUTPUT
