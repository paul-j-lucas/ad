/*
**      ad -- ASCII dump
**      src/dump.c
**
**      Copyright (C) 2015-2021  Paul J. Lucas
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
#include "ad.h"
#include "color.h"
#include "match.h"
#include "options.h"
#include "unicode.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>                   /* for PRIu64, etc. */
#include <libgen.h>                     /* for basename() */
#include <stddef.h>                     /* for size_t */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

#define SGR_START_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( fout, sgr_start, (EXPR) ); )

#define SGR_END_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( fout, "%s", sgr_end ); )

#define SGR_HEX_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_hex_match ); )

#define SGR_ASCII_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_ascii_match ); )

struct row_buf {
  char8_t       bytes[ ROW_BYTES_MAX ]; // bytes in buffer, left-to-right
  size_t        len;                    // length of buffer
  match_bits_t  match_bits;             // which bytes match, right-to-left
};
typedef struct row_buf row_buf_t;

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets whether to print an extra space between byte columns for readability.
 *
 * @param byte_pos The current byte position from the beginning of a line.
 * @return Returns \c true only if
 */
NODISCARD
static inline bool print_readability_space( size_t byte_pos ) {
  return byte_pos == 8 && opt_group_by < 8;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Collects the bytes starting at \a buf_pos into a UTF-8 character.
 *
 * @param cur A pointer to the current row.
 * @param buf_pos The position within the row.
 * @param next A pointer to the next row.
 * @param utf8_char A pointer to the buffer to receive the UTF-8 character.
 * @return Returns the number of bytes comprising the UTF-8 character or 0 if
 * the bytes do not comprise a valid UTF-8 character.
 */
NODISCARD
static size_t utf8_collect( row_buf_t const *cur, size_t buf_pos,
                            row_buf_t const *next, char8_t *utf8_char ) {
  assert( cur != NULL );
  assert( next != NULL );
  assert( utf8_char != NULL );

  size_t const len = utf8_len( STATIC_CAST( char, cur->bytes[ buf_pos ] ) );
  if ( len > 1 ) {
    row_buf_t const *row = cur;
    *utf8_char++ = row->bytes[ buf_pos++ ];

    for ( size_t i = 1; i < len; ++i, ++buf_pos ) {
      if ( buf_pos == row->len ) {      // ran off the end of the row
        if ( row == next || next->len == 0 )
          return 0;                     // incomplete UTF-8 character
        row = next;                     // continue on the next row
        buf_pos = 0;
      }

      char8_t const byte = row->bytes[ buf_pos ];
      if ( unlikely( !utf8_is_cont( STATIC_CAST(char, byte) ) ) )
        return 0;
      *utf8_char++ = byte;
    } // for

    *utf8_char = '\0';
  }
  return len;
}

/**
 * Dumps a single row of bytes containing the offset and hex and ASCII parts.
 *
 * @param off_fmt The \c printf() format for the offset.
 * @param cur A pointer to the current \a row_buf.
 * @param next A pointer to the next \a row_buf.
 */
static void dump_row( char const *off_fmt, row_buf_t const *cur,
                      row_buf_t const *next ) {
  assert( off_fmt != NULL );
  assert( cur != NULL );
  assert( next != NULL );

  static bool   any_dumped = false;     // any data dumped yet?
  static off_t  dumped_offset = -1;     // offset of most recently dumped row

  if ( dumped_offset == -1 )
    dumped_offset = fin_offset;

  size_t  buf_pos;
  bool    prev_matches;

  // print row separator (if necessary)
  if ( !opt_only_matching && !opt_only_printing ) {
    uint64_t const offset_delta = STATIC_CAST( uint64_t,
      fin_offset - dumped_offset - STATIC_CAST( off_t, row_bytes )
    );
    if ( offset_delta > 0 && any_dumped ) {
      SGR_START_IF( sgr_elided );
      for ( size_t i = get_offset_width(); i > 0; --i )
        FPUTC( ELIDED_SEP_CHAR, fout );
      SGR_END_IF( sgr_elided );
      SGR_START_IF( sgr_sep );
      FPUTC( ':', fout );
      SGR_END_IF( sgr_sep );
      FPUTC( ' ', fout );
      SGR_START_IF( sgr_elided );
      FPRINTF( fout, "(%" PRIu64 " | 0x%" PRIX64 ")", offset_delta, offset_delta );
      SGR_END_IF( sgr_elided );
      FPUTC( '\n', fout );
    }
  }

  // print offset & column separator
  if ( opt_offset_fmt != OFMT_NONE ) {
    SGR_START_IF( sgr_offset );
    FPRINTF( fout, off_fmt, STATIC_CAST(uint64_t, fin_offset) );
    SGR_END_IF( sgr_offset );
    SGR_START_IF( sgr_sep );
    FPUTC( ':', fout );
    SGR_END_IF( sgr_sep );
  }

  // dump hex part
  prev_matches = false;
  for ( buf_pos = 0; buf_pos < cur->len; ++buf_pos ) {
    bool const matches = (cur->match_bits & (1u << buf_pos)) != 0;
    bool const matches_changed = matches != prev_matches;

    if ( buf_pos % opt_group_by == 0 ) {
      SGR_END_IF( prev_matches );
      if ( opt_offset_fmt != OFMT_NONE || buf_pos > 0 )
        FPUTC( ' ', fout );             // print space between hex columns
      if ( print_readability_space( buf_pos ) )
        FPUTC( ' ', fout );
      SGR_HEX_START_IF( prev_matches );
    }
    if ( matches )
      SGR_HEX_START_IF( matches_changed );
    else
      SGR_END_IF( matches_changed );
    FPRINTF( fout, "%02X", STATIC_CAST(unsigned, cur->bytes[ buf_pos ]) );
    prev_matches = matches;
  } // for
  SGR_END_IF( prev_matches );

  if ( opt_print_ascii ) {
    // print padding if necessary (last row only)
    for ( ; buf_pos < row_bytes; ++buf_pos ) {
      if ( buf_pos % opt_group_by == 0 )
        FPUTC( ' ', fout );             // print space between hex columns
      if ( print_readability_space( buf_pos ) )
        FPUTC( ' ', fout );
      FPUTS( "  ", fout );
    } // for

    // dump ASCII part
    FPUTS( "  ", fout );
    prev_matches = false;
    for ( buf_pos = 0; buf_pos < cur->len; ++buf_pos ) {
      bool const matches = (cur->match_bits & (1u << buf_pos)) != 0;
      bool const matches_changed = matches != prev_matches;
      char8_t const byte = cur->bytes[ buf_pos ];

      if ( matches )
        SGR_ASCII_START_IF( matches_changed );
      else
        SGR_END_IF( matches_changed );

      static size_t utf8_count;
      if ( utf8_count > 1 ) {
        FPUTS( opt_utf8_pad, fout );
        --utf8_count;
      } else {
        char8_t utf8_char[ UTF8_LEN_MAX + 1 /*NULL*/ ];
        utf8_count = opt_utf8 ?
          utf8_collect( cur, buf_pos, next, utf8_char ) : 1;
        if ( utf8_count > 1 )
          FPUTS( POINTER_CAST( char*, utf8_char ), fout );
        else
          FPUTC( ascii_is_print( STATIC_CAST( char, byte ) ) ? byte : '.', fout );
      }

      prev_matches = matches;
    } // for
    SGR_END_IF( prev_matches );
  }

  FPUTC( '\n', fout );

  any_dumped = true;
  dumped_offset = fin_offset;
}

/**
 * Dumps a single row of bytes as C array data.
 *
 * @param off_fmt The \c printf() format for the offset.
 * @param buf A pointer to the current set of bytes to dump.
 * @param buf_len The number of bytes pointed to by \a buf.
 */
static void dump_row_c( char const *off_fmt, char8_t const *buf,
                        size_t buf_len ) {
  assert( off_fmt != NULL );
  assert( buf != NULL );

  // print offset
  FPUTS( "  /* ", fout );
  FPRINTF( fout, off_fmt, fin_offset );
  FPUTS( " */", fout );

  // dump hex part
  char8_t const *const end = buf + buf_len;
  while ( buf < end )
    FPRINTF( fout, " 0x%02X,", STATIC_CAST(unsigned, *buf++) );
  FPUTC( '\n', fout );
}

/////////// extern functions //////////////////////////////////////////////////

void dump_file( void ) {
  bool        any_matches = false;      // if matching, any data matched yet?
  row_buf_t   buf[2], *cur = buf, *next = buf + 1;
  bool        is_same_row = false;      // current row same as previous?
  kmp_t      *kmps = NULL;              // used only by match_byte()
  char8_t    *match_buf = NULL;         // used only by match_byte()
  char const *off_fmt = get_offset_fmt_format();

  if ( search_len > 0 ) {               // searching for anything?
    kmps = (kmp_t*)free_later( kmp_init( search_buf, search_len ) );
    match_buf = (char8_t*)free_later( MALLOC( char8_t, search_len ) );
  }

  // prime the pump by reading the first row
  cur->len =
    match_row( cur->bytes, row_bytes, &cur->match_bits, kmps, match_buf );

  while ( cur->len > 0 ) {
    //
    // We need to know whether the current row is the last row.  The current
    // row is the last if its length < row_bytes.  However, if the file's
    // length is an exact multiple of row_bytes, then we don't know the current
    // row is the last.  We therefore have to read the next row: the current
    // row is the last row if the length of the next row is zero.
    //
    next->len = cur->len < row_bytes ? 0 :
      match_row( next->bytes, row_bytes, &next->match_bits, kmps, match_buf );

    if ( opt_matches != MATCHES_ONLY_PRINT ) {
      bool const is_last_row = next->len == 0;

      if ( cur->match_bits != 0 || (    // always dump matching rows
          // Otherwise dump only if:
          //  + for non-matching rows, if not -m
          !opt_only_matching &&
          //  + and if -v, not the same row, or is the last row
          (opt_verbose || !is_same_row || is_last_row) &&
          //  + and if not -p or any printable bytes
          (!opt_only_printing ||
            ascii_any_printable( (char*)cur->bytes, cur->len )) ) ) {

        dump_row( off_fmt, cur, next );
      }

      // Check if the next row is the same as the current row, but only if:
      is_same_row =
        !(opt_verbose || is_last_row) &&// + neither -v or is the last row
        cur->len == next->len &&        // + the two row lengths are equal
        memcmp( cur->bytes, next->bytes, row_bytes ) == 0;
    }

    if ( cur->match_bits != 0 )
      any_matches = true;

    row_buf_t *const temp = cur;        // swap row pointers to avoid memcpy()
    cur = next;
    next = temp;

    fin_offset += STATIC_CAST( off_t, row_bytes );
  } // while

  if ( opt_matches != MATCHES_NO_PRINT ) {
    FFLUSH( fout );
    EPRINTF( "%lu\n", total_matches );
  }

  exit( search_len > 0 && !any_matches ? EX_NO_MATCHES : EX_OK );
}

void dump_file_c( void ) {
  size_t      array_len = 0;
  char const *array_name = NULL;
  char const *off_fmt = get_offset_fmt_format();
  size_t      row_len;

  if ( fin == stdin ) {
    array_name = "stdin";
  } else {
    char *const temp = (char*)free_later( check_strdup( fin_path ) );
    array_name = (char*)free_later( identify( basename( temp ) ) );
  }
  FPRINTF( fout,
    "%sunsigned char %s%s[] = {\n",
    ((opt_c_fmt & CFMT_STATIC) != 0 ? "static " : ""),
    ((opt_c_fmt & CFMT_CONST ) != 0 ? "const "  : ""),
    array_name
  );

  do {
    char8_t  bytes[ ROW_BYTES_C ];      // bytes in buffer
    match_bits_t match_bits;            // not used when dumping in C
    row_len = match_row( bytes, ROW_BYTES_C, &match_bits, NULL, NULL );
    dump_row_c( off_fmt, bytes, row_len );
    fin_offset += STATIC_CAST( off_t, row_len );
    array_len += row_len;
  } while ( row_len == ROW_BYTES_C );

  FPUTS( "};\n", fout );

  if ( CFMT_HAS_TYPE( opt_c_fmt ) )
    FPRINTF( fout,
      "%s%s%s%s%s%s%s_len = %zu%s%s;\n",
      ((opt_c_fmt & CFMT_STATIC  ) != 0 ? "static "   : ""),
      ((opt_c_fmt & CFMT_UNSIGNED) != 0 ? "unsigned " : ""),
      ((opt_c_fmt & CFMT_LONG    ) != 0 ? "long "     : ""),
      ((opt_c_fmt & CFMT_INT     ) != 0 ? "int "      : ""),
      ((opt_c_fmt & CFMT_SIZE_T  ) != 0 ? "size_t "   : ""),
      ((opt_c_fmt & CFMT_CONST   ) != 0 ? "const "    : ""),
      array_name, array_len,
      ((opt_c_fmt & CFMT_UNSIGNED) != 0 ? "u" : ""),
      ((opt_c_fmt & CFMT_LONG    ) != 0 ? "L" : "")
    );

  exit( EX_OK );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
