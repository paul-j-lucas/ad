/*
**      ad -- ASCII dump
**      src/options.c
**
**      Copyright (C) 2015-2019  Paul J. Lucas
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
#include "color.h"
#include "common.h"
#include "options.h"
#include "unicode.h"

// standard
#include <assert.h>
#include <ctype.h>                      /* for islower(), toupper() */
#include <fcntl.h>                      /* for O_CREAT, O_RDONLY, O_WRONLY */
#include <getopt.h>
#include <inttypes.h>                   /* for PRIu64, etc. */
#include <libgen.h>                     /* for basename() */
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for fdopen() */
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for fstat() */
#include <sys/types.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

#define GAVE_OPTION(OPT)    (opts_given[ (char8_t)(OPT) ])
#define OPT_BUF_SIZE        32          /* used for format_opt() */
#define SET_OPTION(OPT)     (opts_given[ (char8_t)(OPT) ] = (char)(OPT))

// option extern variable definitions
bool                opt_case_insensitive;
c_fmt_t             opt_c_fmt;
unsigned            opt_group_by = GROUP_BY_DEFAULT;
size_t              opt_max_bytes = SIZE_MAX;
matches_t           opt_matches;
offset_fmt_t        opt_offset_fmt = OFMT_HEX;
bool                opt_only_matching;
bool                opt_only_printing;
bool                opt_print_ascii = true;
bool                opt_reverse;
bool                opt_utf8;
char const         *opt_utf8_pad = UTF8_PAD_CHAR_DEFAULT;
bool                opt_verbose;

// other extern variable definitions
FILE               *fin;
off_t               fin_offset;
char const         *fin_path = "-";
FILE               *fout;
char const         *fout_path = "-";
char const         *me;
char               *search_buf;
endian_t            search_endian;
size_t              search_len;
uint64_t            search_number;

