/*
**      PJL Library
**      src/filebuf.h
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

#ifndef pjl_filebuf_H
#define pjl_filebuf_H

/**
 * @file
 * Declares a file buffer type and functions for manipulating it.
 */

// local
#include "pjl_config.h"                 /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdio.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * <code>%filebuf</code> keeps a buffer read from a file that may not be
 * seekable, e.g., from a pipe.
 */
struct filebuf {
  FILE   *file;                         ///< The `FILE` to read from.
  char   *buf;                          ///< Bytes comprising buffer.
  size_t  cap;                          ///< Capacity of \a buf.
  size_t  len;                          ///< Length of \a buf.
  size_t  pos;                          ///< Current position within \a buf.
};
typedef struct filebuf filebuf_t;

///////////////////////////////////////////////////////////////////////////////

/**
 * Cleans-up all memory associated with \a fbuf but does _not_ free \a fbuf
 * itself.
 *
 * @param fbuf A pointer to the \ref filebuf to clean up.
 *
 * @sa filebuf_init()
 */
void filebuf_cleanup( filebuf_t *fbuf );

/**
 * Initializes a \ref filebuf.
 *
 * @param fbuf A pointer to the \ref filebuf to initialize.
 *
 * @sa filebuf_cleanup()
 */
inline void filebuf_init( filebuf_t *fbuf, FILE *file ) {
  *fbuf = (filebuf_t){ .file = file };
}

/**
 * Reads \a bytes_to_read bytes from \a fbuf into \a dst.
 *
 * @warning The file position is _not_ advanced.
 *
 * @param fbuf A pointer to the \ref filebuf to read from.
 * @param dst A pointer to the buffer to receive the bytes read.
 * @param bytes_to_read The number of bytes to read.
 * @return Returns `true` only if \a bytes_to_read bytes were read
 * successfully.
 *
 * @sa filebuf_skip()
 */
bool filebuf_read( filebuf_t *fbuf, char *dst, size_t bytes_to_read );

/**
 * Skips over \a bytes_to_skip bytes.
 * If an error occurs, prints an error message and exits.
 *
 * @param fbuf A pointer to the \ref filebuf to skip from.
 * @param bytes_to_skip The number of bytes to skip.
 *
 * @sa filebuf_read()
 */
void filebuf_skip( filebuf_t *fbuf, size_t bytes_to_skip );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* pjl_filebuf_H */
/* vim:set et sw=2 ts=2: */
