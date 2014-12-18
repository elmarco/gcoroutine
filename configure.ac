AC_PREREQ([2.63])

AC_INIT([gcoroutine], [m4_esyscmd(build-aux/git-version-gen .tarball-version)],
                      [spice-devel@lists.freedesktop.org])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([src/gcoroutine.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 no-define foreign -Wno-portability dist-xz no-dist-gzip tar-ustar])
AM_EXTRA_RECURSIVE_TARGETS([memcheck])

AM_SILENT_RULES([yes])

AC_USE_SYSTEM_EXTENSIONS

AC_CANONICAL_HOST
AC_MSG_CHECKING([for native Win32])

AS_CASE([$host],
        [*-*-mingw*|*-*-cygwin*],
        [os_win32=yes],

        [os_win32=no])
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL([OS_WIN32],[test "$os_win32" = "yes"])

LT_PREREQ([2.2.6])
LT_INIT

AC_CHECK_PROG([STOW], [stow], [yes], [no])
AS_IF([test "x$STOW" = "xyes" && test -d /usr/local/stow], [
      AC_MSG_NOTICE([*** Found /usr/local/stow: default install prefix set to /usr/local/stow/${PACKAGE_NAME} ***])
      ac_default_prefix="/usr/local/stow/${PACKAGE_NAME}"
])

GCOROUTINE_MAJOR_VERSION=`echo $PACKAGE_VERSION | cut -d. -f1`
GCOROUTINE_MINOR_VERSION=`echo $PACKAGE_VERSION | cut -d. -f2`
GCOROUTINE_MICRO_VERSION=`echo $PACKAGE_VERSION | cut -d. -f3 | cut -d- -f1`
AS_IF([test "x$GCOROUTINE_MICRO_VERSION" = "x"], [GCOROUTINE_MICRO_VERSION=0])

AC_SUBST(GCOROUTINE_MAJOR_VERSION)
AC_SUBST(GCOROUTINE_MINOR_VERSION)
AC_SUBST(GCOROUTINE_MICRO_VERSION)

PKG_CHECK_MODULES(GLIB,
                  [glib-2.0 >= 2.40],
                  [has_glib=yes],
                  [has_glib=no])

dnl === coroutine implementation ===============================================

AC_ARG_WITH([coroutine],
AS_HELP_STRING([--with-coroutine=@<:@ucontext/gthread/winfiber/auto@:>@],
               [select coroutine implementation @<:@default=auto@:>@]), [],
               [with_coroutine=auto])

case $with_coroutine in
     ucontext|gthread|winfiber|auto) ;;
     *) AC_MSG_ERROR(Unsupported coroutine type)
esac

if test "$with_coroutine" = "auto"; then
    if test "$os_win32" = "yes"; then
        with_coroutine=winfiber
    else
        with_coroutine=ucontext
    fi
fi

AM_CONDITIONAL(COROUTINE_UCONTEXT, [test "$with_coroutine" = "ucontext"])
AM_CONDITIONAL(COROUTINE_WINFIBER, [test "$with_coroutine" = "winfiber"])
AM_CONDITIONAL(COROUTINE_GTHREAD, [test "$with_coroutine" = "gthread"])

dnl === Visibility ============================================================

GCOROUTINE_VISIBILITY_CFLAGS=""
AS_CASE(["$host"],

        [*-*-mingw*],
        [
          dnl on mingw32 we do -fvisibility=hidden and __declspec(dllexport)
          AC_DEFINE([_GCOROUTINE_PUBLIC],
                    [__attribute__((visibility("default"))) __declspec(dllexport) extern],
                    [defines how to decorate public symbols while building])
          GCOROUTINE_VISIBILITY_CFLAGS="-fvisibility=hidden"
        ],

        [
          dnl on other compilers, check if we can do -fvisibility=hidden
          SAVED_CFLAGS="${CFLAGS}"
          CFLAGS="-fvisibility=hidden"
          AC_MSG_CHECKING([for -fvisibility=hidden compiler flag])
          AC_TRY_COMPILE([], [int main (void) { return 0; }],
                         [
                           AC_MSG_RESULT(yes)
                           enable_fvisibility_hidden=yes
                         ],
                         [
                           AC_MSG_RESULT(no)
                           enable_fvisibility_hidden=no
                         ])
          CFLAGS="${SAVED_CFLAGS}"

          AS_IF([test "${enable_fvisibility_hidden}" = "yes"],
                [
                  AC_DEFINE([_GCOROUTINE_PUBLIC],
                            [__attribute__((visibility("default"))) extern],
                            [defines how to decorate public symbols while building])
                  GCOROUTINE_VISIBILITY_CFLAGS="-fvisibility=hidden"
                ])
        ]
)

