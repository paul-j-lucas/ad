/*
**      ad -- ASCII dump
**      options.c
**
**      Copyright (C) 2015-2016  Paul J. Lucas
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
#include "utf8.h"

// standard
#include <assert.h>
#include <ctype.h>                      /* for islower(), toupper() */
#include <fcntl.h>                      /* for O_CREAT, O_RDONLY, O_WRONLY */
#include <getopt.h>
#include <inttypes.h>                   /* for PRIu64, etc. */
#include <libgen.h>                     /* for basename() */
#include <stdio.h>                      /* for fdopen() */
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for fstat() */
#include <sys/types.h>

///////////////////////////////////////////////////////////////////////////////

#define GAVE_OPTION(OPT)  isalpha( OPTION_VALUE(OPT) )
#define OPTION_VALUE(OPT) opts_given[ !islower(OPT) ][ toupper(OPT) - 'A' ]
#define SET_OPTION(OPT)   OPTION_VALUE(OPT) = (OPT)

// option extern variable definitions
bool              opt_case_insensitive;
c_fmt_t           opt_c_fmt;
size_t            opt_max_bytes_to_read = SIZE_MAX;
matches_t         opt_matches;
offset_fmt_t      opt_offset_fmt = OFMT_HEX;
bool              opt_only_matching;
bool              opt_only_printing;
bool              opt_reverse;
bool              opt_utf8;
char const       *opt_utf8_pad = UTF8_PAD_CHAR_DEFAULT;
bool              opt_verbose;

// other extern variable definitions
FILE             *fin;
off_t             fin_offset;
char const       *fin_path = "<stdin>";
FILE             *fout;
char const       *fout_path = "<stdout>";
char const       *me;
char             *search_buf;
endian_t          search_endian;
size_t            search_len;
uint64_t          search_number;

// local constant definitions
static struct option const LONG_OPTS[] = {
  { "bits",               required_argument,  NULL, 'b' },
  { "bytes",              required_argument,  NULL, 'B' },
  { "color",              required_argument,  NULL, 'c' },
  { "c-array",            optional_argument,  NULL, 'C' },
  { "decimal",            no_argument,        NULL, 'd' },
  { "little-endian",      required_argument,  NULL, 'e' },
  { "big-endian",         required_argument,  NULL, 'E' },
  { "help",               no_argument,        NULL, 'H' },
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
  { "total-matches",      no_argument,        NULL, 't' },
  { "total-matches-only", no_argument,        NULL, 'T' },
  { "utf8",               required_argument,  NULL, 'u' },
  { "utf8-padding",       required_argument,  NULL, 'U' },
  { "verbose",            no_argument,        NULL, 'v' },
  { "version",            no_argument,        NULL, 'V' },
  { NULL,                 0,                  NULL, 0   }
};
static char const SHORT_OPTS[] = "b:B:c:C:de:E:hHij:mN:oprs:S:tTu:U:vV";

// local variable definitions
static char       opts_given[ 2 /* lower/upper */ ][ 26 + 1 /*NULL*/ ];

/////////// local functions ///////////////////////////////////////////////////

/**
 * Gets the corresponding name of the long option for the given short option.
 *
 * @param short_opt The short option to get the corresponding long option for.
 * @return Returns the said option.
 */
static char const* get_long_opt( char short_opt ) {
  for ( struct option const *long_opt = LONG_OPTS; long_opt->name; ++long_opt )
    if ( long_opt->val == short_opt )
      return long_opt->name;
  assert( false );
}

/**
 * Checks that no options were given that are among the two given mutually
 * exclusive sets of short options.
 * Prints an error message and exits if any such options are found.
 *
 * @param opts1 The first set of short options.
 * @param opts2 The second set of short options.
 */
