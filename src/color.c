/*
**      ad -- ASCII dump
**      src/color.c
**
**      Copyright (C) 2018-2025  Paul J. Lucas
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

/**
 * @file
 * Defines functions for parsing color specifications.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define COLOR_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "color.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>                      /* for isdigit() */
#include <stdbool.h>
#include <stdio.h>                      /* for fileno() */
#include <stdlib.h>                     /* for exit(), getenv() */
#include <string.h>                     /* for str...() */
#include <unistd.h>                     /* for isatty() */

#define CALL_FN(FN)               NULL, (sgr_ ## FN)
#define SET_SGR(VAR)              &(sgr_ ## VAR), NULL

/// @endcond

/**
 * @addtogroup printing-color-group
 * @{
 */

//
// Color capabilities.  Names containing Upper-case are unique to ad and upper-
// case to avoid conflict with gcc.
//
#define COLOR_CAP_CARET           "caret"
#define COLOR_CAP_BYTE_OFFSET     "bn"
#define COLOR_CAP_ERROR           "error"
#define COLOR_CAP_LOCUS           "locus"
#define COLOR_CAP_ELIDED_COUNT    "EC"
#define COLOR_CAP_MATCHED_BOTH    "MB"
#define COLOR_CAP_SEPARATOR       "se"
#define COLOR_CAP_WARNING         "warning"

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

///////////////////////////////////////////////////////////////////////////////

// extern variable definitions
char const *sgr_ascii_match;
char const *sgr_caret;
char const *sgr_elided;
char const *sgr_error;
char const *sgr_hex_match;
char const *sgr_locus;
char const *sgr_offset;
char const *sgr_sep;
char const *sgr_warning;

// local variables
static char const *color_capabilities;  ///< Parsed color capabilities.

// local functions
NODISCARD
static bool sgr_is_valid( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans-up all memory used by colors at program termination.
 *
 * @note This function is called only via **atexit**(3).
 *
 * @sa colors_init()
 */
static void colors_cleanup( void ) {
  FREE( color_capabilities );
}

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
 * Parses and sets the sequence of grep color capabilities.
 *
 * @param capabilities The SGR capabilities to parse.  It's of the form:
 *  <table border="0">
 *    <tr><td>&nbsp;</td><td>&nbsp;</td></tr>
 *    <tr>
 *      <td><i>capapilities</i></td>
 *      <td>::= <i>capability</i> [<tt>:</tt><i>capability</i>]*</td>
 *    </tr>
 *    <tr>
 *      <td><i>capability</i></td>
 *      <td>::= <i>cap-name</i><tt>=</tt><i>sgr-list</i></td>
 *    </tr>
 *    <tr>
 *      <td><i>cap-name</i></td>
 *      <td>::= [<tt>a-zA-Z-</tt>]+</td>
 *    </tr>
 *    <tr>
 *      <td><i>sgr-list</i></td>
 *      <td>::= <i>sgr</i>[<tt>;</tt><i>sgr</i>]*</td>
 *    </tr>
 *    <tr>
 *      <td><i>sgr</i></td>
 *      <td>::= [<tt>1-9</tt>][<tt>0-9</tt>]*</td>
 *    </tr>
 *    <tr><td>&nbsp;</td><td>&nbsp;</td></tr>
 *  </table>
 * where <i>sgr</i> is a [Select Graphics
 * Rendition](https://en.wikipedia.org/wiki/ANSI_escape_code#SGR) code.  An
 * example \a capabilities is: `caret=42;1:error=41;1:warning=43;1`.  If NULL,
 * does nothing.
 * @return Returns a pointer to the parsed color capabilities string only if at
 * least one capability was parsed successfully.  The caller is responsible for
 * freeing it.  Otherwise returns NULL.
 */
NODISCARD
static char const* colors_parse( char const *capabilities ) {
  if ( null_if_empty( capabilities ) == NULL )
    return NULL;

  char *const capabilities_dup = check_strdup( capabilities );
  bool set_any = false;

  static color_cap_t const COLOR_CAPS[] = {
    { "bn", SET_SGR( offset       ) },    // grep: byte offset
    { "EC", SET_SGR( elided       ) },    // elided count
    { "MA", SET_SGR( ascii_match  ) },    // matched ASCII
    { "MH", SET_SGR( hex_match    ) },    // matched hex
    { "MB", CALL_FN( set_cap_MB   ) },    // matched both
    { "mt", CALL_FN( set_cap_MB   ) },    // grep: matched text (both)
    { "se", SET_SGR( sep          ) },    // grep: separator
  };

  for ( char *next_cap = capabilities_dup, *cap_name_val;
        (cap_name_val = strsep( &next_cap, SGR_CAP_SEP )) != NULL; ) {
    char const *const cap_name = strsep( &cap_name_val, "=" );
    FOREACH_ARRAY_ELEMENT( color_cap_t, cap, COLOR_CAPS ) {
      if ( strcmp( cap_name, cap->cap_name ) == 0 ) {
        char const *const cap_value = strsep( &cap_name_val, "=" );
        if ( sgr_cap_set( cap, cap_value ) )
          set_any = true;
        break;
      }
    } // for
  } // for

  if ( set_any )
    return capabilities_dup;

  free( capabilities_dup );
  return NULL;
}

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param when The \ref color_when value.
 * @return Returns `true` only if we should do color.
 */
NODISCARD
static bool should_colorize( color_when_t when ) {
  switch ( when ) {
    case COLOR_ALWAYS:
      return true;
    case COLOR_ISATTY:
      return isatty( STDOUT_FILENO );   // emulate gcc's --color=auto
    case COLOR_NEVER:
      return false;
    case COLOR_NOT_FILE:
      //
      // Do color only we're writing either to a TTY or to a pipe (so the
      // common case of piping to less(1) will still show color) but NOT when
      // writing to a file because we don't want the escape sequences polluting
      // it.
      //
      // Results from testing using isatty(3) and fstat(3) are given in the
      // following table:
      //
      //      COMMAND   Should? isatty ISCHR ISFIFO ISREG
      //      ========= ======= ====== ===== ====== =====
      //      ad           T      T      T     F      F
      //      ad > file    F      F      F     F    >>T<<
      //      ad | less    T      F      F     T      F
      //
      // Hence, we want to do color _except_ when ISREG=T.
      //
      return !fd_is_file( STDOUT_FILENO );
  } // switch
  UNEXPECTED_INT_VALUE( when );
}

////////// extern functions ///////////////////////////////////////////////////

void colors_init( void ) {
  ASSERT_RUN_ONCE();

  if ( !should_colorize( opt_color_when ) )
    return;

  color_capabilities = colors_parse( getenv( "AD_COLORS" ) );
  if ( color_capabilities == NULL ) {
    char const COLORS_DEFAULT[] =
      COLOR_CAP_CARET         "=" SGR_FG_GREEN    SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_BYTE_OFFSET   "=" SGR_FG_GREEN                      SGR_CAP_SEP
      COLOR_CAP_ELIDED_COUNT  "=" SGR_FG_MAGENTA                    SGR_CAP_SEP
      COLOR_CAP_ERROR         "=" SGR_FG_RED      SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_LOCUS         "="                         SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_MATCHED_BOTH  "=" SGR_BG_RED      SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_SEPARATOR     "=" SGR_FG_CYAN                       SGR_CAP_SEP
      COLOR_CAP_WARNING       "=" SGR_FG_YELLOW   SGR_SEP SGR_BOLD  SGR_CAP_SEP;

    color_capabilities = colors_parse( COLORS_DEFAULT );
    assert( color_capabilities != NULL );
  }

  ATEXIT( &colors_cleanup );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
