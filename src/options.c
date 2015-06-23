/*
**      ad -- ASCII dump
**      options.c
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
#include "color.h"
#include "common.h"
#include "config.h"
#include "options.h"

// system
#include <assert.h>
#include <libgen.h>                     /* for basename() */
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for fstat() */
#include <sys/types.h>
#include <unistd.h>                     /* for getopt() */

///////////////////////////////////////////////////////////////////////////////

FILE         *file_input;
off_t         file_offset;
char const   *file_path = "<stdin>";
char const   *me;

bool          opt_case_insensitive;
size_t        opt_max_bytes_to_read = SIZE_MAX;
offset_fmt_t  opt_offset_fmt = OFMT_HEX;
bool          opt_only_matching;
bool          opt_only_printing;
bool          opt_verbose;

char         *search_buf;
endian_t      search_endian;
size_t        search_len;
uint64_t      search_number;

/////////// local functions ///////////////////////////////////////////////////

static void check_number_size( size_t given_size, size_t actual_size,
                               char opt ) {
  if ( given_size < actual_size )
    PMESSAGE_EXIT( USAGE,
      "\"%zu\": value for -%c option is too small for \"%llu\""
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
    { "not_file",  COLOR_NOT_FILE },    // !ISREG( stdout )
    { "not_isreg", COLOR_NOT_FILE },    // synonym for not_isfile
    { "tty",       COLOR_ISATTY   },    // synonym for isatty
    { NULL,        COLOR_NEVER    }
  };

  char const *const when_lc = tolower_s( freelist_add( check_strdup( when ) ) );
  size_t names_buf_size = 1;            // for trailing NULL

  for ( colorize_map_t const *m = colorize_map; m->map_when; ++m ) {
    if ( strcmp( when_lc, m->map_when ) == 0 )
      return m->map_colorization;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = freelist_add( MALLOC( char, names_buf_size ) );
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

////////// extern functions ///////////////////////////////////////////////////

void parse_options( int argc, char *argv[] ) {
  colorization_t  colorization = COLOR_NOT_FILE;
  int             opt;                  // command-line option
  char const      opts[] = "b:B:c:de:E:hij:mN:ops:S:vV";
  size_t          size_in_bits = 0, size_in_bytes = 0;

  me = basename( argv[0] );
  opterr = 1;

  while ( (opt = getopt( argc, argv, opts )) != EOF ) {
    switch ( opt ) {
      case 'b': size_in_bits = parse_ull( optarg );                   break;
      case 'B': size_in_bytes = parse_ull( optarg );                  break;
      case 'c': colorization = parse_colorization( optarg );          break;
      case 'd': opt_offset_fmt = OFMT_DEC;                            break;
      case 'e': search_number = parse_ull( optarg );
                search_endian = ENDIAN_LITTLE;                        break;
      case 'E': search_number = parse_ull( optarg );
                search_endian = ENDIAN_BIG;                           break;
      case 'h': opt_offset_fmt = OFMT_HEX;                            break;
      case 'S': search_buf = freelist_add( check_strdup( optarg ) );  
      case 'i': opt_case_insensitive = true;                          break;
      case 'j': file_offset += parse_offset( optarg );                break;
      case 'm': opt_only_matching = true;                             break;
      case 'N': opt_max_bytes_to_read = parse_offset( optarg );       break;
      case 'o': opt_offset_fmt = OFMT_OCT;                            break;
      case 'p': opt_only_printing = true;                             break;
      case 's': search_buf = freelist_add( check_strdup( optarg ) );  break;
      case 'v': opt_verbose = true;                                   break;
      case 'V': PRINT_ERR( "%s\n", PACKAGE_STRING );        exit( EXIT_OK );
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
      case 64:
        search_len = size_in_bits * 8;
        break;
      default:
        PMESSAGE_EXIT( USAGE,
          "\"%zu\": invalid value for -%c option; must be one of: 8, 16, 32, 64"
          "\n", size_in_bits, 'b'
        );
    } // switch
    if ( !search_endian )
      option_required( "b", "eE" );
    check_number_size( size_in_bits, int_len( search_number ) * 8, 'b' );
  }

  if ( size_in_bytes ) {
    switch ( size_in_bytes ) {
      case 1:
      case 2:
      case 4:
      case 8:
        search_len = size_in_bytes;
        break;
      default:
        PMESSAGE_EXIT( USAGE,
          "\"%zu\": invalid value for -%c option; must be one of: 1, 2, 4, 8"
          "\n", size_in_bytes, 'B'
        );
    } // switch
    if ( !search_endian )
      option_required( "B", "eE" );
    check_number_size( size_in_bytes, int_len( search_number ), 'B' );
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
      file_input = fdopen( STDIN_FILENO, "r" );
      break;

    case 1:                             // offset OR file
      if ( *argv[1] == '+' ) {
        file_offset += parse_offset( argv[1] );
        file_input = fdopen( STDIN_FILENO, "r" );
        fskip( file_offset, file_input );
      } else {
        file_path = argv[1];
        file_input = open_file( file_path, file_offset );
      }
      break;

    case 2:                             // offset & file OR file & offset
      if ( *argv[1] == '+' ) {
        if ( *argv[2] == '+' )
          PMESSAGE_EXIT( USAGE,
            "'%c': can not specify for more than one argument\n", '+'
          );
        file_offset += parse_offset( argv[1] );
        file_path = argv[2];
      } else {
        file_path = argv[1];
        file_offset += parse_offset( argv[2] );
      }
      file_input = open_file( file_path, file_offset );
      break;

    default:
      usage();
  } // switch
}

void usage( void ) {
  PRINT_ERR(
"usage: %s [options] [+offset] [file]\n"
"       %s [options] [file] [[+]offset]\n"
"\n"
"options:\n"
"       -b bits    Set number size in bits: 8, 16, 32, 64 [default: auto].\n"
"       -B bytes   Set number size in bytes: 1, 2, 4, 8 [default: auto].\n"
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

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