static void check_mutually_exclusive( char const *opts1, char const *opts2 ) {
  assert( opts1 );
  assert( opts2 );

  int gave_count = 0;
  char const *opt = opts1;
  char gave_opt1 = '\0';

  for ( int i = 0; i < 2; ++i ) {
    for ( ; *opt; ++opt ) {
      if ( GAVE_OPTION( *opt ) ) {
        if ( ++gave_count > 1 ) {
          char const gave_opt2 = *opt;
          PMESSAGE_EXIT( EX_USAGE,
            "--%s/-%c and --%s/-%c are mutually exclusive\n",
            get_long_opt( gave_opt1 ), gave_opt1,
            get_long_opt( gave_opt2 ), gave_opt2
          );
        }
        gave_opt1 = *opt;
        break;
      }
    } // for
    if ( !gave_count )
      break;
    opt = opts2;
  } // for
}

/**
 * Checks that the number of bits or bytes given for \c search_number are
 * sufficient to contain it.
 * Prints an error message and exits if \a given_size &lt; \a actual_size.
 *
 * @param given_size The given size in bits or bytes.
 * @param actual_size The actual size of \c search_number in bits or bytes.
 */
static void check_number_size( size_t given_size, size_t actual_size,
                               char opt ) {
  if ( given_size < actual_size )
    PMESSAGE_EXIT( EX_USAGE,
      "\"%zu\": value for --%s/-%c is too small for \"%" PRIu64 "\";"
      " must be at least %zu\n",
      given_size, get_long_opt( opt ), opt, search_number, actual_size
    );
}

/**
 * For each option in \a opts that was given, checks that at least one of
 * \a req_opts was also given.
 * If not, prints an error message and exits.
 *
 * @param opts The set of short options.
 * @param req_opts The set of required options for \a opts.
 */
static void check_required( char const *opts, char const *req_opts ) {
  assert( opts );
  assert( req_opts );
  for ( char const *opt = opts; *opt; ++opt ) {
    if ( GAVE_OPTION( *opt ) ) {
      for ( char const *req_opt = req_opts; *req_opt; ++req_opt )
        if ( GAVE_OPTION( *req_opt ) )
          return;
      bool const reqs_multiple = strlen( req_opts ) > 1;
      PMESSAGE_EXIT( EX_USAGE,
        "--%s/-%c requires %sthe -%s option%s to be given also\n",
        get_long_opt( *opt ), *opt,
        (reqs_multiple ? "one of " : ""),
        req_opts, (reqs_multiple ? "s" : "")
      );
    }
  } // for
}

