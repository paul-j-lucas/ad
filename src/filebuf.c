/*
**      PJL Library
**      src/filebuf.c
**
**      Copyright (C) 2025  Paul J. Lucas, et al.
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

/**
 * @file
 * Defines functions for manipulating a file buffer.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define FILEBUF_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "filebuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

////////// local functions ////////////////////////////////////////////////////

/**
 * Ensures at least \a res_len additional bytes of capacity exist in \a fbuf.
 *
 * @param fbuf A pointer to the \ref filebuf to reserve \a res_len additional
 * bytes for.
 * @param res_len The number of additional bytes to reserve.
 * @return Returns `true` only if a memory reallocation was necessary.
 */
PJL_DISCARD
static bool filebuf_reserve( filebuf_t *fbuf, size_t res_len ) {
  assert( fbuf != NULL );
  if ( res_len < fbuf->cap - fbuf->len )
    return false;
  if ( fbuf->cap == 0 )
    fbuf->cap = 2;
  size_t const new_len = fbuf->len + res_len;
  while ( fbuf->cap <= new_len )
    fbuf->cap <<= 1;
  REALLOC( fbuf->buf, fbuf->cap );
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool filebuf_advance( filebuf_t *fbuf, size_t size ) {
  assert( fbuf != NULL );

  size_t advanced = 0;

  while ( advanced < size ) {
    if ( fbuf->pos == fbuf->len ) {
      fbuf->len = fread( fbuf->buf, 1, fbuf->cap, stdin );
      if ( fbuf->len < fbuf->cap )
        return false;
      fbuf->pos = 0;
    }

    size_t avail = fbuf->len - fbuf->pos;
  } // while

  return true;
}

void filebuf_cleanup( filebuf_t *fbuf ) {
  assert( fbuf != NULL );
  free( fbuf->buf );
  *fbuf = (filebuf_t){ 0 };
}

bool filebuf_read( filebuf_t *fbuf, char *dst, size_t size ) {
  assert( fbuf != NULL );
  assert( dst != NULL );

  size_t dst_pos = 0;

  for (;;) {
    size_t const avail = fbuf->len - fbuf->pos;
    if ( avail < size ) {

      filebuf_reserve( fbuf, size );
      size_t const rest = size - avail;
      size_t const bytes_read = fread( fbuf->buf + fbuf->pos, rest, 1, fbuf->file );
      if ( unlikely( ferror( fbuf->file ) ) )
        return false;

      memcpy( dst, fbuf->buf, avail );
      dst_pos += avail;
    }
    memcpy( dst + dst_pos, fbuf->buf, avail );

  } // for

  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

