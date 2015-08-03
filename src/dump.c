/*
**      ad -- ASCII dump
**      dump.c
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
#include "utf8.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <libgen.h>                     /* for basename() */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */

///////////////////////////////////////////////////////////////////////////////

#define HEX_COLUMN_WIDTH  2             /* bytes per hex column */

#define FFLUSH(F) \
  BLOCK( if ( fflush( (F) ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

#define FPRINTF(...) \
  BLOCK( if ( fprintf( fout, __VA_ARGS__ ) < 0 ) PERROR_EXIT( WRITE_ERROR ); )

#define FPUTC(C) \
  BLOCK( if ( fputc( (C), fout ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

#define FPUTS(S) \
  BLOCK( if ( fputs( (S), fout ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

#define SGR_START_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( sgr_start, (EXPR) ); )

#define SGR_END_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( "%s", sgr_end ); )

#define SGR_HEX_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_hex_match ); )

#define SGR_ASCII_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_ascii_match ); )

////////// extern variables ///////////////////////////////////////////////////

extern char *elided_separator;          // separator used for elided rows

////////// local data structures //////////////////////////////////////////////

struct row_buf {
  uint8_t   bytes[ ROW_SIZE ];          // bytes in buffer, left-to-right
  size_t    len;                        // length of buffer
  uint16_t  match_bits;                 // which bytes match, right-to-left
};
typedef struct row_buf row_buf_t;

////////// local functions ////////////////////////////////////////////////////

/**
 * Collects the bytes starting at \a buf_pos into a UTF-8 character.
 *
 * @param cur A pointer to the current row.
 * @param buf_pos The position within the row.
 * @param next A pointer to the next row.
 * @param utf8_char A pointer to the buffer to receive the UTF-8 character.
 * @return Returns the number of bytes comprising the UTF-8 character
 * or 0 if the bytes do not comprise a valid UTF-8 character.
 */
static size_t utf8_collect( row_buf_t const *cur, size_t buf_pos,
                            row_buf_t const *next, uint8_t *utf8_char ) {
  size_t const len = utf8_len( cur->bytes[ buf_pos ] );
  if ( len > 1 ) {
    row_buf_t const *row = cur;
    *utf8_char++ = row->bytes[ buf_pos++ ];

    for ( size_t i = 1; i < len; ++i, ++buf_pos ) {
      if ( buf_pos == row->len ) {      // ran off the end of the row
        if ( row == next || !next->len )
          return 0;                     // incomplete UTF-8 character
        row = next;                     // continue on the next row
        buf_pos = 0;
      }

      uint8_t const byte = row->bytes[ buf_pos ];
      if ( !utf8_is_cont( (char)byte ) )
        return 0;
      *utf8_char++ = byte;
    } // for

    *utf8_char = '\0';
  }
  return len;
}

static void dump_row( char const *off_fmt, row_buf_t const *cur,
                      row_buf_t const *next ) {
  static bool   any_dumped = false;     // any data dumped yet?
  static off_t  dumped_offset = -1;     // offset of most recently dumped row

  if ( dumped_offset == -1 )
    dumped_offset = fin_offset;

  size_t  buf_pos;
  bool    prev_matches;

  // print row separator (if necessary)
  if ( !opt_only_matching && !opt_only_printing ) {
    off_t const offset_delta = fin_offset - dumped_offset - ROW_SIZE;
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
  for ( buf_pos = 0; buf_pos < cur->len; ++buf_pos ) {
    bool const matches = cur->match_bits & (1 << buf_pos);
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
    FPRINTF( "%02X", (unsigned)cur->bytes[ buf_pos ] );
    prev_matches = matches;
  } // for
  SGR_END_IF( prev_matches );

  // print padding if necessary (last row only)
  while ( buf_pos < ROW_SIZE ) {
    if ( buf_pos++ % HEX_COLUMN_WIDTH == 0 )
      FPUTC( ' ' );                     // print space between hex columns
    FPRINTF( "  " );
  } // while

  // dump ASCII part
  FPRINTF( "  " );
  prev_matches = false;
  for ( buf_pos = 0; buf_pos < cur->len; ++buf_pos ) {
    bool const matches = cur->match_bits & (1 << buf_pos);
    bool const matches_changed = matches != prev_matches;
    uint8_t const byte = cur->bytes[ buf_pos ];

    if ( matches )
      SGR_ASCII_START_IF( matches_changed );
    else
      SGR_END_IF( matches_changed );

    static size_t utf8_count;
    if ( utf8_count > 1 ) {
      FPUTS( opt_utf8_pad );
      --utf8_count;
    } else {
      uint8_t utf8_char[ UTF8_LEN_MAX + 1 /*NULL*/ ];
      utf8_count = opt_utf8 ? utf8_collect( cur, buf_pos, next, utf8_char ) : 1;
      if ( utf8_count > 1 )
        FPUTS( (char*)utf8_char );
      else
        FPUTC( ascii_is_print( byte ) ? byte : '.' );
    }

    prev_matches = matches;
  } // for
  SGR_END_IF( prev_matches );
  FPUTC( '\n' );

  any_dumped = true;
  dumped_offset = fin_offset;
}

static void dump_row_c( char const *off_fmt, uint8_t const *buf,
                        size_t buf_len ) {
  // print offset
  FPUTS( "  /* " );
  FPRINTF( off_fmt, fin_offset );
  FPUTS( " */" );

  // dump hex part
  uint8_t const *const end = buf + buf_len;
  while ( buf < end )
    FPRINTF( " 0x%02X,", (unsigned)*buf++ );
  FPUTC( '\n' );
}

/////////// extern functions //////////////////////////////////////////////////

void dump_file( void ) {
  bool        any_matches = false;      // if matching, any data matched yet?
  row_buf_t   buf[2], *cur = buf, *next = buf + 1;
  bool        is_same_row = false;      // current row same as previous?
  kmp_t      *kmps = NULL;              // used only by match_byte()
  uint8_t    *match_buf = NULL;         // used only by match_byte()
  char const *off_fmt = get_offset_fmt_format();

  if ( search_len ) {                   // searching for anything?
    kmps = freelist_add( kmp_init( search_buf, search_len ) );
    match_buf = freelist_add( MALLOC( uint8_t, search_len ) );
  }

  // prime the pump by reading the first row
  cur->len =
    match_row( cur->bytes, ROW_SIZE, &cur->match_bits, kmps, match_buf );

  while ( cur->len ) {
    //
    // We need to know whether the current row is the last row.  The current
    // row is the last if its length < ROW_SIZE.  However, if the file's length
    // is an exact multiple of ROW_SIZE, then we don't know the current row is
    // the last.  We therefore have to read the next row: the current row is
    // the last row if the length of the next row is zero.
    //
    next->len = cur->len < ROW_SIZE ? 0 :
      match_row( next->bytes, ROW_SIZE, &next->match_bits, kmps, match_buf );

    if ( opt_matches != MATCHES_ONLY ) {
      bool const is_last_row = next->len == 0;

      if ( cur->match_bits || (         // always dump matching rows
          // Otherwise dump only if:
          //  + for non-matching rows, if not -m
          !opt_only_matching &&
          //  + and if -v, not the same row, or is the last row
          (opt_verbose || !is_same_row || is_last_row) &&
          //  + and if not -p or any printable bytes
          (!opt_only_printing ||
            any_printable( (char*)cur->bytes, cur->len )) ) ) {

        dump_row( off_fmt, cur, next );
      }

      // Check if the next row is the same as the current row, but only if:
      is_same_row =
        !(opt_verbose || is_last_row) &&// + neither -v or is the last row
        cur->len == next->len &&        // + the two row lengths are equal
        memcmp( cur->bytes, next->bytes, ROW_SIZE ) == 0;
    }

    if ( cur->match_bits )
      any_matches = true;

    row_buf_t *const temp = cur;        // swap row pointers to avoid memcpy()
    cur = next, next = temp;

    fin_offset += ROW_SIZE;
  } // while

  if ( opt_matches ) {
    FFLUSH( fout );
    PRINT_ERR( "%lu\n", total_matches );
  }

  exit( search_len && !any_matches ? EXIT_NO_MATCHES : EXIT_SUCCESS );
}

void dump_file_c( void ) {
  size_t      array_len = 0;
  char const *array_name = NULL;
  char const *off_fmt = get_offset_fmt_format();
  size_t      row_len;

  if ( fin == stdin ) {
    array_name = "stdin";
  } else {
    char *const temp = freelist_add( check_strdup( fin_path ) );
    array_name = freelist_add( identify( basename( temp ) ) );
  }
  FPRINTF(
    "%sunsigned char %s%s[] = {\n",
    (opt_c_fmt & CFMT_STATIC ? "static " : ""),
    (opt_c_fmt & CFMT_CONST  ? "const "  : ""),
    array_name
  );

  do {
    uint8_t  bytes[ ROW_SIZE_C ];       // bytes in buffer
    uint16_t match_bits;                // not used when dumping in C
    row_len = match_row( bytes, ROW_SIZE_C, &match_bits, NULL, NULL );
    dump_row_c( off_fmt, bytes, row_len );
    fin_offset += row_len;
    array_len += row_len;
  } while ( row_len == ROW_SIZE_C );

  FPRINTF( "};\n" );

  if ( CFMT_HAS_TYPE( opt_c_fmt ) )
    FPRINTF(
      "%s%s%s%s%s%s%s_len = %zu%s%s;\n",
      (opt_c_fmt & CFMT_STATIC   ? "static "   : ""),
      (opt_c_fmt & CFMT_UNSIGNED ? "unsigned " : ""),
      (opt_c_fmt & CFMT_LONG     ? "long "     : ""),
      (opt_c_fmt & CFMT_INT      ? "int "      : ""),
      (opt_c_fmt & CFMT_SIZE_T   ? "size_t "   : ""),
      (opt_c_fmt & CFMT_CONST    ? "const "    : ""),
      array_name, array_len,
      (opt_c_fmt & CFMT_UNSIGNED ? "u" : ""),
      (opt_c_fmt & CFMT_LONG     ? "L" : "")
    );

  exit( EXIT_SUCCESS );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