// local constant definitions
static struct option const LONG_OPTS[] = {
  { "bits",               required_argument,  NULL, 'b' },
  { "bytes",              required_argument,  NULL, 'B' },
  { "color",              required_argument,  NULL, 'c' },
  { "c-array",            optional_argument,  NULL, 'C' },
  { "decimal",            no_argument,        NULL, 'd' },
  { "little-endian",      required_argument,  NULL, 'e' },
  { "big-endian",         required_argument,  NULL, 'E' },
  { "group-by",           required_argument,  NULL, 'g' },
  { "help",               no_argument,        NULL, 'H' },
  { "hexadecimal",        no_argument,        NULL, 'h' },
  { "ignore-case",        no_argument,        NULL, 'i' },
  { "skip-bytes",         required_argument,  NULL, 'j' },
  { "max-lines",          required_argument,  NULL, 'L' },
  { "matching-only",      no_argument,        NULL, 'm' },
  { "max-bytes",          required_argument,  NULL, 'N' },
  { "no-ascii",           no_argument,        NULL, 'A' },
  { "no-offsets",         no_argument,        NULL, 'O' },
  { "octal",              no_argument,        NULL, 'o' },
  { "printable-only",     no_argument,        NULL, 'p' },
  { "plain",              no_argument,        NULL, 'P' },
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
static char const SHORT_OPTS[] = "Ab:B:c:C:de:E:g:hHij:L:mN:oOpPrs:S:tTu:U:vV";

// local variable definitions
static char         opts_given[ 128 ];

// local functions
PJL_WARN_UNUSED_RESULT
static char const*  get_long_opt( char );

/////////// local functions ///////////////////////////////////////////////////

/**
 * Formats an option as <code>[--%s/]-%c</code> where \c %s is the long option
 * (if any) and %c is the short option.
 *
 * @param short_opt The short option (along with its corresponding long option,
 * if any) to format.
 * @param buf The buffer to use.
 * @param buf_size The size of \a buf.
 * @return Returns \a buf.
 */
PJL_WARN_UNUSED_RESULT
static char* format_opt( char short_opt, char buf[], size_t size ) {
  char const *const long_opt = get_long_opt( short_opt );
  snprintf(
    buf, size, "%s%s%s-%c",
    *long_opt ? "--" : "", long_opt, *long_opt ? "/" : "", short_opt
  );
  return buf;
}

/**
 * Gets the corresponding name of the long option for the given short option.
 *
 * @param short_opt The short option to get the corresponding long option for.
 * @return Returns the said option or the empty string if none.
 */
PJL_WARN_UNUSED_RESULT
static char const* get_long_opt( char short_opt ) {
  for ( struct option const *long_opt = LONG_OPTS; long_opt->name; ++long_opt )
    if ( long_opt->val == short_opt )
      return long_opt->name;
  return "";
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
  assert( opts1 != NULL );
  assert( opts2 != NULL );

  unsigned gave_count = 0;
  char const *opt = opts1;
  char gave_opt1 = '\0';

  for ( unsigned i = 0; i < 2; ++i ) {
    for ( ; *opt; ++opt ) {
      if ( GAVE_OPTION( *opt ) ) {
        if ( ++gave_count > 1 ) {
          char const gave_opt2 = *opt;
          char opt1_buf[ OPT_BUF_SIZE ];
          char opt2_buf[ OPT_BUF_SIZE ];
          PMESSAGE_EXIT( EX_USAGE,
            "%s and %s are mutually exclusive\n",
            format_opt( gave_opt1, opt1_buf, sizeof opt1_buf ),
            format_opt( gave_opt2, opt2_buf, sizeof opt2_buf  )
          );
        }
        gave_opt1 = *opt;
        break;
      }
    } // for
    if ( gave_count == 0 )
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
  if ( given_size < actual_size ) {
    char opt_buf[ OPT_BUF_SIZE ];
    PMESSAGE_EXIT( EX_USAGE,
      "\"%zu\": value for %s is too small for \"%" PRIu64 "\";"
      " must be at least %zu\n",
      given_size, format_opt( opt, opt_buf, sizeof opt_buf ),
      search_number, actual_size
    );
  }
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
  assert( opts != NULL );
  assert( req_opts != NULL );
  for ( char const *opt = opts; *opt; ++opt ) {
    if ( GAVE_OPTION( *opt ) ) {
      for ( char const *req_opt = req_opts; req_opt[0] != '\0'; ++req_opt )
        if ( GAVE_OPTION( *req_opt ) )
          return;
      char opt_buf[ OPT_BUF_SIZE ];
      bool const reqs_multiple = strlen( req_opts ) > 1;
      PMESSAGE_EXIT( EX_USAGE,
        "%s requires %sthe -%s option%s to be given also\n",
        format_opt( *opt, opt_buf, sizeof opt_buf ),
        (reqs_multiple ? "one of " : ""),
        req_opts, (reqs_multiple ? "s" : "")
      );
    }
  } // for
}

#define ADD_CFMT(F) \
  BLOCK( if ( (c_fmt & CFMT_##F) != 0 ) goto dup_format; c_fmt |= CFMT_##F; )

/**
 * Parses a C array format value.
 *
 * @param s The NULL-terminated string to parse that may contain exactly zero
 * or one of each of the letters \c cilstu in any order; or NULL for none.
 * @return Returns the corresponding \c c_fmt_t
 * or prints an error message and exits if \a s is invalid.
 */
PJL_WARN_UNUSED_RESULT
static c_fmt_t parse_c_fmt( char const *s ) {
  c_fmt_t c_fmt = CFMT_DEFAULT;
  char const *fmt;
  char opt_buf[ OPT_BUF_SIZE ];

  if ( s != NULL && s[0] != '\0' ) {
    for ( fmt = s; fmt[0] != '\0'; ++fmt ) {
      switch ( fmt[0] ) {
        case 'c': ADD_CFMT( CONST );    break;
        case 'i': ADD_CFMT( INT );      break;
        case 'l': ADD_CFMT( LONG );     break;
        case 's': ADD_CFMT( STATIC );   break;
        case 't': ADD_CFMT( SIZE_T );   break;
        case 'u': ADD_CFMT( UNSIGNED ); break;
        default :
          PMESSAGE_EXIT( EX_USAGE,
            "'%c': invalid C format for %s;"
            " must be one of: [cilstu]\n",
            *fmt, format_opt( 'C', opt_buf, sizeof opt_buf )
          );
      } // switch
    } // for
    if ( (c_fmt & CFMT_SIZE_T) != 0 &&
         (c_fmt & (CFMT_INT | CFMT_LONG | CFMT_UNSIGNED)) != 0 ) {
      PMESSAGE_EXIT( EX_USAGE,
        "\"%s\": invalid C format for %s:"
        " 't' and [ilu] are mutually exclusive\n",
        s, format_opt( 'C', opt_buf, sizeof opt_buf )
      );
    }
  }
  return c_fmt;

dup_format:
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid C format for %s:"
    " '%c' specified more than once\n",
    s, format_opt( 'C', opt_buf, sizeof opt_buf ), *fmt
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
PJL_WARN_UNUSED_RESULT
static char32_t parse_codepoint( char const *s ) {
  assert( s != NULL );

  if ( s[0] != '\0' && s[1] == '\0' )   // assume single-char ASCII
    return STATIC_CAST(char32_t, s[0]);

  char const *const s0 = s;
  if ( (s[0] == 'U' || s[0] == 'u') && s[1] == '+' ) {
    // convert [uU]+NNNN to 0xNNNN so strtoull() will grok it
    char *const t = (char*)free_later( check_strdup( s ) );
    s = (char*)memcpy( t, "0x", 2 );
  }
  unsigned long long const cp_candidate = parse_ull( s );
  if ( cp_is_valid( cp_candidate ) )
    return STATIC_CAST(char32_t, cp_candidate);

  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid Unicode code-point for %s\n",
    s0, format_opt( 'U', opt_buf, sizeof opt_buf )
  );
}

/**
 * Parses a color "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c color_when_t
 * or prints an error message and exits if \a when is invalid.
 */
PJL_WARN_UNUSED_RESULT
static color_when_t parse_color_when( char const *when ) {
  struct colorize_map {
    char const   *map_when;
    color_when_t  map_colorization;
  };
  typedef struct colorize_map colorize_map_t;

  static colorize_map_t const COLORIZE_MAP[] = {
    { "always",    COLOR_ALWAYS   },
    { "auto",      COLOR_ISATTY   },    // grep compatibility
    { "isatty",    COLOR_ISATTY   },    // explicit synonym for auto
    { "never",     COLOR_NEVER    },
    { "not_file",  COLOR_NOT_FILE },    // !ISREG( stdout )
    { "not_isreg", COLOR_NOT_FILE },    // synonym for not_isfile
    { "tty",       COLOR_ISATTY   },    // synonym for isatty
    { NULL,        COLOR_NEVER    }
  };

  assert( when != NULL );
  size_t names_buf_size = 1;            // for trailing NULL

  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when != NULL; ++m ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_colorization;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = (char*)free_later( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when != NULL; ++m ) {
    if ( pnames > names_buf ) {
      strcpy( pnames, ", " );
      pnames += 2;
    }
    strcpy( pnames, m->map_when );
    pnames += strlen( m->map_when );
  } // for

  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    when, format_opt( 'c', opt_buf, sizeof opt_buf ), names_buf
  );
}

/**
 * Parses the option for \c --group-by/-g.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the group-by value
 * or prints an error message and exits if the value is invalid.
 */
PJL_WARN_UNUSED_RESULT
static unsigned parse_group_by( char const *s ) {
  unsigned long long const group_by = parse_ull( s );
  switch ( group_by ) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
      return STATIC_CAST(unsigned, group_by);
  } // switch
  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%" PRIu64 "\": invalid value for %s;"
    " must be one of: 1, 2, 4, 8, 16, or 32\n",
    group_by, format_opt( 'g', opt_buf, sizeof opt_buf )
  );
}

/**
 * Parses a UTF-8 "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c utf8_when_t
 * or prints an error message and exits if \a when is invalid.
 */
PJL_WARN_UNUSED_RESULT
static utf8_when_t parse_utf8_when( char const *when ) {
  struct utf8_map {
    char const *map_when;
    utf8_when_t map_utf8;
  };
  typedef struct utf8_map utf8_map_t;

  static utf8_map_t const UTF8_MAP[] = {
    { "always",   UTF8_ALWAYS   },
    { "auto",     UTF8_ENCODING },
    { "encoding", UTF8_ENCODING },      // explicit synonym for auto
    { "never",    UTF8_NEVER    },
    { NULL,       UTF8_NEVER    }
  };

  assert( when != NULL );
  size_t names_buf_size = 1;            // for trailing NULL

  for ( utf8_map_t const *m = UTF8_MAP; m->map_when != NULL; ++m ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_utf8;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = (char*)free_later( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  for ( utf8_map_t const *m = UTF8_MAP; m->map_when != NULL; ++m ) {
    if ( pnames > names_buf ) {
      strcpy( pnames, ", " );
      pnames += 2;
    }
    strcpy( pnames, m->map_when );
    pnames += strlen( m->map_when );
  } // for

  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    when, format_opt( 'u', opt_buf, sizeof opt_buf ), names_buf
  );
}

/**
 * Prints the usage message to standard error and exits.
 */
PJL_NORETURN
static void usage( void ) {
  printf(
"usage: " PACKAGE " [options] [+offset] [infile [outfile]]\n"
"       " PACKAGE " -r [-dho] [infile [outfile]]\n"
"       " PACKAGE " -H\n"
"       " PACKAGE " -V\n"
"options:\n"
"  --big-endian=NUM         (-E)  Search for big-endian number.\n"
"  --bits=NUM               (-b)  Number size in bits: 8-64 [default: auto].\n"
"  --bytes=NUM              (-B)  Number size in bytes: 1-8 [default: auto].\n"
"  --c-array=FMT            (-C)  Dump bytes as a C array.\n"
"  --color=WHEN             (-c)  When to colorize output [default: not_file].\n"
"  --decimal                (-d)  Print offsets in decimal.\n"
"  --group-by=NUM           (-g)  Group bytes by 1/2/4/8/16/32 [default: %u].\n"
"  --help                   (-H)  Print this help and exit.\n"
"  --hexadecimal            (-h)  Print offsets in hexadecimal [default].\n"
"  --ignore-case            (-i)  Ignore case for string searches.\n"
"  --little-endian=NUM      (-e)  Search for little-endian number.\n"
"  --matching-only          (-m)  Only dump rows having matches.\n"
"  --max-bytes=NUM          (-N)  Dump max number of bytes [default: unlimited].\n"
"  --max-lines=NUM          (-L)  Dump max number of lines [default: unlimited].\n"
"  --no-ascii               (-A)  Suppress printing the ASCII part.\n"
"  --no-offsets             (-O)  Suppress printing offsets.\n"
"  --octal                  (-o)  Print offsets in octal.\n"
"  --plain                  (-P)  Dump in plain format; same as: -AOg32.\n"
"  --printing-only          (-p)  Only dump rows having printable characters.\n"
"  --reverse                (-r)  Reverse from dump back to binary.\n"
"  --skip-bytes=NUM         (-j)  Jump to offset before dumping [default: 0].\n"
"  --string=STR             (-s)  Search for string.\n"
"  --string-ignore-case=STR (-S)  Search for case-insensitive string.\n"
"  --total-matches          (-t)  Additionally print total number of matches.\n"
"  --total-matches-only     (-T)  Only print total number of matches.\n"
"  --utf8-padding=NUM       (-U)  Set UTF-8 padding character [default: U+2581].\n"
"  --utf8=WHEN              (-u)  When to dump in UTF-8 [default: never].\n"
"  --verbose                (-v)  Dump repeated rows also.\n"
"  --version                (-V)  Print version and exit.\n"
    , GROUP_BY_DEFAULT
  );
  exit( EX_USAGE );
}

////////// extern functions ///////////////////////////////////////////////////

char const* get_offset_fmt_english( void ) {
  switch ( opt_offset_fmt ) {
    case OFMT_NONE: return "none";
    case OFMT_DEC : return "decimal";
    case OFMT_HEX : return "hexadecimal";
    case OFMT_OCT : return "octal";
  } // switch
  assert( false );
  return NULL;                          // suppress warning (never gets here)
}

char const* get_offset_fmt_format( void ) {
  static char fmt[8];                   // e.g.: "%016llX"
  if ( fmt[0] == '\0' ) {
    switch ( opt_offset_fmt ) {
      case OFMT_NONE:
        strcpy( fmt, "%00" );
        break;
      case OFMT_DEC:
        sprintf( fmt, "%%0%zu" PRIu64, get_offset_width() );
        break;
      case OFMT_HEX:
        sprintf( fmt, "%%0%zu" PRIX64, get_offset_width() );
        break;
      case OFMT_OCT:
        sprintf( fmt, "%%0%zu" PRIo64, get_offset_width() );
        break;
      default:
        assert( false );
    } // switch
  }
  return fmt;
}

size_t get_offset_width( void ) {
  return  (opt_group_by == 1 && opt_print_ascii) ||
          (row_bytes > ROW_BYTES_DEFAULT && !opt_print_ascii) ?
      OFFSET_WIDTH_MIN : OFFSET_WIDTH_MAX;
}

void parse_options( int argc, char *argv[] ) {
  color_when_t  color_when = COLOR_WHEN_DEFAULT;
  size_t        max_lines = 0;
  bool          print_version = false;
  size_t        size_in_bits = 0, size_in_bytes = 0;
  char32_t      utf8_pad = 0;
  utf8_when_t   utf8_when = UTF8_WHEN_DEFAULT;

  me = basename( argv[0] );
  opterr = 1;

  for ( ;; ) {
    int const opt = getopt_long( argc, argv, SHORT_OPTS, LONG_OPTS, NULL );
    if ( opt == -1 )
      break;
    SET_OPTION( opt );
    switch ( opt ) {
      case 'A': opt_print_ascii = false;                                break;
      case 'b': size_in_bits = parse_ull( optarg );                     break;
      case 'B': size_in_bytes = parse_ull( optarg );                    break;
      case 'c': color_when = parse_color_when( optarg );                break;
      case 'C': opt_c_fmt = parse_c_fmt( optarg );                      break;
      case 'd': opt_offset_fmt = OFMT_DEC;                              break;
      case 'e':
      case 'E': search_number = parse_ull( optarg );
                search_endian = opt == 'E' ? ENDIAN_BIG: ENDIAN_LITTLE; break;
      case 'g': opt_group_by = parse_group_by( optarg );                break;
      case 'h': opt_offset_fmt = OFMT_HEX;                              break;
   // case 'H': usage();                // default case handles this
      case 'S': search_buf = (char*)free_later( check_strdup( optarg ) );
                PJL_FALLTHROUGH;
      case 'i': opt_case_insensitive = true;                            break;
      case 'j': fin_offset += (off_t)parse_offset( optarg );            break;
      case 'L': max_lines = parse_ull( optarg );                        break;
      case 'm': opt_only_matching = true;                               break;
      case 'N': opt_max_bytes = parse_offset( optarg );                 break;
      case 'o': opt_offset_fmt = OFMT_OCT;                              break;
      case 'O': opt_offset_fmt = OFMT_NONE;                             break;
      case 'p': opt_only_printing = true;                               break;
      case 'P': opt_group_by = ROW_BYTES_MAX;
                opt_offset_fmt = OFMT_NONE;
                opt_print_ascii = false;
                break;
      case 'r': opt_reverse = true;                                     break;
      case 's': search_buf = (char*)free_later( check_strdup( optarg ) );
                                                                        break;
      case 't': opt_matches = MATCHES_ALSO_PRINT;                       break;
      case 'T': opt_matches = MATCHES_ONLY_PRINT;                       break;
      case 'u': utf8_when = parse_utf8_when( optarg );                  break;
      case 'U': utf8_pad = parse_codepoint( optarg );                   break;
      case 'v': opt_verbose = true;                                     break;
      case 'V': print_version = true;                                   break;
      default : usage();
    } // switch
  } // for
  argc -= optind;
  argv += optind - 1;

  // handle special case of +offset option
  if ( argc && *argv[1] == '+' ) {
    fin_offset += (off_t)parse_offset( argv[1] );
    --argc;
    ++argv;
  }

  // check for mutually exclusive options
  check_mutually_exclusive( "b", "B" );
  check_mutually_exclusive( "C", "ceEgimpsStTuUv" );
  check_mutually_exclusive( "d", "hoOP" );
  check_mutually_exclusive( "eE", "sS" );
  check_mutually_exclusive( "g", "P" );
  check_mutually_exclusive( "L", "N" );
  check_mutually_exclusive( "mp", "v" );
  check_mutually_exclusive( "r", "AbBcCeEgimLNOpPsStTuUv" );
  check_mutually_exclusive( "t", "T" );
  check_mutually_exclusive( "V", "AbBcCdeEghHijmLNoOpPrsStTuUv" );

  // check for options that require other options
  check_required( "bB", "eE" );
  check_required( "i", "s" );
  check_required( "mtT", "eEsS" );
  check_required( "U", "u" );

  if ( print_version ) {
    PRINT_ERR( "%s\n", PACKAGE_STRING );
    exit( EX_OK );
  }

  char opt_buf[ OPT_BUF_SIZE ];

  if ( GAVE_OPTION( 'b' ) ) {
    if ( size_in_bits % 8 != 0 || size_in_bits > 64 )
      PMESSAGE_EXIT( EX_USAGE,
        "\"%zu\": invalid value for %s;"
        " must be a multiple of 8 in 8-64\n",
        size_in_bits, format_opt( 'b', opt_buf, sizeof opt_buf )
      );
    search_len = size_in_bits * 8;
    check_number_size( size_in_bits, int_len( search_number ) * 8, 'b' );
  }

  if ( GAVE_OPTION( 'B' ) ) {
    if ( size_in_bytes > 8 )
      PMESSAGE_EXIT( EX_USAGE,
        "\"%zu\": invalid value for %s; must be in 1-8\n",
        size_in_bytes, format_opt( 'B', opt_buf, sizeof opt_buf )
      );
    search_len = size_in_bytes;
    check_number_size( size_in_bytes, int_len( search_number ), 'B' );
  }

  if ( opt_case_insensitive )
    tolower_s( search_buf );

  if ( opt_group_by > row_bytes )
    row_bytes = opt_group_by;

  if ( max_lines > 0 )
    opt_max_bytes = max_lines * row_bytes;

  fin  = stdin;
  fout = stdout;

  switch ( argc ) {
    case 2:                             // infile & outfile
      fout_path = argv[2];
      if ( strcmp( fout_path, "-" ) != 0 ) {
        //
        // We can't use fopen(3) because there's no mode that specifies opening
        // a file for writing and NOT truncating it to zero length if it
        // exists.
        //
        // Hence we have to use open(2) first so we can specify only O_WRONLY
        // and O_CREAT but not O_TRUNC, then use fdopen(3) to wrap a FILE
        // around it.
        //
        fout = fdopen( check_open( fout_path, O_WRONLY | O_CREAT, 0 ), "w" );
      }
      PJL_FALLTHROUGH;

    case 1:                             // infile only
      fin_path = argv[1];
      PJL_FALLTHROUGH;

    case 0:
      if ( strcmp( fin_path, "-" ) == 0 )
        fskip( (size_t)fin_offset, stdin );
      else
        fin = check_fopen( fin_path, "r", fin_offset );
      break;

    default:
      usage();
  } // switch

  colorize = should_colorize( color_when );
  if ( colorize ) {
    if ( !(parse_grep_colors( getenv( "AD_COLORS"   ) )
        || parse_grep_colors( getenv( "GREP_COLORS" ) )
        || parse_grep_color ( getenv( "GREP_COLOR"  ) )) ) {
      PJL_IGNORE_RV( parse_grep_colors( COLORS_DEFAULT ) );
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
