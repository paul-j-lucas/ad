/*
**      ad -- ASCII dump
**      src/reverse.c
**
**      Copyright (C) 2015-2024  Paul J. Lucas
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
 * Defines types and functions for reverse dumping (patching) a file.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "options.h"
#include "unicode.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>                   /* for SCNu64 */
#include <stddef.h>                     /* fir size_t */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sysexits.h>

/// @endcond

/**
 * @defgroup reverse-dump-group Reverse Dumping
 * Types and functions for reverse dumping (patching) a file.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Calls **fwrite**(3) on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param PTR A pointer to the object(s) to write.
 * @param SIZE The size of each object in bytes.
 * @param N The number of objects to write.
 * @param STREAM The `FILE` stream to write to.
 */
#define FWRITE(PTR,SIZE,N,STREAM) \
  PERROR_EXIT_IF( fwrite( (PTR), (SIZE), (N), (STREAM) ) < (N), EX_IOERR )

/**
 * Convenience macro for calling fatal_error().
 *
 * @param LINE The line in the **ad** file on which the error occurred.
 * @param COL The column in the **ad** file at which the error occurred.
 * @param FORMAT The `printf()` format.
 * @param .. The `printf()` arguments.
 */
#define INVALID_EXIT(LINE,COL,FORMAT,...)                               \
  fatal_error( EX_DATAERR,                                              \
    "%s:%zu:%zu: error: " FORMAT, fin_path, (LINE), (COL), __VA_ARGS__  \
  )

/**
 * The kinds of rows in an **ad** dump file.
 */
enum row_kind {
  ROW_BYTES,                            ///< Ordinary row of bytes.
  ROW_ELIDED,                           ///< Elided row.
  ROW_IGNORE                            ///< Row to ignore.
};
typedef enum row_kind row_kind_t;

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a c is an offset delimiter character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is an offset delimiter character.
 */
NODISCARD
static inline bool is_offset_delim( char c ) {
  return c == ':' || isspace( c );
}

/**
 * Converts a single hexadecimal digit `[0-9A-Fa-f]` to its integer value.
 *
 * @param c The hexadecimal character.
 * @return Returns \a c converted to an integer.
 */
