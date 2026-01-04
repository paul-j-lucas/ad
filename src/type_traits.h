/*
**      PJL Library
**      src/type_traits.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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

#ifndef pjl_type_traits_H
#define pjl_type_traits_H

/**
 * @file
 * Declares macros for type traits.
 */

// local
#include "pjl_config.h"                 /* must go first */

/**
 * @defgroup type-traits-group Type Traits
 * Macros for type traits.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks (at compile-time) whether \a A is an array.
 *
 * @param A The alleged array to check.
 * @return Returns 1 (true) only if \a A is an array; 0 (false) otherwise.
 *
 * @sa https://stackoverflow.com/a/77881417/99089
 */
#ifdef HAVE_TYPEOF
# define IS_ARRAY_EXPR(A)     \
    _Generic( &(A),           \
      typeof(*(A)) (*)[]: 1,  \
      default           : 0   \
    )
#else
# define IS_ARRAY_EXPR(A)         1
#endif /* HAVE_TYPEOF */

/**
 * Checks (at compile-time) whether the type of \a EXPR is a C string type,
 * i.e., <code>char*</code> or <code>char const*</code>.
 *
 * @param EXPR An expression. It is _not_ evaluated.
 * @return Returns 1 (true) only if \a EXPR is a C string type; 0 (false)
 * otherwise.
 */
#define IS_C_STR_EXPR(EXPR) \
  _Generic( (EXPR),         \
    char*       : 1,        \
    char const* : 1,        \
    default     : 0         \
  )

/**
 * Checks (at compile-time) whether the type of \a EXPR is an integral type.
 *
 * @param EXPR An expression. It is _not_ evaluated.
 * @return Returns 1 (true) only if \a EXPR is an integral type; 0 (false)
 * otherwise.
 *
 * @sa #IS_INTEGRAL_TYPE()
 * @sa #IS_SIGNED_EXPR()
 * @sa #IS_UNSIGNED_EXPR()
 */
#define IS_INTEGRAL_EXPR(EXPR) \
  (IS_SIGNED_EXPR((EXPR)) || IS_UNSIGNED_EXPR((EXPR)))

/**
 * Checks (at compile-time) whether \a TYPE is an integral type.
 *
 * @param TYPE A type.
 * @return Returns 1 (true) only if \a TYPE is an integral type; 0 (false)
 * otherwise.
 *
 * @sa #IS_INTEGRAL_EXPR()
 * @sa #IS_SIGNED_TYPE()
 * @sa #IS_UNSIGNED_TYPE()
 */
#define IS_INTEGRAL_TYPE(TYPE)    IS_INTEGRAL_EXPR( *(TYPE*)0 )

/**
 * Checks (at compile-time) whether the type of \a EXPR is a signed integral
 * type.
 *
 * @param EXPR An expression. It is _not_ evaluated.
 * @return Returns 1 (true) only if \a EXPR is of a signed integral type; 0
 * (false) otherwise.
 *
 * @sa #IS_SIGNED_TYPE()
 * @sa #IS_UNSIGNED_EXPR()
 */
#define IS_SIGNED_EXPR(EXPR)              \
  _Generic( (EXPR),                       \
    char       : IS_SIGNED_TYPE(char),    \
    signed char: 1,                       \
    short      : 1,                       \
    int        : 1,                       \
    long       : 1,                       \
    long long  : 1,                       \
    default    : 0                        \
  )

/**
 * Checks (at compile-time) whether \a TYPE is a signed type.
 *
 * @return Returns 1 (true) only if \a TYPE is signed; 0 (false) otherwise.
 *
 * @sa #IS_INTEGRAL_TYPE()
 * @sa #IS_SIGNED_EXPR()
 * @sa #IS_UNSIGNED_TYPE()
 */
#define IS_SIGNED_TYPE(TYPE)      !IS_UNSIGNED_TYPE(TYPE)

/**
 * Checks (at compile-time) whether the type of \a EXPR is an unsigned integral
 * type.
 *
 * @param EXPR An expression. It is _not_ evaluated.
 * @return Returns 1 (true) only if \a EXPR is of an unsigned integral type; 0
 * (false) otherwise.
 *
 * @sa #IS_SIGNED_EXPR()
 * @sa #IS_UNSIGNED_TYPE()
 */
#define IS_UNSIGNED_EXPR(EXPR)                  \
  _Generic( (EXPR),                             \
    _Bool             : 1,                      \
    char              : IS_UNSIGNED_TYPE(char), \
    unsigned char     : 1,                      \
    unsigned short    : 1,                      \
    unsigned int      : 1,                      \
    unsigned long     : 1,                      \
    unsigned long long: 1,                      \
    default           : 0                       \
  )

/**
 * Checks (at compile-time) whether \a TYPE is an unsigned type.
 *
 * @return Returns 1 (true) only if \a TYPE is signed; 0 (false) otherwise.
 *
 * @sa #IS_INTEGRAL_TYPE()
 * @sa #IS_SIGNED_TYPE()
 * @sa #IS_UNSIGNED_EXPR()
 */
#define IS_UNSIGNED_TYPE(TYPE)    ((TYPE)-1 > 0)

/**
 * Like C11's `_Static_assert()` except that it can be used in an expression.
 *
 * @param EXPR The expression to check.
 * @param MSG The string literal of the error message to print only if \a EXPR
 * evaluates to 0 (false).
 * @return Always returns 1.
 */
#define STATIC_ASSERT_EXPR(EXPR,MSG) \
  (!!sizeof( struct { _Static_assert( (EXPR), MSG ); char c; } ))

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* pjl_type_traits_H */
/* vim:set et sw=2 ts=2: */
