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

// local
#include "pjl_config.h"                 /* must go first */
#include "slist.h"
#include "sname.h"
#include "types.h"
#include "util.h"

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
  sname_t     scope_sname;              ///< Current scope name, if any.
  ad_type_t  *cur_type;                 ///< Current type.
  ad_stmnt_t *statement;                ///< Current field name, if any.
};
typedef struct in_attr in_attr_t;

////////// extern variables ///////////////////////////////////////////////////

extern slist_t in_attr_list;            ///< Inherited attributes.

#define IN_ATTR \
  POINTER_CAST( in_attr_t*, slist_front( &in_attr_list ) )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Frees all memory used by \ref in_attr "inherited attributes".
 *
 * @param ia The \ref in_attr to free.
 */
void ia_free( in_attr_t *ia );

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
 * @param ia The \ref in_attr to use.
 * @param name The local name.  Ownership is taken.
 * @return Returns the current full scoped name.
 */
NODISCARD
sname_t ia_sname( in_attr_t const *ia, char *name );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_in_attr_H */
/* vim:set et sw=2 ts=2: */
