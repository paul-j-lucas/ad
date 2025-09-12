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
  size_t fbuf_pos;

  for (;;) {
    size_t const avail = fbuf->len - fbuf->pos;
    if ( avail < size ) {
      if ( avail == 0 ) {
        size_t const bytes_read = fread( fbuf->buf, size, 1, fbuf->file );
        if ( unlikely( ferror( fbuf->file ) ) )
          return false;
      }
      else {
        memcpy( dst + dst_pos, fbuf->buf, avail );
      }
    }

  } // for

  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

