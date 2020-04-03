/*
**      ad -- ASCII dump
**      src/color.h
**
**      Copyright (C) 2015-2018  Paul J. Lucas
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
#include "ad.h"                         /* must go first */

// standard
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

#define SGR_BG_BLACK              "40"
#define SGR_BG_RED                "41"
#define SGR_BG_GREEN              "42"
#define SGR_BG_YELLOW             "43"
#define SGR_BG_BLUE               "44"
#define SGR_BG_MAGENTA            "45"
#define SGR_BG_CYAN               "46"
#define SGR_BG_WHITE              "47"

#define SGR_FG_BLACK              "30"
#define SGR_FG_RED                "31"
#define SGR_FG_GREEN              "32"
#define SGR_FG_YELLOW             "33"
#define SGR_FG_BLUE               "34"
#define SGR_FG_MAGENTA            "35"
#define SGR_FG_CYAN               "36"
#define SGR_FG_WHITE              "37"

#define SGR_BOLD                  "1"
#define SGR_CAP_SEP               ":"   /* capability separator */
#define SGR_SEP                   ";"   /* attribute/RGB separator */

#define COLOR_WHEN_DEFAULT        COLOR_NOT_FILE

#define COLORS_DEFAULT                                                  \
  /* byte offset  */ "bn=" SGR_FG_GREEN                     SGR_CAP_SEP \
  /* elided count */ "EC=" SGR_FG_MAGENTA                   SGR_CAP_SEP \
  /* matched both */ "MB=" SGR_BG_RED     SGR_SEP SGR_BOLD  SGR_CAP_SEP \
  /* separator    */ "se=" SGR_FG_CYAN

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
AD_WARN_UNUSED_RESULT
bool parse_grep_color( char const *sgr_color );

/**
 * Parses and sets the sequence of grep color capabilities.
 *
 * @param capabilities The grep capabilities to parse.
 * @return Returns \c true only if at least one capability was parsed
 * successfully.
 */
AD_WARN_UNUSED_RESULT
bool parse_grep_colors( char const *capabilities );

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param c The color_when value.
 * @return Returns \c true only if we should do color.
 */
AD_WARN_UNUSED_RESULT
bool should_colorize( color_when_t c );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_color_H */
/* vim:set et sw=2 ts=2: */
