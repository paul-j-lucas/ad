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
#define AD_TYPES_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "types.h"

// standard
#include <assert.h>

////////// extern functions ///////////////////////////////////////////////////

void ad_type_free( ad_type_t *type ) {
  switch ( type->type_id & T_MASK_TYPE ) {
    case T_STRUCT:
      break;
    case T_SWITCH:
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

ad_type_t* ad_type_new( ad_type_id_t tid ) {
  assert( tid != T_NONE );

  ad_type_t *const type = MALLOC( ad_type_t, 1 );
  MEM_ZERO( type );

  return type;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
