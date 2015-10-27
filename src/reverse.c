/*
**      ad -- ASCII dump
**      reverse.c
**
**      Copyright (C) 2015  Paul J. Lucas
**
**      This program is free software; you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation; either version 2 of the Licence, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program; if not, write to the Free Software
**      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// local
#include "config.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>                   /* for PRIu64 */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */

///////////////////////////////////////////////////////////////////////////////

#define INVALID_EXIT(FORMAT,...)                                    \
  PMESSAGE_EXIT( EX_DATAERR,                                        \
    "%s:%zu:%zu: error: " FORMAT, fin_path, line, col, __VA_ARGS__  \
  )

#define FWRITE(PTR,SIZE,N,STREAM) \
  BLOCK( if ( fwrite( (PTR), (SIZE), (N), (STREAM) ) < (N) ) PERROR_EXIT( EX_IOERR ); )

////////// local types ////////////////////////////////////////////////////////

enum row_kind {
  ROW_BYTES,
  ROW_ELIDED
};
typedef enum row_kind row_kind_t;

////////// extern variables ///////////////////////////////////////////////////

extern char *elided_separator;

////////// inline functions ///////////////////////////////////////////////////

/**
 * Converts a single hexadecimal digit [0-9A-Fa-f] to its integer value.
 *
 * @param c The hexadecimal character.
 * @return Returns \a c converted to an integer.
 */
static inline unsigned xtoi( char c ) {
  return isdigit( c ) ? c - '0' : 0xA + toupper( c ) - 'A';
}

////////// local functions ////////////////////////////////////////////////////

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
  assert( buf );
  assert( poffset );
  assert( bytes );
  assert( pbytes_len );

  size_t col = 1;

  // maybe parse row separator for elided lines
  if ( strncmp( buf, elided_separator, ROW_SIZE ) == 0 ) {
    col += OFFSET_WIDTH;
    uint64_t delta;
    if ( sscanf( buf + OFFSET_WIDTH, ": (%" PRIu64 " | 0x%*X)", &delta ) != 1 )
      INVALID_EXIT(
        "expected '%c' followed by elided counts \"%s\"\n", ':', "(DD | 0xHH)"
      );
    *pbytes_len = (size_t)delta;
    return ROW_ELIDED;
  }

  // parse offset
  char const *end = NULL;
  errno = 0;
  *poffset = strtoull( buf, (char**)&end, opt_offset_fmt );
  if ( errno || *end != ':' )
    INVALID_EXIT(
      "\"%s\": unexpected character in %s file offset\n",
      printable_char( *end ), get_offset_fmt_english()
    );
  col += OFFSET_WIDTH;

  char const *p = end;
  end = buf + buf_len;
  size_t bytes_len = 0;
  int consec_spaces = 0;

  // parse hexadecimal bytes
  while ( bytes_len < ROW_SIZE ) {
    ++p, ++col;

    // handle whitespace
    if ( isspace( *p ) ) {
      if ( *p == '\n' /* short row */ || ++consec_spaces == 2 )
        break;
      continue;
    }
    consec_spaces = 0;

    // parse first nybble
    if ( !isxdigit( *p ) )
      goto expected_hex_digit;
    uint8_t byte = xtoi( *p ) << 4;

    // parse second nybble
    ++col;
    if ( ++p == end )
      INVALID_EXIT(
        "unexpected end of data; expected %d hexadecimal bytes\n",
        ROW_SIZE
      );
    if ( !isxdigit( *p ) )
      goto expected_hex_digit;
    byte |= xtoi( *p );

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
  uint8_t bytes[ ROW_SIZE ];
  size_t  bytes_len;
  off_t   fout_offset = -ROW_SIZE;
  size_t  line = 0;
  char    msg_fmt[ 128 ];
  off_t   new_offset;

  for ( ;; ) {
    size_t row_len;
    char *const row_buf = fgetln( fin, &row_len );
    if ( !row_buf ) {
      if ( ferror( fin ) )
        PMESSAGE_EXIT( EX_IOERR, "can not read: %s\n", STRERROR );
      break;
    }
    switch ( parse_row( ++line, row_buf, row_len, &new_offset,
                        bytes, &bytes_len ) ) {
      case ROW_BYTES:
        if ( new_offset < fout_offset + ROW_SIZE )
          goto backwards_offset;
        if ( new_offset > fout_offset + ROW_SIZE )
          FSEEK( fout, new_offset, SEEK_SET );
        FWRITE( bytes, 1, bytes_len, fout );
        fout_offset = new_offset;
        break;

      case ROW_ELIDED:
        assert( bytes_len % ROW_SIZE == 0 );
        fout_offset += bytes_len / ROW_SIZE;
        for ( ; bytes_len; bytes_len -= ROW_SIZE )
          FWRITE( bytes, 1, ROW_SIZE, fout );
        break;
    } // switch

  } // for
  exit( EX_OK );

backwards_offset:
  snprintf( msg_fmt, sizeof( msg_fmt ),
    "%%s:%%zu:1: error: \"%s\": %s offset goes backwards\n",
    get_offset_fmt_format(), get_offset_fmt_english()
  );
  PRINT_ERR( msg_fmt, fin_path, line, new_offset );
  exit( EX_DATAERR );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
