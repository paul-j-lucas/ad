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
/// @cond DOXYGEN_IGNORE
#define COLOR_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "color.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>                      /* for isdigit() */
#include <stdio.h>                      /* for fileno() */
#include <stdlib.h>                     /* for exit(), getenv() */
#include <string.h>                     /* for str...() */
#include <unistd.h>                     /* for isatty() */

//
// Color capabilities.  Names containing Upper-case are unique to ad and upper-
// case to avoid conflict with gcc.
//
#define CAP_CARET                 "caret"
#define CAP_BYTE_OFFSET           "bn"
#define CAP_ERROR                 "error"
#define CAP_LOCUS                 "locus"
#define CAP_MATCH_ASCII           "MA"
#define CAP_MATCH_HEX             "MH"
#define CAP_MATCH_BOTH            "MB"
#define CAP_ELIDED_COUNT          "EC"
#define CAP_SEPARATOR             "se"
#define CAP_WARNING               "warning"

///////////////////////////////////////////////////////////////////////////////

/**
 * Color capability used to map an AD_COLORS/GREP_COLORS "capability" either to
 * the variable to set or the function to call.
 */
struct color_cap {
  char const   *cap_name;               ///< Capability name.
  char const  **cap_var_to_set;         ///< Pointer to variable to set.
  void        (*cap_func)(char const*); ///< Pointer to function to call.
};
typedef struct color_cap color_cap_t;

// extern constant definitions
char const  COLORS_DEFAULT[] =
  CAP_CARET         "=" SGR_FG_GREEN    SGR_SEP SGR_BOLD  SGR_CAP_SEP
  CAP_BYTE_OFFSET   "=" SGR_FG_GREEN                      SGR_CAP_SEP
  CAP_ELIDED_COUNT  "=" SGR_FG_MAGENTA                    SGR_CAP_SEP
  CAP_ERROR         "=" SGR_FG_RED      SGR_SEP SGR_BOLD  SGR_CAP_SEP
  CAP_LOCUS         "="                         SGR_BOLD  SGR_CAP_SEP
  CAP_MATCH_BOTH    "=" SGR_BG_RED      SGR_SEP SGR_BOLD  SGR_CAP_SEP
  CAP_SEPARATOR     "=" SGR_FG_CYAN
  CAP_WARNING       "=" SGR_FG_YELLOW   SGR_SEP SGR_BOLD  SGR_CAP_SEP;

// extern variable definitions
bool        colorize;
char const *sgr_caret;
char const *sgr_start = SGR_START SGR_EL;
char const *sgr_end   = SGR_END SGR_EL;
char const *sgr_error;
char const *sgr_offset;
char const *sgr_sep;
char const *sgr_elided;
char const *sgr_hex_match;
char const *sgr_ascii_match;

// local functions
NODISCARD
static bool sgr_is_valid( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Sets the SGR color for the given capability.
 *
 * @param cap The color capability to set the color for.
 * @param sgr_color The SGR color to set; or null or empty to unset.
 * @return Returns `true` only if \a sgr_color is valid.
 */
NODISCARD
static bool sgr_cap_set( color_cap_t const *cap, char const *sgr_color ) {
  assert( cap != NULL );
  assert( cap->cap_var_to_set != NULL || cap->cap_func != NULL );

  if ( sgr_color != NULL ) {
    if ( sgr_color[0] == '\0' )         // empty string -> NULL = unset
      sgr_color = NULL;
    else if ( !sgr_is_valid( sgr_color ) )
      return false;
  }
  if ( cap->cap_var_to_set != NULL )
    *cap->cap_var_to_set = sgr_color;
  else
    (*cap->cap_func)( sgr_color );
  return true;
}

/**
 * Parses an SGR (Select Graphic Rendition) value that matches the regular
 * expression of `n(;n)*` or a semicolon-separated list of integers in the
 * range 0-255.
 *
 * @param sgr_color The null-terminated allegedly SGR string to parse.
 * @return Returns `true` only if \a sgr_color contains a valid SGR value.
 *
 * @sa [ANSI escape code](http://en.wikipedia.org/wiki/ANSI_escape_code)
 */
NODISCARD
static bool sgr_is_valid( char const *sgr_color ) {
  if ( sgr_color == NULL )
    return false;
  for (;;) {
    if ( unlikely( !isdigit( *sgr_color ) ) )
      return false;
    char *end;
    errno = 0;
    unsigned long long const n = strtoull( sgr_color, &end, 10 );
    if ( unlikely( errno || n > 255 ) )
      return false;
    switch ( *end ) {
      case '\0':
        return true;
      case ';':
        sgr_color = end + 1;
        continue;
      default:
        return false;
    } // switch
  } // for
}

/**
 * Sets both the hex and ASCII match color.
 * (This function is needed for the color capabilities table to support the
 * "MB" and "mt" capabilities.)
 *
 * @param sgr_color The SGR color to set; or null or empty to unset.
 */
static void sgr_set_cap_MB( char const *sgr_color ) {
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
static void sgr_set_cap_ne( char const *sgr_color ) {
  (void)sgr_color;                      // suppress warning
  sgr_start = SGR_START;
  sgr_end   = SGR_END;
}

#define CALL_FN(FN)   NULL, (sgr_ ## FN)
#define SET_SGR(VAR)  &(sgr_ ## VAR), NULL

/**
 * Color capabilities table.
 */
static color_cap_t const COLOR_CAPS[] = {
  { CAP_BYTE_OFFSET,  SET_SGR( offset       ) },
  { CAP_CARET,        SET_SGR( caret        ) },
  { CAP_ELIDED_COUNT, SET_SGR( elided       ) },
  { CAP_MATCH_ASCII,  SET_SGR( ascii_match  ) },
  { CAP_MATCH_HEX,    SET_SGR( hex_match    ) },
  { CAP_MATCH_BOTH,   CALL_FN( set_cap_MB   ) },
  { "mt",             CALL_FN( set_cap_MB   ) },  // grep: matched text (both)
  { CAP_SEPARATOR,    SET_SGR( sep          ) },
  { "ne",             CALL_FN( set_cap_ne   ) },  // grep: no EL on SGR
  { NULL,             NULL, NULL              }
};

////////// extern functions ///////////////////////////////////////////////////

bool parse_grep_color( char const *sgr_color ) {
  if ( sgr_is_valid( sgr_color ) ) {
    sgr_set_cap_MB( sgr_color );
    return true;
  }
  return false;
}

bool parse_grep_colors( char const *capabilities ) {
  bool set_something = false;

  if ( capabilities != NULL ) {
    // free this later since the sgr_* variables point to substrings
    char *next_cap = (char*)free_later( check_strdup( capabilities ) );

    for ( char *cap_name_val;
          (cap_name_val = strsep( &next_cap, ":" )) != NULL; ) {
      char const *const cap_name = strsep( &cap_name_val, "=" );
      for ( color_cap_t const *cap = COLOR_CAPS; cap->cap_name; ++cap ) {
        if ( strcmp( cap_name, cap->cap_name ) == 0 ) {
          char const *const cap_value = strsep( &cap_name_val, "=" );
          if ( sgr_cap_set( cap, cap_value ) )
            set_something = true;
          break;
        }
      } // for
    } // for
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
