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

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#include "lexer.h"
#include "options.h"
#include "typedef.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>                     /* for atexit() */
#include <string.h>                     /* for memset(), str...() */
#include <sys/types.h>                  /* for off_t */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// extern function declarations
_Noreturn void dump_file( void );
_Noreturn void dump_file_c( void );
_Noreturn void reverse_dump_file( void );

// extern variable definitions
off_t       fin_offset;
char const *fin_path = "-";
char const *fout_path = "-";
char const *me;
size_t      row_bytes = ROW_BYTES_DEFAULT;

/////////// local functions ///////////////////////////////////////////////////

/**
 * Cleans up by doing:
 *  + Freeing dynamicaly allocated memory.
 *  + Closing files.
 * This function is called via \c atexit().
 */
static void ad_cleanup( void ) {
  free_now();
}

/**
 * Performs initialization by doing:
 *  + Parsing command-line options.
 *  + Initializing search variables.
 *  + Initializing the elided separator.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 */
static void ad_init( int argc, char const *argv[const] ) {
  me = base_name( argv[0] );
  ATEXIT( ad_cleanup );
  parse_options( argc, argv );
  colors_init();
  lexer_init();
  ad_typedefs_init();

  if ( opt_search_buf != NULL ) {       // searching for a string?
    opt_search_len = strlen( opt_search_buf );
  }
  else if ( opt_search_endian != ENDIAN_NONE ) {  // searching for a number?
    if ( opt_search_len == 0 )          // default to smallest possible size
      opt_search_len = int_len( opt_search_number );
    int_rearrange_bytes(
      &opt_search_number, opt_search_len, opt_search_endian
    );
    opt_search_buf = POINTER_CAST( char*, &opt_search_number );
  }

  if ( opt_max_bytes == 0 )             // degenerate case
    exit( opt_search_len > 0 ? EX_NO_MATCHES : EX_OK );
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
  ad_init( argc, argv );
  if ( opt_reverse )
    reverse_dump_file(); 
  else if ( opt_c_fmt != CFMT_NONE )
    dump_file_c();
  else
    dump_file();
  unreachable();                        // none of the above functions returns
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
