# Process this file with autoconf to produce a configure script.

##
#       ad -- ASCII dump
#       configure.ac
#
#       Copyright (C) 2016-2024  Paul J. Lucas
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
AC_INIT([ad],[3.5],[https://github.com/paul-j-lucas/ad/issues],[],[https://github.com/paul-j-lucas/ad])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_LANG(C)
AC_PROG_CC
AS_IF([test "x$ac_cv_prog_cc_c11" = xno],
  [AC_MSG_ERROR([a C11 compiler is required to compile $PACKAGE_NAME])])
gl_EARLY

# Check for flex & bison only if lexer.c & parser.c aren't present. If present,
# it likely means a released version was downloaded so flex & bison shouldn't
# be required.
flex_min_version="2.5.30"
bison_min_version="3.4.2"
AS_IF([test ! -f "$srcdir/src/lexer.c"], [
  #
  # AX_PROG_FLEX calls AM_PROG_LEX that in turn calls AC_PROG_LEX without
  # arguments.  However, AC_PROG_LEX now wants an argument of either yywrap
  # or noyywrap and warns if omitted.  To work around this, call AC_PROG_LEX
  # with an argument ourselves first.
  #
  # Unfortunately, we can't just stop there because AX_PROG_FLEX calls
  # AC_REQUIRE([AM_PROG_LEX]) and would generate an "expanded before it was
  # required" warning.  To work around this, call AM_PROG_LEX ourselves too.
  #
  AC_PROG_LEX([noyywrap])
  AM_PROG_LEX

  AX_PROG_FLEX([FLEX=$LEX],
    [AC_MSG_ERROR([required program "flex" not found])])
  AX_PROG_FLEX_VERSION([$flex_min_version],[],
    [AC_MSG_ERROR([flex version $flex_min_version or later required])])
])
AS_IF([test ! -f "$srcdir/src/parser.c"], [
  AX_PROG_BISON([BISON=$YACC],
    [AC_MSG_ERROR([required program "bison" not found])])
  AX_PROG_BISON_VERSION([$bison_min_version],[],
    [AC_MSG_ERROR([bison version $bison_min_version or later required])])
])

AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([langinfo.h])
AC_CHECK_HEADERS([libgen.h])
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sysexits.h])
AC_CHECK_HEADERS([unistd.h])
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
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
PJL_CHECK_TYPEDEF(char8_t,[uchar.h],
  AC_DEFINE([HAVE_CHAR8_T], [1], [Define to 1 if `char8_t' is supported.]),
  AC_DEFINE([HAVE_CHAR8_T], [0], [Define to 1 if `char8_t' is supported.])
)
PJL_CHECK_TYPEDEF(char16_t,[uchar.h],
  AC_DEFINE([HAVE_CHAR16_T], [1], [Define to 1 if `char16_t' is supported.]),
  AC_DEFINE([HAVE_CHAR16_T], [0], [Define to 1 if `char16_t' is supported.])
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

# Program feature: Flex debug (disabled by default)
AC_ARG_ENABLE([flex-debug],
  AS_HELP_STRING([--enable-flex-debug], [enable Flex debugging]),
  [],
  [enable_flex_debug=no]
)
AS_IF([test x$enable_flex_debug = xyes],
  [AC_DEFINE([ENABLE_FLEX_DEBUG], [1],
    [Define to 1 if Flex debugging is enabled.])]
)

# Program feature: Bison debug (disabled by default)
AC_ARG_ENABLE([bison-debug],
  AS_HELP_STRING([--enable-bison-debug], [enable for Bison debugging]),
  [],
  [enable_bison_debug=no]
)
AS_IF([test x$enable_bison_debug = xyes],
  [AC_DEFINE([YYDEBUG], [1],
    [Define to 1 if Bison debugging is enabled.])]
)

# Makefile conditionals.
AM_CONDITIONAL([ENABLE_BISON_DEBUG],  [test x$enable_bison_debug  = xyes])
AM_CONDITIONAL([ENABLE_FLEX_DEBUG],   [test x$enable_flex_debug   = xyes])

# Miscellaneous.
AX_C___ATTRIBUTE__

# Compiler warnings.
AC_SUBST([AD_CFLAGS])
AD_CFLAGS="-D_ISOC11_SOURCE"
AX_CFLAGS_WARN_ALL([AD_CFLAGS])
AX_CHECK_COMPILE_FLAG([-Wcast-align], [AD_CFLAGS="$AD_CFLAGS -Wcast-align"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wcast-function-type], [AD_CFLAGS="$AD_CFLAGS -Wcast-function-type"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wcomma], [AD_CFLAGS="$AD_CFLAGS -Wcomma"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconditional-type-mismatch], [AD_CFLAGS="$AD_CFLAGS -Wconditional-type-mismatch"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconditional-uninitialized], [AD_CFLAGS="$AD_CFLAGS -Wconditional-uninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wconversion], [AD_CFLAGS="$AD_CFLAGS -Wconversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wduplicate-enum], [AD_CFLAGS="$AD_CFLAGS -Wduplicate-enum"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wembedded-directive], [AD_CFLAGS="$AD_CFLAGS -Wembedded-directive"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wenum-enum-conversion], [AD_CFLAGS="$AD_CFLAGS -Wenum-enum-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wenum-float-conversion], [AD_CFLAGS="$AD_CFLAGS -Wenum-float-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wextra], [AD_CFLAGS="$AD_CFLAGS -Wextra"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wfloat-equal], [AD_CFLAGS="$AD_CFLAGS -Wfloat-equal"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wfor-loop-analysis], [AD_CFLAGS="$AD_CFLAGS -Wfor-loop-analysis"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wformat-signedness], [AD_CFLAGS="$AD_CFLAGS -Wformat-signedness"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wformat-type-confusion], [AD_CFLAGS="$AD_CFLAGS -Wformat-type-confusion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wformat=2], [AD_CFLAGS="$AD_CFLAGS -Wformat=2"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Widiomatic-parentheses], [AD_CFLAGS="$AD_CFLAGS -Widiomatic-parentheses"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough], [AD_CFLAGS="$AD_CFLAGS -Wimplicit-fallthrough"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wlogical-op-parentheses], [AD_CFLAGS="$AD_CFLAGS -Wlogical-op-parentheses"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wmisleading-indentation], [AD_CFLAGS="$AD_CFLAGS -Wmisleading-indentation"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wnewline-eof], [AD_CFLAGS="$AD_CFLAGS -Wnewline-eof"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wno-format-nonliteral], [AD_CFLAGS="$AD_CFLAGS -Wno-format-nonliteral"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wredundant-decls], [AD_CFLAGS="$AD_CFLAGS -Wredundant-decls"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wreserved-identifier], [AD_CFLAGS="$AD_CFLAGS -Wreserved-identifier"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshadow], [AD_CFLAGS="$AD_CFLAGS -Wshadow"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshift-sign-overflow], [AD_CFLAGS="$AD_CFLAGS -Wshift-sign-overflow"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wshorten-64-to-32], [AD_CFLAGS="$AD_CFLAGS -Wshorten-64-to-32"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsign-compare], [AD_CFLAGS="$AD_CFLAGS -Wsign-compare"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsign-conversion], [AD_CFLAGS="$AD_CFLAGS -Wsign-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wsometimes-uninitialized], [AD_CFLAGS="$AD_CFLAGS -Wsometimes-uninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wstring-conversion], [AD_CFLAGS="$AD_CFLAGS -Wstring-conversion"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wtautological-compare], [AD_CFLAGS="$AD_CFLAGS -Wtautological-compare"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wtautological-type-limit-compare], [AD_CFLAGS="$AD_CFLAGS -Wtautological-type-limit-compare"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wundef], [AD_CFLAGS="$AD_CFLAGS -Wundef"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wuninitialized], [AD_CFLAGS="$AD_CFLAGS -Wuninitialized"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code-break], [AD_CFLAGS="$AD_CFLAGS -Wunreachable-code-break"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code-return], [AD_CFLAGS="$AD_CFLAGS -Wunreachable-code-return"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunreachable-code], [AD_CFLAGS="$AD_CFLAGS -Wunreachable-code"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunused], [AD_CFLAGS="$AD_CFLAGS -Wunused"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wwrite-strings], [AD_CFLAGS="$AD_CFLAGS -Wwrite-strings"], [], [-Werror])
AX_CHECK_COMPILE_FLAG([-Wzero-as-null-pointer-constant], [AD_CFLAGS="$AD_CFLAGS -Wzero-as-null-pointer-constant"], [], [-Werror])

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
