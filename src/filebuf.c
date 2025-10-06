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

void filebuf_cleanup( filebuf_t *fbuf ) {
  assert( fbuf != NULL );
  free( fbuf->buf );
  *fbuf = (filebuf_t){ 0 };
}

bool filebuf_read( filebuf_t *fbuf, char *dst, size_t bytes_to_read ) {
  assert( fbuf != NULL );
  assert( dst != NULL );

  size_t const avail = fbuf->len - fbuf->pos;
  if ( bytes_to_read <= avail ) {
    memcpy( dst, fbuf->buf + fbuf->pos, bytes_to_read );
  }
  else {
    memcpy( dst, fbuf->buf + fbuf->pos, avail );
    bytes_to_read -= avail;

    filebuf_reserve( fbuf, bytes_to_read );
    size_t const bytes_read = fread( fbuf->buf, bytes_to_read, 1, fbuf->file );
    if ( unlikely( ferror( fbuf->file ) ) )
      return false;
    if ( unlikely( bytes_read < bytes_to_read ) )
      return false;
    memcpy( dst + avail, fbuf->buf, bytes_to_read );
    fbuf->len = fbuf->pos = 0;
  }

  return true;
}

void filebuf_skip( filebuf_t *fbuf, size_t bytes_to_skip ) {
  assert( fbuf != NULL );

  size_t const avail = fbuf->len - fbuf->pos;
  if ( bytes_to_skip <= avail ) {
    fbuf->pos += bytes_to_skip;
  }
  else {
    fskip( STATIC_CAST( off_t, bytes_to_skip - avail ), fbuf->file );
    fbuf->len = fbuf->pos = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////

extern inline void filebuf_init( filebuf_t*, FILE* );

/* vim:set et sw=2 ts=2: */

