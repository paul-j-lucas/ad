/*
**      ad -- ASCII dump
**      src/options.h
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

#ifndef ad_options_H
#define ad_options_H

/**
 * @file
 * Declares global variables and functions for **ad** options.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#include "types.h"
#include "unicode.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */

/// @endcond

/**
 * @defgroup ad-options-group Ad Options
 * Global variables and functions for **ad** options.
 * @{
 */

/**
 * Convenience macro for iterating over all **ad** command-line options.
 *
 * @param VAR The `struct option` loop variable.
 *
 * @sa cli_option_next()
 */
#define FOREACH_CLI_OPTION(VAR) \
  for ( struct option const *VAR = NULL; (VAR = cli_option_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

/**
 * C array dump formats.
 */
enum ad_c_array {
  C_ARRAY_NONE          = 0,            ///< No format.
  C_ARRAY_DEFAULT       = 1 << 0,       ///< Default format.
  C_ARRAY_CHAR8_T       = 1 << 1,       ///< Declare array type as `char8_t`.
  C_ARRAY_CONST         = 1 << 2,       ///< Declare variables as `const`.
  C_ARRAY_LEN_UNSIGNED  = 1 << 3,       ///< Declare length type as `unsigned`.
  C_ARRAY_LEN_INT       = 1 << 4,       ///< Declare length type as `int`.
  C_ARRAY_LEN_LONG      = 1 << 5,       ///< Declare length type as `long`.
  C_ARRAY_LEN_SIZE_T    = 1 << 6,       ///< Declare length type as `size_t`.
  C_ARRAY_STATIC        = 1 << 7,       ///< Declare variables as `static`.
};
typedef enum ad_c_array ad_c_array_t;

/**
 * Shorthand for any C dump format length: #C_ARRAY_LEN_INT, #C_ARRAY_LEN_LONG,
 * #C_ARRAY_LEN_UNSIGNED, or #C_ARRAY_LEN_SIZE_T.
 *
 * @sa #C_ARRAY_LEN_ANY_INT
 */
#define C_ARRAY_LEN_ANY           ( C_ARRAY_LEN_ANY_INT | C_ARRAY_LEN_SIZE_T )

/**
 * Shorthand for any `int` C dump format length: #C_ARRAY_LEN_INT,
 * #C_ARRAY_LEN_LONG, or #C_ARRAY_LEN_UNSIGNED.
 *
 * @sa #C_ARRAY_LEN_ANY
 */
#define C_ARRAY_LEN_ANY_INT       ( C_ARRAY_LEN_INT | C_ARRAY_LEN_LONG \
                                  | C_ARRAY_LEN_UNSIGNED )

/**
 * Whether to print the total number of matches.
 */
enum ad_matches {
  MATCHES_NO_PRINT,                     ///< Don't print total matches.
  MATCHES_ALSO_PRINT,                   ///< Also print total matches.
  MATCHES_ONLY_PRINT                    ///< Only print total matches.
};
typedef enum ad_matches ad_matches_t;

/**
 * Offset formats.
 */
enum ad_offsets {
  OFFSETS_NONE =  0,                    ///< No offsets.
  OFFSETS_DEC  = 10,                    ///< Decimal offsets.
  OFFSETS_HEX  = 16,                    ///< Hexadecimal offsets.
  OFFSETS_OCT  =  8                     ///< Octal offsets.
};
typedef enum ad_offsets ad_offsets_t;

/**
 * Options for **strings**(1)-like searches.
 */
enum ad_strings {
  STRINGS_NONE      = 0,                ///< No options.
  STRINGS_FORMFEED  = 1 << 0,           ///< Include form-feed characters.
  STRINGS_LINEFEED  = 1 << 1,           ///< Include line-feed characters.
  STRINGS_NULL      = 1 << 2,           ///< Must end with null byte.
  STRINGS_RETURN    = 1 << 3,           ///< Include carriage return characters.
  STRINGS_SPACE     = 1 << 4,           ///< Include space characters.
  STRINGS_TAB       = 1 << 5,           ///< Include tab characters.
  STRINGS_VTAB      = 1 << 6,           ///< Include vertical tab characters.
};
typedef enum ad_strings ad_strings_t;

////////// extern variables ///////////////////////////////////////////////////

extern ad_debug_t     opt_ad_debug;
extern ad_c_array_t   opt_c_array;      ///< Dump as C array in this format.
extern color_when_t   opt_color_when;   ///< When to colorize output.
extern bool           opt_dump_ascii;   ///< Dump ASCII part?
extern char const    *opt_format_path;  ///< Dump format path.
extern unsigned       opt_group_by;     ///< Group by this number of bytes.
extern bool           opt_ignore_case;  ///< Case-insensitive matching?
extern size_t         opt_max_bytes;    ///< Maximum number of bytes to dump.
extern ad_matches_t   opt_matches;      ///< When to print total matches.
extern ad_offsets_t   opt_offsets;      ///< Dump offsets in this format.
extern bool           opt_only_matching;///< Only dump matching rows?
extern bool           opt_only_printing;///< Only dump printable rows?
#ifdef __APPLE__
extern bool           opt_resource_fork;///< Dump file's macOS resource fork?
#endif /* __APPLE__ */
extern bool           opt_reverse;      ///< Reverse dump (patch)?

/**
 * The bytes of what to search for, if any.
 *
 * @remarks When searching for:
 * + A specific string: this points to the null-terminated string.
 * + Any string: not used.
 * + A number: this points to \ref search_number.
 *
 * @sa opt_search_len
 */
extern char          *opt_search_buf;

extern endian_t       opt_search_endian;///< Numeric search endianness.
extern size_t         opt_search_len;   ///< Bytes in \ref opt_search_buf.

extern bool           opt_strings;      ///< **strings**(1)-like search?
extern ad_strings_t   opt_strings_opts; ///< **strings**(1)-like options.
extern bool           opt_utf8;         ///< Dump as UTF-8?
extern char8_t const *opt_utf8_pad;     ///< UTF-8 padding character.
extern bool           opt_verbose;      ///< Dump _all_ rows of data?

////////// extern functions ///////////////////////////////////////////////////

/**
 * Iterates to the next **ad** command-line option.
 *
 * @param opt A pointer to the previous option. For the first iteration, NULL
 * should be passed.
 * @return Returns the next command-line option or NULL for none.
 *
 * @note This function isn't normally called directly; use the
 * #FOREACH_CLI_OPTION() macro instead.
 *
 * @sa #FOREACH_CLI_OPTION()
 */
NODISCARD
struct option const* cli_option_next( struct option const *opt );

/**
 * Gets the English word for the current offset format.
 *
 * @return Returns said word.
 *
 * @sa get_offsets_format()
 */
NODISCARD
char const* gets_offsets_english( void );

/**
 * Gets the **printf**(3) format for the current offset format.
 *
 * @return Returns said **printf**(3) format.
 *
 * @sa gets_offsets_english()
 */
NODISCARD
char const* get_offsets_format( void );

/**
 * Gets the offsets width.
 *
 * @return Returns said width.
 */
NODISCARD
unsigned get_offsets_width( void );

/**
 * Initializes **ad** options from the command-line.
 *
 * @param argc The argument count from \c main().
 * @param argv The argument values from \c main().
 *
 * @note This function must be called exactly once.
 */
void options_init( int argc, char const *argv[] );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_options_H */
/* vim:set et sw=2 ts=2: */
