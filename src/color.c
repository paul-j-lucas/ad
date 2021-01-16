/*
**      ad -- ASCII dump
**      src/color.c
**
**      Copyright (C) 2018-2021  Paul J. Lucas
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

// local
#include "pjl_config.h"                 /* must go first */
#include "color.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdio.h>                      /* for fileno() */
#include <stdlib.h>                     /* for exit(), getenv() */
#include <string.h>                     /* for str...() */
#include <unistd.h>                     /* for isatty() */

///////////////////////////////////////////////////////////////////////////////

// local constant definitions
#define SGR_START "\33[%sm"             /* start color sequence */
#define SGR_END   "\33[m"               /* end color sequence */
#define SGR_EL    "\33[K"               /* Erase in Line (EL) sequence */

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

// extern variable definitions
bool        colorize;
char const *sgr_start = SGR_START SGR_EL;
char const *sgr_end   = SGR_END SGR_EL;
char const *sgr_offset;
char const *sgr_sep;
char const *sgr_elided;
char const *sgr_hex_match;
char const *sgr_ascii_match;

////////// local functions ////////////////////////////////////////////////////

/**
 * Sets the SGR color for the given capability.
 *
 * @param cap The color capability to set the color for.
 * @param sgr_color The SGR color to set; or null or empty to unset.
 * @return Returns \c true only if \a sgr_color is valid.
 */
PJL_WARN_UNUSED_RESULT
static bool cap_set( color_cap_t const *cap, char const *sgr_color ) {
  assert( cap != NULL );
  assert( cap->cap_var_to_set != NULL || cap->cap_func != NULL );

  if ( sgr_color != NULL ) {
    if ( sgr_color[0] == '\0' )         // empty string -> NULL = unset
      sgr_color = NULL;
    else if ( !parse_sgr( sgr_color ) )
      return false;
  }
  if ( cap->cap_var_to_set != NULL )
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
 * @param sgr_color The SGR color to set; or null or empty to unset.
 */
static void cap_MB( char const *sgr_color ) {
  assert( sgr_color != NULL );
  if ( sgr_color[0] == '\0' )           // empty string -> NULL = unset
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

#define CALL_FN(FN)   NULL, (FN)
#define SET_SGR(VAR)  &(sgr_ ## VAR), NULL

/**
 * Color capabilities table.  Upper-case names are unique to us and upper-case
 * to avoid conflict with grep.  Lower-case names are for grep compatibility.
 */
static color_cap_t const COLOR_CAPS[] = {
  { "bn", SET_SGR( offset       ) },    // grep: byte offset
  { "EC", SET_SGR( elided       ) },    // elided count
  { "MA", SET_SGR( ascii_match  ) },    // matched ASCII
  { "MH", SET_SGR( hex_match    ) },    // matched hex
  { "MB", CALL_FN( cap_MB       ) },    // matched both
  { "mt", CALL_FN( cap_MB       ) },    // grep: matched text (both)
  { "se", SET_SGR( sep          ) },    // grep: separator
  { "ne", CALL_FN( cap_ne       ) },    // grep: no EL on SGR
  { NULL, NULL, NULL              }
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

  if ( capabilities != NULL ) {
    // free this later since the sgr_* variables point to substrings
    char *next_cap = (char*)free_later( check_strdup( capabilities ) );
    char *cap_name_val;

    while ( (cap_name_val = strsep( &next_cap, ":" )) != NULL ) {
      char const *const cap_name = strsep( &cap_name_val, "=" );
      for ( color_cap_t const *cap = COLOR_CAPS; cap->cap_name; ++cap ) {
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

bool should_colorize( color_when_t when ) {
  switch ( when ) {                     // handle easy cases
    case COLOR_ALWAYS: return true;
    case COLOR_NEVER : return false;
    default          : break;
  } // switch

  //
  // If TERM is unset, empty, or "dumb", color probably won't work.
  //
  char const *const term = getenv( "TERM" );
  if ( term == NULL || term[0] == '\0' || strcmp( term, "dumb" ) == 0 )
    return false;

  int const fd_out = fileno( fout );
  if ( when == COLOR_ISATTY )           // emulate grep's --color=auto
    return isatty( fd_out );

  assert( when == COLOR_NOT_FILE );
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
