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
#include <ctype.h>                      /* for isprint() */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/types.h>

///////////////////////////////////////////////////////////////////////////////

#define COLUMN_WIDTH    2               /* bytes per hex column */
#define OFFSET_WIDTH    16              /* number of offset digits */

// local functions
static void init( int, char*[] );

/////////// dumping ///////////////////////////////////////////////////////////

#define SGR_START_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) PRINTF( sgr_start, (EXPR) ); )

#define SGR_END_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) PRINTF( "%s", sgr_end ); )

#define SGR_HEX_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_hex_match ); )

#define SGR_ASCII_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_ascii_match ); )

int main( int argc, char *argv[] ) {
  struct row_buf {
    uint8_t   bytes[ ROW_BUF_SIZE ];    // bytes in buffer, left-to-right
    size_t    len;                      // length of buffer
    uint16_t  match_bits;               // which bytes match, right-to-left
  };
  typedef struct row_buf row_buf_t;

  static char const *const offset_fmt_printf[] = {
    "%0" STRINGIFY(OFFSET_WIDTH) "llu", // decimal
    "%0" STRINGIFY(OFFSET_WIDTH) "llX", // hex
    "%0" STRINGIFY(OFFSET_WIDTH) "llo"  // octal
  };

  init( argc, argv );

  bool      any_dumped = false;         // any data dumped yet?
  bool      any_matches = false;        // if matching, any data matched yet?
  row_buf_t buf[2], *cur = buf, *next = buf + 1;
  off_t     dumped_offset = file_offset;// most recently dumped row offset
  bool      is_same_row = false;        // current row same as previous?
  kmp_t    *kmps = NULL;                // used only by match_byte()
  uint8_t  *match_buf = NULL;           // used only by match_byte()

  if ( search_len ) {                   // searching for anything?
    kmps = freelist_add( kmp_init( search_buf, search_len ) );
    match_buf = freelist_add( MALLOC( uint8_t, search_len ) );
  }

  // prime the pump by reading the first row
  cur->len = match_row( cur->bytes, &cur->match_bits, kmps, match_buf );

  while ( cur->len ) {
    size_t  buf_pos;
    bool    prev_matches;

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
        (!opt_only_printing || any_printable( (char*)cur->bytes, cur->len )))
       ) {

      // print row separator (if necessary)
      if ( !opt_only_matching && !opt_only_printing ) {
        off_t const offset_delta = file_offset - dumped_offset - ROW_BUF_SIZE;
        if ( offset_delta && any_dumped ) {
          SGR_START_IF( sgr_elided );
          for ( size_t i = 0; i < OFFSET_WIDTH; ++i )
            PUTCHAR( '-' );
          SGR_END_IF( sgr_elided );
          SGR_START_IF( sgr_sep );
          PUTCHAR( ':' );
          SGR_END_IF( sgr_sep );
          PUTCHAR( ' ' );
          SGR_START_IF( sgr_elided );
          PRINTF( "(%lld | 0x%llX)", offset_delta, offset_delta );
          SGR_END_IF( sgr_elided );
          PUTCHAR( '\n' );
        }
      }

      // print offset & column separator
      SGR_START_IF( sgr_offset );
      PRINTF( offset_fmt_printf[ opt_offset_fmt ], file_offset );
      SGR_END_IF( sgr_offset );
      SGR_START_IF( sgr_sep );
      PUTCHAR( ':' );
      SGR_END_IF( sgr_sep );

      // dump hex part
      prev_matches = false;
      for ( buf_pos = 0; buf_pos < cur->len; ++buf_pos ) {
        bool const matches = cur->match_bits & (1 << buf_pos);
        bool const matches_changed = matches != prev_matches;

        if ( buf_pos % COLUMN_WIDTH == 0 ) {
          SGR_END_IF( prev_matches );
          PUTCHAR( ' ' );               // print space between hex columns
          SGR_HEX_START_IF( prev_matches );
        }
        if ( matches )
          SGR_HEX_START_IF( matches_changed );
        else
          SGR_END_IF( matches_changed );
        PRINTF( "%02X", (unsigned)cur->bytes[ buf_pos ] );
        prev_matches = matches;
      } // for
      SGR_END_IF( prev_matches );

      // print padding if necessary (last row only)
      while ( buf_pos < ROW_BUF_SIZE ) {
        if ( buf_pos++ % COLUMN_WIDTH == 0 )
          PUTCHAR( ' ' );             // print space between hex columns
        PRINTF( "  " );
      } // while

      // dump ASCII part
      PRINTF( "  " );
      prev_matches = false;
      for ( buf_pos = 0; buf_pos < cur->len; ++buf_pos ) {
        bool const matches = cur->match_bits & (1 << buf_pos);
        bool const matches_changed = matches != prev_matches;
        uint8_t const byte = cur->bytes[ buf_pos ];

        if ( matches )
          SGR_ASCII_START_IF( matches_changed );
        else
          SGR_END_IF( matches_changed );
        PUTCHAR( isprint( byte ) ? byte : '.' );
        prev_matches = matches;
      } // for
      SGR_END_IF( prev_matches );
      PUTCHAR( '\n' );

      any_dumped = true;
      if ( cur->match_bits )
        any_matches = true;
      dumped_offset = file_offset;
    }

    // Check whether the next row is the same as the current row, but only if:
    is_same_row =
      !(opt_verbose || is_last_row) &&  // + neither -v or is the last row
      cur->len == next->len &&          // + the two row lengths are equal
      memcmp( cur->bytes, next->bytes, ROW_BUF_SIZE ) == 0;

    row_buf_t *const temp = cur;        // swap row pointers to avoid memcpy()
    cur = next, next = temp;

    file_offset += ROW_BUF_SIZE;
  } // while

  exit( search_len && !any_matches ? EXIT_NO_MATCHES : EXIT_OK );
}

/////////// initialization & clean-up /////////////////////////////////////////

static void clean_up( void ) {
  freelist_free();
  if ( file_input )
    fclose( file_input );
}

static void init( int argc, char *argv[] ) {
  atexit( clean_up );
  parse_options( argc, argv );

  if ( search_buf )                     // searching for a string?
    search_len = strlen( search_buf );
  else if ( search_endian ) {           // searching for a number?
    if ( !search_len )                  // default to smallest possible size
      search_len = ulong_len( search_number );
    ulong_rearrange_bytes( &search_number, search_len, search_endian );
    search_buf = (char*)&search_number;
  }

  if ( !opt_max_bytes_to_read )
    exit( search_len ? EXIT_NO_MATCHES : EXIT_OK );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
