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
 * Dumps \a expr in [JSON5](https://json5.org) format (for debugging).
 *
 * @param expr The expression to dump.  If NULL and \a key is not NULL, dumps
 * only \a key followed by `=&nbsp;NULL`.
 * @param fout The `FILE` to dump to.
 *
 * @sa ad_expr_list_dump()
 */
void ad_expr_dump( ad_expr_t const *expr, FILE *fout );

/**
 * Dumps \a rep in [JSON5](https://json5.org) format (for debugging).
 *
 * @param rep The \ref ad_rep to dump.
 * @param fout The `FILE` to dump to.
 */
void ad_rep_dump( ad_rep_t const *rep, FILE *fout );

/**
 * Dump \a statement in [JSON5](https://json5.org) format (for debugging).
 *
 * @param statement The \ref ad_statement to dump.
 * @param fout The `FILE` to dump to.
 */
void ad_statement_dump( ad_statement_t const *statement, FILE *fout );

/**
 * Dumps \a tid in [JSON5](https://json5.org) format (for debugging).
 *
 * @param tid The \ref ad_tid_t to dump.
 * @param fout The `FILE` to dump to.
 *
 * @sa ad_type_dump()
 */
void ad_tid_dump( ad_tid_t tid, FILE *fout );

/**
 * Dumps \a type in [JSON5](https://json5.org) format (for debugging).
 *
 * @param type The \ref ad_type to dump.
 * @param fout The `FILE` to dump to.
 *
 * @sa c_tid_dump()
 */
void ad_type_dump( ad_type_t const *type, FILE *fout );

/**
 * Dumps a Boolean value in [JSON5](https://json5.org) format (for debugging).
 *
 * @param b The Boolean to dump.
 * @param fout The `FILE` to dump to.
 */
void bool_dump( bool b, FILE *fout );

/**
 * Gets the name of \a e.
 *
 * @param e The \ref endian to get the name of.
 * @return Returns said name.
 */
NODISCARD
char const* endian_name( endian_t e );

/**
 * Dumps \a sname in [JSON5](https://json5.org) format (for debugging).
 *
 * @param sname The scoped name to dump.  If empty, prints `null` instead.
 * @param fout The `FILE` to dump to.
 */
void sname_dump( sname_t const *sname, FILE *fout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_dump_H */
/* vim:set et sw=2 ts=2: */