NODISCARD
static inline unsigned xtoi( char c ) {
  return isdigit( c ) ?
    STATIC_CAST( unsigned, c - '0' ) :
    0xAu + STATIC_CAST( unsigned, toupper( c ) - 'A' );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Parses an elided separator.
 *
 * @param buf A pointer to the buffer to parse.
 * @param buf_len The number of characters pointer to by \a buf.
 * @return Returns the width of the separator if between the minimum and
 * maximum valid offset widths.
 */
NODISCARD
static size_t parse_elided_separator( char const *buf, size_t buf_len ) {
  size_t n = 0;
  while ( n < buf_len && buf[n] == ELIDED_SEP_CHAR ) {
    if ( unlikely( ++n > OFFSET_WIDTH_MAX ) )
      return 0;
  } // while
  return n >= OFFSET_WIDTH_MIN ? n : 0;
}

/**
 * Parses a row of dump data.
 *
 * @param line The line number within the file being parsed.
 * @param buf A pointer to the buffer to parse.
 * @param buf_len The number of characters pointer to by \a buf.
 * @param poffset The parsed offset.
 * @param bytes The parsed bytes.
 * @param pbytes_len The length of \a bytes.
 * @return Returns the kind of row that was parsed.
 */
NODISCARD
static row_kind_t parse_row( size_t line, char const *buf, size_t buf_len,
                             off_t *poffset, char8_t *bytes,
                             size_t *pbytes_len ) {
  assert( buf != NULL );
  assert( poffset != NULL );
  assert( bytes != NULL );
  assert( pbytes_len != NULL );

  size_t col = 1;

  // maybe parse row separator for elided lines
  size_t const elided_sep_width = parse_elided_separator( buf, buf_len );
  if ( elided_sep_width > 0 ) {
    col += elided_sep_width;
    uint64_t delta;
    buf += elided_sep_width;
    if ( unlikely( sscanf( buf, ": (%" SCNu64 " | 0x%*X)", &delta ) != 1 ) ) {
      INVALID_EXIT( line, col,
        "expected '%c' followed by elided counts \"%s\"\n", ':', "(DD | 0xHH)"
      );
    }
    *pbytes_len = STATIC_CAST(size_t, delta);
    return ROW_ELIDED;
  }

  // parse offset
  char const *end = NULL;
  errno = 0;
  *poffset = STATIC_CAST( off_t,
    strtoull(
      buf, POINTER_CAST( char**, &end ), STATIC_CAST( int, opt_offsets )
    )
  );
  if ( unlikely( errno != 0 || (*end != '\0' && !is_offset_delim( *end )) ) ) {
    INVALID_EXIT( line, col,
      "\"%s\": unexpected character in %s file offset\n",
      printable_char( *end ), gets_offsets_english()
    );
  }
  if ( unlikely( end[0] == '\n' || end[0] == '\0' ) )
    return ROW_IGNORE;
  col += STATIC_CAST( size_t, end - buf );

  char const *p = end;
  end = buf + buf_len;
  size_t bytes_len = 0;
  unsigned consec_spaces = 0;

  // parse hexadecimal bytes
  while ( bytes_len < row_bytes ) {
    ++p;
    ++col;

    // handle whitespace
    if ( isspace( *p ) ) {
      if ( unlikely( *p == '\n' ) )
        break;                          // unexpected (expected ASCII), but OK
      if ( ++consec_spaces == 2 + (bytes_len == 8) )
        break;                          // short row
      continue;
    }
    consec_spaces = 0;

    // parse first nybble
    if ( unlikely( !isxdigit( *p ) ) )
      goto expected_hex_digit;
    char8_t byte = STATIC_CAST( char8_t, xtoi( *p  ) << 4 );

    // parse second nybble
    ++col;
    if ( unlikely( ++p == end ) )
      INVALID_EXIT( line, col,
        "unexpected end of data; expected %zu hexadecimal bytes\n",
        row_bytes
      );
    if ( unlikely( !isxdigit( *p ) ) )
      goto expected_hex_digit;
    byte |= STATIC_CAST( char8_t, xtoi( *p ) );

    bytes[ bytes_len++ ] = byte;
  } // while

  *pbytes_len = bytes_len;
  return ROW_BYTES;

expected_hex_digit:
  INVALID_EXIT( line, col,
    "'%s': unexpected character; expected hexadecimal digit\n",
    printable_char( *p )
  );
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Reverse dumps (patches) a file.
 */
void reverse_dump_file( void ) {
  char8_t bytes[ ROW_BYTES_MAX ];
  size_t  bytes_len;
  off_t   offset = -STATIC_CAST( off_t, row_bytes );
  size_t  line = 0;
  char    msg_fmt[ 128 ];
  off_t   new_offset;

  for (;;) {
    size_t row_len;
    char *const row_buf = fgetln( stdin, &row_len );
    if ( row_buf == NULL ) {
      if ( unlikely( ferror( stdin ) ) )
        fatal_error( EX_IOERR, "can not read: %s\n", STRERROR() );
      break;
    }
    switch ( parse_row( ++line, row_buf, row_len, &new_offset,
                        bytes, &bytes_len ) ) {
      case ROW_BYTES: {
        off_t const row_end_offset = offset + STATIC_CAST( off_t, row_bytes );
        if ( unlikely( new_offset < row_end_offset ) )
          goto backwards_offset;
        if ( new_offset > row_end_offset )
          FSEEK( stdout, new_offset, SEEK_SET );
        FWRITE( bytes, 1, bytes_len, stdout );
        offset = new_offset;
        break;
      }

      case ROW_ELIDED:
        assert( bytes_len % row_bytes == 0 );
        offset += STATIC_CAST( off_t, bytes_len / row_bytes );
        for ( ; bytes_len > 0; bytes_len -= row_bytes )
          FWRITE( bytes, 1, row_bytes, stdout );
        break;

      case ROW_IGNORE:
        break;
    } // switch

  } // for
  exit( EX_OK );

backwards_offset:
  snprintf( msg_fmt, sizeof msg_fmt,
    "%%s:%%zu:1: error: \"%s\": %s offset goes backwards\n",
    get_offsets_format(), gets_offsets_english()
  );
  EPRINTF( msg_fmt, fin_path, line, new_offset );
  exit( EX_DATAERR );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
