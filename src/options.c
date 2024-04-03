/*
**      ad -- ASCII dump
**      src/options.c
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
**      along with this program.  If not, see <http
*/

/**
 * @file
 * Defines global variables and functions for **ad** options.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#include "options.h"
#include "unicode.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>                      /* for islower(), toupper() */
#include <fcntl.h>                      /* for O_CREAT, O_RDONLY, O_WRONLY */
#include <getopt.h>
#include <inttypes.h>                   /* for PRIu64, etc. */
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_H */
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for fdopen() */
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for fstat() */
#include <sys/types.h>
#include <sysexits.h>
#include <unistd.h>                     /* for close(2), STDOUT_FILENO */
#include <string.h>

// Undefine these since they clash with our command-line options.
#ifdef BIG_ENDIAN
# undef BIG_ENDIAN
#endif /* BIG_ENDIAN */
#ifdef LITTLE_ENDIAN
# undef LITTLE_ENDIAN
#endif /* LITTLE_ENDIAN */

// in ascending option character ASCII order; sort using: sort -bdfk3
#define OPT_NO_ASCII            A
#define OPT_BITS                b
#define OPT_BYTES               B
#define OPT_COLOR               c
#define OPT_C_ARRAY             C
#define OPT_DECIMAL             d
#define OPT_BIG_ENDIAN          E
#define OPT_LITTLE_ENDIAN       e
#define OPT_GROUP_BY            g
#define OPT_HELP                h
#define OPT_HOST_ENDIAN         H
#define OPT_IGNORE_CASE         i
#define OPT_SKIP_BYTES          j
#define OPT_MAX_LINES           L
#define OPT_MATCHING_ONLY       m
#define OPT_STRINGS             n
#define OPT_MAX_BYTES           N
#define OPT_NO_OFFSETS          O
#define OPT_OCTAL               o
#define OPT_PLAIN               P
#define OPT_PRINTING_ONLY       p
#define OPT_REVERSE             r
#define OPT_STRING              s
#define OPT_STRINGS_OPTS        S
#define OPT_TOTAL_MATCHES       t
#define OPT_TOTAL_MATCHES_ONLY  T
#define OPT_UTF8                u
#define OPT_UTF8_PADDING        U
#define OPT_VERSION             v
#define OPT_VERBOSE             V
#define OPT_HEXADECIMAL         x

/// Command-line option character as a character literal.
#define COPT(X)                   CHARIFY(OPT_##X)

/// Command-line option character as a single-character string literal.
#define SOPT(X)                   STRINGIFY(OPT_##X)

/// Command-line short option as a parenthesized, dashed string literal for the
/// usage message.
#define UOPT(X)                   " (-" SOPT(X) ") "

/// @endcond

/**
 * @addtogroup ad-options-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * An unsigned integer literal of \a N `0xF`s, e.g., <code>%NF(3)</code> =
 * `0xFFF`.
 *
 * @param N The number of `0xF`s of the literal in the range [1,16].
 * @return Returns said literal.
 */
#define NF(N)                     (~0ull >> ((sizeof(long long)*2 - (N)) * 4))

#define OPT_BUF_SIZE        32          /**< Maximum size for an option. */

///////////////////////////////////////////////////////////////////////////////

/**
 * When to dump in UTF-8.
 */
enum utf8_when {
  UTF8_NEVER,                           ///< Never dump in UTF-8.
  UTF8_ENCODING,                        ///< Dump in UTF-8 only if encoding is.
  UTF8_ALWAYS                           ///< Always dump in UTF-8.
};
typedef enum utf8_when utf8_when_t;

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// option extern variable definitions
ad_carray_t     opt_carray;
bool            opt_case_insensitive;
color_when_t    opt_color_when = COLOR_WHEN_DEFAULT;
bool            opt_dump_ascii = true;
unsigned        opt_group_by = GROUP_BY_DEFAULT;
size_t          opt_max_bytes = SIZE_MAX;
ad_matches_t    opt_matches;
ad_offsets_t    opt_offsets = OFFSETS_HEX;
bool            opt_only_matching;
bool            opt_only_printing;
bool            opt_reverse;
char           *opt_search_buf;
endian_t        opt_search_endian;
size_t          opt_search_len;
bool            opt_strings;
ad_strings_t    opt_strings_opts = STRINGS_LINEFEED
                                 | STRINGS_NULL
                                 | STRINGS_SPACE
                                 | STRINGS_TAB ;
