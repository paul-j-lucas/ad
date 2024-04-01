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

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */

/// @endcond

/**
 * @defgroup ad-options-group Ad Options
 * Global variables and functions for **ad** options.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * C array dump formats.
 */
enum ad_carray {
  CARRAY_NONE     = 0,                  ///< No format.
  CARRAY_DEFAULT  = 1 << 0,             ///< Default format.
  CARRAY_CHAR8_T  = 1 << 1,             ///< Declare array type as `char8_t`.
  CARRAY_UNSIGNED = 1 << 2,             ///< Declare len type as `unsigned`.
  CARRAY_INT      = 1 << 3,             ///< Declare len type as `int`.
  CARRAY_LONG     = 1 << 4,             ///< Declare len type as `long`.
  CARRAY_SIZE_T   = 1 << 5,             ///< Declare len type as `size_t`.
  CARRAY_CONST    = 1 << 6,             ///< Declare variables as `const`.
  CARRAY_STATIC   = 1 << 7,             ///< Declare variables as `static`.
};
typedef enum ad_carray ad_carray_t;

/**
 * Shorthand for any C dump format length: #CARRAY_INT, #CARRAY_LONG,
 * #CARRAY_UNSIGNED, or #CARRAY_SIZE_T.
 *
 * @sa #CARRAY_INT_LENGTH
 */
#define CARRAY_ANY_LENGTH         ( CARRAY_INT_LENGTH | CARRAY_SIZE_T )

/**
 * Shorthand for any `int` C dump format length: #CARRAY_INT, #CARRAY_LONG, or
 * #CARRAY_UNSIGNED.
 *
 * @sa #CARRAY_ANY_LENGTH
 */
#define CARRAY_INT_LENGTH         ( CARRAY_UNSIGNED | CARRAY_INT | CARRAY_LONG )

/**
 * Whether to print the total number of matches.
 */
enum ad_matches {
  MATCHES_NO_PRINT,                     ///< Don't print total matches.
  MATCHES_ALSO_PRINT,                   ///< Additionally print total matches.
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
  STRINGS_FORMFEED  = (1u << 0),        ///< Include form-feed characters.
  STRINGS_NEWLINE   = (1u << 1),        ///< Include newline characters.
  STRINGS_NULL      = (1u << 2),        ///< Must end with null byte.
  STRINGS_RETURN    = (1u << 3),        ///< Include carriage return characters.
  STRINGS_SPACE     = (1u << 4),        ///< Include space characters.
  STRINGS_TAB       = (1u << 5),        ///< Include tab characters.
  STRINGS_VTAB      = (1u << 6),        ///< Include vertical tab characters.
};
typedef enum ad_strings ad_strings_t;

////////// extern variables ///////////////////////////////////////////////////

extern ad_carray_t    opt_carray;        ///< Dump as C array in this format.
extern bool           opt_case_insensitive; ///< Case-insensitive matching?
extern color_when_t   opt_color_when;   ///< When to colorize output.
extern bool           opt_dump_ascii;   ///< Dump ASCII part?
extern unsigned       opt_group_by;     ///< Group by this number of bytes.
extern size_t         opt_max_bytes;    ///< Maximum number of bytes to dump.
extern ad_matches_t   opt_matches;      ///< When to print total matches.
extern ad_offsets_t   opt_offsets;      ///< Dump offsets in this format.
extern bool           opt_only_matching;///< Only dump matching rows?
extern bool           opt_only_printing;///< Only dump printable rows?
extern bool           opt_reverse;      ///< Reverse dump (patch)?

/**
 * The bytes of what to search for, if any.
 *
 * @remarks When searching for a:
 * + A specific string, this points to the null-terminated string.
 * + Any string, not used.
 * + Number, this points to \ref search_number.
 */
extern char          *opt_search_buf;

extern endian_t       opt_search_endian;///< Numeric search endianness.
extern size_t         opt_search_len;   ///< Bytes in \ref opt_search_buf.

extern bool           opt_strings;      ///< **strings**(1)-like search?
extern ad_strings_t   opt_strings_opts; ///< **strings**(1)-like options.
extern bool           opt_utf8;         ///< Dump UTF-8 bytes?
extern char const    *opt_utf8_pad;     ///< UTF-8 padding character.
extern bool           opt_verbose;      ///< Dump _all_ rows of data?

////////// extern functions ///////////////////////////////////////////////////

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
 * @return Returns said printf(3) format.
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
size_t get_offsets_width( void );

/**
 * Parses command-line options and sets global variables.
 *
 * @param argc The argument count from \c main().
 * @param argv The argument values from \c main().
 */
void parse_options( int argc, char const *argv[] );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_options_H */
/* vim:set et sw=2 ts=2: */
