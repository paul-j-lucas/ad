/*
**      ad -- ASCII dump
**      ad.c
**
**      Copyright (C) 1996-2015  Paul J. Lucas
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
#include "config.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdlib.h>                     /* for atexit() */
#include <string.h>                     /* for memset(), str...() */

////////// extern functions ///////////////////////////////////////////////////

extern void dump_file( void );
extern void dump_file_c( void );
extern void reverse_dump_file( void );

////////// extern variables ///////////////////////////////////////////////////

char *elided_separator;                 // separator used for elided rows

/////////// local functions ///////////////////////////////////////////////////

/**
 * Cleans up by doing:
 *  + Freeing dynamicaly allocated memory.
 *  + Closing files.
 * This function is called via \c atexit().
 */
static void clean_up( void ) {
  freelist_free();
  if ( fin )
    fclose( fin );
  if ( fout )
    fclose( fout );
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

  if ( search_buf )                     // searching for a string?
    search_len = strlen( search_buf );
  else if ( search_endian ) {           // searching for a number?
    if ( !search_len )                  // default to smallest possible size
      search_len = int_len( search_number );
    int_rearrange_bytes( &search_number, search_len, search_endian );
    search_buf = (char*)&search_number;
  }

  if ( !opt_max_bytes_to_read )         // degenerate case
    exit( search_len ? EXIT_NO_MATCHES : EXIT_SUCCESS );

  elided_separator = freelist_add( MALLOC( char, OFFSET_WIDTH + 1 /*NULL*/ ) );
  memset( elided_separator, '-', OFFSET_WIDTH );
  elided_separator[ OFFSET_WIDTH ] = '\0';
}

/////////// main //////////////////////////////////////////////////////////////

int main( int argc, char *argv[] ) {
  init( argc, argv );
  if ( opt_reverse )
    reverse_dump_file(); 
  else if ( opt_c_fmt )
    dump_file_c();
  else
    dump_file();
  assert( false );                      // none of the above functions returns
  return 0;                             // suppress warning
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
