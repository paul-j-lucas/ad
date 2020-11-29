/*
**      ad -- ASCII dump
**      src/ad.c
**
**      Copyright (C) 1996-2020  Paul J. Lucas
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
#include "common.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdlib.h>                     /* for atexit() */
#include <string.h>                     /* for memset(), str...() */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// extern function declarations
extern void dump_file( void );
extern void dump_file_c( void );
extern void reverse_dump_file( void );

// extern variable definitions
size_t row_bytes = ROW_BYTES_DEFAULT;

/////////// local functions ///////////////////////////////////////////////////

/**
 * Cleans up by doing:
 *  + Freeing dynamicaly allocated memory.
 *  + Closing files.
 * This function is called via \c atexit().
 */
static void clean_up( void ) {
  free_now();
  if ( fin != NULL )
    (void)fclose( fin );
  if ( fout != NULL )
    (void)fclose( fout );
}

/**
 * Performs initialization by doing:
 *  + Parsing command-line options.
 *  + Initializing search variables.
 *  + Initializing the elided separator.
 */
static void init( int argc, char *argv[] ) {
  atexit( clean_up );
  parse_options( argc, argv );

  if ( search_buf != NULL )             // searching for a string?
    search_len = strlen( search_buf );
  else if ( search_endian ) {           // searching for a number?
    if ( search_len == 0 )              // default to smallest possible size
      search_len = int_len( search_number );
    int_rearrange_bytes( &search_number, search_len, search_endian );
    search_buf = REINTERPRET_CAST(char*, &search_number);
  }

  if ( opt_max_bytes == 0 )             // degenerate case
    exit( search_len ? EX_NO_MATCHES : EX_OK );
}

/////////// main //////////////////////////////////////////////////////////////

int main( int argc, char *argv[] ) {
  init( argc, argv );
  if ( opt_reverse )
    reverse_dump_file(); 
  else if ( opt_c_fmt != 0 )
    dump_file_c();
  else
    dump_file();
  assert( false );                      // none of the above functions returns
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
