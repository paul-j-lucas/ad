/*
**      ad -- ASCII dump
**      src/dump.c
**
**      Copyright (C) 2022  Paul J. Lucas
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
 * Defines functions for dumping types for debugging.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "debug.h"
#include "ad.h"
#include "types.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#define DUMP_COMMA \
  BLOCK( if ( false_set( &comma ) ) FPUTS( ",\n", dout ); )

#define DUMP_FORMAT(...) BLOCK( \
  fprint_spaces( dout, indent * DUMP_INDENT ); FPRINTF( dout, __VA_ARGS__ ); )

#define DUMP_LOC(KEY,LOC)   \
  DUMP_FORMAT( KEY " = " ); \
  ad_loc_dump( (LOC), dout )

#define DUMP_STR(KEY,VALUE) BLOCK(  \
  DUMP_FORMAT( KEY " = " );         \
  str_dump( (VALUE), dout ); )

#define DUMP_TYPE(TYPE) BLOCK(  \
  DUMP_FORMAT( "type = " );     \
  ad_type_dump( (TYPE), dout ); )

/// @endcond

// local functions
static void ad_loc_dump( ad_loc_t const*, FILE* );

// local constants
static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Prints \a n spaces.
 *
 * @param out The `FILE` to print to.
 * @param n The number of spaces to print.
 */
static inline void fprint_spaces( FILE *out, unsigned n ) {
  FPRINTF( out, "%*s", STATIC_CAST( int, n ), "" );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for ad_expr_dump().
 *
 * @param expr The expression to dump.  If NULL and \a key is not NULL, dumps
 * only \a key followed by `=&nbsp;NULL`.
 * @param indent The current indent.
 * @param key If not NULL, prints \a key followed by ` = ` before dumping the
 * value of \a expr.
 * @param dout The `FILE` to dump to.
 *
 * @sa ad_expr_list_dump()
 */
static void ad_expr_dump_impl( ad_expr_t const *expr, unsigned indent,
                               char const *key, FILE *dout ) {
  assert( dout != NULL );
  bool const has_key = key != NULL && key[0] != '\0';

  if ( has_key )
    DUMP_FORMAT( "%s = ", key );

  if ( expr == NULL ) {
    FPUTS( "NULL", dout );
    return;
  }

  if ( has_key )
    FPUTS( "{\n", dout );
  else
    DUMP_FORMAT( "{\n" );

  ++indent;

  //DUMP_SNAME( "sname", &ast->sname );
  FPUTS( ",\n", dout );
  DUMP_LOC( "loc", &expr->loc );
  FPUTS( ",\n", dout );

  bool comma = false;


  FPUTC( '\n', dout );
  --indent;
  DUMP_FORMAT( "}" );
}

/**
 * Dumps \a loc (for debugging).
 *
 * @param loc The location to dump.
 * @param dout The `FILE` to dump to.
 */
static void ad_loc_dump( ad_loc_t const *loc, FILE *dout ) {
  assert( loc != NULL );
  assert( dout != NULL );
  FPRINTF( dout, "%d", loc->first_column );
  if ( loc->last_column != loc->first_column )
    FPRINTF( dout, "-%d", loc->last_column );
}

////////// extern functions ///////////////////////////////////////////////////

void bool_dump( bool value, FILE *dout ) {
  assert( dout != NULL );
  FPUTS( value ? "true" : "false", dout );
}

void ad_expr_dump( ad_expr_t const *ast, char const *key, FILE *dout ) {
  ad_expr_dump_impl( ast, 1, key, dout );
}

void ad_tid_dump( ad_tid_t tid, FILE *dout ) {
  assert( dout != NULL );
#if 0
  FPRINTF( dout,
    "\"%s\" (%s = 0x%" PRIX_C_TID_T ")",
    c_tid_name_c( tid ), c_tpid_name( c_tid_tpid( tid ) ), tid
  );
#endif
}

void ad_tid_name( ad_type_t const *type, FILE *dout ) {
  switch ( type->tid ) {
    case T_BOOL:
      FPUTS( "bool", dout );
      break;
    case T_INT:
      if ( !ad_is_signed( type->tid ) )
        FPUTS( "unsigned ", dout );
      FPUTS( "int", dout );
      break;
    case T_FLOAT:
      FPUTS( "float", dout );
      break;
  } // switch
}

void ad_type_dump( ad_type_t const *type, FILE *dout ) {
  assert( type != NULL );
  assert( dout != NULL );

}

void str_dump( char const *value, FILE *dout ) {
  assert( dout != NULL );
  if ( value == NULL )
    FPUTS( "null", dout );
  else
    FPRINTF( dout, "\"%s\"", value );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
