/*
**      ad -- ASCII dump
**      color.h
**
**      Copyright (C) 2015  Paul J. Lucas
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

#ifndef ad_color_H
#define ad_color_H

// local
#include "util.h"                       /* for bool */

///////////////////////////////////////////////////////////////////////////////

#define DEFAULT_COLORS  "bn=32:EC=35:MB=41;1:se=36"

/**
 * When to colorize output.
 */
enum colorization {
  COLOR_NEVER,                          // never colorize
  COLOR_ISATTY,                         // colorize only if isatty(3)
  COLOR_NOT_FILE,                       // colorize only if !ISREG stdout
  COLOR_ALWAYS                          // always colorize
};
typedef enum colorization colorization_t;

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
bool parse_grep_color( char const *sgr_color );

/**
 * Parses and sets the sequence of grep color capabilities.
 *
 * @param capabilities The grep capabilities to parse.
 * @return Returns \c true only if at least one capability was parsed
 * successfully.
 */
bool parse_grep_colors( char const *capabilities );

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param c The colorization value.
 * @return Returns \c true only if we should do color.
 */
bool should_colorize( colorization_t c );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_color_H */
/* vim:set et sw=2 ts=2: */
