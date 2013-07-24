AC_PREREQ([2.64])

AC_INIT([libevdev],
        [0.2.1],
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
PKG_CHECK_MODULES(CHECK, [check])

if test "x$GCC" = "xyes"; then
	GCC_CFLAGS="-Wall -Wextra -Wno-unused-parameter -g -Wstrict-prototypes -Wmissing-prototypes -fvisibility=hidden"
fi
AC_SUBST(GCC_CFLAGS)

AC_PATH_PROG(DOXYGEN, [doxygen])
if test "x$DOXYGEN" = "x"; then
	AC_MSG_WARN([doxygen not found - required for documentation])
	have_doxygen=no
else
	have_doxygen=yes
fi
AM_CONDITIONAL([HAVE_DOXYGEN], [test "$have_doxygen" = yes])

AC_MSG_CHECKING([whether to build with gcov])
AC_ARG_ENABLE([gcov],
	      [AS_HELP_STRING([--enable-gcov],
			      [Whether to enable coverage testing (default:enabled)])]
	      [],
	      [enable_gcov=yes],
	      )
AS_IF([test "x$enable_gcov" != "xno"],
      [
       GCOV_CFLAGS="-fprofile-arcs -ftest-coverage"
       GCOV_LDFLAGS="-fprofile-arcs -ftest-coverage"
       enable_gcov=yes
       ],
)

AM_CONDITIONAL([GCOV_ENABLED], [test "x$enable_gcov" != "xno"])
AC_SUBST([GCOV_CFLAGS])
AC_SUBST([GCOV_LDFLAGS])
AC_MSG_RESULT([$enable_gcov])

AC_CONFIG_FILES([Makefile
		 libevdev/Makefile
		 doc/Makefile
		 doc/libevdev.doxygen
		 tools/Makefile
		 test/Makefile
		 libevdev.pc])
AC_OUTPUT
