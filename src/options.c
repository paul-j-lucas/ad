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
#include <fcntl.h>                      /* for O_CREAT, O_RDONLY, O_WRONLY */
#include <ctype.h>                      /* for islower(), toupper() */
#include <getopt.h>
#include <libgen.h>                     /* for basename() */
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for fstat() */
#include <sys/types.h>

/////////////////////////////////////////////////////////////////////////////

#define GAVE_OPTION(OPT)  isalpha( OPTION_VALUE(OPT) )
#define OPTION_VALUE(OPT) opts_given[ !islower(OPT) ][ toupper(OPT) - 'A' ]
#define SET_OPTION(OPT)   OPTION_VALUE(OPT) = (OPT)

////////// extern variables ///////////////////////////////////////////////////

FILE         *fin;
off_t         fin_offset;
char const   *fin_path = "<stdin>";
FILE         *fout;
char const   *fout_path = "<stdout>";
char const   *me;

bool          opt_case_insensitive;
c_fmt_t       opt_c_fmt;
size_t        opt_max_bytes_to_read = SIZE_MAX;
offset_fmt_t  opt_offset_fmt = OFMT_HEX;
bool          opt_only_matching;
bool          opt_only_printing;
bool          opt_reverse;
bool          opt_verbose;

char         *search_buf;
endian_t      search_endian;
size_t        search_len;
uint64_t      search_number;

/////////// local variables ///////////////////////////////////////////////////

static struct option const long_opts[] = {
  { "bits",               required_argument,  NULL, 'b' },
  { "bytes",              required_argument,  NULL, 'B' },
  { "color",              required_argument,  NULL, 'c' },
  { "c-array",            optional_argument,  NULL, 'C' },
  { "decimal",            no_argument,        NULL, 'd' },
  { "little-endian",      required_argument,  NULL, 'e' },
  { "big-endian",         required_argument,  NULL, 'E' },
  { "hexadecimal",        no_argument,        NULL, 'h' },
  { "ignore-case",        no_argument,        NULL, 'i' },
  { "skip-bytes",         required_argument,  NULL, 'j' },
  { "matching-only",      no_argument,        NULL, 'm' },
  { "max-bytes",          required_argument,  NULL, 'N' },
  { "octal",              no_argument,        NULL, 'o' },
  { "printable-only",     no_argument,        NULL, 'p' },
  { "reverse",            no_argument,        NULL, 'r' },
  { "revert",             no_argument,        NULL, 'r' },
  { "string",             required_argument,  NULL, 's' },
  { "string-ignore-case", required_argument,  NULL, 'S' },
  { "verbose",            no_argument,        NULL, 'v' },
  { "version",            no_argument,        NULL, 'V' },
  { NULL,                 0,                  NULL, 0   }
};
static char const short_opts[] = "b:B:c:C:de:E:hij:mN:oprs:S:vV";

static char       opts_given[ 2 /* lower/upper */ ][ 26 + 1 /* NULL */ ];

/////////// local functions ///////////////////////////////////////////////////

static char const* get_long_opt( char short_opt ) {
  for ( struct option const *p = long_opts; p->name; ++p )
    if ( p->val == short_opt )
      return p->name;
  assert( false );
}

static void check_mutually_exclusive( char const *opts1, char const *opts2 ) {
  int gave_count = 0;
  char const *opt = opts1;

  for ( int i = 0; i < 2; ++i ) {
    for ( ; *opt; ++opt ) {
      if ( GAVE_OPTION( *opt ) ) {
        if ( ++gave_count > 1 )
          PMESSAGE_EXIT( USAGE,
            "-%s and -%s options are mutually exclusive\n", opts1, opts2
          );
        break;
      }
    } // for
    if ( !gave_count )
      break;
    opt = opts2;
  } // for
}

static void check_number_size( size_t given_size, size_t actual_size,
                               char opt ) {
  if ( given_size < actual_size )
    PMESSAGE_EXIT( USAGE,
      "\"%zu\": value for --%s/-%c option is too small for \"%llu\""
      "; must be at least %zu\n",
      given_size, get_long_opt( opt ), opt, search_number, actual_size
    );
}

static void check_required( char opt, char const *req_opts ) {
  if ( GAVE_OPTION( opt ) ) {
    for ( char const *req_opt = req_opts; *req_opt; ++req_opt )
      if ( GAVE_OPTION( *req_opt ) )
        return;
    bool const reqs_multiple = strlen( req_opts ) > 1;
    PMESSAGE_EXIT( USAGE,
      "--%s/-%c: option requires one of -%s option%s to be given also\n",
      get_long_opt( opt ), opt, req_opts, (reqs_multiple ? "s" : "")
    );
  }
}

static c_fmt_t parse_c_fmt( char const *s ) {
  c_fmt_t c_fmt = CFMT_DEFAULT;
  if ( s && *s ) {
    char const *p = s;
    for ( ; *p; ++p ) {
      switch ( *p ) {
        case 'c': c_fmt |= CFMT_CONST;    break;
        case 'i': c_fmt |= CFMT_INT;      break;
        case 'l': c_fmt |= CFMT_LONG;     break;
        case 's': c_fmt |= CFMT_STATIC;   break;
        case 't': c_fmt |= CFMT_SIZE_T;   break;
        case 'u': c_fmt |= CFMT_UNSIGNED; break;
        default :
          PMESSAGE_EXIT( USAGE,
            "'%c': invalid C format for --%s/-%c option;"
            " must be one of: [cilstu]\n",
            *p, get_long_opt( 'C' ), 'C'
          );
      } // switch
    } // for
    if ( (c_fmt & CFMT_SIZE_T) &&
        (c_fmt & (CFMT_INT | CFMT_LONG | CFMT_UNSIGNED)) ) {
      PMESSAGE_EXIT( USAGE,
        "\"%s\": invalid C format for --%s/-%c option:"
        " 't' and [ilu] are mutually exclusive\n",
        s, get_long_opt( 'C' ), 'C'
      );
    }
  }
  return c_fmt;
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
    "\"%s\": invalid value for --%s/-%c option; must be one of:\n\t%s\n",
    when, get_long_opt( 'c' ), 'c', names_buf
  );
}

