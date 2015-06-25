/*
**      ad -- ASCII dump
**      ad.c
**
**      Copyright (C) 1996-2015  Paul J. Lucas
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
#include "color.h"
#include "config.h"
#include "match.h"
#include "options.h"
#include "util.h"

// system
#include <assert.h>
#include <ctype.h>                      /* for isprint() */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */

///////////////////////////////////////////////////////////////////////////////

#define HEX_COLUMN_WIDTH  2             /* bytes per hex column */
#define OFFSET_WIDTH      16            /* number of offset digits */

#define FPRINTF(...) \
  BLOCK( if ( fprintf( fout, __VA_ARGS__ ) < 0 ) PERROR_EXIT( WRITE_ERROR ); )

#define FPUTC(C) \
  BLOCK( if ( fputc( (C), fout ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

#define FPUTS(S) \
  BLOCK( if ( fputs( (S), fout ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

////////// local variables ////////////////////////////////////////////////////

static char *elided_separator;          // separator used for elided rows

////////// local functions ////////////////////////////////////////////////////

static void dump_file( void );
static void dump_row( char const *off_fmt, uint8_t const*, size_t, uint16_t );
static void init( int, char*[] );
static void reverse( void );

/////////// main //////////////////////////////////////////////////////////////

int main( int argc, char *argv[] ) {
  init( argc, argv );
  if ( opt_reverse )
    reverse(); 
  else
    dump_file();
  // neither of the above two functions returns
}

/////////// dumping ///////////////////////////////////////////////////////////

static void dump_file( void ) {
  struct row_buf {
    uint8_t   bytes[ ROW_BUF_SIZE ];    // bytes in buffer, left-to-right
    size_t    len;                      // length of buffer
    uint16_t  match_bits;               // which bytes match, right-to-left
  };
  typedef struct row_buf row_buf_t;

  bool      any_matches = false;        // if matching, any data matched yet?
  row_buf_t buf[2], *cur = buf, *next = buf + 1;
  bool      is_same_row = false;        // current row same as previous?
  kmp_t    *kmps = NULL;                // used only by match_byte()
  uint8_t  *match_buf = NULL;           // used only by match_byte()

  if ( search_len ) {                   // searching for anything?
    kmps = freelist_add( kmp_init( search_buf, search_len ) );
    match_buf = freelist_add( MALLOC( uint8_t, search_len ) );
  }

  char const *off_fmt;
  switch ( opt_offset_fmt ) {
    case OFMT_DEC: off_fmt = "%0" STRINGIFY(OFFSET_WIDTH) "llu"; break;
    case OFMT_HEX: off_fmt = "%0" STRINGIFY(OFFSET_WIDTH) "llX"; break;
    case OFMT_OCT: off_fmt = "%0" STRINGIFY(OFFSET_WIDTH) "llo"; break;
  } // switch

  // prime the pump by reading the first row
  cur->len = match_row( cur->bytes, &cur->match_bits, kmps, match_buf );

  while ( cur->len ) {
    //
    // We need to know whether the current row is the last row.  The current
    // row is the last if its length < ROW_BUF_SIZE.  However, if the file's
    // length is an exact multiple of ROW_BUF_SIZE, then we don't know the
    // current row is the last.  We therefore have to read the next row: the
    // current row is the last row if the length of the next row is zero.
    //
    next->len = cur->len < ROW_BUF_SIZE ? 0 :
      match_row( next->bytes, &next->match_bits, kmps, match_buf );
    bool const is_last_row = next->len == 0;

    if ( cur->match_bits || (           // always dump matching rows
        // Otherwise dump only if:
        //  + for non-matching rows, if not -m
        !opt_only_matching &&
        //  + and if -v, not the same row, or is the last row
        (opt_verbose || !is_same_row || is_last_row) &&
        //  + and if not -p or any printable bytes
        (!opt_only_printing ||
          any_printable( (char*)cur->bytes, cur->len )) ) ) {

      dump_row( off_fmt, cur->bytes, cur->len, cur->match_bits );
      if ( cur->match_bits )
        any_matches = true;
    }

    // Check whether the next row is the same as the current row, but only if:
    is_same_row =
      !(opt_verbose || is_last_row) &&  // + neither -v or is the last row
      cur->len == next->len &&          // + the two row lengths are equal
      memcmp( cur->bytes, next->bytes, ROW_BUF_SIZE ) == 0;

    row_buf_t *const temp = cur;        // swap row pointers to avoid memcpy()
    cur = next, next = temp;

    fin_offset += ROW_BUF_SIZE;
  } // while
  exit( search_len && !any_matches ? EXIT_NO_MATCHES : EXIT_OK );
}

#define SGR_START_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( sgr_start, (EXPR) ); )

#define SGR_END_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( "%s", sgr_end ); )

#define SGR_HEX_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_hex_match ); )

#define SGR_ASCII_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_ascii_match ); )

static void dump_row( char const *off_fmt, uint8_t const *buf, size_t buf_len,
                      uint16_t match_bits ) {
  static bool   any_dumped = false;     // any data dumped yet?
  static off_t  dumped_offset = -1;     // offset of most recently dumped row

  if ( dumped_offset == -1 )
    dumped_offset = fin_offset;

  size_t  buf_pos;
  bool    prev_matches;

  // print row separator (if necessary)
  if ( !opt_only_matching && !opt_only_printing ) {
    off_t const offset_delta = fin_offset - dumped_offset - ROW_BUF_SIZE;
    if ( offset_delta && any_dumped ) {
      SGR_START_IF( sgr_elided );
      FPUTS( elided_separator );
      SGR_END_IF( sgr_elided );
      SGR_START_IF( sgr_sep );
      FPUTC( ':' );
      SGR_END_IF( sgr_sep );
      FPUTC( ' ' );
      SGR_START_IF( sgr_elided );
      FPRINTF( "(%lld | 0x%llX)", offset_delta, offset_delta );
      SGR_END_IF( sgr_elided );
      FPUTC( '\n' );
    }
  }

  // print offset & column separator
  SGR_START_IF( sgr_offset );
  FPRINTF( off_fmt, fin_offset );
  SGR_END_IF( sgr_offset );
  SGR_START_IF( sgr_sep );
  FPUTC( ':' );
  SGR_END_IF( sgr_sep );

  // dump hex part
  prev_matches = false;
  for ( buf_pos = 0; buf_pos < buf_len; ++buf_pos ) {
    bool const matches = match_bits & (1 << buf_pos);
    bool const matches_changed = matches != prev_matches;

    if ( buf_pos % HEX_COLUMN_WIDTH == 0 ) {
      SGR_END_IF( prev_matches );
      FPUTC( ' ' );                     // print space between hex columns
      SGR_HEX_START_IF( prev_matches );
    }
    if ( matches )
      SGR_HEX_START_IF( matches_changed );
    else
      SGR_END_IF( matches_changed );
    FPRINTF( "%02X", (unsigned)buf[ buf_pos ] );
    prev_matches = matches;
  } // for
  SGR_END_IF( prev_matches );

  // print padding if necessary (last row only)
  while ( buf_pos < ROW_BUF_SIZE ) {
    if ( buf_pos++ % HEX_COLUMN_WIDTH == 0 )
      FPUTC( ' ' );                     // print space between hex columns
    FPRINTF( "  " );
  } // while

  // dump ASCII part
  FPRINTF( "  " );
  prev_matches = false;
  for ( buf_pos = 0; buf_pos < buf_len; ++buf_pos ) {
    bool const matches = match_bits & (1 << buf_pos);
    bool const matches_changed = matches != prev_matches;
    uint8_t const byte = buf[ buf_pos ];

    if ( matches )
      SGR_ASCII_START_IF( matches_changed );
    else
      SGR_END_IF( matches_changed );
    FPUTC( isprint( byte ) ? byte : '.' );
    prev_matches = matches;
  } // for
  SGR_END_IF( prev_matches );
  FPUTC( '\n' );

  any_dumped = true;
  dumped_offset = fin_offset;
}

////////// reverse ////////////////////////////////////////////////////////////

enum row_kind {
  ROW_BYTES,
  ROW_ELIDED
};
typedef enum row_kind row_kind_t;

#define XTOI(C) (isdigit( C ) ? (C) - '0' : 0xA + toupper( C ) - 'A')

#define INVALID_EXIT(FORMAT,...)                                          \
  PMESSAGE_EXIT( INVALID_FORMAT,                                          \
    "%s, line %zu, column %zu: " FORMAT, fin_path, line, col, __VA_ARGS__ \
  )

static row_kind_t parse_row( size_t line, char *buf, size_t buf_len,
                             off_t *poffset, uint8_t *bytes,
                             size_t *pbytes_len ) {
  assert( buf );
  assert( poffset );
  assert( bytes );
  assert( pbytes_len );

  size_t col = 1;
  char const *p = buf;

  // maybe parse row separator for elided lines
  if ( strncmp( p, elided_separator, ROW_BUF_SIZE ) == 0 ) {
    p += OFFSET_WIDTH;
    col += OFFSET_WIDTH;
    if ( sscanf( p, ": (%ld | 0x%*lX)", pbytes_len ) != 1 )
      INVALID_EXIT(
        "expected '%c' followed by elided counts \"%s\"\n", ':', "(D | 0xH)"
      );
    return ROW_ELIDED;
  }

  // parse offset
  char *end = NULL;
  errno = 0;
  *poffset = strtoull( p, &end, opt_offset_fmt );
  if ( errno || end == p ) {
    if ( end > p )
      *end = '\0';
    INVALID_EXIT( "\"%s\": unexpected characters; expected file offset\n", p );
  }
  col += OFFSET_WIDTH;
  if ( *end != ':' )
    INVALID_EXIT(
      "'%c': unexpected character; expected ':' after separator\n", *end
    );

  p = end;
  end = buf + buf_len;
  *pbytes_len = 0;
  int consec_spaces = 0;

  // parse hexedecimal bytes
  while ( *pbytes_len < ROW_BUF_SIZE ) {
    ++col;
    if ( ++p == end )
      INVALID_EXIT(
        "unexpected end of line; expected %d hexedecimal bytes\n", ROW_BUF_SIZE
      );

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
    uint8_t byte = XTOI( *p ) << 4;

    // parse second nybble
    ++col;
    if ( ++p == end )
      INVALID_EXIT(
        "unexpected end of data; expected %d hexedecimal bytes\n",
        ROW_BUF_SIZE
      );
    if ( !isxdigit( *p ) )
      goto expected_hex_digit;
    byte |= XTOI( *p );

    bytes[ (*pbytes_len)++ ] = byte;
  } // while
  return ROW_BYTES;

expected_hex_digit:
  INVALID_EXIT(
    "'%c': unexpected character; expected hexedecimal digit\n", *p
  );
}

static void reverse( void ) {
  char   *row_buf = NULL;
  size_t  row_len = 0;
  uint8_t bytes[ ROW_BUF_SIZE ];
  size_t  line = 0;
  off_t   fout_offset = -ROW_BUF_SIZE;

  for ( ;; ) {
    ssize_t const row_len = getline( &row_buf, &row_len, fin );
    if ( row_len == -1 )
      break;
    off_t new_offset;
    size_t bytes_len;
    switch ( parse_row( ++line, row_buf, row_len, &new_offset,
                        bytes, &bytes_len ) ) {
      case ROW_BYTES:
        if ( new_offset < fout_offset + ROW_BUF_SIZE )
          PMESSAGE_EXIT( INVALID_FORMAT,
            "line %zu: \"%lld\": offset goes backwards\n", line, new_offset
          );
        if ( new_offset > fout_offset + ROW_BUF_SIZE )
          FSEEK( fout, new_offset, SEEK_SET );
        if ( fwrite( bytes, 1, bytes_len, fout ) < bytes_len )
          goto write_error;
        break;
      case ROW_ELIDED:
        for ( ; bytes_len; bytes_len -= ROW_BUF_SIZE ) {
          if ( fwrite( bytes, 1, ROW_BUF_SIZE, fout ) < ROW_BUF_SIZE )
            goto write_error;
        } // for
        break;
    } // switch
    fout_offset = new_offset;
  } // for
  exit( EXIT_OK );

write_error:
  PMESSAGE_EXIT( WRITE_ERROR, "%s: write failed: %s\n", fout_path, ERROR_STR );
}

/////////// initialization & clean-up /////////////////////////////////////////

static void clean_up( void ) {
  freelist_free();
  if ( fin )
    fclose( fin );
  if ( fout )
    fclose( fout );
}

static void init( int argc, char *argv[] ) {
  atexit( clean_up );
  parse_options( argc, argv );

  if ( search_buf )                     // searching for a string?
    search_len = strlen( search_buf );
  else if ( search_endian ) {           // searching for a number?
    if ( !search_len )                  // default to smallest possible size
      search_len = int_len( search_number );
    int_rearrange_bytes( &search_number, search_len, search_endian );
    search_buf = (char*)&search_number;
  }

  if ( !opt_max_bytes_to_read )
    exit( search_len ? EXIT_NO_MATCHES : EXIT_OK );

  elided_separator = freelist_add( MALLOC( char, OFFSET_WIDTH + 1 ) );
  memset( elided_separator, '-', OFFSET_WIDTH );
  elided_separator[ OFFSET_WIDTH ] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
