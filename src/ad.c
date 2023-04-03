/*
**      ad -- ASCII dump
**      src/ad.c
**
**      Copyright (C) 1996-2021  Paul J. Lucas
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
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>                     /* for atexit() */
#include <stdnoreturn.h>
#include <string.h>                     /* for memset(), str...() */
#include <sys/types.h>                  /* for off_t */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// extern function declarations
noreturn void dump_file( void );
noreturn void dump_file_c( void );
noreturn void reverse_dump_file( void );

// extern variable definitions
FILE       *fin;
off_t       fin_offset;
char const *fin_path = "-";
FILE       *fout;
char const *fout_path = "-";
char const *me;
size_t      row_bytes = ROW_BYTES_DEFAULT;
char       *search_buf;
endian_t    search_endian;
size_t      search_len;
uint64_t    search_number;

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
    PJL_IGNORE_RV( fclose( fin ) );
  if ( fout != NULL )
    PJL_IGNORE_RV( fclose( fout ) );
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
static void init( int argc, char const *argv[const] ) {
  me = base_name( argv[0] );
  check_atexit( clean_up );
  parse_options( argc, argv );

  if ( search_buf != NULL )             // searching for a string?
    search_len = strlen( search_buf );
  else if ( search_endian ) {           // searching for a number?
    if ( search_len == 0 )              // default to smallest possible size
      search_len = int_len( search_number );
    int_rearrange_bytes( &search_number, search_len, search_endian );
    search_buf = POINTER_CAST( char*, &search_number );
  }

  if ( opt_max_bytes == 0 )             // degenerate case
    exit( search_len ? EX_NO_MATCHES : EX_OK );
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
  init( argc, argv );
  if ( opt_reverse )
    reverse_dump_file(); 
  else if ( opt_c_fmt != 0 )
    dump_file_c();
  else
    dump_file();
  unreachable();                        // none of the above functions returns
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
