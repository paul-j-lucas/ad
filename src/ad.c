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

// local
#include "config.h"
#include "util.h"

// system
#include <assert.h>
#include <ctype.h>                      /* for isprint() */
#include <errno.h>
#include <libgen.h>                     /* for basename() */
#include <stdio.h>
#include <stdlib.h>                     /* for exit(), strtoul(), ... */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for fstat() */
#include <sys/types.h>
#include <unistd.h>                     /* for getopt() */

///////////////////////////////////////////////////////////////////////////////

#define COLUMN_WIDTH    2               /* bytes per hex column */
#define DEFAULT_COLORS  "bn=32:EC=35:MB=41;1:se=36"
#define OFFSET_WIDTH    16              /* number of offset digits */
#define OFFSET_WIDTH_S  STRINGIFY(OFFSET_WIDTH)
#define ROW_BUF_SIZE    16              /* bytes displayed in a row */

#define SGR_START       "\33[%sm"       /* start color sequence */
#define SGR_END         "\33[m"         /* end color sequence */
#define SGR_EL          "\33[K"         /* Erase in Line (EL) sequence */

enum colorization {
  COLOR_NEVER,                          // never colorize
  COLOR_ISATTY,                         // colorize only if isatty(3)
  COLOR_NOT_FILE,                       // colorize only if !ISREG stdout
  COLOR_ALWAYS                          // always colorize
};
typedef enum colorization colorization_t;

typedef size_t kmp_t;

enum offset_fmt {
  OFMT_DEC,
  OFMT_HEX,
  OFMT_OCT
};
typedef enum offset_fmt offset_fmt_t;

// extern global variables
char const*           me;               // executable name
char const*           path_name = "<stdin>";

static bool           colorize;         // dump in color?
static FILE*          file;             // file to read from
static free_node_t*   free_head;        // linked list of stuff to free
static off_t          offset;           // curent offset into file
static bool           opt_case_insensitive;
static size_t         opt_max_bytes_to_read = SIZE_MAX;
static offset_fmt_t   opt_offset_fmt = OFMT_HEX;
static bool           opt_only_matching;
static bool           opt_only_printing;
static bool           opt_verbose;

static char*          search_buf;       // not NULL-terminated when numeric
static endian_t       search_endian;    // if searching for a number
static size_t         search_len;       // number of bytes in search_buf
static unsigned long  search_number;    // the number to search for

static char const*    sgr_start = SGR_START SGR_EL;
static char const*    sgr_end   = SGR_END SGR_EL;
static char const*    sgr_offset;       // offset color
static char const*    sgr_sep;          // separator color
static char const*    sgr_elided;       // elided byte count
static char const*    sgr_hex_match;    // hex match color
static char const*    sgr_ascii_match;  // ASCII match color

// local functions
static void           init( int, char*[] );
static kmp_t*         kmp_init( char const*, size_t );
static bool           match_byte( uint8_t*, bool*, kmp_t const*, uint8_t* );
static size_t         match_row( uint8_t*, uint16_t*, kmp_t const*, uint8_t* );
static bool           parse_grep_color( char const* );
static bool           parse_grep_colors( char const* );
static bool           should_colorize( colorization_t );
static void           usage( void );

/**
 * Adds the mempory pointed to by \a P to a to-free list of memory to be freed
 * later (specifically, just before program exis).
 *
 * @param P A pointer to the memory to be freed later.
 * \hideinitializer
 */
