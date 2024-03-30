/*
**      ad -- ASCII dump
**      src/dump.c
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

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#include "match.h"
#include "options.h"
#include "unicode.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>                   /* for PRIu64, etc. */
#include <stddef.h>                     /* for size_t */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sysexits.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

#define COLOR_START_IF(EXPR,COLOR) \
  BLOCK( if ( EXPR ) color_start( stdout, (COLOR) ); )

#define COLOR_END_IF(EXPR,COLOR) \
  BLOCK( if ( EXPR ) color_end( stdout, (COLOR) ); )

/**
 * Buffer for row of data.
 */
struct row_buf {
  char8_t       bytes[ ROW_BYTES_MAX ]; ///< Bytes in buffer, left-to-right.
  size_t        len;                    ///< Length of buffer.
  match_bits_t  match_bits;             ///< Which bytes match, right-to-left.
};
typedef struct row_buf row_buf_t;

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets whether to print an extra space between byte columns for readability.
 *
 * @param byte_pos The current byte position from the beginning of a line.
 * @return Returns \c true only if the extra space should be printed.
 */
NODISCARD
static inline bool print_readability_space( size_t byte_pos ) {
  return byte_pos == 8 && opt_group_by < 8;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Collects the bytes starting at \a curr_pos into a UTF-8 character.
 *
 * @param curr A pointer to the current row.
 * @param curr_pos The position within the row.
 * @param next A pointer to the next row.
 * @param utf8_char A pointer to the buffer to receive the UTF-8 character.
 * @return Returns the number of bytes comprising the UTF-8 character or 0 if
 * the bytes do not comprise a valid UTF-8 character.
 */
NODISCARD
static unsigned utf8_collect( row_buf_t const *curr, size_t curr_pos,
                              row_buf_t const *next, char8_t *utf8_char ) {
  assert( curr != NULL );
  assert( next != NULL );
  assert( utf8_char != NULL );

  unsigned const len = utf8_char_len( curr->bytes[ curr_pos ] );
  if ( len > 1 ) {
    row_buf_t const *row = curr;
    *utf8_char++ = row->bytes[ curr_pos++ ];

    for ( size_t i = 1; i < len; ++i, ++curr_pos ) {
      if ( curr_pos == row->len ) {     // ran off the end of the row
        if ( row == next || next->len == 0 )
          return 0;                     // incomplete UTF-8 character
        row = next;                     // continue on the next row
        curr_pos = 0;
      }

      char8_t const byte = row->bytes[ curr_pos ];
      if ( unlikely( !utf8_is_cont( byte ) ) )
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
 * @param curr A pointer to the current \a row_buf.
 * @param next A pointer to the next \a row_buf.
 */
static void dump_row( char const *off_fmt, row_buf_t const *curr,
                      row_buf_t const *next ) {
  assert( off_fmt != NULL );
  assert( curr != NULL );
  assert( next != NULL );

  static bool   any_dumped = false;     // any data dumped yet?
  static off_t  dumped_offset = -1;     // offset of most recently dumped row

  if ( dumped_offset == -1 )
    dumped_offset = fin_offset;

  size_t  curr_pos;
  bool    prev_matches;

  // print row separator (if necessary)
  if ( !opt_only_matching && !opt_only_printing ) {
    uint64_t const offset_delta = STATIC_CAST( uint64_t,
      fin_offset - dumped_offset - STATIC_CAST( off_t, row_bytes )
    );
    if ( offset_delta > 0 && any_dumped ) {
      color_start( stdout, sgr_elided );
      for ( size_t i = get_offset_width(); i > 0; --i )
        PUTC( ELIDED_SEP_CHAR );
      color_end( stdout, sgr_elided );
      color_start( stdout, sgr_sep );
      PUTC( ':' );
      color_end( stdout, sgr_sep );
      PUTC( ' ' );
      color_start( stdout, sgr_elided );
      PRINTF( "(%" PRIu64 " | 0x%" PRIX64 ")", offset_delta, offset_delta );
      color_end( stdout, sgr_elided );
      PUTC( '\n' );
    }
  }

  // print offset & column separator
  if ( opt_offset_fmt != OFMT_NONE ) {
    color_start( stdout, sgr_offset );
    PRINTF( off_fmt, STATIC_CAST(uint64_t, fin_offset) );
    color_end( stdout, sgr_offset );
    color_start( stdout, sgr_sep );
    PUTC( ':' );
    color_end( stdout, sgr_sep );
  }

  // dump hex part
  prev_matches = false;
  for ( curr_pos = 0; curr_pos < curr->len; ++curr_pos ) {
    bool const matches = (curr->match_bits & (1u << curr_pos)) != 0;
    bool const matches_changed = matches != prev_matches;

    if ( curr_pos % opt_group_by == 0 ) {
      COLOR_END_IF( prev_matches, sgr_hex_match );
      if ( opt_offset_fmt != OFMT_NONE || curr_pos > 0 )
        PUTC( ' ' );                    // print space between hex columns
      if ( print_readability_space( curr_pos ) )
        PUTC( ' ' );
      COLOR_START_IF( prev_matches, sgr_hex_match );
    }
    if ( matches )
      COLOR_START_IF( matches_changed, sgr_hex_match );
    else
      COLOR_END_IF( matches_changed, sgr_hex_match );
    PRINTF( "%02X", STATIC_CAST(unsigned, curr->bytes[ curr_pos ]) );
    prev_matches = matches;
  } // for
  COLOR_END_IF( prev_matches, sgr_hex_match );

  if ( opt_print_ascii ) {
    unsigned spaces = 2;

    // add padding spaces if necessary (last row only)
    for ( ; curr_pos < row_bytes; ++curr_pos ) {
      if ( curr_pos % opt_group_by == 0 )
        ++spaces;                       // print space between hex columns
      if ( print_readability_space( curr_pos ) )
        ++spaces;
      spaces += 2;
    } // for

    FPUTNSP( spaces, stdout );

    // dump ASCII part
    prev_matches = false;
    for ( curr_pos = 0; curr_pos < curr->len; ++curr_pos ) {
      bool const matches = (curr->match_bits & (1u << curr_pos)) != 0;
      bool const matches_changed = matches != prev_matches;
      char8_t const byte = curr->bytes[ curr_pos ];

      if ( matches )
        COLOR_START_IF( matches_changed, sgr_ascii_match );
      else
        COLOR_END_IF( matches_changed, sgr_ascii_match );

      static unsigned utf8_count;
      if ( utf8_count > 1 ) {
        PUTS( opt_utf8_pad );
        --utf8_count;
      } else {
        char8_t utf8_char[ UTF8_CHAR_SIZE_MAX + 1 /*NULL*/ ];
        utf8_count = opt_utf8 ?
          utf8_collect( curr, curr_pos, next, utf8_char ) : 1;
        if ( utf8_count > 1 )
          PUTS( POINTER_CAST( char*, utf8_char ) );
        else
          PUTC( ascii_is_print( STATIC_CAST( char, byte ) ) ? byte : '.' );
      }

      prev_matches = matches;
    } // for
    COLOR_END_IF( prev_matches, sgr_ascii_match );
  }

  PUTC( '\n' );

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

  if ( unlikely( buf_len == 0 ) )
    return;

  if ( opt_offset_fmt == OFMT_NONE ) {
    PUTC( ' ' );
  } else {
    // print offset
    PUTS( "  /* " );
    PRINTF( off_fmt, fin_offset );
    PUTS( " */" );
  }

  // dump hex part
  char8_t const *const end = buf + buf_len;
  while ( buf < end )
    PRINTF( " 0x%02X,", STATIC_CAST(unsigned, *buf++) );
  PUTC( '\n' );
}

/////////// extern functions //////////////////////////////////////////////////

void dump_file( void ) {
  bool          any_matches = false;    // if matching, any data matched yet?
  row_buf_t     buf[2], *curr = buf, *next = buf + 1;
  bool          is_same_row = false;    // current row same as previous?
  kmp_t const  *kmps = NULL;            // used only by match_row()
  char8_t      *match_buf = NULL;       // used only by match_row()
  size_t        match_len = 0;          // used only by match_row()
  char const   *off_fmt = get_offset_fmt_format();

  if ( opt_search_len > 0 ) {           // searching for anything?
    if ( opt_strings ) {
      match_len = opt_search_len < STRINGS_LEN_DEFAULT ?
        STRINGS_LEN_DEFAULT : opt_search_len;
    } else {
      kmps = kmp_init( opt_search_buf, opt_search_len );
      match_len = opt_search_len;
    }
    match_buf = MALLOC( char8_t, opt_search_len );
  }

  // prime the pump by reading the first row
  curr->len = match_row(
    curr->bytes, row_bytes, &curr->match_bits, kmps, &match_buf, &match_len
  );

  while ( curr->len > 0 ) {
    //
    // We need to know whether the current row is the last row.  The current
    // row is the last if its length < row_bytes.  However, if the file's
    // length is an exact multiple of row_bytes, then we don't know the current
    // row is the last.  We therefore have to read the next row: the current
    // row is the last row if the length of the next row is zero.
    //
    next->len = curr->len < row_bytes ? 0 :
      match_row(
        next->bytes, row_bytes, &next->match_bits, kmps, &match_buf, &match_len
      );

    if ( opt_matches != MATCHES_ONLY_PRINT ) {
      bool const is_last_row = next->len == 0;

      if ( curr->match_bits != 0 || (   // always dump matching rows
          // Otherwise dump only if:
          //  + for non-matching rows, if not -m
          !opt_only_matching &&
          //  + and if -v, not the same row, or is the last row
          (opt_verbose || !is_same_row || is_last_row) &&
          //  + and if not -p or any printable bytes
          (!opt_only_printing ||
            ascii_any_printable( (char*)curr->bytes, curr->len )) ) ) {

        dump_row( off_fmt, curr, next );
      }

      // Check if the next row is the same as the current row, but only if:
      is_same_row =
        !(opt_verbose || is_last_row) &&// + neither -v or is the last row
        curr->len == next->len &&       // + the two row lengths are equal
        memcmp( curr->bytes, next->bytes, row_bytes ) == 0;
    }

    if ( curr->match_bits != 0 )
      any_matches = true;

    row_buf_t *const temp = curr;       // swap row pointers to avoid memcpy()
    curr = next;
    next = temp;

    fin_offset += STATIC_CAST( off_t, row_bytes );
  } // while

  if ( opt_matches != MATCHES_NO_PRINT ) {
    FFLUSH( stdout );
    EPRINTF( "%lu\n", total_matches );
  }

  FREE( kmps );
  free( match_buf );

  exit( opt_search_len > 0 && !any_matches ? EX_NO_MATCHES : EX_OK );
}

void dump_file_c( void ) {
  char8_t       bytes[ ROW_BYTES_C ];   // bytes in buffer
  match_bits_t  match_bits;             // not used when dumping in C

  // prime the pump by reading the first row
  size_t row_len = match_row(
    bytes, ROW_BYTES_C, &match_bits, /*kmps=*/NULL,
    /*pmatch_buf=*/NULL, /*pmatch_len=*/NULL
  );
  if ( row_len == 0 )
    goto empty;

  char const *const array_name = strcmp( fin_path, "-" ) == 0 ?
    "stdin" : free_later( identify( base_name( fin_path ) ) );

  PRINTF(
    "%s%s %s%s[] = {\n",
    ((opt_c_fmt & CFMT_STATIC ) != 0 ? "static " : ""),
    ((opt_c_fmt & CFMT_CHAR8_T) != 0 ? "char8_t" : "unsigned char"),
    ((opt_c_fmt & CFMT_CONST  ) != 0 ? "const "  : ""),
    array_name
  );

  size_t array_len = 0;
  char const *const off_fmt = get_offset_fmt_format();

  for (;;) {
    dump_row_c( off_fmt, bytes, row_len );
    fin_offset += STATIC_CAST( off_t, row_len );
    array_len += row_len;
    if ( row_len != ROW_BYTES_C )
      break;
    row_len = match_row(
      bytes, ROW_BYTES_C, &match_bits, /*kmps=*/NULL,
      /*pmatch_buf=*/NULL, /*pmatch_len=*/NULL
    );
  } // for

  PUTS( "};\n" );

  if ( (opt_c_fmt & CFMT_ANY_LENGTH) != CFMT_NONE )
    PRINTF(
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

empty:
  exit( EX_OK );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
