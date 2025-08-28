/*
**      ad -- ASCII dump
**      src/in_attr.h
**
**      Copyright (C) 2019-2024  Paul J. Lucas, et al.
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

#ifndef ad_in_attr_H
#define ad_in_attr_H

/**
 * @file
 * Declares types, functions, and a global variable for inherited attributes
 * for the format parser.
 */

/** @cond DOXYGEN_IGNORE */

#include "symbol.h"
#include "sname.h"
#include "types.h"

/**
 * @addtogroup parser-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Inherited attributes.
 *
 * @remarks These are grouped into a `struct` (rather than having them as
 * separate global variables) so that they can all be reset (mostly) via a
 * single assignment from `{0}`.
 *
 * @sa ia_cleanup()
 */
struct in_attr {
  sname_t         scope_sname;          ///< Current scope name, if any.
  ad_type_t      *cur_type;             ///< Current type.
  ad_statement_t *statement;            ///< Current field name, if any.
};
typedef struct in_attr in_attr_t;

////////// extern variables ///////////////////////////////////////////////////

extern in_attr_t in_attr;               ///< Inherited attributes.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans-up all resources used by \ref in_attr "inherited attributes".
 */
void ia_cleanup( void );

/**
 * Gets the current full scoped name.
 *
 * @remarks
 * Given the declarations:
 * @code
 *  struct APP0 {
 *    enum unit_t : uint<8> {
 *      // ...
 * @endcode
 * and a \a name of `"unit_t"`, this function would return the scoped name of
 * `APP0::unit_t`.
 *
 * @param name The local name.  Ownership is taken.
 * @return Returns the current full scoped name.
 */
NODISCARD
sname_t ia_sname( char *name );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_in_attr_H */
/* vim:set et sw=2 ts=2: */
