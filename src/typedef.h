/*
**      ad -- ASCII dump
**      src/typedef.h
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

#ifndef ad_typedef_H
#define ad_typedef_H

/**
 * @file
 * Declares types and functions for adding and looking up `typedef`
 * declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

/**
 * Convenience macro for specifying a \ref ad_typedef literal having \a TYPE.
 *
 * @param TYPE The type.
 */
#define AD_TYPEDEF_LIT(TYPE)      (ad_typedef_t const){ (TYPE) }

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup c-typedef-group C/C++ Typedef Declarations
 * Types and functions for adding and looking up C `typedef` declarations.
 * @{
 */

/**
 * **ad** C/C++ `typedef` information.
 */
struct ad_typedef {
  ad_type_t const *type;                ///< The underlying type.
};

/**
 * The signature for a function passed to ad_typedef_visit().
 *
 * @param tdef The \ref ad_typedef to visit.
 * @param v_data Optional data passed to the visitor.
 * @return Returning `true` will cause traversal to stop and a pointer to the
 * \ref ad_typedef the visitor stopped on to be returned to the caller of
 * ad_typedef_visit().
 */
typedef bool (*ad_typedef_visit_fn_t)( ad_typedef_t const *tdef, void *v_data );

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a new `typedef` to the global set.
 *
 * @param type_ast The AST of the type to add.  Ownership is taken only if the
 * type was added.
 * @return Returns the \ref ad_typedef of either:
 * + The newly added type (its AST's \ref c_ast.unique_id "unique_id" is equal
 *   to \a type_ast's `unique_id`); or:
 * + The previously added type having the same scoped name.
 */
NODISCARD
ad_typedef_t const* ad_typedef_add( c_ast_t const *type_ast );

/**
 * Gets the \ref ad_typedef for \a name.
 *
 * @param name The name to find.  It may contain `::`.
 * @return Returns a pointer to the corresponding \ref ad_typedef or NULL for
 * none.
 */
NODISCARD
ad_typedef_t const* ad_typedef_find_name( char const *name );

/**
 * Initializes all \ref ad_typedef data.
 */
void ad_typedef_init( void );

/**
 * Does an in-order traversal of all \ref ad_typedef.
 *
 * @param visit_fn The visitor function to use.
 * @param v_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the \ref ad_typedef the visitor stopped on or
 * NULL.
 */
PJL_DISCARD
ad_typedef_t const* ad_typedef_visit( ad_typedef_visit_fn_t visit_fn,
                                      void *v_data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_typedef_H */
/* vim:set et sw=2 ts=2: */
