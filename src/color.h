/*
**      ad -- ASCII dump
**      src/color.h
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
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ad_color_H
#define ad_color_H

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

#define SGR_BG_BLACK        "40"        /**< Background black.            */
#define SGR_BG_RED          "41"        /**< Background red.              */
#define SGR_BG_GREEN        "42"        /**< Background green.            */
#define SGR_BG_YELLOW       "43"        /**< Background yellow.           */
#define SGR_BG_BLUE         "44"        /**< Background blue.             */
#define SGR_BG_MAGENTA      "45"        /**< Background magenta.          */
#define SGR_BG_CYAN         "46"        /**< Background cyan.             */
#define SGR_BG_WHITE        "47"        /**< Background white.            */

#define SGR_FG_BLACK        "30"        /**< Foreground black.            */
#define SGR_FG_RED          "31"        /**< Foreground red.              */
#define SGR_FG_GREEN        "32"        /**< Foreground green.            */
#define SGR_FG_YELLOW       "33"        /**< Foreground yellow.           */
#define SGR_FG_BLUE         "34"        /**< Foreground blue.             */
#define SGR_FG_MAGENTA      "35"        /**< Foreground magenta.          */
#define SGR_FG_CYAN         "36"        /**< Foreground cyan.             */
#define SGR_FG_WHITE        "37"        /**< Foreground white.            */

#define SGR_BOLD            "1"         /**< Bold.                        */
#define SGR_CAP_SEP         ":"         /**< Capability separator.        */
#define SGR_SEP             ";"         /**< Attribute/RGB separator.     */

#define SGR_START           "\33[%sm"   /**< Start color sequence.        */
#define SGR_END             "\33[m"     /**< End color sequence.          */
#define SGR_EL              "\33[K"     /**< Erase in Line (EL) sequence. */

/** When to colorize default. */
#define COLOR_WHEN_DEFAULT  COLOR_NOT_FILE

/**
 * Starts printing in the predefined \a COLOR.
 *
 * @param STREAM The `FILE` to print to.
 * @param COLOR The predefined color without the `sgr_` prefix.
 *
 * @sa #SGR_END_COLOR
 */
#define SGR_START_COLOR(STREAM,COLOR) BLOCK(  \
  if ( colorize && (sgr_ ## COLOR) != NULL )  \
    FPRINTF( (STREAM), SGR_START SGR_EL, (sgr_ ## COLOR) ); )

/**
 * Ends printing in color.
 *
 * @param STREAM The `FILE` to print to.
 *
 * @sa #SGR_START_COLOR
 */
#define SGR_END_COLOR(STREAM) \
  BLOCK( if ( colorize ) FPUTS( SGR_END SGR_EL, (STREAM) ); )

/**
 * When to colorize output.
 */
enum color_when {
  COLOR_NEVER,                          // never colorize
  COLOR_ISATTY,                         // colorize only if isatty(3)
  COLOR_NOT_FILE,                       // colorize only if !ISREG stdout
  COLOR_ALWAYS                          // always colorize
};
typedef enum color_when color_when_t;

// extern constants
extern char const   COLORS_DEFAULT[];   ///< Default colors.

// extern variables
extern bool         colorize;           // dump in color?
extern char const  *sgr_start;          // start color output
extern char const  *sgr_end;            // end color output
extern char const  *sgr_offset;         // offset color
extern char const  *sgr_sep;            // separator color
extern char const  *sgr_elided;         // elided byte count color
extern char const  *sgr_hex_match;      // hex match color
extern char const  *sgr_ascii_match;    // ASCII match color

///////////////////////////////////////////////////////////////////////////////

/**
 * Parses a single SGR color and, if successful, sets the match color.
 *
 * @param sgr_color An SGR color to parse.
 * @return Returns \c true only if the value was parsed successfully.
 */
NODISCARD
bool parse_grep_color( char const *sgr_color );

/**
 * Parses and sets the sequence of grep color capabilities.
 *
 * @param capabilities The grep capabilities to parse.
 * @return Returns \c true only if at least one capability was parsed
 * successfully.
 */
NODISCARD
bool parse_grep_colors( char const *capabilities );

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param c The color_when value.
 * @return Returns \c true only if we should do color.
 */
NODISCARD
bool should_colorize( color_when_t c );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_color_H */
/* vim:set et sw=2 ts=2: */