////////// extern functions ///////////////////////////////////////////////////

void parse_options( int argc, char *argv[] ) {
  colorization_t  colorization = COLOR_NOT_FILE;
  size_t          size_in_bits = 0, size_in_bytes = 0;

  // just so it's pretty-printable when debugging
  memset( opts_given, '.', sizeof( opts_given ) );
  opts_given[0][26] = opts_given[1][26] = '\0';

  me = basename( argv[0] );
  opterr = 1;

  for ( ;; ) {
    int const opt = getopt_long( argc, argv, short_opts, long_opts, NULL );
    if ( opt == -1 )
      break;
    SET_OPTION( opt );
    switch ( opt ) {
      case 'b': size_in_bits = parse_ull( optarg );                     break;
      case 'B': size_in_bytes = parse_ull( optarg );                    break;
      case 'c': colorization = parse_colorization( optarg );            break;
      case 'C': opt_c_fmt = parse_c_fmt( optarg );                      break;
      case 'd': opt_offset_fmt = OFMT_DEC;                              break;
      case 'e':
      case 'E': search_number = parse_ull( optarg );
                search_endian = opt == 'E' ? ENDIAN_BIG: ENDIAN_LITTLE; break;
      case 'h': opt_offset_fmt = OFMT_HEX;                              break;
      case 'S': search_buf = freelist_add( check_strdup( optarg ) );
      case 'i': opt_case_insensitive = true;                            break;
      case 'j': fin_offset += parse_offset( optarg );                   break;
      case 'm': opt_only_matching = true;                               break;
      case 'N': opt_max_bytes_to_read = parse_offset( optarg );         break;
      case 'o': opt_offset_fmt = OFMT_OCT;                              break;
      case 'p': opt_only_printing = true;                               break;
      case 'r': opt_reverse = true;                                     break;
      case 's': search_buf = freelist_add( check_strdup( optarg ) );    break;
      case 'v': opt_verbose = true;                                     break;
      case 'V': PRINT_ERR( "%s\n", PACKAGE_STRING );          exit( EXIT_OK );
      default : usage();
    } // switch
  } // for
  argc -= optind, argv += optind - 1;

  // handle special case of +offset option
  if ( argc && *argv[1] == '+' ) {
    fin_offset += parse_offset( argv[1] );
    --argc, ++argv;
  }

  // check for mutually exclusive options
  check_mutually_exclusive( "b", "B" );
  check_mutually_exclusive( "C", "ceEimpsSv" );
  check_mutually_exclusive( "eE", "sS" );
  check_mutually_exclusive( "m", "v" );
  check_mutually_exclusive( "p", "v" );
  check_mutually_exclusive( "r", "bBeEimNpsSv" );
  check_mutually_exclusive( "V", "bBcCdeEhijmNoprsSv" );

  // check for options that require other options
  check_required( 'b', "eE" );
  check_required( 'B', "eE" );
  check_required( 'i', "s" );
  check_required( 'm', "eEsS" );

  if ( GAVE_OPTION( 'b' ) ) {
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
    check_number_size( size_in_bits, int_len( search_number ) * 8, 'b' );
  }

  if ( GAVE_OPTION( 'B' ) ) {
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
    check_number_size( size_in_bytes, int_len( search_number ), 'B' );
  }

  if ( opt_case_insensitive )
    tolower_s( search_buf );

  switch ( argc ) {
    case 0:
      fin  = stdin;
      fout = stdout;
      fskip( fin_offset, fin );
      break;

    case 1:                             // infile only
      fin_path = argv[1];
      fin  = check_fopen( fin_path, "r", fin_offset );
      fout = stdout;
      break;

    case 2:                             // infile & outfile
      fin_path  = argv[1];
      fout_path = argv[2];
      fin = check_fopen( fin_path, "r", fin_offset );
      //
      // We can't use fopen(3) because there's no mode that specifies opening a
      // file for writing and NOT truncating it to zero length if it exists.
      //
      // Hence we have to use open(2) first so we can specify only O_WRONLY and
      // O_CREAT but not O_TRUNC, then use fdopen(3) to wrap a FILE around it.
      //
      fout = fdopen( check_open( fout_path, O_WRONLY | O_CREAT, 0 ), "w" );
      break;

    default:
      usage();
  } // switch

  colorize = should_colorize( colorization );
  if ( colorize ) {
    if ( !(parse_grep_colors( getenv( "AD_COLORS"   ) )
        || parse_grep_colors( getenv( "GREP_COLORS" ) )
        || parse_grep_color ( getenv( "GREP_COLOR"  ) )) ) {
      parse_grep_colors( DEFAULT_COLORS );
    }
  }
}

void usage( void ) {
  PRINT_ERR(
"usage: %s [options] [+offset] [infile [outfile]]\n"
"       %s -r [-dho] [infile [outfile]]\n"
"       %s -V\n"
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
"       -r         Reverse from dump back to binary [default: no].\n"
"       -s string  Search for string.\n"
"       -S string  Search for case-insensitive string.\n"
"       -v         Dump all data, including repeated rows [default: no].\n"
"       -V         Print version and exit.\n"
    , me, me, me
  );
  exit( EXIT_USAGE );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
