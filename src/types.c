/*
**      ad -- ASCII dump
**      src/types.c
**
**      Copyright (C) 2019  Paul J. Lucas
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

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define AD_TYPES_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "expr.h"
#include "slist.h"
#include "types.h"

// standard
#include <assert.h>

////////// local functions ////////////////////////////////////////////////////

/**
 * Frees all memory used by \a value _including_ \a value itself.
 *
 * @param value The \ref ad_enum_value to free.
 */
static void ad_enum_value_free( ad_enum_value_t *value ) {
  if ( value != NULL ) {
    FREE( value->name );
    free( value );
  }
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_type_equal( ad_type_t const *i_type, ad_type_t const *j_type ) {
  if ( i_type == j_type )
    return true;
  if ( i_type == NULL || j_type == NULL )
    return false;

  // ...

  return true;
}

void ad_type_free( ad_type_t *type ) {
  if ( type == NULL )
    return;
  switch ( type->tid & T_MASK_TYPE ) {
    case T_ENUM:
      slist_cleanup(
        &type->ad_enum.values,
        POINTER_CAST( slist_free_fn_t, &ad_enum_value_free )
      );
      break;
    case T_STRUCT:
      break;
    case T_BOOL:
    case T_ERROR:
    case T_FLOAT:
    case T_INT:
    case T_UTF:
      // nothing to do
      break;
  } // switch
}

ad_type_t* ad_type_new( ad_tid_t tid ) {
  assert( tid != T_NONE );

  ad_type_t *const type = MALLOC( ad_type_t, 1 );
  *type = (ad_type_t){ 0 };

  return type;
}

unsigned ad_type_size( ad_type_t const *t ) {
  unsigned const bits = ad_tid_size( t->tid );
  if ( bits != 0 )
    return bits;
  assert( ad_expr_is_value( t->size_expr ) );
  return t->size_expr->value.u32;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
