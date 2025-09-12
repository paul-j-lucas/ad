/*
**      ad -- ASCII dump
**      src/dump_format.c
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
#include "symbol.h"
#include "types.h"
#include "util.h"
#include "ad_parser.h"                  /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <sysexits.h>

/// @endcond

// extern function declarations
bool ad_stmnt_exec( ad_stmnt_list_t const* );

/**
 * @addtogroup dump-group
 * @{
 */

/////////// extern functions //////////////////////////////////////////////////

/**
 * Dumps a file in a specified format.
 *
 * @param format_path The path of the format file to parse.
 */
void dump_file_format( char const *format_path ) {
  assert( format_path != NULL );
  FILE *const file = fopen( format_path, "r" );
  if ( file == NULL )
    fatal_error( EX_NOINPUT, "\"%s\": %s\n", format_path, STRERROR() );

  lexer_init();
  parser_init();
  sym_table_init();

  yyrestart( file );
  int const rv = yyparse();
  fclose( file );
  if ( unlikely( rv == 2 ) ) {
    //
    // Bison has already printed "memory exhausted" via yyerror() that doesn't
    // print a newline, so print one now.
    //
    EPUTC( '\n' );
    _Exit( EX_SOFTWARE );
  }
  if ( rv != 0 )
    exit( EX_DATAERR );

  extern ad_stmnt_list_t statement_list;
  exit( ad_stmnt_exec( &statement_list ) ? EX_OK : EX_DATAERR );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
