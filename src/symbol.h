/*
**      ad -- ASCII dump
**      src/symbol.h
**
**      Copyright (C) 2024  Paul J. Lucas
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

#ifndef ad_symbol_H
#define ad_symbol_H

/**
 * @file
 * Declares a type to represent a symbol table as well as functions for
 * manipulating it.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "slist.h"
#include "sname.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>

_GL_INLINE_HEADER_BEGIN
#ifndef SYMBOL_H_INLINE
# define SYMBOL_H_INLINE _GL_INLINE
#endif /* SYMBOL_H_INLINE */

/// @endcond

/**
 * @defgroup symbol-group Symbol Table.
 * A type to represent a symbol table and functions for manipulating it.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Kinds of symbols in the %symbol table.
 */
enum sym_kind {
  SYM_DECL,                       ///< A declaration.
  SYM_TYPE,                       ///< A type: `enum`, `struct`, or `typedef`.
};
typedef enum sym_kind sym_kind_t;

/**
 * SYmbol iNFO.
 */
struct synfo {
  unsigned      scope;                  ///< Scope level.
  sym_kind_t    kind;                   ///< Kind of symbol information.
  ad_loc_t      first_loc;              ///< Location first mentioned.
  bool          is_used;                ///< True only if actually used.
  
  union {
    ad_decl_t  *decl;
    ad_type_t  *type;
    void       *obj;
  };
};
typedef struct synfo synfo_t;

/**
 * A %symbol in a %symbol table.
 */
struct symbol {
  sname_t sname;                        ///< Symbol name.
  slist_t synfo_list;                   ///< Per-scope information.
};
typedef struct symbol symbol_t;

/**
 * The signature for a function passed to sym_visit().
 *
 * @param sym The \ref symbol to visit.
 * @param visit_data Optional data passed to the visitor.
 * @return Returning `true` will cause traversal to stop and a pointer to the
 * \ref symbol the visitor stopped on to be returned to the caller of
 * sym_visit().
 */
typedef bool (*sym_visit_fn_t)( symbol_t const *sym, void *visit_data );

extern unsigned sym_scope;              ///< The current scope level.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Possibly adds \a obj to the symbol table.
 *
 * @param obj A pointer to the object to add.
 * @param sname The scoped name for the symbol.
 * @param scope The scope of \a obj.
 * @return Returns a pointer to either the \ref synfo for an existing symbol if
 * that \ref synfo's scope is &le; \a scope (the object already exists at the
 * same scope) or a new \ref synfo if added.
 */
NODISCARD
synfo_t* sym_add( void *obj, sname_t const *sname, sym_kind_t kind,
                  unsigned scope );

/**
 * Closes the current scope for the symbol table.
 *
 * @sa sym_open_scope()
 */
void sym_close_scope( void );

/**
 * Attempts to find a symbol in the symbol table having \a name.
 *
 * @param name The name to find.
 * @return Returns a pointer to the \ref symbol having \a name or NULL for
 * none.
 *
 * @sa sym_find_sname()
 */
NODISCARD
synfo_t* sym_find_name( char const *name );

/**
 * Attempts to find a symbol in the symbol table having \a sname.
 *
 * @param sname The scoped name to find.
 * @return Returns a pointer to the \ref symbol having \a sname or NULL for
 * none.
 *
 * @sa sym_find_name()
 */
NODISCARD
synfo_t* sym_find_sname( sname_t const *sname );

/**
 * Opens a new scope for the symbol table.
 *
 * @sa sym_close_scope()
 */
SYMBOL_H_INLINE
void sym_open_scope( void ) {
  ++sym_scope;
}

/**
 * Does an in-order traversal of all \ref ad_type.
 *
 * @param visit_fn The visitor function to use.
 * @param visit_data Optional data passed to \a visit_fn.
 */
void sym_visit( sym_visit_fn_t visit_fn, void *visit_data );

/**
 * Initializes the symbol table.
 *
 * @note This function must be called exactly once.
 */
void sym_table_init( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* ad_symbol_H */
/* vim:set et sw=2 ts=2: */
