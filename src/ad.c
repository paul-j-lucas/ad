/*
**      ad -- ASCII dump
**      ad.c: implementation
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

/* local */
#include "config.h"
#include "sgr_color.h"
#include "util.h"

/* system */
#include <assert.h>
#include <ctype.h>                      /* for isprint() */
#include <errno.h>
#include <libgen.h>                     /* for basename() */
#include <stdio.h>
#include <stdlib.h>                     /* for exit(), strtoul(), ... */
#include <string.h>                     /* for str...() */
#include <sys/types.h>
#include <unistd.h>                     /* for getopt() */

/*****************************************************************************/

#define BUF_SIZE      16                /* bytes displayed on a line */

typedef int kmp_value;

enum offset_fmt {
  OFMT_DEC,
  OFMT_HEX,
  OFMT_OCT
};
typedef enum offset_fmt offset_fmt_t;

char const*           me;               /* executable name */
char const*           path_name = "<stdin>";

static FILE*          file;             /* file to read from */
static free_node_t*   free_head;        /* linked list of stuff to free */
static off_t          offset;           /* curent offset into file */
static bool           opt_case_insensitive = false;
static size_t         opt_max_bytes_to_read = SIZE_MAX;
static offset_fmt_t   opt_offset_fmt = OFMT_HEX;

static char*          search_buf;       /* not NULL-terminated when numeric */
static endian_t       search_endian;    /* if searching for a number */
static size_t         search_len;       /* number of bytes in search_buf */
static char const*    search_match_color = SGR_BG_COLOR_RED;
static unsigned long  search_number;    /* the number to search for */

/* local functions */
static void           clean_up();
static void           init( int, char*[] );
static kmp_value*     kmp_init( char const*, size_t );
static bool           match_byte( uint8_t*, bool*, kmp_value const*, uint8_t* );
static void           parse_options( int, char*[] );
static void           set_match_color();
static void           usage();

/*****************************************************************************/

#define COLOR_ON_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_COLOR( search_match_color ); )
#define COLOR_OFF_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_END_COLOR(); )

#define COLOR_ON()  COLOR_ON_IF( matches )
#define COLOR_OFF() COLOR_OFF_IF( matches )

int main( int argc, char *argv[] ) {
  static char const *const offset_fmt_printf[] = {
    "%016llu: ",                        /* decimal */
    "%016llX: ",                        /* hex */
    "%016llo: ",                        /* octal */
  };

  uint8_t buf[ BUF_SIZE ];              /* store bytes to print ASCII later */
  size_t buf_pos = 0;
  uint16_t color_bits = 0;              /* bit set = print in color */
  bool done = false;
  bool matches_prev = false;
  size_t i;

  kmp_value *kmp_values;
  uint8_t *match_buf;                   /* working storage for match_byte() */

  init( argc, argv );

  if ( search_len ) {
    kmp_values = freelist_add( kmp_init( search_buf, search_len ), &free_head );
    match_buf = freelist_add( check_realloc( NULL, search_len ), &free_head );
  } else {
    kmp_values = NULL;
    match_buf = NULL;
  }

  while ( !done ) {
    bool matches;
    done = !match_byte( buf + buf_pos, &matches, kmp_values, match_buf );
    if ( done ) {
      COLOR_OFF_IF( matches_prev );
      if ( buf_pos ) {                  /* print padding */
        for ( i = BUF_SIZE - buf_pos; i; --i )
          PRINTF( "  " );
        for ( i = (BUF_SIZE - buf_pos) / 2; i; --i )
          PUTCHAR( ' ' );
        goto ascii;                     /* print final ASCII part */
      }
    } else {
      if ( !buf_pos ) {                 /* print offset */
        COLOR_OFF();
        PRINTF( offset_fmt_printf[ opt_offset_fmt ], offset );
        COLOR_ON();
      } else if ( buf_pos % 2 == 0 ) {  /* print separator between columns */
        COLOR_OFF_IF( matches_prev );
        PUTCHAR( ' ' );
        COLOR_ON_IF( matches_prev );
      }
      ++offset;

      if ( matches ) {                  /* print hex part */
        COLOR_ON_IF( matches != matches_prev );
        color_bits |= 1 << buf_pos;
      } else {
        COLOR_OFF_IF( matches != matches_prev );
      }
      PRINTF( "%02X", (unsigned)buf[ buf_pos ] );
      matches_prev = matches;

      if ( ++buf_pos == BUF_SIZE ) {    /* print ASCII part */
ascii:  COLOR_OFF();
        matches_prev = false;
        PRINTF( "  " );
        for ( i = 0; i < buf_pos; ++i ) {
          matches = color_bits & (1 << i);
          if ( matches )
            COLOR_ON_IF( matches != matches_prev );
          else
            COLOR_OFF_IF( matches != matches_prev );
          PUTCHAR( isprint( buf[i] ) ? buf[i] : '.' );
          matches_prev = matches;
        } /* for */
        COLOR_OFF();
        PUTCHAR( '\n' );
        COLOR_ON();
        buf_pos = 0;
        color_bits = 0;
      }
    }
  } /* while */

  exit( EXIT_OK );
}

