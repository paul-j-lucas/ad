/*
**      ad -- ASCII dump
**      color.c
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

// local
#include "color.h"
#include "options.h"

// system
#include <assert.h>
#include <stdlib.h>                     /* for exit(), getenv() */
#include <string.h>                     /* for str...() */
#include <unistd.h>                     /* for isatty() */

///////////////////////////////////////////////////////////////////////////////

#define SGR_START   "\33[%sm"           /* start color sequence */
#define SGR_END     "\33[m"             /* end color sequence */
#define SGR_EL      "\33[K"             /* Erase in Line (EL) sequence */

/**
 * Color capability used to map an AD_COLORS/GREP_COLORS "capability" either to
 * the variable to set or the function to call.
 */
struct color_cap {
  char const *cap_name;                 // capability name
  char const **cap_var_to_set;          // variable to set ...
  void (*cap_func)( char const* );      // ... OR function to call
};
typedef struct color_cap color_cap_t;

/////////// extern variables //////////////////////////////////////////////////

bool        colorize;
char const *sgr_start = SGR_START SGR_EL;
char const *sgr_end   = SGR_END SGR_EL;
char const *sgr_offset;
char const *sgr_sep;
char const *sgr_elided;
char const *sgr_hex_match;
char const *sgr_ascii_match;

/////////// local functions ///////////////////////////////////////////////////

/**
 * Sets the SGR color for the given capability.
 *
 * @param cap The color capability to set the color for.
 * @param sgr_color The SGR color to set or empty or NULL to unset.
 * @return Returns \c true only if \a sgr_color is valid.
 */
static bool cap_set( color_cap_t const *cap, char const *sgr_color ) {
  assert( cap );
  assert( cap->cap_var_to_set || cap->cap_func );

  if ( sgr_color ) {
    if ( !*sgr_color )                  // empty string -> NULL = unset
      sgr_color = NULL;
    else if ( !parse_sgr( sgr_color ) )
      return false;
  }
  if ( cap->cap_var_to_set )
    *cap->cap_var_to_set = sgr_color;
  else
    (*cap->cap_func)( sgr_color );
  return true;
}

/**
 * Sets both the hex and ASCII match color.
 * (This function is needed for the color capabilities table to support the
 * "MB" and "mt" capabilities.)
 *
 * @param sgr_color The SGR color to set or empty or NULL to unset.
 */
static void cap_MB( char const *sgr_color ) {
  if ( !*sgr_color )                    // empty string -> NULL = unset
    sgr_color = NULL;
  sgr_ascii_match = sgr_hex_match = sgr_color;
}

/**
 * Turns off using the EL (Erase in Line) sequence.
 * (This function is needed for the color capabilities table to support the
 * "ne" capability.)
 *
 * @param sgr_color Not used.
 */
static void cap_ne( char const *sgr_color ) {
  (void)sgr_color;                      // suppress warning
  sgr_start = SGR_START;
  sgr_end   = SGR_END;
}

/**
 * Color capabilities table.  Upper-case names are unique to us and upper-case
 * to avoid conflict with grep.  Lower-case names are for grep compatibility.
 */
static color_cap_t const color_caps[] = {
  { "bn", &sgr_offset,      NULL   },   // grep: byte offset
  { "EC", &sgr_elided,      NULL   },   // elided count
  { "MA", &sgr_ascii_match, NULL   },   // matched ASCII
  { "MH", &sgr_hex_match,   NULL   },   // matched hex
  { "MB", NULL,             cap_MB },   // matched both
  { "mt", NULL,             cap_MB },   // grep: matched text (both)
  { "se", &sgr_sep,         NULL   },   // grep: separator
  { "ne", NULL,             cap_ne },   // grep: no EL on SGR
  { NULL, NULL,             NULL   }
};

////////// extern functions ///////////////////////////////////////////////////

bool parse_grep_color( char const *sgr_color ) {
  if ( parse_sgr( sgr_color ) ) {
    cap_MB( sgr_color );
    return true;
  }
  return false;
}

bool parse_grep_colors( char const *capabilities ) {
  bool set_something = false;

  if ( capabilities ) {
    // free this later since the sgr_* variables point to substrings
    char *const capabilities_dup = freelist_add( check_strdup( capabilities ) );
    char *next_cap = capabilities_dup;
    char *cap_name_val;

    while ( (cap_name_val = strsep( &next_cap, ":" )) ) {
      char const *const cap_name = strsep( &cap_name_val, "=" );
      for ( color_cap_t const *cap = color_caps; cap->cap_name; ++cap ) {
        if ( strcmp( cap_name, cap->cap_name ) == 0 ) {
          char const *const cap_value = strsep( &cap_name_val, "=" );
          if ( cap_set( cap, cap_value ) )
            set_something = true;
          break;
        }
      } // for
    } // while
  }
  return set_something;
}

bool should_colorize( colorization_t c ) {
  switch ( c ) {                        // handle easy cases
    case COLOR_ALWAYS: return true;
    case COLOR_NEVER : return false;
    default          : break;
  } // switch

  //
  // If TERM is unset, empty, or "dumb", color probably won't work.
  //
  char const *const term = getenv( "TERM" );
  if ( !term || !*term || strcmp( term, "dumb" ) == 0 )
    return false;

  int const fd_out = fileno( fout );
  if ( c == COLOR_ISATTY )              // emulate grep's --color=auto
    return isatty( fd_out );

  assert( c == COLOR_NOT_FILE );
  //
  // Otherwise we want to do color only we're writing either to a TTY or to a
  // pipe (so the common case of piping to less(1) will still show color) but
  // NOT when writing to a file because we don't want the escape sequences
  // polluting it.
  //
  // Results from testing using isatty(3) and fstat(3) are given in the
  // following table:
  //
  //    COMMAND   Should? isatty ISCHR ISFIFO ISREG
  //    ========= ======= ====== ===== ====== =====
  //    ad           T      T      T     F      F
  //    ad > file    F      F      F     F    >>T<<
  //    ad | less    T      F      F     T      F
  //
  // Hence, we want to do color _except_ when ISREG=T.
  //
  return !is_file( fd_out );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
