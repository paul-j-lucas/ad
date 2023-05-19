/*
**      ad -- ASCII dump
**      src/options.c
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
**      along with this program.  If not, see <http
*/

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#include "options.h"
#include "unicode.h"

// standard
#include <assert.h>
#include <ctype.h>                      /* for islower(), toupper() */
#include <fcntl.h>                      /* for O_CREAT, O_RDONLY, O_WRONLY */
#include <inttypes.h>                   /* for PRIu64, etc. */
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for fdopen() */
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for fstat() */
#include <sys/types.h>
#include <sysexits.h>

// in ascending option character ASCII order; sort using: sort -bdfk3
#define OPT_NO_ASCII            A
#define OPT_BITS                b
#define OPT_BYTES               B
#define OPT_COLOR               c
#define OPT_C_ARRAY             C
#define OPT_DEBUG               D
#define OPT_DECIMAL             d
#define OPT_BIG_ENDIAN          E
#define OPT_LITTLE_ENDIAN       e
#define OPT_LITTLE_ENDIAN       e
#define OPT_GROUP_BY            g
#define OPT_HELP                h
#define OPT_HOST_ENDIAN         H
#define OPT_IGNORE_CASE         i
#define OPT_SKIP_BYTES          j
#define OPT_MAX_LINES           L
#define OPT_MATCHING_ONLY       m
#define OPT_MAX_BYTES           N
#define OPT_NO_OFFSETS          O
#define OPT_OCTAL               o
#define OPT_PLAIN               P
#define OPT_PRINTING_ONLY       p
#define OPT_REVERSE             r
#define OPT_STRING              s
#define OPT_STRING_IGNORE_CASE  S
#define OPT_TOTAL_MATCHES       t
#define OPT_TOTAL_MATCHES_ONLY  T
#define OPT_UTF8                u
#define OPT_UTF8_PADDING        U
#define OPT_VERBOSE             v
#define OPT_VERSION             V
#define OPT_HEXADECIMAL         x

/// Command-line option character as a character literal.
#define COPT(X)                   CHARIFY(OPT_##X)

/// Command-line option character as a single-character string literal.
#define SOPT(X)                   STRINGIFY(OPT_##X)

///////////////////////////////////////////////////////////////////////////////

#define GAVE_OPTION(OPT)    (opts_given[ STATIC_CAST( char8_t, (OPT) ) ])
#define OPT_BUF_SIZE        32          /* used for opt_format() */

// extern variable definitions
bool          opt_ad_debug;
bool          opt_case_insensitive;
c_fmt_t       opt_c_fmt;
unsigned      opt_group_by = GROUP_BY_DEFAULT;
size_t        opt_max_bytes = SIZE_MAX;
matches_t     opt_matches;
offset_fmt_t  opt_offset_fmt = OFMT_HEX;
bool          opt_only_matching;
bool          opt_only_printing;
bool          opt_print_ascii = true;
bool          opt_reverse;
bool          opt_utf8;
char const   *opt_utf8_pad = UTF8_PAD_CHAR_DEFAULT;
bool          opt_verbose;

/**
 * Command-line options.
 */