/*****************************************************************************/

static void clean_up() {
  freelist_free( free_head );
  if ( file )
    fclose( file );
}

static void init( int argc, char *argv[] ) {
  me = basename( argv[0] );
  atexit( clean_up );
  parse_options( argc, argv );

  if ( search_buf )
    search_len = strlen( search_buf );
  else if ( search_endian ) {
    if ( !search_len )                  /* default to smallest possible size */
      search_len = ulong_len( search_number );
    ulong_rearrange_bytes( &search_number, search_len, search_endian );
    search_buf = (char*)&search_number;
  }

  if ( search_buf )
    set_match_color();
}

/**
 * Consructs the partial-match table used by the Knuth-Morris-Pratt (KMP)
 * string searching algorithm.
 *
 * For the small search patterns and there being no requirement for super-fast
 * performance for this application, brute-force searching would have been
 * fine.  However, KMP has the advantage of never having to back up within the
 * string being searched which is a requirement when reading from stdin.
 *
 * @param pattern The search pattern to use.
 * @param pattern_len The length of the pattern.
 * @return Returns an array containing the values comprising the partial-match
 * table.  The caller is responsible for freeing the array.
 */
static kmp_value* kmp_init( char const *pattern, size_t pattern_len ) {
  kmp_value *kmp_values;
  size_t i, j = 0;

  assert( pattern );
  kmp_values = check_realloc( NULL, pattern_len * sizeof( kmp_value ) );

  kmp_values[0] = -1;
  for ( i = 1; i < pattern_len; ) {
    if ( pattern[i] == pattern[j] )
      kmp_values[++i] = ++j;
    else if ( j > 0 )
      j = kmp_values[j-1];
    else
      kmp_values[++i] = 0;
  } /* for */
  return kmp_values;
}

static bool match_byte( uint8_t *pbyte, bool *matches,
                        kmp_value const *kmp_values, uint8_t *buf ) {
  typedef enum {
    S_READING,                          /* just reading; not matching */
    S_MATCHING,                         /* matching search string */
    S_MATCHING_CONT,                    /* matching after a mismatch */
    S_MATCHED,                          /* a complete match */
    S_NOT_MATCHED,                      /* didn't match after all */
    S_DONE                              /* no more input */
  } state_t;

  static size_t buf_pos;
  static state_t state = S_READING;
  static size_t buf_drain;
  static size_t kmp;

  uint8_t byte;

  assert( pbyte );
  assert( matches );
  assert( state != S_DONE );

  *matches = false;

  for ( ;; ) {
    switch ( state ) {

#define GOTO_STATE(S)       { buf_pos = 0; state = (S); continue; }
#define MAYBE_NO_CASE(BYTE) ( opt_case_insensitive ? tolower( BYTE ) : (BYTE) )
#define RETURN(BYTE)        BLOCK( *pbyte = (BYTE); return true; )

      case S_READING:
        if ( !get_byte( &byte, opt_max_bytes_to_read, file ) )
          GOTO_STATE( S_DONE );
        if ( !search_len )
          RETURN( byte );
        if ( MAYBE_NO_CASE( byte ) != search_buf[0] )
          RETURN( byte );
        buf[ 0 ] = byte;
        kmp = 0;
        GOTO_STATE( S_MATCHING );

      case S_MATCHING:
        if ( ++buf_pos == search_len ) {
          *matches = true;
          buf_drain = buf_pos;
          GOTO_STATE( S_MATCHED );
        }
        /* no break; */
      case S_MATCHING_CONT:
        if ( !get_byte( &byte, opt_max_bytes_to_read, file ) ) {
          buf_drain = buf_pos;
          GOTO_STATE( S_NOT_MATCHED );
        }
        if ( MAYBE_NO_CASE( byte ) == search_buf[ buf_pos ] ) {
          buf[ buf_pos ] = byte;
          state = S_MATCHING;
          continue;
        }
        unget_byte( byte, file );
        kmp = kmp_values[ buf_pos ];
        buf_drain = buf_pos - kmp;
        GOTO_STATE( S_NOT_MATCHED );

      case S_MATCHED:
      case S_NOT_MATCHED:
        if ( buf_pos == buf_drain ) {
          buf_pos = kmp;
          state = buf_pos ? S_MATCHING_CONT : S_READING;
          continue;
        }
        *matches = state == S_MATCHED;
        RETURN( buf[ buf_pos++ ] );

      case S_DONE:
        return false;

#undef GOTO_STATE
#undef MAYBE_NO_CASE
#undef RETURN

    } /* switch */
  } /* for */
}

static void check_number_size( size_t given_size, size_t actual_size,
                               char opt ) {
  if ( given_size < actual_size )
    PMESSAGE_EXIT( USAGE,
      "%lu: value for -%c option is too small for %lu; must be at least %lu\n",
      given_size, opt, search_number, actual_size
    );
}

static void options_mutually_exclusive( char const *opt1, char const *opt2 ) {
  PMESSAGE_EXIT( USAGE,
    "-%s and -%s options are mutually exclusive\n", opt1, opt2
  );
}

static void option_required( char const *opt, char const *req ) {
  bool const opt_multiple = strlen( opt ) > 1;
  bool const req_multiple = strlen( req ) > 1;
  PMESSAGE_EXIT( USAGE,
    "-%s: option%s require%s -%s option%s to be given also\n",
    opt, (opt_multiple ? "s" : ""), (opt_multiple ? "" : "s"),
    req, (req_multiple ? "s" : "")
  );
}