bool            opt_utf8;
char const     *opt_utf8_pad = "\xE2\x96\xA1"; /* U+25A1: "white square" */
bool            opt_verbose;

/// @endcond

/**
 * Command-line options.
 */
static struct option const OPTIONS[] = {
  { "bits",               required_argument,  NULL, COPT(BITS)                },
  { "bytes",              required_argument,  NULL, COPT(BYTES)               },
  { "color",              required_argument,  NULL, COPT(COLOR)               },
  { "c-array",            optional_argument,  NULL, COPT(C_ARRAY)             },
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
  { "strings",            optional_argument,  NULL, COPT(STRINGS)             },
  { "strings-opts",       required_argument,  NULL, COPT(STRINGS_OPTS)        },
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

/**
 * The number to search for, if any.
 *
 * @remarks The bytes comprising the number are rearranged according to \ref
 * opt_search_endian.
 */
static uint64_t     search_number;

// local functions
static void         set_all_or_none( char const**, char const* );
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
      if ( opts_given[ STATIC_CAST( char8_t, *opt ) ] ) {
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
 * Checks that the number of bits or bytes given for \ref search_number are
 * sufficient to contain it: if not, prints an error message and exits if \a
 * given_size &lt; \a actual_size.
 *
 * @param given_size The given size in bits or bytes.
 * @param actual_size The actual size of \ref search_number in bits or bytes.
 * @param opt The short option used to specify \a given_size.
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
    if ( opts_given[ STATIC_CAST( char8_t, *opt ) ] ) {
      for ( char const *req_opt = req_opts; req_opt[0] != '\0'; ++req_opt )
        if ( opts_given[ STATIC_CAST( char8_t, *req_opt ) ] )
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
 * Gets the minimum number of bytes required to contain the given `uint64_t`
 * value.
 *
 * @param n The number to get the number of bytes for.
 * @return Returns the minimum number of bytes required to contain \a n
 * in the range [1,8].
 */
NODISCARD
static unsigned int_len( uint64_t n ) {
  return n <= NF(8) ?
    (n <= NF( 4) ? (n <= NF( 2) ? 1 : 2) : (n <= NF( 6) ? 3 : 4)) :
    (n <= NF(12) ? (n <= NF(10) ? 5 : 6) : (n <= NF(14) ? 7 : 8));
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
 * @param size The size of \a buf.
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
  for ( struct option const *opt = OPTIONS; opt->name; ++opt )
    if ( opt->val == short_opt )
      return opt->name;
  return "";
}

/// @cond DOXYGEN_IGNORE
#define ADD_CARRAY(F) BLOCK( \
  if ( (carray & CARRAY_##F) != CARRAY_NONE ) goto dup_format; carray |= CARRAY_##F; )
/// @endcond

/**
 * Parses a C array format value.
 *
 * @param carray_format The NULL-terminated string to parse that may contain
 * exactly zero or one of each of the letters \c cilstu in any order; or NULL
 * for none.
 * @return Returns the corresponding \ref ad_carray or prints an error message
 * and exits if \a carray_format is invalid.
 */
NODISCARD
static ad_carray_t parse_carray( char const *carray_format ) {
  ad_carray_t carray = CARRAY_DEFAULT;
  char const *s;
  char opt_buf[ OPT_BUF_SIZE ];

  if ( carray_format != NULL && carray_format[0] != '\0' ) {
    for ( s = carray_format; *s != '\0'; ++s ) {
      switch ( *s ) {
        case '8': ADD_CARRAY( CHAR8_T );  break;
        case 'c': ADD_CARRAY( CONST );    break;
        case 'i': ADD_CARRAY( INT );      break;
        case 'l': ADD_CARRAY( LONG );     break;
        case 's': ADD_CARRAY( STATIC );   break;
        case 't': ADD_CARRAY( SIZE_T );   break;
        case 'u': ADD_CARRAY( UNSIGNED ); break;
        default :
          fatal_error( EX_USAGE,
            "'%c': invalid C format for %s;"
            " must be one of: [8cilstu]\n",
            *s, opt_format( COPT(C_ARRAY), opt_buf, sizeof opt_buf )
          );
      } // switch
    } // for
    if ( (carray & CARRAY_SIZE_T) != CARRAY_NONE &&
         (carray & CARRAY_INT_LENGTH) != CARRAY_NONE ) {
      fatal_error( EX_USAGE,
        "\"%s\": invalid C format for %s:"
        " 't' and [ilu] are mutually exclusive\n",
        carray_format, opt_format( COPT(C_ARRAY), opt_buf, sizeof opt_buf )
      );
    }
  }
  return carray;

dup_format:
  fatal_error( EX_USAGE,
    "\"%s\": invalid C format for %s:"
    " '%c' specified more than once\n",
    carray_format, opt_format( COPT(C_ARRAY), opt_buf, sizeof opt_buf ), *s
  );
}

#undef ADD_CARRAY

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
    s = memcpy( check_strdup( s ), "0x", 2 );
  }
  unsigned long long const cp_candidate = parse_ull( s );
  if ( s != s0 )
    FREE( s );
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
  };

  assert( when != NULL );
  size_t names_buf_size = 1;            // for trailing NULL

  FOREACH_ARRAY_ELEMENT( colorize_map_t, m, COLORIZE_MAP ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_colorization;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = free_later( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  FOREACH_ARRAY_ELEMENT( colorize_map_t, m, COLORIZE_MAP ) {
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
 * Parses a string into an offset.
 * Unlike **strtoull(3)**:
 *  + Insists that \a s is non-negative.
 *  + May be followed by one of `b`, `k`, or `m`
 *    for 512-byte blocks, kilobytes, and megabytes, respectively.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the parsed offset only if \a s is a non-negative number or
 * prints an error message and exits if there was an error.
 */
NODISCARD
static off_t parse_offset( char const *s ) {
  SKIP_WS( s );
  if ( unlikely( s[0] == '\0' || s[0] == '-' ) )
    goto error;                         // strtoull(3) wrongly allows '-'

  { // local scope
    char *end = NULL;
    errno = 0;
    unsigned long long n = strtoull( s, &end, 0 );
    if ( unlikely( errno != 0 || end == s ) )
      goto error;
    if ( end[0] != '\0' ) {             // possibly 'b', 'k', or 'm'
      if ( end[1] != '\0' )             // not a single char
        goto error;
      switch ( end[0] ) {
        case 'b': n *=         512; break;
        case 'k': n *=        1024; break;
        case 'm': n *= 1024 * 1024; break;
        default : goto error;
      } // switch
    }
    return STATIC_CAST( off_t, n );
  } // local scope

error:
  fatal_error( EX_USAGE, "\"%s\": invalid offset\n", s );
}

/**
 * Parses a `--strings-opts` value.
 *
 * @param opts_format The null-terminated string search options format to
 * parse.
 * @return Returns the corresponding \ref strings_opts or prints an error
 * message and exits if \a opts_format is invalid.
 */
NODISCARD
static ad_strings_t parse_strings_opts( char const *opts_format ) {
  set_all_or_none( &opts_format, "0w" );
  char opt_buf[ OPT_BUF_SIZE ];
  ad_strings_t opts = STRINGS_NONE;

  for ( char const *s = opts_format; *s != '\0'; ++s ) {
    switch ( *s ) {
      case '0': opts |= STRINGS_NULL;     break;
      case 'f': opts |= STRINGS_FORMFEED; break;
      case 'l':
      case 'n': opts |= STRINGS_LINEFEED; break;
      case 'r': opts |= STRINGS_RETURN;   break;
      case 's': opts |= STRINGS_SPACE;    break;
      case 't': opts |= STRINGS_TAB;      break;
      case 'v': opts |= STRINGS_VTAB;     break;
      case 'w': opts |= STRINGS_FORMFEED
                     |  STRINGS_LINEFEED
                     |  STRINGS_RETURN
                     |  STRINGS_SPACE
                     |  STRINGS_TAB
                     |  STRINGS_VTAB;     break;
      default :
        fatal_error( EX_USAGE,
          "'%c': invalid option for %s; must be one of: [0flnrstvw]\n",
          *s, opt_format( COPT(STRINGS_OPTS), opt_buf, sizeof opt_buf )
        );
    } // switch
  } // for

  return opts;
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
  };

  assert( when != NULL );
  size_t names_buf_size = 1;            // for trailing NULL

  FOREACH_ARRAY_ELEMENT( utf8_map_t, m, UTF8_MAP ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_utf8;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = free_later( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  FOREACH_ARRAY_ELEMENT( utf8_map_t, m, UTF8_MAP ) {
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
 * If \a *pformat is:
 *
 *  + `"*"`: sets \a *pformat to \a all_value.
 *  + `"-"`: sets \a *pformat to `""` (the empty string).
 *
 * Otherwise does nothing.
 *
 * @param pformat A pointer to the format string to possibly set.
 * @param all_value The "all" value for when \a *pformat is `"*"`.
 */
static void set_all_or_none( char const **pformat, char const *all_value ) {
  assert( pformat != NULL );
  assert( *pformat != NULL );
  assert( all_value != NULL );
  assert( all_value[0] != '\0' );

  if ( strcmp( *pformat, "*" ) == 0 )
    *pformat = all_value;
  else if ( strcmp( *pformat, "-" ) == 0 )
    *pformat = "";
}

/**
 * Determines whether we should dump in UTF-8.
 *
 * @param when The UTF-8 when value.
 * @return Returns `true` only if we should do UTF-8.
 */
NODISCARD
static bool should_utf8( utf8_when_t when ) {
  switch ( when ) {                     // handle easy cases
    case UTF8_ALWAYS: return true;
    case UTF8_NEVER : return false;
    default         : break;
  } // switch

#if defined( HAVE_SETLOCALE ) && defined( HAVE_NL_LANGINFO )
  setlocale( LC_CTYPE, "" );
  char const *const encoding = nl_langinfo( CODESET );
  return  strcasecmp( encoding, "utf8"  ) == 0 ||
          strcasecmp( encoding, "utf-8" ) == 0;
#else
  return false;
#endif
}

/**
 * Prints the usage message to standard error and exits.
 */
_Noreturn
static void usage( int status ) {
  fprintf( status == EX_OK ? stdout : stderr,
"usage: %s [options] [+offset] [infile [outfile]]\n"
"       %s --reverse [-" SOPT(DECIMAL) SOPT(OCTAL) SOPT(HEXADECIMAL) "] [infile [outfile]]\n"
"       %s --help\n"
"       %s --version\n"
"options:\n"
"  --big-endian=NUM      " UOPT(BIG_ENDIAN)
                          "Search for big-endian number.\n"
"  --bits=NUM            " UOPT(BITS)
                          "Number size in bits: 8-64 [default: auto].\n"
"  --bytes=NUM           " UOPT(BYTES)
                          "Number size in bytes: 1-8 [default: auto].\n"
"  --c-array=FMT         " UOPT(C_ARRAY)
                          "Dump bytes as a C array.\n"
"  --color=WHEN          " UOPT(COLOR)
                          "When to colorize output [default: not_file].\n"
"  --decimal             " UOPT(DECIMAL)
                          "Print offsets in decimal.\n"
"  --group-by=NUM        " UOPT(GROUP_BY)
                          "Group bytes by 1/2/4/8/16/32 [default: " STRINGIFY(GROUP_BY_DEFAULT) "].\n"
"  --help                " UOPT(HELP)
                          "Print this help and exit.\n"
"  --hexadecimal         " UOPT(HEXADECIMAL)
                          "Print offsets in hexadecimal [default].\n"
"  --host-endian=NUM     " UOPT(HOST_ENDIAN)
                          "Search for host-endian number.\n"
"  --ignore-case         " UOPT(IGNORE_CASE)
                          "Ignore case for string searches.\n"
"  --little-endian=NUM   " UOPT(LITTLE_ENDIAN)
                          "Search for little-endian number.\n"
"  --matching-only       " UOPT(MATCHING_ONLY)
                          "Only dump rows having matches.\n"
"  --max-bytes=NUM       " UOPT(MAX_BYTES)
                          "Dump max number of bytes [default: unlimited].\n"
"  --max-lines=NUM       " UOPT(MAX_LINES)
                          "Dump max number of lines [default: unlimited].\n"
"  --no-ascii            " UOPT(NO_ASCII)
                          "Suppress printing the ASCII part.\n"
"  --no-offsets          " UOPT(NO_OFFSETS)
                          "Suppress printing offsets.\n"
"  --octal               " UOPT(OCTAL)
                          "Print offsets in octal.\n"
"  --plain               " UOPT(PLAIN)
                          "Dump in plain format; same as: -AOg32.\n"
"  --printing-only       " UOPT(PRINTING_ONLY)
                          "Only dump rows having printable characters.\n"
"  --reverse             " UOPT(REVERSE)
                          "Reverse from dump back to binary.\n"
"  --skip-bytes=NUM      " UOPT(SKIP_BYTES)
                          "Jump to offset before dumping [default: 0].\n"
"  --string=STR          " UOPT(STRING)
                          "Search for string.\n"
"  --strings[=NUM]       " UOPT(STRINGS)
                          "Search for all strings.\n"
"  --strings-opts=OPTS   " UOPT(STRINGS_OPTS)
                          "Options for --strings matches [default: 0nst].\n"
"  --total-matches       " UOPT(TOTAL_MATCHES)
                          "Additionally print total number of matches.\n"
"  --total-matches-only  " UOPT(TOTAL_MATCHES_ONLY)
                          "Only print total number of matches.\n"
"  --utf8=WHEN           " UOPT(UTF8)
                          "When to dump in UTF-8 [default: never].\n"
"  --utf8-padding=NUM    " UOPT(UTF8_PADDING)
                          "Set UTF-8 padding character [default: U+2581].\n"
"  --verbose             " UOPT(VERBOSE)
                          "Dump repeated rows also.\n"
"  --version             " UOPT(VERSION)
                          "Print version and exit.\n"
"\n"
PACKAGE_NAME " home page: " PACKAGE_URL "\n"
"Report bugs to: " PACKAGE_BUGREPORT "\n",
    me,
    me,
    me,
    me
  );
  exit( status );
}

////////// extern functions ///////////////////////////////////////////////////

char const* gets_offsets_english( void ) {
  switch ( opt_offsets ) {
    case OFFSETS_NONE: return "none";
    case OFFSETS_DEC : return "decimal";
    case OFFSETS_HEX : return "hexadecimal";
    case OFFSETS_OCT : return "octal";
  } // switch
  UNEXPECTED_INT_VALUE( opt_offsets );
}

char const* get_offsets_format( void ) {
  static char format[8];                // e.g.: "%016llX"
  if ( format[0] == '\0' ) {
    switch ( opt_offsets ) {
      case OFFSETS_NONE:
        strcpy( format, "%00" );
        break;
      case OFFSETS_DEC:
        sprintf( format, "%%0%zu" PRIu64, get_offsets_width() );
        break;
      case OFFSETS_HEX:
        sprintf( format, "%%0%zu" PRIX64, get_offsets_width() );
        break;
      case OFFSETS_OCT:
        sprintf( format, "%%0%zu" PRIo64, get_offsets_width() );
        break;
    } // switch
  }
  return format;
}

size_t get_offsets_width( void ) {
  return  (opt_group_by == 1 && opt_dump_ascii) ||
          (row_bytes > ROW_BYTES_DEFAULT && !opt_dump_ascii) ?
      OFFSET_WIDTH_MIN : OFFSET_WIDTH_MAX;
}

void parse_options( int argc, char const *argv[] ) {
  size_t            max_lines = 0;
  int               opt;
  bool              opt_help = false;
  bool              opt_version = false;
  char const *const short_opts = make_short_opts( OPTIONS );
  size_t            size_in_bits = 0, size_in_bytes = 0;
  char32_t          utf8_pad = 0;
  utf8_when_t       utf8_when = UTF8_NEVER;

  opterr = 1;

  for (;;) {
    opt = getopt_long(
      argc, CONST_CAST( char**, argv ), short_opts, OPTIONS,
      /*longindex=*/NULL
    );
    if ( opt == -1 )
      break;
    switch ( opt ) {
      case COPT(BIG_ENDIAN):
        search_number = STATIC_CAST( uint64_t, parse_ull( optarg ) );
        opt_search_endian = ENDIAN_BIG;
        break;
      case COPT(BITS):
        size_in_bits = STATIC_CAST( size_t, parse_ull( optarg ) );
        break;
      case COPT(BYTES):
        size_in_bytes = STATIC_CAST( size_t, parse_ull( optarg ) );
        break;
      case COPT(C_ARRAY):
        opt_carray = parse_carray( optarg );
        break;
      case COPT(COLOR):
        opt_color_when = parse_color_when( optarg );
        break;
      case COPT(DECIMAL):
        opt_offsets = OFFSETS_DEC;
        break;
      case COPT(GROUP_BY):
        opt_group_by = parse_group_by( optarg );
        break;
      case COPT(HELP):
        opt_help = true;
        break;
      case COPT(HEXADECIMAL):
        opt_offsets = OFFSETS_HEX;
        break;
      case COPT(HOST_ENDIAN):
        search_number = STATIC_CAST( uint64_t, parse_ull( optarg ) );
#ifdef WORDS_BIGENDIAN
        opt_search_endian = ENDIAN_BIG;
#else
        opt_search_endian = ENDIAN_LITTLE;
#endif /* WORDS_BIGENDIAN */
        break;
      case COPT(IGNORE_CASE):
        opt_case_insensitive = true;
        break;
      case COPT(LITTLE_ENDIAN):
        search_number = STATIC_CAST( uint64_t, parse_ull( optarg ) );
        opt_search_endian = ENDIAN_LITTLE;
        break;
      case COPT(MATCHING_ONLY):
        opt_only_matching = true;
        break;
      case COPT(MAX_BYTES):
        opt_max_bytes = STATIC_CAST( size_t, parse_offset( optarg ) );
        break;
      case COPT(MAX_LINES):
        max_lines = STATIC_CAST( size_t, parse_ull( optarg ) );
        break;
      case COPT(NO_ASCII):
        opt_dump_ascii = false;
        break;
      case COPT(NO_OFFSETS):
        opt_offsets = OFFSETS_NONE;
        break;
      case COPT(OCTAL):
        opt_offsets = OFFSETS_OCT;
        break;
      case COPT(PLAIN):
        opt_group_by = ROW_BYTES_MAX;
        opt_offsets = OFFSETS_NONE;
        opt_dump_ascii = false;
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
        opt_search_buf = free_later( check_strdup( optarg ) );
        break;
      case COPT(STRINGS):
        opt_strings = true;
        opt_search_len = optarg != NULL ?
          STATIC_CAST( size_t, parse_ull( optarg ) ) : STRINGS_LEN_DEFAULT;
        break;
      case COPT(STRINGS_OPTS):
        opt_strings_opts = parse_strings_opts( optarg );
        opt_strings = true;
        if ( opt_search_len == 0 )
          opt_search_len = STRINGS_LEN_DEFAULT;
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
        opt_version = true;
        break;

      case ':':
        goto missing_arg;
      case '?':
        goto invalid_opt;

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
    SOPT(STRINGS)
    SOPT(STRINGS_OPTS)
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
    SOPT(STRING) SOPT(STRINGS) SOPT(STRINGS_OPTS)
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
    SOPT(STRINGS)
    SOPT(STRINGS_OPTS)
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
  check_mutually_exclusive( SOPT(STRINGS),
    SOPT(LITTLE_ENDIAN) SOPT(BIG_ENDIAN) SOPT(HOST_ENDIAN)
    SOPT(IGNORE_CASE)
    SOPT(STRING)
  );
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
    SOPT(STRINGS)
    SOPT(STRINGS_OPTS)
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
    SOPT(STRINGS)
  );
  check_required( SOPT(UTF8_PADDING), SOPT(UTF8) );

  if ( opt_help )
    usage( argc > 2 ? EX_USAGE : EX_OK );

  if ( opt_version ) {
    puts( PACKAGE_STRING );
    exit( EX_OK );
  }

  char opt_buf[ OPT_BUF_SIZE ];

  if ( opts_given[ COPT(BITS) ] ) {
    if ( size_in_bits % 8 != 0 || size_in_bits > 64 )
      fatal_error( EX_USAGE,
        "\"%zu\": invalid value for %s;"
        " must be a multiple of 8 in 8-64\n",
        size_in_bits, opt_format( COPT(BITS), opt_buf, sizeof opt_buf )
      );
    opt_search_len = size_in_bits * 8;
    check_number_size(
      size_in_bits, int_len( search_number ) * 8, COPT(BITS)
    );
  }

  if ( opts_given[ COPT(BYTES) ] ) {
    if ( size_in_bytes > 8 )
      fatal_error( EX_USAGE,
        "\"%zu\": invalid value for %s; must be in 1-8\n",
        size_in_bytes, opt_format( COPT(BYTES), opt_buf, sizeof opt_buf )
      );
    opt_search_len = size_in_bytes;
    check_number_size(
      size_in_bytes, int_len( search_number ), COPT(BYTES)
    );
  }

  if ( opt_case_insensitive )
    tolower_s( opt_search_buf );

  if ( opt_group_by > row_bytes )
    row_bytes = opt_group_by;

  if ( max_lines > 0 )
    opt_max_bytes = max_lines * row_bytes;

  switch ( argc ) {
    case 2:                             // infile & outfile
      fout_path = argv[2];
      if ( strcmp( fout_path, "-" ) != 0 ) {
        //
        // We can't use fopen(3) because there's no mode that specifies opening
        // a file for writing and NOT truncating it to zero length if it
        // exists.
        //
        // Hence we have to use open(2) so we can specify only O_WRONLY and
        // O_CREAT but not O_TRUNC.
        //
        int const fd = open( fout_path, O_WRONLY | O_CREAT, 0644 );
        if ( fd == -1 )
          fatal_error( EX_CANTCREAT, "\"%s\": %s\n", fout_path, STRERROR() );
        DUP2( fd, STDOUT_FILENO );
        close( fd );
      }
      FALLTHROUGH;

    case 1:                             // infile only
      fin_path = argv[1];
      if ( strcmp( fin_path, "-" ) != 0 && !freopen( fin_path, "r", stdin ) )
        fatal_error( EX_NOINPUT, "\"%s\": %s\n", fin_path, STRERROR() );
      FALLTHROUGH;

    case 0:
      fskip( fin_offset, stdin );
      break;

    default:
      usage( EX_USAGE );
  } // switch

  if ( !opt_strings ) {
    if ( opt_search_buf != NULL ) {
      // searching for a string
      opt_search_len = strlen( opt_search_buf );
    }
    else if ( opt_search_endian != ENDIAN_NONE ) {
      // searching for a number
      if ( opt_search_len == 0 )        // default to smallest possible size
        opt_search_len = int_len( search_number );
      int_rearrange_bytes( &search_number, opt_search_len, opt_search_endian );
      opt_search_buf = POINTER_CAST( char*, &search_number );
    }
  }

  if ( opt_max_bytes == 0 )             // degenerate case
    exit( opt_search_len > 0 ? EX_NO_MATCHES : EX_OK );

  opt_utf8 = should_utf8( utf8_when );
  if ( utf8_pad ) {
    static char utf8_pad_buf[ UTF8_CHAR_SIZE_MAX + 1 /*NULL*/ ];
    utf32_8( utf8_pad, utf8_pad_buf );
    opt_utf8_pad = utf8_pad_buf;
  }

  return;

invalid_opt:
  NO_OP;
  // Determine whether the invalid option was short or long.
  char const *const invalid_opt = argv[ optind - 1 ];
  EPRINTF( "%s: ", me );
  if ( invalid_opt != NULL && strncmp( invalid_opt, "--", 2 ) == 0 )
    EPRINTF( "\"%s\"", invalid_opt + 2/*skip over "--"*/ );
  else
    EPRINTF( "'%c'", STATIC_CAST( char, optopt ) );
  EPRINTF( ": invalid option; use --help or -h for help\n" );
  exit( EX_USAGE );

missing_arg:
  fatal_error( EX_USAGE,
    "\"%s\" requires an argument\n",
    opt_format(
      STATIC_CAST( char, opt == ':' ? optopt : opt ),
      opt_buf, sizeof opt_buf
    )
  );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
