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
#include "types.h"

// standard
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */

/**
 * Checks whether the given \c c_fmt specifies a type.
 *
 * @param FMT The \c c_fmt to check.
 * @return Returns \c true only if \a FMT specifies a type.
 * @hideinitializer
 */
#define CFMT_HAS_TYPE(FMT) \
  (((FMT) & (CFMT_UNSIGNED | CFMT_INT | CFMT_LONG | CFMT_SIZE_T)) != 0)

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
typedef unsigned c_fmt_t;               ///< Bitwise-or of c_fmt options.

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

////////// extern variables ///////////////////////////////////////////////////

#ifdef ENABLE_AD_DEBUG
extern bool           opt_ad_debug;
#endif /* ENABLE_AD_DEBUG */
extern bool           opt_case_insensitive;
extern color_when_t   opt_color_when;
extern unsigned       opt_c_fmt;
extern unsigned       opt_group_by;
extern size_t         opt_max_bytes;
extern matches_t      opt_matches;
extern offset_fmt_t   opt_offset_fmt;
extern bool           opt_only_matching;
extern bool           opt_only_printing;
extern bool           opt_print_ascii;
extern bool           opt_reverse;
extern bool           opt_utf8;
extern char const    *opt_utf8_pad;
extern bool           opt_verbose;

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