static struct option const OPTIONS[] = {
  { "bits",               required_argument,  NULL, COPT(BITS)                },
  { "bytes",              required_argument,  NULL, COPT(BYTES)               },
  { "color",              required_argument,  NULL, COPT(COLOR)               },
  { "c-array",            optional_argument,  NULL, COPT(C_ARRAY)             },
#ifdef ENABLE_AD_DEBUG
  { "--debug",            no_argument,        NULL, COPT(DEBUG)               },
#endif /* ENABLE_AD_DEBUG */
  { "decimal",            no_argument,        NULL, COPT(DECIMAL)             },
  { "little-endian",      required_argument,  NULL, COPT(LITTLE_ENDIAN)       },
  { "big-endian",         required_argument,  NULL, COPT(BIG_ENDIAN)          },
  { "group-by",           required_argument,  NULL, COPT(GROUP_BY)            },
  { "help",               no_argument,        NULL, COPT(HELP)                },
  { "hexadecimal",        no_argument,        NULL, COPT(HEXADECIMAL)         },
  { "ignore-case",        no_argument,        NULL, COPT(IGNORE_CASE)         },
  { "skip-bytes",         required_argument,  NULL, COPT(SKIP_BYTES)          },
  { "max-lines",          required_argument,  NULL, COPT(MAX_LINES)           },
  { "matching-only",      no_argument,        NULL, COPT(MATCHING_ONLY)       },
  { "max-bytes",          required_argument,  NULL, COPT(MAX_BYTES)           },
  { "no-ascii",           no_argument,        NULL, COPT(NO_ASCII)            },
  { "no-offsets",         no_argument,        NULL, COPT(NO_OFFSETS)          },
  { "octal",              no_argument,        NULL, COPT(OCTAL)               },
  { "printable-only",     no_argument,        NULL, COPT(PRINTING_ONLY)       },
  { "plain",              no_argument,        NULL, COPT(PLAIN)               },
  { "reverse",            no_argument,        NULL, COPT(REVERSE)             },
  { "revert",             no_argument,        NULL, COPT(REVERSE)             },
  { "string",             required_argument,  NULL, COPT(STRING)              },
  { "string-ignore-case", required_argument,  NULL, COPT(STRING_IGNORE_CASE)  },
  { "total-matches",      no_argument,        NULL, COPT(TOTAL_MATCHES)       },
  { "total-matches-only", no_argument,        NULL, COPT(TOTAL_MATCHES_ONLY)  },
  { "utf8",               required_argument,  NULL, COPT(UTF8)                },
  { "utf8-padding",       required_argument,  NULL, COPT(UTF8_PADDING)        },
  { "verbose",            no_argument,        NULL, COPT(VERBOSE)             },
  { "version",            no_argument,        NULL, COPT(VERSION)             },
  { NULL,                 0,                  NULL, 0                         }
};

// local variable definitions
static bool         opts_given[ 128 ];

// local functions
NODISCARD
static char const*  opt_format( char, char[const], size_t ),
                 *  opt_get_long( char );

