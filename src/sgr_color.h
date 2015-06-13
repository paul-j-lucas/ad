/*
**      ad -- ASCII dump
**      color.h: implementation
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

#ifndef pjl_color_H
#define pjl_color_H

/* standard */
#include <stdio.h>

/*****************************************************************************/

/*
 * SGR Colors
 *
 * SGR = Select Graphic Rendition
 *     = Set Graphics Mode
 *
 * See: <http://en.wikipedia.org/wiki/ANSI_escape_code#graphics>
 */

#define SGR_FG_COLOR_BLACK          "30"
#define SGR_FG_COLOR_RED            "31"
#define SGR_FG_COLOR_GREEN          "32"
#define SGR_FG_COLOR_YELLOW         "33"
#define SGR_FG_COLOR_BLUE           "34"
#define SGR_FG_COLOR_MAGENTA        "35"
#define SGR_FG_COLOR_CYAN           "36"
#define SGR_FG_COLOR_WHITE          "37"

#define SGR_FG_COLOR_BRIGHT_BLACK   SGR_FG_COLOR_BLACK    ";1"
#define SGR_FG_COLOR_BRIGHT_RED     SGR_FG_COLOR_RED      ";1"
#define SGR_FG_COLOR_BRIGHT_GREEN   SGR_FG_COLOR_GREEN    ";1"
#define SGR_FG_COLOR_BRIGHT_YELLOW  SGR_FG_COLOR_YELLOW   ";1"
#define SGR_FG_COLOR_BRIGHT_BLUE    SGR_FG_COLOR_BLUE     ";1"
#define SGR_FG_COLOR_BRIGHT_MAGENTA SGR_FG_COLOR_MAGENTA  ";1"
#define SGR_FG_COLOR_BRIGHT_CYAN    SGR_FG_COLOR_CYAN     ";1"
#define SGR_FG_COLOR_BRIGHT_WHITE   SGR_FG_COLOR_WHITE    ";1"

#define SGR_BG_COLOR_BLACK          "40"
#define SGR_BG_COLOR_RED            "41"
#define SGR_BG_COLOR_GREEN          "42"
#define SGR_BG_COLOR_YELLOW         "43"
#define SGR_BG_COLOR_BLUE           "44"
#define SGR_BG_COLOR_MAGENTA        "45"
#define SGR_BG_COLOR_CYAN           "46"
#define SGR_BG_COLOR_WHITE          "47"

#define SGR_BG_COLOR_BRIGHT_BLACK   SGR_BG_COLOR_BLACK    ";1"
#define SGR_BG_COLOR_BRIGHT_RED     SGR_BG_COLOR_RED      ";1"
#define SGR_BG_COLOR_BRIGHT_GREEN   SGR_BG_COLOR_GREEN    ";1"
#define SGR_BG_COLOR_BRIGHT_YELLOW  SGR_BG_COLOR_YELLOW   ";1"
#define SGR_BG_COLOR_BRIGHT_BLUE    SGR_BG_COLOR_BLUE     ";1"
#define SGR_BG_COLOR_BRIGHT_MAGENTA SGR_BG_COLOR_MAGENTA  ";1"
#define SGR_BG_COLOR_BRIGHT_CYAN    SGR_BG_COLOR_CYAN     ";1"
#define SGR_BG_COLOR_BRIGHT_WHITE   SGR_BG_COLOR_WHITE    ";1"

#define SGR_START_FMT               "\33[%sm\33[K"
#define SGR_END_FMT                 "\33[m\33[K"

#define SGR_START_COLOR(COLOR)      printf( SGR_START_FMT, COLOR )
#define SGR_END_COLOR()             printf( SGR_END_FMT )

/*****************************************************************************/

#endif /* pjl_color_H */
/* vim:set et sw=2 ts=2: */