#define FREE_LATER(P) freelist_add( (P), &free_head )

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
    uint8_t   bytes[ ROW_BUF_SIZE ];
    size_t    len;
    uint16_t  match_bits;
  };
  typedef struct row_buf row_buf_t;

  static char const *const offset_fmt_printf[] = {
    "%0" OFFSET_WIDTH_S "llu",          // decimal
    "%0" OFFSET_WIDTH_S "llX",          // hex
    "%0" OFFSET_WIDTH_S "llo",          // octal
  };

  bool        any_dumped = false;       // any data dumped yet?
  bool        any_matches = false;      // if matching, any data matched yet?
  row_buf_t   buf[2];
  row_buf_t  *cur = buf, *next = buf + 1;
  bool        is_same_row = false;
  kmp_t      *kmp_values = NULL;        // used only by match_byte()
  uint8_t    *match_buf = NULL;         // used only by match_byte()

  init( argc, argv );                   // sets offset
  off_t dumped_offset = offset;

  if ( search_len ) {                   // is user searching for anything?
    kmp_values = FREE_LATER( kmp_init( search_buf, search_len ) );
    match_buf = FREE_LATER( MALLOC( uint8_t, search_len ) );
  }

  cur->len = match_row( cur->bytes, &cur->match_bits, kmp_values, match_buf );

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
      match_row( next->bytes, &next->match_bits, kmp_values, match_buf );
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
        off_t const offset_delta = offset - dumped_offset - ROW_BUF_SIZE;
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
      PRINTF( offset_fmt_printf[ opt_offset_fmt ], offset );
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
      dumped_offset = offset;
    }

    is_same_row = !opt_verbose && !is_last_row &&
      memcmp( cur->bytes, next->bytes, ROW_BUF_SIZE ) == 0;

    row_buf_t *const temp = cur;        // swap to avoid memcpy()
    cur = next, next = temp;

    offset += ROW_BUF_SIZE;
  } // while

  exit( search_len && !any_matches ? EXIT_NO_MATCHES : EXIT_OK );
}

/////////// matching //////////////////////////////////////////////////////////

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
static kmp_t* kmp_init( char const *pattern, size_t pattern_len ) {
  assert( pattern );
  kmp_t *const kmps = MALLOC( kmp_t, pattern_len );
  kmps[0] = 0;
  for ( size_t i = 1, j = 0; i < pattern_len; ) {
    if ( pattern[i] == pattern[j] )
      kmps[++i] = ++j;
    else if ( j > 0 )
      j = kmps[j-1];
    else
      kmps[++i] = 0;
  } // for
  return kmps;
}

/**
 * Gets a byte and whether it matches one of the bytes in the search buffer.
 *
 * @param pbyte A pointer to receive the byte.
 * @param matches A pointer to receive whether the byte matches.
 * @param kmp_values A pointer to the array of KMP values to use.
 * @param match_buf A pointer to a buffer to use while matching.
 * It must be at least as large as the search buffer.
 * @return Returns \c true if a byte was read successfully
 * and the number of bytes read does not exceed \a max_bytes_to_read.
 */
