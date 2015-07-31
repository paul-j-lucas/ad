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
#include "utf8.h"
#include "util.h"

// system
#include <assert.h>
#include <ctype.h>
#include <libgen.h>                     /* for basename() */
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

#define FWRITE(PTR,SIZE,N,STREAM) \
  BLOCK( if ( fwrite( (PTR), (SIZE), (N), (STREAM) ) < (N) ) PERROR_EXIT( WRITE_ERROR ); )

////////// local data structures //////////////////////////////////////////////

struct row_buf {
  uint8_t   bytes[ ROW_SIZE ];          // bytes in buffer, left-to-right
  size_t    len;                        // length of buffer
  uint16_t  match_bits;                 // which bytes match, right-to-left
};
typedef struct row_buf row_buf_t;

////////// local variables ////////////////////////////////////////////////////

static char *elided_separator;          // separator used for elided rows

////////// local functions ////////////////////////////////////////////////////

static void         dump_file( void );
static void         dump_file_c( void );
static void         dump_row( char const*, row_buf_t const*, row_buf_t const* );
static void         dump_row_c( char const*, uint8_t const*, size_t );
static char const*  get_offset_fmt_english();
static char const*  get_offset_fmt_format();
static void         init( int, char*[] );
static void         reverse_dump_file( void );

/////////// main //////////////////////////////////////////////////////////////

int main( int argc, char *argv[] ) {
  init( argc, argv );
  if ( opt_reverse )
    reverse_dump_file(); 
  if ( opt_c_fmt )
    dump_file_c();
  else
    dump_file();
  // none of the above functions returns
}

/////////// dumping ///////////////////////////////////////////////////////////

static void dump_file( void ) {
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

      dump_row( off_fmt, cur, next );
      if ( cur->match_bits )
        any_matches = true;
    }

    // Check whether the next row is the same as the current row, but only if:
    is_same_row =
      !(opt_verbose || is_last_row) &&  // + neither -v or is the last row
      cur->len == next->len &&          // + the two row lengths are equal
      memcmp( cur->bytes, next->bytes, ROW_SIZE ) == 0;

    row_buf_t *const temp = cur;        // swap row pointers to avoid memcpy()
    cur = next, next = temp;

    fin_offset += ROW_SIZE;
  } // while
  exit( search_len && !any_matches ? EXIT_NO_MATCHES : EXIT_SUCCESS );
}

static void dump_file_c( void ) {
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
    uint16_t match_bits;
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

#define SGR_START_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( sgr_start, (EXPR) ); )

#define SGR_END_IF(EXPR) \
  BLOCK( if ( colorize && (EXPR) ) FPRINTF( "%s", sgr_end ); )

#define SGR_HEX_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_hex_match ); )

#define SGR_ASCII_START_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_IF( sgr_ascii_match ); )

static size_t utf8_collect( row_buf_t const *cur, size_t buf_pos,
                            row_buf_t const *next, uint8_t *utf8_char ) {
  size_t const len = utf8_len( cur->bytes[ buf_pos ] );
  if ( len > 1 ) {
    row_buf_t const *row = cur;
    *utf8_char++ = row->bytes[ buf_pos++ ];

    for ( size_t i = 1; i < len; ++i, ++buf_pos ) {
      if ( buf_pos == cur->len ) {
        if ( !next->len )
          return 0;
        buf_pos = 0;
        row = next;
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

////////// reverse ////////////////////////////////////////////////////////////

enum row_kind {
  ROW_BYTES,
  ROW_ELIDED
};
typedef enum row_kind row_kind_t;

#define INVALID_EXIT(FORMAT,...)                                    \
  PMESSAGE_EXIT( INVALID_FORMAT,                                    \
    "%s:%zu:%zu: error: " FORMAT, fin_path, line, col, __VA_ARGS__  \
  )

/**
 * Converts a single hexadecimal digit [0-9A-Fa-f] to its integer value.
 *
 * @param C The hexadecimal character.
 * @return Returns \a C converted to an integer.
 * @hideinitializer
 */
#define XTOI(C) (isdigit( C ) ? (C) - '0' : 0xA + toupper( C ) - 'A')

static row_kind_t parse_row( size_t line, char *buf, size_t buf_len,
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
    if ( sscanf( buf + OFFSET_WIDTH, ": (%ld | 0x%*X)", pbytes_len ) != 1 )
      INVALID_EXIT(
        "expected '%c' followed by elided counts \"%s\"\n", ':', "(DD | 0xHH)"
      );
    return ROW_ELIDED;
  }

  // parse offset
  char *end = NULL;
  errno = 0;
  *poffset = strtoull( buf, &end, opt_offset_fmt );
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
    uint8_t byte = XTOI( *p ) << 4;

    // parse second nybble
    ++col;
    if ( ++p == end )
      INVALID_EXIT(
        "unexpected end of data; expected %d hexadecimal bytes\n",
        ROW_SIZE
      );
    if ( !isxdigit( *p ) )
      goto expected_hex_digit;
    byte |= XTOI( *p );

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

static void reverse_dump_file( void ) {
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
        PMESSAGE_EXIT( READ_ERROR, "can not read: %s\n", STRERROR );
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
  exit( EXIT_SUCCESS );

backwards_offset:
  snprintf( msg_fmt, sizeof( msg_fmt ),
    "%%s:%%zu:1: error: \"%s\": %s offset goes backwards\n",
    get_offset_fmt_format(), get_offset_fmt_english()
  );
  PRINT_ERR( msg_fmt, fin_path, line, new_offset );
  exit( EXIT_INVALID_FORMAT );
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
    exit( search_len ? EXIT_NO_MATCHES : EXIT_SUCCESS );

  elided_separator = freelist_add( MALLOC( char, OFFSET_WIDTH + 1 ) );
  memset( elided_separator, '-', OFFSET_WIDTH );
  elided_separator[ OFFSET_WIDTH ] = '\0';
}

////////// misc. functions ////////////////////////////////////////////////////

static char const* get_offset_fmt_english() {
  switch ( opt_offset_fmt ) {
    case OFMT_DEC: return "decimal";
    case OFMT_HEX: return "hexadecimal";
    case OFMT_OCT: return "octal";
  } // switch
  assert( false );
}

static char const* get_offset_fmt_format() {
  switch ( opt_offset_fmt ) {
    case OFMT_DEC: return "%0" STRINGIFY(OFFSET_WIDTH) "lld";
    case OFMT_HEX: return "%0" STRINGIFY(OFFSET_WIDTH) "llX";
    case OFMT_OCT: return "%0" STRINGIFY(OFFSET_WIDTH) "llo";
  } // switch
  assert( false );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
