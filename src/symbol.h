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
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdio.h>

_GL_INLINE_HEADER_BEGIN
#ifndef RED_BLACK_H_INLINE
# define RED_BLACK_H_INLINE _GL_INLINE
#endif /* RED_BLACK_H_INLINE */

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
  char const *name;                     ///< Symbol name.
  slist_t     synfo_list;               ///< Per-scope information.
};
typedef struct symbol symbol_t;

////////// extern functions ///////////////////////////////////////////////////

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
 */
symbol_t* sym_find_name( char const *name );

/**
 * Initializes all symbol table data.
 *
 * @note This function must be called exactly once.
 */
void sym_init( void );

/**
 * Opens a new scope for the symbol table.
 *
 * @sa sym_close_scope()
 */
void sym_open_scope( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* ad_symbol_H */
/* vim:set et sw=2 ts=2: */
