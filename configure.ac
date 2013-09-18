AC_PREREQ([2.64])

AC_INIT([libevdev],
        [0.4],
        [],
        [libevdev],
        [])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS

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
LT_PATH_LD

with_ldflags=""
if test "x$lt_cv_prog_gnu_ld" = "xyes"; then
	CC_CHECK_FLAGS_APPEND([with_ldflags], [LDFLAGS], [\
				-Wl,--as-needed \
				-Wl,--gc-sections \
				-Wl,-z,relro \
				-Wl,-z,now])
fi
AC_SUBST([GNU_LD_FLAGS], $with_ldflags)

PKG_PROG_PKG_CONFIG()
PKG_CHECK_MODULES(CHECK, [check >= 0.9.9], [HAVE_CHECK="yes"], [HAVE_CHECK="no"])
if test "x$HAVE_CHECK" != "xyes"; then
       AC_MSG_WARN([check not found - skipping building unit tests])
fi
AM_CONDITIONAL(BUILD_TESTS, [test "x$HAVE_CHECK" = "xyes"])

with_cflags=""
if test "x$GCC" = "xyes"; then
	CC_CHECK_FLAGS_APPEND([with_cflags], [CFLAGS], [\
				-Wall \
				-Wextra \
				-Wno-unused-parameter \
				-Wstrict-prototypes \
				-Wmissing-prototypes \
				-fvisibility=hidden \
				-pipe \
				-fno-strict-aliasing \
				-ffunction-sections \
				-fdata-sections \
				-fno-strict-aliasing \
				-fdiagnostics-show-option \
				-fno-common])
fi
AC_SUBST([GCC_CFLAGS], $with_cflags)

AC_PATH_PROG(DOXYGEN, [doxygen])
if test "x$DOXYGEN" = "x"; then
	AC_MSG_WARN([doxygen not found - required for documentation])
fi
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AC_MSG_CHECKING([whether to build with gcov])
AC_ARG_ENABLE([gcov],
	      [AS_HELP_STRING([--enable-gcov],
			      [Whether to enable coverage testing (default:enabled)])],
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