/////////// local functions ///////////////////////////////////////////////////

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
    for ( ; *opt != '\0'; ++opt ) {
      if ( GAVE_OPTION( *opt ) ) {
        if ( ++gave_count > 1 ) {
          char const gave_opt2 = *opt;
          char opt1_buf[ OPT_BUF_SIZE ];
          char opt2_buf[ OPT_BUF_SIZE ];
          fatal_error( EX_USAGE,
            "%s and %s are mutually exclusive\n",
            opt_format( gave_opt1, opt1_buf, sizeof opt1_buf ),
            opt_format( gave_opt2, opt2_buf, sizeof opt2_buf  )
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
    fatal_error( EX_USAGE,
      "\"%zu\": value for %s is too small for \"%" PRIu64 "\";"
      " must be at least %zu\n",
      given_size, opt_format( opt, opt_buf, sizeof opt_buf ),
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
  assert( opts[0] != '\0' );
  assert( req_opts != NULL );
  assert( req_opts[0] != '\0' );

  for ( char const *opt = opts; *opt; ++opt ) {
    if ( GAVE_OPTION( *opt ) ) {
      for ( char const *req_opt = req_opts; req_opt[0] != '\0'; ++req_opt )
        if ( GAVE_OPTION( *req_opt ) )
          return;
      char opt_buf[ OPT_BUF_SIZE ];
      bool const reqs_multiple = req_opts[1] != '\0';
      fatal_error( EX_USAGE,
        "%s requires %sthe -%s option%s to be given also\n",
        opt_format( *opt, opt_buf, sizeof opt_buf ),
        (reqs_multiple ? "one of " : ""),
        req_opts, (reqs_multiple ? "s" : "")
      );
    }
  } // for
}

/**
 * Makes the `optstring` (short option) equivalent of \a opts for the third
 * argument of `getopt_long()`.
 *
 * @param opts An array of options to make the short option string from.  Its
 * last element must be all zeros.
 * @return Returns the `optstring` for the third argument of `getopt_long()`.
 * The caller is responsible for freeing it.
 */
NODISCARD
static char const* make_short_opts( struct option const opts[static const 2] ) {
  // pre-flight to calculate string length
  size_t len = 1;                       // for leading ':'
  for ( struct option const *opt = opts; opt->name != NULL; ++opt ) {
    assert( opt->has_arg >= 0 && opt->has_arg <= 2 );
    len += 1 + STATIC_CAST( unsigned, opt->has_arg );
  } // for

  char *const short_opts = MALLOC( char, len + 1/*\0*/ );
  char *s = short_opts;

  *s++ = ':';                           // return missing argument as ':'
  for ( struct option const *opt = opts; opt->name != NULL; ++opt ) {
    assert( opt->val > 0 && opt->val < 128 );
    *s++ = STATIC_CAST( char, opt->val );
    switch ( opt->has_arg ) {
      case optional_argument:
        *s++ = ':';
        FALLTHROUGH;
      case required_argument:
        *s++ = ':';
    } // switch
  } // for
  *s = '\0';

  return short_opts;
}

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
NODISCARD
static char const* opt_format( char short_opt, char buf[const], size_t size ) {
  char const *const long_opt = opt_get_long( short_opt );
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
NODISCARD
static char const* opt_get_long( char short_opt ) {
  FOREACH_CLI_OPTION( opt ) {
    if ( opt->val == short_opt )
      return opt->name;
  } // for
  return "";
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
NODISCARD
static c_fmt_t parse_c_fmt( char const *s ) {
  c_fmt_t c_fmt = CFMT_DEFAULT;
  char const *fmt;
  char opt_buf[ OPT_BUF_SIZE ];

  if ( s != NULL && s[0] != '\0' ) {
    for ( fmt = s; fmt[0] != '\0'; ++fmt ) {
      switch ( fmt[0] ) {
        case '8': ADD_CFMT( CHAR8_T );  break;
        case 'c': ADD_CFMT( CONST );    break;
        case 'i': ADD_CFMT( INT );      break;
        case 'l': ADD_CFMT( LONG );     break;
        case 's': ADD_CFMT( STATIC );   break;
        case 't': ADD_CFMT( SIZE_T );   break;
        case 'u': ADD_CFMT( UNSIGNED ); break;
        default :
          fatal_error( EX_USAGE,
            "'%c': invalid C format for %s;"
            " must be one of: [8cilstu]\n",
            *fmt, opt_format( COPT(C_ARRAY), opt_buf, sizeof opt_buf )
          );
      } // switch
    } // for
    if ( (c_fmt & CFMT_SIZE_T) != 0 &&
         (c_fmt & (CFMT_INT | CFMT_LONG | CFMT_UNSIGNED)) != 0 ) {
      fatal_error( EX_USAGE,
        "\"%s\": invalid C format for %s:"
        " COPT(TOTAL_MATCHES) and [ilu] are mutually exclusive\n",
        s, opt_format( COPT(C_ARRAY), opt_buf, sizeof opt_buf )
      );
    }
  }
  return c_fmt;

dup_format:
  fatal_error( EX_USAGE,
    "\"%s\": invalid C format for %s:"
    " '%c' specified more than once\n",
    s, opt_format( COPT(C_ARRAY), opt_buf, sizeof opt_buf ), *fmt
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
NODISCARD
static char32_t parse_codepoint( char const *s ) {
  assert( s != NULL );

  if ( s[0] != '\0' && s[1] == '\0' )   // assume single-char ASCII
    return STATIC_CAST(char32_t, s[0]);

  char const *const s0 = s;
  if ( (s[0] == 'U' || s[0] == 'u') && s[1] == '+' ) {
    // convert [uU]+NNNN to 0xNNNN so strtoull() will grok it
    char *const t = free_later( check_strdup( s ) );
    s = memcpy( t, "0x", 2 );
  }
  unsigned long long const cp_candidate = parse_ull( s );
  if ( cp_is_valid( cp_candidate ) )
    return STATIC_CAST(char32_t, cp_candidate);

  char opt_buf[ OPT_BUF_SIZE ];
  fatal_error( EX_USAGE,
    "\"%s\": invalid Unicode code-point for %s\n",
    s0, opt_format( COPT(UTF8_PADDING), opt_buf, sizeof opt_buf )
  );
}

/**
 * Parses a color "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c color_when_t
 * or prints an error message and exits if \a when is invalid.
 */
NODISCARD
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
  char *const names_buf = free_later( MALLOC( char, names_buf_size ) );
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
  fatal_error( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    when, opt_format( COPT(COLOR), opt_buf, sizeof opt_buf ), names_buf
  );
}

/**
 * Parses the option for \c --group-by/-g.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the group-by value
 * or prints an error message and exits if the value is invalid.
 */
NODISCARD
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
  fatal_error( EX_USAGE,
    "\"%llu\": invalid value for %s;"
    " must be one of: 1, 2, 4, 8, 16, or 32\n",
    group_by, opt_format( COPT(GROUP_BY), opt_buf, sizeof opt_buf )
  );
}

/**
 * Parses a UTF-8 "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c utf8_when_t
 * or prints an error message and exits if \a when is invalid.
 */
NODISCARD
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
  char *const names_buf = free_later( MALLOC( char, names_buf_size ) );
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
  fatal_error( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    when, opt_format( COPT(UTF8), opt_buf, sizeof opt_buf ), names_buf
  );
}

/**
 * Prints the usage message to standard error and exits.
 */
_Noreturn
static void usage( int status ) {
  fprintf( status == EX_OK ? stdout : stderr,
"usage: %s [options] [+offset] [infile [outfile]]\n"
"       %s -%c [-%c%c%c] [infile [outfile]]\n"
"       %s -%c\n"
"       %s -%c\n"
"options:\n"
"  --big-endian=NUM         (-%c) Search for big-endian number.\n"
"  --bits=NUM               (-%c) Number size in bits: 8-64 [default: auto].\n"
"  --bytes=NUM              (-%c) Number size in bytes: 1-8 [default: auto].\n"
"  --c-array=FMT            (-%c) Dump bytes as a C array.\n"
"  --color=WHEN             (-%c) When to colorize output [default: not_file].\n"
#ifdef ENABLE_AD_DEBUG
"  --debug                  (-%c) Print " PACKAGE_NAME " debug output.\n"
#endif /* ENABLE_AD_DEBUG */
"  --decimal                (-%c) Print offsets in decimal.\n"
"  --group-by=NUM           (-%c) Group bytes by 1/2/4/8/16/32 [default: %u].\n"
"  --help                   (-%c) Print this help and exit.\n"
"  --hexadecimal            (-%c) Print offsets in hexadecimal [default].\n"
"  --host-endian=NUM        (-%c) Search for host-endian number.\n"
"  --ignore-case            (-%c) Ignore case for string searches.\n"
"  --little-endian=NUM      (-%c) Search for little-endian number.\n"
"  --matching-only          (-%c) Only dump rows having matches.\n"
"  --max-bytes=NUM          (-%c) Dump max number of bytes [default: unlimited].\n"
"  --max-lines=NUM          (-%c) Dump max number of lines [default: unlimited].\n"
"  --no-ascii               (-%c) Suppress printing the ASCII part.\n"
"  --no-offsets             (-%c) Suppress printing offsets.\n"
"  --octal                  (-%c) Print offsets in octal.\n"
"  --plain                  (-%c) Dump in plain format; same as: -AOg32.\n"
"  --printing-only          (-%c) Only dump rows having printable characters.\n"
"  --reverse                (-%c) Reverse from dump back to binary.\n"
"  --skip-bytes=NUM         (-%c) Jump to offset before dumping [default: 0].\n"
"  --string=STR             (-%c) Search for string.\n"
"  --string-ignore-case=STR (-%c) Search for case-insensitive string.\n"
"  --total-matches          (-%c) Additionally print total number of matches.\n"
"  --total-matches-only     (-%c) Only print total number of matches.\n"
"  --utf8=WHEN              (-%c) When to dump in UTF-8 [default: never].\n"
"  --utf8-padding=NUM       (-%c) Set UTF-8 padding character [default: U+2581].\n"
"  --verbose                (-%c) Dump repeated rows also.\n"
"  --version                (-%c) Print version and exit.\n"
"\n"
"Report bugs to: " PACKAGE_BUGREPORT "\n"
PACKAGE_NAME " home page: " PACKAGE_URL "\n",
    me,
    me, COPT(REVERSE), COPT(DECIMAL), COPT(OCTAL), COPT(HEXADECIMAL),
    me, COPT(HELP),
    me, COPT(VERSION),
    COPT(BIG_ENDIAN),
    COPT(BITS),
    COPT(BYTES),
    COPT(C_ARRAY),
    COPT(COLOR),
#ifdef ENABLE_AD_DEBUG
    COPT(DEBUG),
#endif /* ENABLE_AD_DEBUG */
    COPT(DECIMAL),
    COPT(GROUP_BY), GROUP_BY_DEFAULT,
    COPT(HELP),
    COPT(HEXADECIMAL),
    COPT(HOST_ENDIAN),
    COPT(IGNORE_CASE),
    COPT(LITTLE_ENDIAN),
    COPT(MATCHING_ONLY),
    COPT(MAX_BYTES),
    COPT(MAX_LINES),
    COPT(NO_ASCII),
    COPT(NO_OFFSETS),
    COPT(OCTAL),
    COPT(PLAIN),
    COPT(PRINTING_ONLY),
    COPT(REVERSE),
    COPT(SKIP_BYTES),
    COPT(STRING),
    COPT(STRING_IGNORE_CASE),
    COPT(TOTAL_MATCHES),
    COPT(TOTAL_MATCHES_ONLY),
    COPT(UTF8),
    COPT(UTF8_PADDING),
    COPT(VERBOSE),
    COPT(VERSION)
  );
  exit( status );
}

////////// extern functions ///////////////////////////////////////////////////

struct option const* cli_option_next( struct option const *opt ) {
  return opt == NULL ? OPTIONS : (++opt)->name == NULL ? NULL : opt;
}

char const* get_offset_fmt_english( void ) {
  switch ( opt_offset_fmt ) {
    case OFMT_NONE: return "none";
    case OFMT_DEC : return "decimal";
    case OFMT_HEX : return "hexadecimal";
    case OFMT_OCT : return "octal";
  } // switch
  UNEXPECTED_INT_VALUE( opt_offset_fmt );
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
    } // switch
  }
  return fmt;
}

size_t get_offset_width( void ) {
  return  (opt_group_by == 1 && opt_print_ascii) ||
          (row_bytes > ROW_BYTES_DEFAULT && !opt_print_ascii) ?
      OFFSET_WIDTH_MIN : OFFSET_WIDTH_MAX;
}

void parse_options( int argc, char const *argv[] ) {
  color_when_t      color_when = COLOR_WHEN_DEFAULT;
  size_t            max_lines = 0;
  bool              print_usage = false;
  bool              print_version = false;
  char const *const short_opts = make_short_opts( OPTIONS );
  size_t            size_in_bits = 0, size_in_bytes = 0;
  char32_t          utf8_pad = 0;
  utf8_when_t       utf8_when = UTF8_WHEN_DEFAULT;

  opterr = 1;

  for (;;) {
    int const opt = getopt_long(
      argc, CONST_CAST( char**, argv ), short_opts, OPTIONS,
      /*longindex=*/NULL
    );
    if ( opt == -1 )
      break;
    switch ( opt ) {
      case COPT(BIG_ENDIAN):
        search_number = parse_ull( optarg );
        search_endian = ENDIAN_BIG;
        break;
      case COPT(BITS):
        size_in_bits = parse_ull( optarg );
        break;
      case COPT(BYTES):
        size_in_bytes = parse_ull( optarg );
        break;
      case COPT(C_ARRAY):
        opt_c_fmt = parse_c_fmt( optarg );
        break;
      case COPT(COLOR):
        color_when = parse_color_when( optarg );
        break;
#ifdef ENABLE_AD_DEBUG
      case COPT(DEBUG):
        opt_ad_debug = true;
        break;
#endif /* ENABLE_AD_DEBUG */
      case COPT(DECIMAL):
        opt_offset_fmt = OFMT_DEC;
        break;
      case COPT(GROUP_BY):
        opt_group_by = parse_group_by( optarg );
        break;
      case COPT(HELP):
        print_usage = true;
        break;
      case COPT(HEXADECIMAL):
        opt_offset_fmt = OFMT_HEX;
        break;
      case COPT(HOST_ENDIAN):
        search_number = parse_ull( optarg );
#ifdef WORDS_BIGENDIAN
        search_endian = ENDIAN_BIG;
#else
        search_endian = ENDIAN_LITTLE;
#endif /* WORDS_BIGENDIAN */
        break;
      case COPT(STRING_IGNORE_CASE):
        search_buf = free_later( check_strdup( optarg ) );
        FALLTHROUGH;
      case COPT(IGNORE_CASE):
        opt_case_insensitive = true;
        break;
      case COPT(LITTLE_ENDIAN):
        search_number = parse_ull( optarg );
        search_endian = ENDIAN_LITTLE;
        break;
      case COPT(MATCHING_ONLY):
        opt_only_matching = true;
        break;
      case COPT(MAX_BYTES):
        opt_max_bytes = parse_offset( optarg );
        break;
      case COPT(MAX_LINES):
        max_lines = parse_ull( optarg );
        break;
      case COPT(NO_ASCII):
        opt_print_ascii = false;
        break;
      case COPT(NO_OFFSETS):
        opt_offset_fmt = OFMT_NONE;
        break;
      case COPT(OCTAL):
        opt_offset_fmt = OFMT_OCT;
        break;
      case COPT(PLAIN):
        opt_group_by = ROW_BYTES_MAX;
        opt_offset_fmt = OFMT_NONE;
        opt_print_ascii = false;
        break;
      case COPT(PRINTING_ONLY):
        opt_only_printing = true;
        break;
      case COPT(REVERSE):
        opt_reverse = true;
        break;
      case COPT(SKIP_BYTES):
        fin_offset += STATIC_CAST( off_t, parse_offset( optarg ) );
        break;
      case COPT(STRING):
        search_buf = free_later( check_strdup( optarg ) );
        break;
      case COPT(TOTAL_MATCHES):
        opt_matches = MATCHES_ALSO_PRINT;
        break;
      case COPT(TOTAL_MATCHES_ONLY):
        opt_matches = MATCHES_ONLY_PRINT;
        break;
      case COPT(UTF8):
        utf8_when = parse_utf8_when( optarg );
        break;
      case COPT(UTF8_PADDING):
        utf8_pad = parse_codepoint( optarg );
        break;
      case COPT(VERBOSE):
        opt_verbose = true;
        break;
      case COPT(VERSION):
        print_version = true;
        break;

      case ':': {                       // option missing required argument
        char opt_buf[ OPT_BUF_SIZE ];
        fatal_error( EX_USAGE,
          "\"%s\" requires an argument\n",
          opt_format( STATIC_CAST( char, optopt ), opt_buf, sizeof opt_buf )
        );
      }

      case '?':                         // invalid option
        fatal_error( EX_USAGE,
          "%s: '%c': invalid option; use --help or -%c for help\n",
          me, STATIC_CAST( char, optopt ), COPT(HELP)
        );

      default:
        if ( isprint( opt ) )
          INTERNAL_ERROR(
            "'%c': unaccounted-for getopt_long() return value\n", opt
          );
        INTERNAL_ERROR(
          "%d: unaccounted-for getopt_long() return value\n", opt
        );
    } // switch
    opts_given[ opt ] = true;
  } // for

  FREE( short_opts );

  argc -= optind;
  argv += optind - 1;

  // handle special case of +offset option
  if ( argc > 0 && *argv[1] == '+' ) {
    fin_offset += STATIC_CAST( off_t, parse_offset( argv[1] ) );
    --argc;
    ++argv;
  }

  // check for mutually exclusive options
  check_mutually_exclusive( SOPT(BITS), SOPT(BYTES) );
  check_mutually_exclusive( SOPT(C_ARRAY),
    SOPT(BIG_ENDIAN)
    SOPT(COLOR)
    SOPT(GROUP_BY)
    SOPT(IGNORE_CASE)
    SOPT(LITTLE_ENDIAN)
    SOPT(MATCHING_ONLY)
    SOPT(PRINTING_ONLY)
    SOPT(STRING)
    SOPT(STRING_IGNORE_CASE)
    SOPT(TOTAL_MATCHES)
    SOPT(TOTAL_MATCHES_ONLY)
    SOPT(UTF8)
    SOPT(UTF8_PADDING)
    SOPT(VERBOSE)
  );
  check_mutually_exclusive( SOPT(DECIMAL),
    SOPT(HEXADECIMAL)
    SOPT(OCTAL)
  );
  check_mutually_exclusive( SOPT(DECIMAL) SOPT(HEXADECIMAL) SOPT(OCTAL),
    SOPT(NO_OFFSETS)
    SOPT(PLAIN)
  );
  check_mutually_exclusive( SOPT(GROUP_BY), SOPT(PLAIN) );
  check_mutually_exclusive( SOPT(LITTLE_ENDIAN),
    SOPT(BIG_ENDIAN)
    SOPT(HOST_ENDIAN)
  );
  check_mutually_exclusive(
    SOPT(LITTLE_ENDIAN) SOPT(BIG_ENDIAN) SOPT(HOST_ENDIAN),
    SOPT(STRING) SOPT(STRING_IGNORE_CASE)
  );
  check_mutually_exclusive( SOPT(HELP),
    SOPT(BIG_ENDIAN)
    SOPT(BITS)
    SOPT(BYTES)
    SOPT(CONFIG)
    SOPT(DECIMAL)
    SOPT(GROUP_BY)
    SOPT(HEXADECIMAL)
    SOPT(HOST_ENDIAN)
    SOPT(IGNORE_CASE)
    SOPT(LITTLE_ENDIAN)
    SOPT(MATCHING_ONLY)
    SOPT(MAX_BYTES)
    SOPT(MAX_LINES)
    SOPT(NO_ASCII)
    SOPT(NO_CONFIG)
    SOPT(NO_OFFSETS)
    SOPT(OCTAL)
    SOPT(PLAIN)
    SOPT(PRINTING_ONLY)
    SOPT(REVERSE)
    SOPT(SKIP_BYTES)
    SOPT(STRING)
    SOPT(STRING_IGNORE_CASE)
    SOPT(TOTAL_MATCHES)
    SOPT(TOTAL_MATCHES_ONLY)
    SOPT(UTF8)
    SOPT(UTF8_PADDING)
    SOPT(VERBOSE)
    SOPT(VERSION)
  );
  check_mutually_exclusive( SOPT(HEXADECIMAL), SOPT(DECIMAL) SOPT(OCTAL) );
  check_mutually_exclusive( SOPT(MATCH_BYTES), SOPT(MAX_LINES) );
  check_mutually_exclusive( SOPT(MATCHING_ONLY) SOPT(PRINTING_ONLY),
    SOPT(VERBOSE)
  );
  check_mutually_exclusive( SOPT(OCTAL), SOPT(DECIMAL) SOPT(HEXADECIMAL) );
  check_mutually_exclusive( SOPT(REVERSE),
    "AbBcCeEgimLNOpPsStTuUv"
  );
  check_mutually_exclusive( SOPT(TOTAL_MATCHES), SOPT(TOTAL_MATCHES_ONLY) );
  check_mutually_exclusive( SOPT(VERSION),
    SOPT(BIG_ENDIAN)
    SOPT(BITS)
    SOPT(BYTES)
    SOPT(CONFIG)
    SOPT(DECIMAL)
    SOPT(GROUP_BY)
    SOPT(HEXADECIMAL)
    SOPT(HOST_ENDIAN)
    SOPT(IGNORE_CASE)
    SOPT(LITTLE_ENDIAN)
    SOPT(MATCHING_ONLY)
    SOPT(MAX_BYTES)
    SOPT(MAX_LINES)
    SOPT(NO_ASCII)
    SOPT(NO_CONFIG)
    SOPT(NO_OFFSETS)
    SOPT(OCTAL)
    SOPT(PLAIN)
    SOPT(PRINTING_ONLY)
    SOPT(REVERSE)
    SOPT(SKIP_BYTES)
    SOPT(STRING)
    SOPT(STRING_IGNORE_CASE)
    SOPT(TOTAL_MATCHES)
    SOPT(TOTAL_MATCHES_ONLY)
    SOPT(UTF8)
    SOPT(UTF8_PADDING)
    SOPT(VERBOSE)
  );

  // check for options that require other options
  check_required( SOPT(BITS) SOPT(BYTES),
    SOPT(BIG_ENDIAN) SOPT(LITTLE_ENDIAN)
  );
  check_required( SOPT(IGNORE_CASE), SOPT(STRING) );
  check_required(
    SOPT(MATCHING_ONLY) SOPT(TOTAL_MATCHES) SOPT(TOTAL_MATCHES_ONLY),
    SOPT(BIG_ENDIAN)
    SOPT(LITTLE_ENDIAN)
    SOPT(STRING)
    SOPT(STRING_IGNORE_CASE)
  );
  check_required( SOPT(UTF8_PADDING), SOPT(UTF8) );

  if ( print_usage )
    usage( argc > 2 ? EX_USAGE : EX_OK );

  if ( print_version ) {
    EPRINTF( "%s\n", PACKAGE_STRING );
    exit( EX_OK );
  }

  char opt_buf[ OPT_BUF_SIZE ];

  if ( GAVE_OPTION( COPT(BITS) ) ) {
    if ( size_in_bits % 8 != 0 || size_in_bits > 64 )
      fatal_error( EX_USAGE,
        "\"%zu\": invalid value for %s;"
        " must be a multiple of 8 in 8-64\n",
        size_in_bits, opt_format( COPT(BITS), opt_buf, sizeof opt_buf )
      );
    search_len = size_in_bits * 8;
    check_number_size( size_in_bits, int_len( search_number ) * 8, COPT(BITS) );
  }

  if ( GAVE_OPTION( COPT(BYTES) ) ) {
    if ( size_in_bytes > 8 )
      fatal_error( EX_USAGE,
        "\"%zu\": invalid value for %s; must be in 1-8\n",
        size_in_bytes, opt_format( COPT(BYTES), opt_buf, sizeof opt_buf )
      );
    search_len = size_in_bytes;
    check_number_size( size_in_bytes, int_len( search_number ), COPT(BYTES) );
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
      FALLTHROUGH;

    case 1:                             // infile only
      fin_path = argv[1];
      FALLTHROUGH;

    case 0:
      if ( strcmp( fin_path, "-" ) == 0 )
        fskip( STATIC_CAST( size_t, fin_offset ), stdin );
      else
        fin = check_fopen( fin_path, "r", fin_offset );
      break;

    default:
      usage( EX_USAGE );
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
    (void)utf32_8( utf8_pad, utf8_pad_buf );
    opt_utf8_pad = utf8_pad_buf;
  }
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
