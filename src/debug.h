/*
**      ad -- ASCII dump
**      src/dump.h
**
**      Copyright (C) 2022  Paul J. Lucas
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

#ifndef ad_dump_H
#define ad_dump_H

/**
 * @file
 * Declares functions for dumping types for debugging.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup dump-group Debug Output
 * Functions for dumping types for debugging.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Dumps \a expr (for debugging).
 *
 * @param expr The expression to dump.  If NULL and \a key is not NULL, dumps
 * only \a key followed by `=&nbsp;NULL`.
 * @param key The key for which \a ast is the value, or NULL for none.
 * @param dout The `FILE` to dump to.
 *
 * @sa ad_expr_list_dump()
 */
void ad_expr_dump( ad_expr_t const *expr, char const *key, FILE *dout );

/**
 * Dumps \a tid (for debugging).
 *
 * @param tid The \ref ad_tid_t to dump.
 * @param dout The `FILE` to dump to.
 *
 * @sa ad_type_dump()
 */
void ad_tid_dump( ad_tid_t tid, FILE *dout );

/**
 * Dumps \a type (for debugging).
 *
 * @param type The \ref ad_type to dump.
 * @param dout The `FILE` to dump to.
 *
 * @sa c_tid_dump()
 */
void ad_type_dump( ad_type_t const *type, FILE *dout );

/**
 * Dumps a Boolean value (for debugging).
 *
 * @param b The Boolean to dump.
 * @param dout The `FILE` to dump to.
 */
void bool_dump( bool b, FILE *dout );

/**
 * Gets the name of \a e.
 *
 * @param e The \ref endian to get the name of.
 * @return Returns said name.
 */
NODISCARD
char const* endian_name( endian_t e );

/**
 * Dumps a string value (for debugging).
 *
 * @param s The string to dump, if any.  If NULL, `null` is printed instead.
 * @param dout The `FILE` to dump to.
 */
void str_dump( char const *s, FILE *dout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_dump_H */
/* vim:set et sw=2 ts=2: */