static bool match_byte( uint8_t *pbyte, bool *matches,
                        kmp_t const *kmp_values, uint8_t *match_buf ) {
  enum state {
    S_READING,                          // just reading; not matching
    S_MATCHING,                         // matching search bytes
    S_MATCHING_CONT,                    // matching after a mismatch
    S_MATCHED,                          // a complete match
    S_NOT_MATCHED,                      // didn't match after all
    S_DONE                              // no more input
  };
  typedef enum state state_t;

  static size_t buf_pos;
  static size_t buf_drain;
  static size_t kmp;
  static state_t state = S_READING;

  uint8_t byte;

  assert( pbyte );
  assert( matches );
  assert( state != S_DONE );

  *matches = false;

  for ( ;; ) {
    switch ( state ) {

#define GOTO_STATE(S)       { buf_pos = 0; state = (S); continue; }
#define RETURN(BYTE)        BLOCK( *pbyte = (BYTE); return true; )

#define MAYBE_NO_CASE(BYTE) \
  ( opt_case_insensitive ? (uint8_t)tolower( (char)(BYTE) ) : (BYTE) )

      case S_READING:
        if ( !get_byte( &byte, opt_max_bytes_to_read, file ) )
          GOTO_STATE( S_DONE );
        if ( !search_len )              // user isn't searching for anything
          RETURN( byte );
        if ( MAYBE_NO_CASE( byte ) != (uint8_t)search_buf[0] )
          RETURN( byte );               // searching, but no match yet
        match_buf[ 0 ] = byte;
        kmp = 0;
        GOTO_STATE( S_MATCHING );

      case S_MATCHING:
        if ( ++buf_pos == search_len ) {
          *matches = true;
          buf_drain = buf_pos;
          GOTO_STATE( S_MATCHED );
        }
        // no break;
      case S_MATCHING_CONT:
        if ( !get_byte( &byte, opt_max_bytes_to_read, file ) ) {
          buf_drain = buf_pos;
          GOTO_STATE( S_NOT_MATCHED );
        }
        if ( MAYBE_NO_CASE( byte ) == (uint8_t)search_buf[ buf_pos ] ) {
          match_buf[ buf_pos ] = byte;
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
        RETURN( match_buf[ buf_pos++ ] );

      case S_DONE:
        return false;

#undef GOTO_STATE
#undef MAYBE_NO_CASE
#undef RETURN

    } // switch
  } // for
}

/**
 * Gets a row of bytes (of \c ROW_BUF_SIZE) and whether each byte matches bytes
 * in the search buffer.
 *
 * @param row_buf A pointer to the row buffer.
 * @param match_bits A pointer to receive which bytes matched.  Note that the
 * bytes in the buffer are numbered left-to-right where as their corresponding
 * bits are numbered right-to-left.
 * @param kmp_values A pointer to the array of KMP values to use.
 * @param match_buf A pointer to a buffer to use while matching.
 * @return Returns the number of bytes in \a row_buf.  It should always be
 * \c ROW_BUF_SIZE except on the last row in which case it will be less than
 * \c ROW_BUF_SIZE.
 */
static size_t match_row( uint8_t *row_buf, uint16_t *match_bits,
                         kmp_t const *kmp_values, uint8_t *match_buf ) {
  assert( match_bits );
  *match_bits = 0;

  size_t buf_len;
  for ( buf_len = 0; buf_len < ROW_BUF_SIZE; ++buf_len ) {
    bool matches;
    if ( !match_byte( row_buf + buf_len, &matches, kmp_values, match_buf ) ) {
      // pad remainder of line
      memset( row_buf + buf_len, 0, ROW_BUF_SIZE - buf_len );
      break;
    }
    if ( matches )
      *match_bits |= 1 << buf_len;
  } // for
  return buf_len;
}

/////////// option parsing ////////////////////////////////////////////////////

static void check_number_size( size_t given_size, size_t actual_size,
                               char opt ) {
  if ( given_size < actual_size )
    PMESSAGE_EXIT( USAGE,
      "\"%zu\": value for -%c option is too small for \"%lu\""
      "; must be at least %zu\n",
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

/**
 * Parses a colorization "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c colorization_t.
 * or prints an error message and exits.
 */
static colorization_t parse_colorization( char const *when ) {
  struct colorize_map {
    char const *map_when;
    colorization_t map_colorization;
  };
  typedef struct colorize_map colorize_map_t;

  static colorize_map_t const colorize_map[] = {
    { "always",    COLOR_ALWAYS   },
    { "auto",      COLOR_ISATTY   },    // grep compatibility
    { "isatty",    COLOR_ISATTY   },    // explicit synonym for auto
    { "never",     COLOR_NEVER    },
    { "not_file",  COLOR_NOT_FILE },
    { "not_isreg", COLOR_NOT_FILE },    // synonym for not_isfile
    { "tty",       COLOR_ISATTY   },    // synonym for isatty
    { NULL,        COLOR_NEVER    }
  };

  char const *const when_lc = tolower_s( FREE_LATER( check_strdup( when ) ) );
  size_t names_buf_size = 1;            // for trailing NULL

  for ( colorize_map_t const *m = colorize_map; m->map_when; ++m ) {
    if ( strcmp( when_lc, m->map_when ) == 0 )
      return m->map_colorization;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = FREE_LATER( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  for ( colorize_map_t const *m = colorize_map; m->map_when; ++m ) {
    if ( pnames > names_buf ) {
      strcpy( pnames, ", " );
      pnames += 2;
    }
    strcpy( pnames, m->map_when );
    pnames += strlen( m->map_when );
  } // for
  PMESSAGE_EXIT( USAGE,
    "\"%s\": invalid value for -c option; must be one of:\n\t%s\n",
    when, names_buf
  );
}

static void parse_options( int argc, char *argv[] ) {
  colorization_t  colorization = COLOR_NOT_FILE;
  int             opt;                  // command-line option
  char const      opts[] = "b:B:c:de:E:hij:mN:ops:S:vV";
  size_t          size_in_bits = 0, size_in_bytes = 0;

  opterr = 1;
  while ( (opt = getopt( argc, argv, opts )) != EOF ) {
    switch ( opt ) {
      case 'b': size_in_bits = parse_ul( optarg );                  break;
      case 'B': size_in_bytes = parse_ul( optarg );                 break;
      case 'c': colorization = parse_colorization( optarg );        break;
      case 'd': opt_offset_fmt = OFMT_DEC;                          break;
      case 'e': search_number = parse_ul( optarg );
                search_endian = ENDIAN_LITTLE;                      break;
      case 'E': search_number = parse_ul( optarg );
                search_endian = ENDIAN_BIG;                         break;
      case 'h': opt_offset_fmt = OFMT_HEX;                          break;
      case 'S': search_buf = FREE_LATER( check_strdup( optarg ) );
      case 'i': opt_case_insensitive = true;                        break;
      case 'j': offset += parse_offset( optarg );                   break;
      case 'm': opt_only_matching = true;                           break;
      case 'N': opt_max_bytes_to_read = parse_offset( optarg );     break;
      case 'o': opt_offset_fmt = OFMT_OCT;                          break;
      case 'p': opt_only_printing = true;                           break;
      case 's': search_buf = FREE_LATER( check_strdup( optarg ) );  break;
      case 'v': opt_verbose = true;                                 break;
      case 'V': PRINT_ERR( "%s\n", PACKAGE_STRING );      exit( EXIT_OK );
      default : usage();
    } // switch
  } // while
  argc -= optind, argv += optind - 1;

  if ( size_in_bits && size_in_bytes )
    options_mutually_exclusive( "b", "B" );
  if ( search_endian && search_buf )
    options_mutually_exclusive( "eE", "s" );
  if ( opt_only_matching && opt_verbose )
    options_mutually_exclusive( "m", "v" );
  if ( opt_only_printing && opt_verbose )
    options_mutually_exclusive( "p", "v" );

  if ( opt_case_insensitive ) {
    if ( !search_buf )
      option_required( "i", "s" );
    tolower_s( search_buf );
  }

  if ( opt_only_matching && !(search_endian || search_buf) )
    option_required( "m", "eEsS" );

  if ( size_in_bits ) {
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
          "\"%zu\": invalid value for -%c option; must be one of: 8, 16, 32"
#if SIZEOF_UNSIGNED_LONG == 8
          ", 64"
#endif /* SIZEOF_UNSIGNED_LONG */
          "\n", size_in_bits, 'b'
        );
    } // switch
    if ( !search_endian )
      option_required( "b", "eE" );
    check_number_size( size_in_bits, ulong_len( search_number ) * 8, 'b' );
  }

  if ( size_in_bytes ) {
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
          "\"%zu\": invalid value for -%c option; must be one of: 1, 2, 4"
#if SIZEOF_UNSIGNED_LONG == 8
          ", 8"
#endif /* SIZEOF_UNSIGNED_LONG */
          "\n", size_in_bytes, 'B'
        );
    } // switch
    if ( !search_endian )
      option_required( "B", "eE" );
    check_number_size( size_in_bytes, ulong_len( search_number ), 'B' );
  }

  colorize = should_colorize( colorization );
  if ( colorize ) {
    if ( !(parse_grep_colors( getenv( "AD_COLORS"   ) )
        || parse_grep_colors( getenv( "GREP_COLORS" ) )
        || parse_grep_color ( getenv( "GREP_COLOR"  ) )) ) {
      parse_grep_colors( DEFAULT_COLORS );
    }
  }

  switch ( argc ) {
    case 0:                             // read from stdin with no offset
      file = fdopen( STDIN_FILENO, "r" );
      break;

    case 1:                             // offset OR file
      if ( *argv[1] == '+' ) {
        file = fdopen( STDIN_FILENO, "r" );
        fskip( parse_offset( argv[1] ), file );
      } else {
        path_name = argv[1];
        file = open_file( path_name, offset );
      }
      break;

    case 2:                             // offset & file OR file & offset
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
  } // switch
}

static void usage( void ) {
  PRINT_ERR(
"usage: %s [options] [+offset] [file]\n"
"       %s [options] [file] [[+]offset]\n"
"\n"
"options:\n"
"       -b bits    Specify number size in bits: 8, 16, 32"
#if SIZEOF_UNSIGNED_LONG == 8
                   ", 64"
#endif /* SIZEOF_UNSIGNED_LONG */
                   " [default: auto].\n"
"       -B bytes   Specify number size in bytes: 1, 2, 4"
#if SIZEOF_UNSIGNED_LONG == 8
                   ", 8"
#endif /* SIZEOF_UNSIGNED_LONG */
                   " [default: auto].\n"
"       -c when    Specify when to colorize output [default: not_file].\n"
"       -d         Print offset in decimal.\n"
"       -e number  Search for little-endian number.\n"
"       -E number  Search for big-endian number.\n"
"       -h         Print offset in hexadecimal [default].\n"
"       -i         Search for case-insensitive string [default: no].\n"
"       -j offset  Jump to offset before dumping [default: 0].\n"
"       -m         Only dump rows having matches [default: no].\n"
"       -N bytes   Dump max number of bytes [default: unlimited].\n"
"       -o         Print offset in octal.\n"
"       -p         Only dump rows having printable characters [default: no].\n"
"       -s string  Search for string.\n"
"       -S string  Search for case-insensitive string.\n"
"       -v         Print version and exit.\n"
"       -V         Dump all data, including repeated rows [default: no].\n"
    , me, me
  );
  exit( EXIT_USAGE );
}

/////////// color /////////////////////////////////////////////////////////////

/**
 * Color capability used to map an AD_COLORS/GREP_COLORS "capability" either to
 * the variable to set or the function to call.
 */
struct color_cap {
  char const *cap_name;                 // capability name
  char const **cap_var_to_set;          // variable to set ...
  void (*cap_func)( char const* );      // ... OR function to call
};
typedef struct color_cap color_cap_t;

/**
 * Sets the SGR color for the given capability.
 *
 * @param cap The color capability to set the color for.
 * @param sgr_color The SGR color to set or empty or NULL to unset.
 * @return Returns \c true only if \a sgr_color is valid.
 */
static bool cap_set( color_cap_t const *cap, char const *sgr_color ) {
  assert( cap );
  assert( cap->cap_var_to_set || cap->cap_func );

  if ( sgr_color ) {
    if ( !*sgr_color )                  // empty string -> NULL = unset
      sgr_color = NULL;
    else if ( !parse_sgr( sgr_color ) )
      return false;
  }

  if ( cap->cap_var_to_set )
    *cap->cap_var_to_set = sgr_color;
  else
    (*cap->cap_func)( sgr_color );
  return true;
}

/**
 * Sets both the hex and ASCII match color.
 * (This function is needed for the color capabilities table to support the
 * "MB" and "mt" capabilities.)
 *
 * @param sgr_color The SGR color to set or empty or NULL to unset.
 */
static void cap_mt( char const *sgr_color ) {
  if ( !*sgr_color )                    // empty string -> NULL = unset
    sgr_color = NULL;
  sgr_ascii_match = sgr_hex_match = sgr_color;
}

/**
 * Turns off using the EL (Erase in Line) sequence.
 *
 * @param sgr_color Not used.
 */
static void cap_ne( char const *sgr_color ) {
  (void)sgr_color;                      // suppress warning
  sgr_start = SGR_START;
  sgr_end   = SGR_END;
}

/**
 * Color capabilities table.  Upper-case names are unique to us and upper-case
 * to avoid conflict with grep.  Lower-case names are for grep compatibility.
 */
static color_cap_t const color_caps[] = {
  { "bn", &sgr_offset,      NULL   },   // grep: byte offset
  { "EC", &sgr_elided,      NULL   },   // elided count
  { "MA", &sgr_ascii_match, NULL   },   // matched ASCII
  { "MH", &sgr_hex_match,   NULL   },   // matched hex
  { "MB", NULL,             cap_mt },   // matched both
  { "mt", NULL,             cap_mt },   // grep: matched text (both)
  { "se", &sgr_sep,         NULL   },   // grep: separator
  { "ne", NULL,             cap_ne },   // grep: no EL on SGR
  { NULL, NULL,             NULL   }
};

/**
 * Parses a single SGR color and, if successful, sets the match color.
 *
 * @param sgr_color An SGR color to parse.
 * @return Returns \c true only if the value was parsed successfully.
 */
static bool parse_grep_color( char const *sgr_color ) {
  if ( parse_sgr( sgr_color ) ) {
    cap_mt( sgr_color );
    return true;
  }
  return false;
}

/**
 * Parses and sets the sequence of grep color capabilities.
 *
 * @param capabilities The grep capabilities to parse.
 * @return Returns \c true only if at least one capability was parsed
 * successfully.
 */
static bool parse_grep_colors( char const *capabilities ) {
  bool set_something = false;

  if ( capabilities ) {
    // free this later since the sgr_* variables point to substrings
    char *const capabilities_dup = FREE_LATER( check_strdup( capabilities ) );
    char *next_cap = capabilities_dup;
    char *cap_name_val;

    while ( (cap_name_val = strsep( &next_cap, ":" )) ) {
      char const *const cap_name = strsep( &cap_name_val, "=" );
      for ( color_cap_t const *cap = color_caps; cap->cap_name; ++cap ) {
        if ( strcmp( cap_name, cap->cap_name ) == 0 ) {
          char const *const cap_value = strsep( &cap_name_val, "=" );
          if ( cap_set( cap, cap_value ) )
            set_something = true;
          break;
        }
      } // for
    } // while
  }
  return set_something;
}

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param c The colorization value.
 * @return Returns \c true only if we should do color.
 */
static bool should_colorize( colorization_t c ) {
  switch ( c ) {                        // handle easy cases
    case COLOR_ALWAYS: return true;
    case COLOR_NEVER : return false;
    default          : break;
  }

  //
  // If TERM is unset, empty, or "dumb", color probably won't work.
  //
  char const *const term = getenv( "TERM" );
  if ( !term || !*term || strcmp( term, "dumb" ) == 0 )
    return false;

  if ( c == COLOR_ISATTY )              // emulate grep's --color=auto
    return isatty( STDOUT_FILENO );

  //
  // Otherwise we want to do color only we're writing either to a TTY or to a
  // pipe (so the common case of piping to less(1) will still show color) but
  // NOT when writing to a file because we don't want the escape sequences
  // polluting it.
  //
  // Results from testing using isatty(3) and fstat(3) are given in the
  // following table:
  //
  //    COMMAND   Should? isatty ISCHR ISFIFO ISREG
  //    ========= ======= ====== ===== ====== =====
  //    ad           T      T      T     F      F
  //    ad > file    F      F      F     F    >>T<<
  //    ad | less    T      F      F     T      F
  //
  // Hence, we want to do color _except_ when ISREG=T.
  //
  struct stat stdout_stat;
  FSTAT( STDOUT_FILENO, &stdout_stat );
  return !S_ISREG( stdout_stat.st_mode );
}

/////////// initialization & clean-up /////////////////////////////////////////

static void clean_up( void ) {
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
