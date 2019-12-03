/*
**      ad -- ASCII dump
**      src/reverse.c
**
**      Copyright (C) 2015-2018  Paul J. Lucas
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
#include "common.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>                   /* for SCNu64 */
#include <stddef.h>                     /* fir size_t */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

#define INVALID_EXIT(FORMAT,...)                                    \
  PMESSAGE_EXIT( EX_DATAERR,                                        \
    "%s:%zu:%zu: error: " FORMAT, fin_path, line, col, __VA_ARGS__  \
  )

#define FWRITE(PTR,SIZE,N,STREAM) BLOCK( \
  if ( unlikely( fwrite( (PTR), (SIZE), (N), (STREAM) ) < (N) ) ) perror_exit( EX_IOERR ); )

enum row_kind {
  ROW_BYTES,
  ROW_ELIDED,
  ROW_IGNORE
};
typedef enum row_kind row_kind_t;

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a c is an offset delimiter character.
 *
 * @param c The character to check.
 * @return Returns \c true only if \a c is an offset delimiter character.
 */
static inline bool is_offset_delim( char c ) {
  return c == ':' || isspace( c );
}

/**
 * Converts a single hexadecimal digit [0-9A-Fa-f] to its integer value.
 *
 * @param c The hexadecimal character.
 * @return Returns \a c converted to an integer.
 */
static inline unsigned xtoi( char c ) {
  return isdigit( c ) ?
    (unsigned)(c - '0') : 0xAu + (unsigned)(toupper( c ) - 'A');
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
static row_kind_t parse_row( size_t line, char const *buf, size_t buf_len,
                             off_t *poffset, uint8_t *bytes,
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
    if ( unlikely( sscanf( buf, ": (%" SCNu64 " | 0x%*X)", &delta ) != 1 ) )
      INVALID_EXIT(
        "expected '%c' followed by elided counts \"%s\"\n", ':', "(DD | 0xHH)"
      );
    *pbytes_len = STATIC_CAST(size_t, delta);
    return ROW_ELIDED;
  }

  // parse offset
  char const *end = NULL;
  errno = 0;
  *poffset = (off_t)strtoull( buf, REINTERPRET_CAST(char**, &end), (int)opt_offset_fmt );
  if ( unlikely( errno != 0 || (end[0] != '\0' && !is_offset_delim( *end )) ) )
    INVALID_EXIT(
      "\"%s\": unexpected character in %s file offset\n",
      printable_char( *end ), get_offset_fmt_english()
    );
  if ( unlikely( end[0] == '\n' || end[0] == '\0' ) )
    return ROW_IGNORE;
  col += (size_t)(end - buf);

  char const *p = end;
  end = buf + buf_len;
  size_t bytes_len = 0;
  unsigned consec_spaces = 0;

  // parse hexadecimal bytes
  while ( bytes_len < row_bytes ) {
    ++p, ++col;

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
    uint8_t byte = (uint8_t)(xtoi( *p ) << 4);

    // parse second nybble
    ++col;
    if ( unlikely( ++p == end ) )
      INVALID_EXIT(
        "unexpected end of data; expected %zu hexadecimal bytes\n",
        row_bytes
      );
    if ( unlikely( !isxdigit( *p ) ) )
      goto expected_hex_digit;
    byte |= (uint8_t)xtoi( *p );

    bytes[ bytes_len++ ] = byte;
  } // while

  *pbytes_len = bytes_len;
  return ROW_BYTES;

expected_hex_digit:
  INVALID_EXIT(
    "'%s': unexpected character; expected hexadecimal digit\n",
    printable_char( *p )
  );
}

////////// extern functions ///////////////////////////////////////////////////

void reverse_dump_file( void ) {
  uint8_t bytes[ row_bytes ];
  size_t  bytes_len;
  off_t   fout_offset = -(off_t)row_bytes;
  size_t  line = 0;
  char    msg_fmt[ 128 ];
  off_t   new_offset;

  for ( ;; ) {
    size_t row_len;
    char *const row_buf = fgetln( fin, &row_len );
    if ( row_buf == NULL ) {
      if ( unlikely( ferror( fin ) ) )
        PMESSAGE_EXIT( EX_IOERR, "can not read: %s\n", STRERROR );
      break;
    }
    switch ( parse_row( ++line, row_buf, row_len, &new_offset,
                        bytes, &bytes_len ) ) {
      case ROW_BYTES: {
        off_t const row_end_offset = fout_offset + (off_t)row_bytes;
        if ( unlikely( new_offset < row_end_offset ) )
          goto backwards_offset;
        if ( new_offset > row_end_offset )
          FSEEK( fout, new_offset, SEEK_SET );
        FWRITE( bytes, 1, bytes_len, fout );
        fout_offset = new_offset;
        break;
      }

      case ROW_ELIDED:
        assert( bytes_len % row_bytes == 0 );
        fout_offset += (off_t)(bytes_len / row_bytes);
        for ( ; bytes_len > 0; bytes_len -= row_bytes )
          FWRITE( bytes, 1, row_bytes, fout );
        break;

      case ROW_IGNORE:
        break;
    } // switch

  } // for
  exit( EX_OK );

backwards_offset:
  snprintf( msg_fmt, sizeof msg_fmt,
    "%%s:%%zu:1: error: \"%s\": %s offset goes backwards\n",
    get_offset_fmt_format(), get_offset_fmt_english()
  );
  PRINT_ERR( msg_fmt, fin_path, line, new_offset );
  exit( EX_DATAERR );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
