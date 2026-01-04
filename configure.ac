# Process this file with autoconf to produce a configure script.

##
#       ad -- ASCII dump
#       configure.ac
#
#       Copyright (C) 2016-2026  Paul J. Lucas
#
#       This program is free software: you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation, either version 3 of the License, or
#       (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public License
#       along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

AC_PREREQ([2.69])
AC_INIT([ad],[3.6.1],[https://github.com/paul-j-lucas/ad/issues],[],[https://github.com/paul-j-lucas/ad])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_LANG(C)
AC_PROG_CC
AS_IF([test "x$ac_cv_prog_cc_c11" = xno],
  [AC_MSG_ERROR([a C11 compiler is required to compile $PACKAGE_NAME])]
)
gl_EARLY
AC_PROG_INSTALL
AC_PROG_RANLIB

# Check host type.
AC_CANONICAL_HOST
case "x$host_os" in
xdarwin*)
  AC_DEFINE([HAVE_RSRC_FORK], [1], [Define to 1 if have resource forks.])
  AM_CONDITIONAL([HAVE_RSRC_FORK], [true])
  ;;
*)
  AC_DEFINE([HAVE_RSRC_FORK], [0], [Define to 1 if have resource forks.])
  AM_CONDITIONAL([HAVE_RSRC_FORK], [false])
  ;;
esac

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([langinfo.h])
AC_CHECK_HEADERS([libgen.h])
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sysexits.h])
AC_HEADER_ASSERT
AC_HEADER_STDBOOL
gl_INIT

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C__GENERIC
AS_IF([test "x$ac_cv_c__Generic" != xyes], [
  AC_MSG_ERROR([C11 _Generic support required to compile $PACKAGE_NAME])
])
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_C_TYPEOF
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
PJL_CHECK_TYPEDEF(char8_t,[uchar.h],
  AC_DEFINE([HAVE_CHAR8_T], [1], [Define to 1 if `char8_t' is supported.]),
  AC_DEFINE([HAVE_CHAR8_T], [0], [Define to 1 if `char8_t' is supported.])
)
PJL_CHECK_TYPEDEF(char32_t,[uchar.h],
  AC_DEFINE([HAVE_CHAR32_T], [1], [Define to 1 if `char32_t' is supported.]),
  AC_DEFINE([HAVE_CHAR32_T], [0], [Define to 1 if `char32_t' is supported.])
)
PJL_COMPILE([__typeof__],[],[__typeof__(1) x __attribute__((unused)) = 1;])

# Checks for library functions.
AC_FUNC_FSEEKO
AC_FUNC_REALLOC
AC_CHECK_FUNCS([basename fgetln getline nl_langinfo setlocale strdup strerror strsep])

# Miscellaneous.
AX_C___ATTRIBUTE__

# Compiler warnings.
AC_SUBST([AD_CFLAGS])
AD_CFLAGS="-D_ISOC11_SOURCE"
AX_CFLAGS_WARN_ALL([AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wcast-align], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wcast-function-type], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wcomma], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wconditional-type-mismatch], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wconditional-uninitialized], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wconversion], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wduplicate-enum], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wembedded-directive], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wenum-enum-conversion], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wenum-float-conversion], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wextra], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wfloat-equal], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wfor-loop-analysis], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wformat-signedness], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wformat-type-confusion], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wformat=2], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Widiomatic-parentheses], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wlogical-op-parentheses], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wmisleading-indentation], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wnewline-eof], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wno-format-nonliteral], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wredundant-decls], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wreserved-identifier], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wshadow], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wshift-sign-overflow], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wshorten-64-to-32], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wsign-compare], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wsign-conversion], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wsometimes-uninitialized], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wstring-conversion], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wtautological-compare], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wtautological-type-limit-compare], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wuninitialized], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wunreachable-code-break], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wunreachable-code-return], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wunreachable-code], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wunused], [AD_CFLAGS])
PJL_CHECK_COMPILE_FLAG([-Wwrite-strings], [AD_CFLAGS])

# Generate files.
AH_TOP([#ifndef ad_config_H
#define ad_config_H])
AH_BOTTOM([#endif /* ad_config_H */])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([
  Makefile
  lib/Makefile
  src/Makefile
  test/Makefile
  man/Makefile
  man/man1/Makefile
])
AC_OUTPUT

# vim:set et sw=2 ts=2:
