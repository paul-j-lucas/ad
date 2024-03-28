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

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */

///////////////////////////////////////////////////////////////////////////////

/**
 * C dump formats.
 */
enum c_fmt {
  CFMT_NONE     = 0,                    ///< No format.
  CFMT_DEFAULT  = 1 << 0,               ///< Default format.
  CFMT_CHAR8_T  = 1 << 1,               ///< Declare array type as `char8_t`.
  CFMT_UNSIGNED = 1 << 2,               ///< Declare len type as `unsigned`.
  CFMT_INT      = 1 << 3,               ///< Declare len type as `int`.
  CFMT_LONG     = 1 << 4,               ///< Declare len type as `long`.
  CFMT_SIZE_T   = 1 << 5,               ///< Declare len type as `size_t`.
  CFMT_CONST    = 1 << 6,               ///< Declare variables as `const`.
  CFMT_STATIC   = 1 << 7,               ///< Declare variables as `static`.
};
typedef enum c_fmt c_fmt_t;             ///< Bitwise-or of c_fmt options.

/**
 * Shorthand for any C dump format length: #CFMT_INT, #CFMT_LONG,
 * #CFMT_UNSIGNED, or #CFMT_SIZE_T.
 */
#define CFMT_ANY_LENGTH           ( CFMT_UNSIGNED | CFMT_INT | CFMT_LONG \
                                  | CFMT_SIZE_T )

/**
 * Whether to print the total number of matches.
 */
enum matches {
  MATCHES_NO_PRINT,                     ///< Don't print total matches.
  MATCHES_ALSO_PRINT,                   ///< Additionally print total matches.
  MATCHES_ONLY_PRINT                    ///< Only print total matches.
};
typedef enum matches matches_t;

/**
 * Offset formats.
 */
enum offset_fmt {
  OFMT_NONE =  0,
  OFMT_DEC  = 10,
  OFMT_HEX  = 16,
  OFMT_OCT  =  8
};
typedef enum offset_fmt offset_fmt_t;

/**
 * Options for **strings**(1)-like searches.
 */
enum strings_opts {
  STRINGS_OPT_NONE      = 0,            ///< No options.
  STRINGS_OPT_FORMFEED  = (1u << 0),    ///< Include form-feed characters.
  STRINGS_OPT_NEWLINE   = (1u << 1),    ///< Include newline characters.
  STRINGS_OPT_NULL      = (1u << 2),    ///< Must end with null byte.
  STRINGS_OPT_RETURN    = (1u << 3),    ///< Include carriage return characters.
  STRINGS_OPT_SPACE     = (1u << 4),    ///< Include space characters.
  STRINGS_OPT_TAB       = (1u << 5),    ///< Include tab characters.
  STRINGS_OPT_VTAB      = (1u << 6),    ///< Include vertical tab characters.
};
typedef enum strings_opts strings_opts_t;

////////// extern variables ///////////////////////////////////////////////////

extern bool           opt_case_insensitive; ///< Case-insensitive matching?
extern color_when_t   opt_color_when;   ///< When to colorize output.
extern c_fmt_t        opt_c_fmt;        ///< Dump as C array in this format.
extern unsigned       opt_group_by;     ///< Group by this number of bytes.
extern size_t         opt_max_bytes;    ///< Maximum number of bytes to dump.
extern matches_t      opt_matches;      ///< When to print total matches.
extern offset_fmt_t   opt_offset_fmt;   ///< Dump offsets in this format.
extern bool           opt_only_matching;///< Only dump matching rows?
extern bool           opt_only_printing;///< Only dump printable rows?
extern bool           opt_print_ascii;  ///< Dump ASCII part?
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
extern strings_opts_t opt_strings_opts; ///< **strings**(1)-like options.
extern bool           opt_utf8;         ///< Dump UTF-8 bytes?
extern char const    *opt_utf8_pad;     ///< UTF-8 padding character.
extern bool           opt_verbose;      ///< Dump _all_ rows of data?

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the English word for the current offset format.
 *
 * @return Returns said word.
 */
NODISCARD
char const* get_offset_fmt_english( void );

/**
 * Gets the **printf**(3) format for the current offset format.
 *
 * @return Returns said printf(3) format.
 */
NODISCARD
char const* get_offset_fmt_format( void );

/**
 * Gets the offset width.
 *
 * @return Returns said width.
 */
NODISCARD
size_t get_offset_width( void );

/**
 * Parses command-line options and sets global variables.
 *
 * @param argc The argument count from \c main().
 * @param argv The argument values from \c main().
 */
void parse_options( int argc, char const *argv[] );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_options_H */
/* vim:set et sw=2 ts=2: */