AC_SUBST(GCOROUTINE_VISIBILITY_CFLAGS)
dnl === Test suite (require GLib) =============================================

AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--enable-tests], [Enable test suite])],
              [enable_tests=$enableval],
              [enable_tests=check])

GLIB_TESTS

AS_CASE([$enable_tests],
        [check],
        [
          AS_IF([test "x$has_glib" = xyes],
                [build_tests=yes],
                [build_tests="disabled (missing GLib)"])
        ],

        [yes],
        [
          AS_IF([test "x$has_glib" = xyes],
                [build_tests=yes],
                [AC_MSG_ERROR([Cannot build the test suite without GLib])])
        ],

        [no],
        [ build_tests=disabled ])

AM_CONDITIONAL(BUILD_TESTS, [test "x$build_tests" = xyes])

dnl === Code coverage =========================================================

AC_ARG_ENABLE([code-coverage],
              [AS_HELP_STRING([--enable-code-coverage], [Enable test suite code coverage])],
              [use_gcov=$enableval],
              [use_gcov=no])

dnl Disable code coverage if tests were disabled
AS_IF([test "x$build_tests" = disabled ], [ use_gcov=disabled ])

AS_IF([test "x$use_gcov" = "xyes"],
      [
        dnl we need a test suite
        AS_IF([test "x$build_tests" = xno],
              [AC_MSG_ERROR([Cannot enable code coverage when the test suite is disabled])])

        dnl we need gcc
        AS_IF([test "$GCC" != "yes"], [AC_MSG_ERROR([GCC is required for --enable-code-coverage])])

        dnl Check if ccache is being used
        AC_CHECK_PROG(SHTOOL, shtool, shtool)
        AS_CASE([`$SHTOOL path $CC`],
                [*ccache*], [gcc_ccache=yes],
                [gcc_ccache=no])

        if test "$gcc_ccache" = "yes" && (test -z "$CCACHE_DISABLE" || test "$CCACHE_DISABLE" != "1"); then
          AC_MSG_ERROR([ccache must be disabled when --enable-code-coverage option is used. You can disable ccache by setting environment variable CCACHE_DISABLE=1.])
        fi

        ltp_version_list="1.6 1.7 1.8 1.9 1.10"
        AC_CHECK_PROG(LTP, lcov, lcov)
        AC_CHECK_PROG(LTP_GENHTML, genhtml, genhtml)

        if test "$LTP"; then
          AC_CACHE_CHECK([for ltp version], gcov_cv_ltp_version,
                         [
                           gcov_cv_ltp_version=invalid
                           ltp_version=`$LTP -v 2>/dev/null | $SED -e 's/^.* //'`
                           for ltp_check_version in $ltp_version_list; do
                             if test "$ltp_version" = "$ltp_check_version"; then
                               gcov_cv_ltp_version="$ltp_check_version (ok)"
                             fi
                           done
                         ])
        else
          ltp_msg="To enable code coverage reporting you must have one of the following LTP versions installed: $ltp_version_list"
          AC_MSG_ERROR([$ltp_msg])
        fi

        case $gcov_cv_ltp_version in
          ""|invalid[)]
          ltp_msg="You must have one of the following versions of LTP: $ltp_version_list (found: $ltp_version)."
          AC_MSG_ERROR([$ltp_msg])
          LTP="exit 0;"
          ;;
        esac

        if test -z "$LTP_GENHTML"; then
          AC_MSG_ERROR([Could not find genhtml from the LTP package])
        fi

        AC_DEFINE(HAVE_GCOV, 1, [Whether you have gcov])

        dnl Remove all compiler optimization flags
        changequote({,})
        BASE_CFLAGS=`echo "$BASE_CFLAGS" | $SED -e 's/-O[s0-9]//g'`
        changequote([,])

        dnl Define the special gcc flags
        GCOV_CFLAGS="-O0 -fprofile-arcs -ftest-coverage"
        GCOV_LDADD="-lgcov"
        AC_SUBST(GCOV_CFLAGS)
        AC_SUBST(GCOV_LDADD)
      ])

AM_CONDITIONAL(ENABLE_GCOV, [test "x$use_gcov" = xyes])

dnl === API Reference =========================================================

GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
AC_SUBST(GLIB_PREFIX)

GTK_DOC_CHECK([1.20], [--flavour no-tmpl])

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        tests/Makefile
        src/gcoroutine.pc
        src/gcoroutine-version.h
        doc/Makefile
])

AC_OUTPUT

dnl === Summary ===============================================================

AC_MSG_NOTICE([

GCoroutine - $VERSION

  • Prefix: ${prefix}
  • Coroutine: ${with_coroutine}
  • Test suite: ${build_tests}
  • Code coverage: ${use_gcov}
])
