/*
**      PJL Library
**      src/pjl_config.h
**
**      Copyright (C) 2018-2021  Paul J. Lucas, et al.
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef pjl_config_H
#define pjl_config_H

/**
 * @file
 * Includes platform configuration information in the right order.  Always
 * `#include` this file rather than `config.h` directly.
 */

#ifdef ad_config_H
#error "Must #include pjl_config.h instead."
#endif

#include <attribute.h>

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

#ifdef __APPLE__
//
// From config.h:
//
//    Suppress extern inline (with or without __attribute__((__gnu_inline__)))
//    on configurations that mistakenly use 'static inline' to implement
//    functions or macros in standard C headers like <ctype.h>.  For example,
//    if isdigit is mistakenly implemented via a static inline function, a
//    program containing an extern inline function that calls isdigit may not
//    work since the C standard prohibits extern inline functions from calling
//    static functions (ISO C 99 section 6.7.4.(3).  This bug is known to occur
//    on:
//
//      OS X 10.8 and earlier; see:
//      https://lists.gnu.org/r/bug-gnulib/2012-12/msg00023.html
//
//      ...
//
//    MacOS 10.9 and later define __header_inline indicating the bug is fixed
//    for C and for clang but remains for g++; see
//    <https://trac.macports.org/ticket/41033>.
//
// MacOS 10.9 and later define __header_inline in sys/cdefs.h that is included
// from stdlib.h, so include the latter to define it if applicable.  This MUST
// be #include'd before config.h since it tests for __header_inline.
//
# include <stdlib.h>
#endif

/// @endcond

// local
#include "config.h"                     /* must go first */

////////// compiler attributes ////////////////////////////////////////////////

/**
 * Denote that a function's return value may be ignored without warning.
 *
 * @note
 * There is no compiler attribute for this.  It's just a visual cue in code
 * that `NODISCARD` wasn't forgotten.
 */
#define PJL_DISCARD               /* nothing */

#ifdef HAVE___TYPEOF__
/**
 * Ignore the return value of a function even if it was declared with
 * #AD_WARN_UNUSED_RESULT.
 *
 * @param FN_CALL The function call.
 */
#define PJL_IGNORE_RV(FN_CALL) \
  do { MAYBE_UNUSED __typeof__(FN_CALL) _rv = (FN_CALL); } while (0)
#else
#define PJL_IGNORE_RV(FN_CALL)    ((void)(FN_CALL))
#endif /* HAVE___TYPEOF__ */

///////////////////////////////////////////////////////////////////////////////

#endif /* pjl_config_H */
/* vim:set et sw=2 ts=2: */
