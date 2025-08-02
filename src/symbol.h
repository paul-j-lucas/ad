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
  ad_loc_num_t  first_line;             ///< Line number first mentioned.
  bool          is_used;                ///< True only if actually used.
  
  union {
    ad_decl_t  *decl;
    ad_type_t  *type;
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

extern unsigned sym_scope;              ///< The current scope level.

////////// extern functions ///////////////////////////////////////////////////

/**
 * TODO
 *
 * @param obj TODO
 * @param sname TODO
 */
void* sym_add( void *obj, sname_t *sname, sym_kind_t kind );

void* sym_add_global( void *obj, sname_t *sname );

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
symbol_t* sym_find_name( char const *name );

/**
 * Attempts to find a symbol in the symbol table having \a sname.
 *
 * @param sname The scoped name to find.
 * @return Returns a pointer to the \ref symbol having \a sname or NULL for
 * none.
 *
 * @sa sym_find_name()
 */
symbol_t* sym_find_sname( sname_t const *sname );

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