#define ADD_CFMT(F) \
  BLOCK( if ( c_fmt & CFMT_##F ) goto dup_format; c_fmt |= CFMT_##F; )

/**
 * Parses a C array format value.
 *
 * @param s The NULL-terminated string to parse that may contain exactly zero
 * or one of each of the letters \c cilstu in any order; or NULL for none.
 * @return Returns the corresponding \c c_fmt_t
 * or prints an error message and exits if \a s is invalid.
 */
static c_fmt_t parse_c_fmt( char const *s ) {
  c_fmt_t c_fmt = CFMT_DEFAULT;
  char const *fmt;

  if ( s && *s ) {
    for ( fmt = s; *fmt; ++fmt ) {
      switch ( *fmt ) {
        case 'c': ADD_CFMT( CONST );    break;
        case 'i': ADD_CFMT( INT );      break;
        case 'l': ADD_CFMT( LONG );     break;
        case 's': ADD_CFMT( STATIC );   break;
        case 't': ADD_CFMT( SIZE_T );   break;
        case 'u': ADD_CFMT( UNSIGNED ); break;
        default :
          PMESSAGE_EXIT( EX_USAGE,
            "'%c': invalid C format for --%s/-%c;"
            " must be one of: [cilstu]\n",
            *fmt, get_long_opt( 'C' ), 'C'
          );
      } // switch
    } // for
    if ( (c_fmt & CFMT_SIZE_T) &&
        (c_fmt & (CFMT_INT | CFMT_LONG | CFMT_UNSIGNED)) ) {
      PMESSAGE_EXIT( EX_USAGE,
        "\"%s\": invalid C format for --%s/-%c:"
        " 't' and [ilu] are mutually exclusive\n",
        s, get_long_opt( 'C' ), 'C'
      );
    }
  }
  return c_fmt;

dup_format:
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid C format for --%s/-%c:"
    " '%c' specified more than once\n",
    s, get_long_opt( 'C' ), 'C', *fmt
  );
}

#undef ADD_CFMT

/**
 * Parses a Unicode code-point value.
 *
 * @param s The NULL-terminated string to parse.  Allows for strings of the
 * form:
 *  + X: a single character.
 *  + NN: two-or-more decimal digits.
 *  + 0xN, u+N, or U+N: one-or-more hexadecimal digits.
 * @return Returns the Unicode code-point value
 * or prints an error message and exits if \a s is invalid.
 */
static uint32_t parse_codepoint( char const *s ) {
  assert( s );

  if ( s[0] && !s[1] )                  // assume single-char ASCII
    return (uint32_t)s[0];

  char const *const s0 = s;
  if ( (s[0] == 'U' || s[0] == 'u') && s[1] == '+' ) {
    // convert [uU]+NNNN to 0xNNNN so strtoull() will grok it
    char *const t = (char*)freelist_add( check_strdup( s ) );
    s = (char*)memcpy( t, "0x", 2 );
  }
  uint64_t const codepoint = parse_ull( s );
  if ( codepoint_is_valid( codepoint ) )
    return (uint32_t)codepoint;
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid Unicode code-point for --%s/-%c\n",
    s0, get_long_opt( 'U' ), 'U'
  );
}

/**
 * Parses a color "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c color_when_t
 * or prints an error message and exits if \a when is invalid.
 */
