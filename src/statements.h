/*
**      ad -- ASCII dump
**      src/statements.h
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

#ifndef ad_statements_H
#define ad_statements_H

// local
#include "pjl_config.h"                 /* must go first */
#include "slist.h"
#include "types.h"

// standard
#include <stdbool.h>
#include <stdint.h>

_GL_INLINE_HEADER_BEGIN
#ifndef AD_STATEMENTS_H_INLINE
# define AD_STATEMENTS_H_INLINE _GL_INLINE
#endif /* AD_STATEMENTS_H_INLINE */

///////////////////////////////////////////////////////////////////////////////

struct ad_compound_statement {
  slist statements;
};

struct ad_declaration {
};

struct ad_switch_statement {
};

struct ad_statement {
  union {
    ad_compound_statement_t compound;   ///< Compound statement.
    ad_declaration_t        declaration;///< Declaration.
    ad_switch_statement_t   st_switch;  ///< `switch` statement.
  } as;                                 ///< Union discriminator.
  ad_statement_kind_t kind;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Frees all the memory used by \a type.
 *
 * @param type The `ad_type` to free.  May be null.
 */
void ad_type_free( ad_type_t *type );

/**
 * Creates a new `ad_type`.
 *
 * @param tid The ID of the type to create.
 */
NODISCARD
ad_type_t* ad_type_new( ad_tid_t tid );

/**
 * Gets the size (in bits) of the type represented by \a tid.
 *
 * @param tid The ID of the type to get the size of.
 * @return Returns said size.
 */
NODISCARD AD_TYPES_H_INLINE
size_t ad_type_size( ad_tid_t tid ) {
  return tid & T_MASK_SIZE;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_statements_H */
/* vim:set et sw=2 ts=2: */
