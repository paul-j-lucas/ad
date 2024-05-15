/*
**      ad -- ASCII dump
**      src/format.c
**
**      Copyright (C) 2024  Paul J. Lucas
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
 * Defines types and functions for dumping a file in a specified format.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "lexer.h"
#include "options.h"
#include "parser.h"
#include "typedef.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <sysexits.h>

/// @endcond

/**
 * @defgroup dump-group Dumping
 * Types and functions for dumping a file.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////


////////// local functions ////////////////////////////////////////////////////

/////////// extern functions //////////////////////////////////////////////////

/**
 * Dumps a file in a specified format.
 */
void dump_file_format( void ) {
  FILE *const file = fopen( opt_format_path, "r" );
  PERROR_EXIT_IF( file == NULL, EX_NOINPUT );
  lexer_init();
  ad_typedefs_init();
  yyrestart( file );
  int const rv = yyparse();
  fclose( file );
  if ( unlikely( rv == 2 ) ) {
    //
    // Bison has already printed "memory exhausted" via yyerror() that
    // doesn't print a newline, so print one now.
    //
    EPUTC( '\n' );
    _Exit( EX_SOFTWARE );
  }

  if ( rv != 0 )
    exit( EX_DATAERR );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
