/*
**      ad -- ASCII dump
**      src/ad.c
**
**      Copyright (C) 1996-2024  Paul J. Lucas
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
 * Defines global variables and main().
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdlib.h>                     /* for atexit() */
#include <sys/types.h>                  /* for off_t */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern function declarations
void dump_file( void );
void dump_file_c( void );
void reverse_dump_file( void );

// extern variable definitions
off_t       fin_offset;
char const *fin_path = "-";
char const *fout_path = "-";
char const *me;
unsigned    row_bytes = ROW_BYTES_DEFAULT;

/////////// local functions ///////////////////////////////////////////////////

/**
 * Cleans up by doing:
 *  + Freeing dynamicaly allocated memory.
 * This function is called via \c atexit().
 */
static void ad_cleanup( void ) {
  free_now();
}

/////////// main //////////////////////////////////////////////////////////////

/**
 * The main entry point.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns 0 on success, non-zero on failure.
 */
int main( int argc, char const *argv[const] ) {
  me = base_name( argv[0] );
  ATEXIT( ad_cleanup );
  options_init( argc, argv );
  colors_init();

  if ( opt_c_array != C_ARRAY_NONE )
    dump_file_c();
  else if ( opt_reverse )
    reverse_dump_file();
  else
    dump_file();
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