static void parse_options( int argc, char *argv[] ) {
  int         opt;                      /* command-line option */
  char const  opts[] = "b:B:de:E:hij:N:os:v";

  size_t size_in_bits = 0, size_in_bytes = 0;

  opterr = 1;
  while ( (opt = getopt( argc, argv, opts )) != EOF ) {
    switch ( opt ) {
      case 'b': size_in_bits = parse_ul( optarg );              break;
      case 'B': size_in_bytes = parse_ul( optarg );             break;
      case 'd': opt_offset_fmt = OFMT_DEC;                      break;
      case 'e': search_number = parse_ul( optarg );
                search_endian = ENDIAN_LITTLE;                  break;
      case 'E': search_number = parse_ul( optarg );
                search_endian = ENDIAN_BIG;                     break;
      case 'h': opt_offset_fmt = OFMT_HEX;                      break;
      case 'i': opt_case_insensitive = true;                    break;
      case 'j': offset += parse_offset( optarg );               break;
      case 'N': opt_max_bytes_to_read = parse_offset( optarg ); break;
      case 'o': opt_offset_fmt = OFMT_OCT;                      break;
      case 's': search_buf = optarg;                            break;
      case 'v': fprintf( stderr, "%s\n", PACKAGE_STRING );      exit( EXIT_OK );
      default : usage();
    } /* switch */
  } /* while */
  argc -= optind, argv += optind - 1;

  if ( search_endian && search_buf )
    options_mutually_exclusive( "eE", "s" );

  if ( opt_case_insensitive ) {
    if ( !search_buf )
      option_required( "i", "s" );
    tolower_s( search_buf );
  }

  if ( size_in_bits && size_in_bytes )
    options_mutually_exclusive( "b", "B" );

  if ( size_in_bits ) {
    if ( !search_endian )
      option_required( "b", "eE" );
    switch ( size_in_bits ) {
      case  8:
      case 16:
      case 32:
#if SIZEOF_UNSIGNED_LONG == 8
      case 64:
#endif /* SIZEOF_UNSIGNED_LONG */
        search_len = size_in_bits * 8;
        break;
      default:
        PMESSAGE_EXIT( USAGE,
          "%lu: invalid value for -%c option; must be one of: 8, 16, 32"
#if SIZEOF_UNSIGNED_LONG == 8
          ", 64"
#endif /* SIZEOF_UNSIGNED_LONG */
          "\n", size_in_bits, 'b'
        );
    } /* switch */
    check_number_size( size_in_bits, ulong_len( search_number ) * 8, 'b' );
  }

  if ( size_in_bytes ) {
    if ( !search_endian )
      option_required( "B", "eE" );
    switch ( size_in_bytes ) {
      case 1:
      case 2:
      case 4:
#if SIZEOF_UNSIGNED_LONG == 8
      case 8:
#endif /* SIZEOF_UNSIGNED_LONG */
        search_len = size_in_bytes;
        break;
      default:
        PMESSAGE_EXIT( USAGE,
          "%lu: invalid value for -%c option; must be one of: 1, 2, 4"
#if SIZEOF_UNSIGNED_LONG == 8
          ", 8"
#endif /* SIZEOF_UNSIGNED_LONG */
          "\n", size_in_bytes, 'B'
        );
    } /* switch */
    check_number_size( size_in_bytes, ulong_len( search_number ), 'B' );
  }

  switch ( argc ) {
    case 0:                             /* read from stdin with no offset */
      file = fdopen( STDIN_FILENO, "r" );
      break;

    case 1:                             /* offset OR file */
      if ( *argv[1] == '+' ) {
        file = fdopen( STDIN_FILENO, "r" );
        fskip( parse_offset( argv[1] ), file );
      } else {
        path_name = argv[1];
        file = open_file( path_name, offset );
      }
      break;

    case 2:                             /* offset & file OR file & offset */
      if ( *argv[1] == '+' ) {
        if ( *argv[2] == '+' )
          PMESSAGE_EXIT( USAGE,
            "'%c': can not specify for more than one argument\n", '+'
          );
        offset += parse_offset( argv[1] );
        path_name = argv[2];
      } else {
        path_name = argv[1];
        offset += parse_offset( argv[2] );
      }
      file = open_file( path_name, offset );
      break;

    default:
      usage();
  } /* switch */

  if ( !opt_max_bytes_to_read )
    exit( EXIT_OK );
}

static void set_match_color() {
  static char const *const env_vars[] = {
    "AD_COLOR",
    "GREP_COLOR",
    0
  };

  char const *const *env_var;
  for ( env_var = env_vars; *env_var; ++env_var ) {
    char const *const color = getenv( *env_var );
    unsigned long dont_care;            /* only care if it will parse */
    if ( color && parse_ul_impl( color, &dont_care ) ) {
      search_match_color = color;
      break;
    }
  } /* for */
}

static void usage() {
  fprintf( stderr,
"usage: %s [-dhiov] [-j offset] [-N length] [[-i] -s search] [file] [[+]offset]\n"
"       %s [-dhiov] [-j offset] [-N length] [[-i] -s search] [+offset] [file]\n"
    , me, me
  );
  exit( EXIT_USAGE );
}

/*****************************************************************************/
/* vim:set et sw=2 ts=2: */
