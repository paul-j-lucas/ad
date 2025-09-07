/*
**      PJL Library
**      src/array.c
**
**      Copyright (C) 2025  Paul J. Lucas
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
#define ARRAY_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "array.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>                     /* for memcpy(3) */

////////// extern functions ///////////////////////////////////////////////////

void array_cleanup( array_t *array, array_free_fn_t free_fn ) {
  assert( array != NULL );

  if ( free_fn != NULL ) {
    char *element = POINTER_CAST( char*, array->elements );
    for ( size_t i = 0; i < array->len; ++i ) {
      (*free_fn)( element );
      element += array->esize;
    } // for
  }

  free( array->elements );
  array_init( array, array->esize );
}

int array_cmp( array_t const *i_array, array_t const *j_array,
               array_cmp_fn_t cmp_fn ) {
  assert( i_array != NULL );
  assert( j_array != NULL );
  assert( cmp_fn != NULL );

  if ( i_array == j_array )
    return 0;

  void const *i_element = i_array->elements, *j_element = j_array->elements;
  void const *const i_end = array_at_nocheck( i_array, i_array->len );
  void const *const j_end = array_at_nocheck( j_array, j_array->len );

  for ( ; i_element < i_end && j_element < j_end;
        i_element += i_array->esize, j_element += j_array->esize ) {
    int const cmp = (*cmp_fn)( i_element, j_element );
    if ( cmp != 0 )
      return cmp;
  } // for

  return i_element == i_end ? (j_element == j_end ? 0 : -1) : 1;
}

void* array_push_back( array_t *array, void const *src ) {
  assert( array != NULL );
  assert( src != NULL );
  size_t const index = array->len;
  array_reserve( array, ++array->len );
  return memcpy( array_at_nocheck( array, index ), src, array->esize );
}

bool array_reserve( array_t *array, size_t res_len ) {
  assert( array != NULL );
  if ( res_len < array->cap - array->len )
    return false;
  if ( array->cap == 0 )
    array->cap = 2;
  size_t const new_len = array->len + res_len;
  while ( array->cap <= new_len )
    array->cap <<= 1;
  REALLOC( array->elements, array->cap * array->esize );
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