static color_when_t parse_color_when( char const *when ) {
  struct colorize_map {
    char const *map_when;
    color_when_t map_colorization;
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

  assert( when );
  char const *const when_lc =
    tolower_s( (char*)freelist_add( check_strdup( when ) ) );
  size_t names_buf_size = 1;            // for trailing NULL

  for ( colorize_map_t const *m = colorize_map; m->map_when; ++m ) {
    if ( strcmp( when_lc, m->map_when ) == 0 )
      return m->map_colorization;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = (char*)freelist_add( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  for ( colorize_map_t const *m = colorize_map; m->map_when; ++m ) {
    if ( pnames > names_buf ) {
      strcpy( pnames, ", " );
      pnames += 2;
    }
    strcpy( pnames, m->map_when );
    pnames += strlen( m->map_when );
  } // for
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for --%s/-%c; must be one of:\n\t%s\n",
    when, get_long_opt( 'c' ), 'c', names_buf
  );
}

/**
 * Parses a UTF-8 "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c utf8_when_t
 * or prints an error message and exits if \a when is invalid.
 */
static utf8_when_t parse_utf8_when( char const *when ) {
  struct utf8_map {
    char const *map_when;
    utf8_when_t map_utf8;
  };
  typedef struct utf8_map utf8_map_t;

  static utf8_map_t const utf8_map[] = {
    { "always",   UTF8_ALWAYS   },
    { "auto",     UTF8_ENCODING },
    { "encoding", UTF8_ENCODING },      // explicit synonym for auto
    { "never",    UTF8_NEVER    },
    { NULL,       UTF8_NEVER    }
  };

  assert( when );
  char const *const when_lc =
    tolower_s( (char*)freelist_add( check_strdup( when ) ) );
  size_t names_buf_size = 1;            // for trailing NULL

  for ( utf8_map_t const *m = utf8_map; m->map_when; ++m ) {
    if ( strcmp( when_lc, m->map_when ) == 0 )
      return m->map_utf8;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = (char*)freelist_add( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  for ( utf8_map_t const *m = utf8_map; m->map_when; ++m ) {
    if ( pnames > names_buf ) {
      strcpy( pnames, ", " );
      pnames += 2;
    }
    strcpy( pnames, m->map_when );
    pnames += strlen( m->map_when );
  } // for
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for --%s/-%c; must be one of:\n\t%s\n",
    when, get_long_opt( 'u' ), 'u', names_buf
  );
}

/**
 * Prints the usage message to standard error and exits.
 */
static void usage( void ) {
  PRINT_ERR(
"usage: %s [options] [+offset] [infile [outfile]]\n"
"       %s -r [-dho] [infile [outfile]]\n"
"       %s -H\n"
"       %s -V\n"
"\n"
"options:\n"
"       -b bits    Set number size in bits: 8-64 [default: auto].\n"
"       -B bytes   Set number size in bytes: 1-8 [default: auto].\n"
"       -c when    Specify when to colorize output [default: not_file].\n"
"       -C format  Dump bytes as a C array [default: no].\n"
"       -d         Print offsets in decimal.\n"
"       -e number  Search for little-endian number.\n"
"       -E number  Search for big-endian number.\n"
"       -h         Print offsets in hexadecimal [default].\n"
"       -H         Print this help and exit [default: no].\n"
"       -i         Search for case-insensitive string [default: no].\n"
"       -j offset  Jump to offset before dumping [default: 0].\n"
"       -m         Only dump rows having matches [default: no].\n"
"       -N bytes   Dump max number of bytes [default: unlimited].\n"
"       -o         Print offsets in octal.\n"
"       -p         Only dump rows having printable characters [default: no].\n"
"       -r         Reverse from dump back to binary [default: no].\n"
"       -s string  Search for string.\n"
"       -S string  Search for case-insensitive string.\n"
"       -t         Additionally print total number of matches [default: no].\n"
"       -T         Only print total number of matches [default: no].\n"
"       -u when    Specify when to dump in UTF-8 [default: never].\n"
"       -U number  Set UTF-8 padding character [default: U+2581].\n"
"       -v         Dump all data including repeated rows [default: no].\n"
"       -V         Print version and exit.\n"
    , me, me, me, me
  );
  exit( EX_USAGE );
}

////////// extern functions ///////////////////////////////////////////////////

char const* get_offset_fmt_english( void ) {
  switch ( opt_offset_fmt ) {
    case OFMT_DEC: return "decimal";
    case OFMT_HEX: return "hexadecimal";
    case OFMT_OCT: return "octal";
  } // switch
  assert( false );
}

char const* get_offset_fmt_format( void ) {
  switch ( opt_offset_fmt ) {
    case OFMT_DEC: return "%0" STRINGIFY(OFFSET_WIDTH) PRIu64;
    case OFMT_HEX: return "%0" STRINGIFY(OFFSET_WIDTH) PRIX64;
    case OFMT_OCT: return "%0" STRINGIFY(OFFSET_WIDTH) PRIo64;
  } // switch
  assert( false );
}

void parse_options( int argc, char *argv[] ) {
  color_when_t  color_when = COLOR_WHEN_DEFAULT;
  bool          print_version = false;
  size_t        size_in_bits = 0, size_in_bytes = 0;
  uint32_t      utf8_pad = 0;
  utf8_when_t   utf8_when = UTF8_WHEN_DEFAULT;

  // just so it's pretty-printable when debugging
  memset( opts_given, '.', sizeof( opts_given ) );
  opts_given[0][26] = opts_given[1][26] = '\0';

  me = basename( argv[0] );
  opterr = 1;

  for ( ;; ) {
    int const opt = getopt_long( argc, argv, SHORT_OPTS, LONG_OPTS, NULL );
    if ( opt == -1 )
      break;
    SET_OPTION( opt );
    switch ( opt ) {
      case 'b': size_in_bits = parse_ull( optarg );                     break;
      case 'B': size_in_bytes = parse_ull( optarg );                    break;
      case 'c': color_when = parse_color_when( optarg );                break;
      case 'C': opt_c_fmt = parse_c_fmt( optarg );                      break;
      case 'd': opt_offset_fmt = OFMT_DEC;                              break;
      case 'e':
      case 'E': search_number = parse_ull( optarg );
                search_endian = opt == 'E' ? ENDIAN_BIG: ENDIAN_LITTLE; break;
      case 'h': opt_offset_fmt = OFMT_HEX;                              break;
   // case 'H': usage();                // default case handles this
      case 'S': search_buf = (char*)freelist_add( check_strdup( optarg ) );
      case 'i': opt_case_insensitive = true;                            break;
      case 'j': fin_offset += parse_offset( optarg );                   break;
      case 'm': opt_only_matching = true;                               break;
      case 'N': opt_max_bytes_to_read = parse_offset( optarg );         break;
      case 'o': opt_offset_fmt = OFMT_OCT;                              break;
      case 'p': opt_only_printing = true;                               break;
      case 'r': opt_reverse = true;                                     break;
      case 's': search_buf = (char*)freelist_add( check_strdup( optarg ) );
                                                                        break;
      case 't': opt_matches = MATCHES_PRINT;                            break;
      case 'T': opt_matches = MATCHES_ONLY;                             break;
      case 'u': utf8_when = parse_utf8_when( optarg );                  break;
      case 'U': utf8_pad = parse_codepoint( optarg );                   break;
      case 'v': opt_verbose = true;                                     break;
      case 'V': print_version = true;                                   break;
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
  check_mutually_exclusive( "C", "ceEimpsStTuUv" );
  check_mutually_exclusive( "eE", "sS" );
  check_mutually_exclusive( "mp", "v" );
  check_mutually_exclusive( "r", "bBcCeEimNpsStTuUv" );
  check_mutually_exclusive( "t", "T" );
  check_mutually_exclusive( "V", "bBcCdeEhHijmNoprsStTuUv" );

  // check for options that require other options
  check_required( "bB", "eE" );
  check_required( "i", "s" );
  check_required( "mtT", "eEsS" );
  check_required( "U", "u" );

  if ( print_version ) {
    PRINT_ERR( "%s\n", PACKAGE_STRING );
    exit( EX_OK );
  }

  if ( GAVE_OPTION( 'b' ) ) {
    if ( size_in_bits % 8 != 0 || size_in_bits > 64 )
      PMESSAGE_EXIT( EX_USAGE,
        "\"%zu\": invalid value for --%s/-%c;"
        " must be a multiple of 8 in 8-64\n",
        size_in_bits, get_long_opt( 'b' ), 'b'
      );
    search_len = size_in_bits * 8;
    check_number_size( size_in_bits, int_len( search_number ) * 8, 'b' );
  }

  if ( GAVE_OPTION( 'B' ) ) {
    if ( size_in_bytes > 8 )
      PMESSAGE_EXIT( EX_USAGE,
        "\"%zu\": invalid value for --%s/-%c; must be in 1-8\n",
        size_in_bytes, get_long_opt( 'B' ), 'B'
      );
    search_len = size_in_bytes;
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

  colorize = should_colorize( color_when );
  if ( colorize ) {
    if ( !(parse_grep_colors( getenv( "AD_COLORS"   ) )
        || parse_grep_colors( getenv( "GREP_COLORS" ) )
        || parse_grep_color ( getenv( "GREP_COLOR"  ) )) ) {
      parse_grep_colors( COLORS_DEFAULT );
    }
  }

  opt_utf8 = should_utf8( utf8_when );
  if ( utf8_pad ) {
    static char utf8_pad_buf[ UTF8_LEN_MAX + 1 /*NULL*/ ];
    utf8_encode( utf8_pad, utf8_pad_buf );
    opt_utf8_pad = utf8_pad_buf;
  }
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
