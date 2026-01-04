/*
**      ad -- ASCII dump
**      src/dump_c.c
**
**      Copyright (C) 2015-2026  Paul J. Lucas
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
 * Defines types and functions for dumping a file as a C array.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "match.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for size_t */
#include <stdio.h>
#include <string.h>                     /* for str...() */

/// @endcond

/**
 * @addtogroup dump-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Dumps a single row of bytes as C array data.
 *
 * @param offset_format The \c printf() format for the offset.
 * @param buf A pointer to the current set of bytes to dump.
 * @param buf_len The number of bytes pointed to by \a buf.
 */
static void dump_row_c( char const *offset_format, char8_t const *buf,
                        size_t buf_len ) {
  assert( offset_format != NULL );
  assert( buf != NULL );

  if ( unlikely( buf_len == 0 ) )
    return;

  if ( opt_offsets == OFFSETS_NONE ) {
    PUTC( ' ' );
  } else {
    // print offset
    PUTS( "  /* " );
    PRINTF( offset_format, fin_offset );
    PUTS( " */" );
  }

  // dump hex part
  char8_t const *const end = buf + buf_len;
  while ( buf < end )
    PRINTF( " 0x%02X,", STATIC_CAST( unsigned, *buf++ ) );
  PUTC( '\n' );
}

/////////// extern functions //////////////////////////////////////////////////

/**
 * Dumps a file as a C array.
 */
void dump_file_c( void ) {
  char8_t       buf[ ROW_BYTES_C ];     // buffer of bytes
  match_bits_t  match_bits;             // not used when dumping in C

  // prime the pump by reading the first row
  size_t row_len = match_row(
    buf, sizeof buf, &match_bits, /*kmps=*/NULL,
    /*pmatch_buf=*/NULL, /*pmatch_len=*/NULL
  );
  if ( row_len == 0 )
    return;

  char const *const array_name = strcmp( fin_path, "-" ) == 0 ?
    "stdin" : free_later( identify( base_name( fin_path ) ) );

  PRINTF(
    "%s%s %s%s[] = {\n",
    ((opt_c_array & C_ARRAY_STATIC ) != 0 ? "static " : ""),
    ((opt_c_array & C_ARRAY_CHAR8_T) != 0 ? "char8_t" : "unsigned char"),
    ((opt_c_array & C_ARRAY_CONST  ) != 0 ? "const "  : ""),
    array_name
  );

  size_t array_len = 0;
  char const *const offset_format = get_offsets_format();

  for (;;) {
    dump_row_c( offset_format, buf, row_len );
    fin_offset += STATIC_CAST( off_t, row_len );
    array_len += row_len;
    if ( row_len != sizeof buf )
      break;
    row_len = match_row(
      buf, sizeof buf, &match_bits, /*kmps=*/NULL,
      /*pmatch_buf=*/NULL, /*pmatch_len=*/NULL
    );
  } // for

  PUTS( "};\n" );

  if ( (opt_c_array & C_ARRAY_LEN_ANY) != C_ARRAY_NONE ) {
    PRINTF(
      "%s%s%s%s%s%s%s_len = %zu%s%s;\n",
      ((opt_c_array & C_ARRAY_STATIC      ) != 0 ? "static "   : ""),
      ((opt_c_array & C_ARRAY_LEN_UNSIGNED) != 0 ? "unsigned " : ""),
      ((opt_c_array & C_ARRAY_LEN_LONG    ) != 0 ? "long "     : ""),
      ((opt_c_array & C_ARRAY_LEN_INT     ) != 0 ? "int "      : ""),
      ((opt_c_array & C_ARRAY_LEN_SIZE_T  ) != 0 ? "size_t "   : ""),
      ((opt_c_array & C_ARRAY_CONST       ) != 0 ? "const "    : ""),
      array_name, array_len,
      ((opt_c_array & C_ARRAY_LEN_UNSIGNED) != 0 ? "u" : ""),
      ((opt_c_array & C_ARRAY_LEN_LONG    ) != 0 ? "L" : "")
    );
  }
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
