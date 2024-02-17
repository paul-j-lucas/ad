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
#include "expr.h"
#include "types.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#define DUMP_FORMAT(...) BLOCK( \
  FPUTNSP( indent * DUMP_INDENT, fout ); FPRINTF( fout, __VA_ARGS__ ); )

#define DUMP_KEY(...) BLOCK( \
  fput_sep( ",\n", &comma, fout ); DUMP_FORMAT( __VA_ARGS__ ); )

#define DUMP_LOC(KEY,LOC) \
  DUMP_KEY( KEY ": " ); ad_loc_dump( (LOC), fout )

#define DUMP_STR(KEY,VALUE) BLOCK( \
  DUMP_KEY( KEY ": " ); str_dump( (VALUE), fout ); )

#define DUMP_TYPE(TYPE) BLOCK( \
  DUMP_KEY( "type: " ); ad_type_dump( (TYPE), fout ); )

/// @endcond

// local functions
static void ad_loc_dump( ad_loc_t const*, FILE* );

// local constants
static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for ad_expr_dump().
 *
 * @param expr The expression to dump.  If NULL and \a key is not NULL, dumps
 * only \a key followed by `=&nbsp;NULL`.
 * @param indent The current indent.
 * @param key If not NULL, prints \a key followed by ` = ` before dumping the
 * value of \a expr.
 * @param fout The `FILE` to dump to.
 *
 * @sa ad_expr_list_dump()
 */
static void ad_expr_dump_impl( ad_expr_t const *expr, unsigned indent,
                               char const *key, FILE *fout ) {
  assert( fout != NULL );
  bool const has_key = key != NULL && key[0] != '\0';

  if ( has_key )
    DUMP_FORMAT( "%s = ", key );

  if ( expr == NULL ) {
    FPUTS( "NULL", fout );
    return;
  }

  if ( has_key )
    FPUTS( "{\n", fout );
  else
    DUMP_FORMAT( "{\n" );

  bool comma = false;
  ++indent;

  //DUMP_SNAME( "sname", &ast->sname );
  FPUTS( ",\n", fout );
  DUMP_STR( "kind", ad_expr_kind_name( expr->expr_kind ) );
  FPUTS( ",\n", fout );
  DUMP_LOC( "loc", &expr->loc );
  FPUTS( ",\n", fout );

  switch ( expr->expr_kind ) {
    case AD_EXPR_NONE:
    case AD_EXPR_ERROR:
      break;

    case AD_EXPR_VALUE:
      break;

    // unary
    case AD_EXPR_BIT_COMPL:
    case AD_EXPR_MATH_NEG:
    case AD_EXPR_PTR_ADDR:
    case AD_EXPR_PTR_DEREF:
    case AD_EXPR_MATH_DEC_POST:
    case AD_EXPR_MATH_DEC_PRE:
    case AD_EXPR_MATH_INC_POST:
    case AD_EXPR_MATH_INC_PRE:
    case AD_EXPR_SIZEOF:
      ad_expr_dump_impl( expr->unary.sub_expr, indent, "sub_expr", fout );
      break;

    // binary
    case AD_EXPR_ARRAY:
    case AD_EXPR_ASSIGN:
    case AD_EXPR_BIT_AND:
    case AD_EXPR_BIT_OR:
    case AD_EXPR_BIT_SHIFT_LEFT:
    case AD_EXPR_BIT_SHIFT_RIGHT:
    case AD_EXPR_BIT_XOR:
    case AD_EXPR_CAST:
    case AD_EXPR_LOG_AND:
    case AD_EXPR_LOG_NOT:
    case AD_EXPR_LOG_OR:
    case AD_EXPR_LOG_XOR:
    case AD_EXPR_MATH_ADD:
    case AD_EXPR_MATH_DIV:
    case AD_EXPR_MATH_MOD:
    case AD_EXPR_MATH_MUL:
    case AD_EXPR_MATH_SUB:
    case AD_EXPR_REL_EQ:
    case AD_EXPR_REL_GREATER:
    case AD_EXPR_REL_GREATER_EQ:
    case AD_EXPR_REL_LESS:
    case AD_EXPR_REL_LESS_EQ:
    case AD_EXPR_REL_NOT_EQ:
      ad_expr_dump_impl( expr->binary.lhs_expr, indent, "lhs_expr", fout );
      FPUTS( ",\n", fout );
      ad_expr_dump_impl( expr->binary.rhs_expr, indent, "rhs_expr", fout );
      break;

    // ternary
    case AD_EXPR_IF_ELSE:
      ad_expr_dump_impl( expr->ternary.cond_expr, indent, "cond_expr", fout );
      FPUTS( ",\n", fout );
      ad_expr_dump_impl( expr->ternary.sub_expr[0], indent, "sub_expr[0]", fout );
      FPUTS( ",\n", fout );
      ad_expr_dump_impl( expr->ternary.sub_expr[1], indent, "sub_expr[1]", fout );
      break;
  } // switch

  FPUTC( '\n', fout );
  --indent;
  DUMP_FORMAT( "}" );
}

/**
 * Dumps \a loc (for debugging).
 *
 * @param loc The location to dump.
 * @param fout The `FILE` to dump to.
 */
static void ad_loc_dump( ad_loc_t const *loc, FILE *fout ) {
  assert( loc != NULL );
  assert( fout != NULL );
  FPRINTF( fout, "%d", loc->first_column );
  if ( loc->last_column != loc->first_column )
    FPRINTF( fout, "-%d", loc->last_column );
}

////////// extern functions ///////////////////////////////////////////////////

void bool_dump( bool value, FILE *fout ) {
  assert( fout != NULL );
  FPUTS( value ? "true" : "false", fout );
}

void ad_expr_dump( ad_expr_t const *ast, char const *key, FILE *fout ) {
  ad_expr_dump_impl( ast, 1, key, fout );
}

void ad_tid_dump( ad_tid_t tid, FILE *fout ) {
  assert( fout != NULL );
  switch ( tid ) {
    case T_BOOL:
      FPRINTF( fout, "bool%u", ad_tid_size( tid ) );
      break;
    case T_ERROR:
      FPUTS( "error", fout );
      break;
    case T_FLOAT:
      FPRINTF( fout, "float%u", ad_tid_size( tid ) );
      break;
    case T_INT:
      if ( !ad_is_signed( tid ) )
        FPUTS( "unsigned ", fout );
      FPRINTF( fout, "int%u", ad_tid_size( tid ) );
      break;
    case T_UTF:
      FPRINTF( fout, "utf%u", ad_tid_size( tid ) );
      break;
  } // switch
  FPUTS( endian_name( ad_tid_endian( tid ) ), fout );
}

void ad_type_dump( ad_type_t const *type, FILE *fout ) {
  assert( type != NULL );
  assert( fout != NULL );

  // TODO
}

char const* endian_name( endian_t e ) {
  switch ( e ) {
    case ENDIAN_NONE    : return "none";
    case ENDIAN_LITTLE  : return "little";
    case ENDIAN_BIG     : return "big";
    case ENDIAN_HOST    : return "host";
  } // switch
  UNEXPECTED_INT_VALUE( e );
  return NULL;
}

void str_dump( char const *value, FILE *fout ) {
  assert( fout != NULL );
  if ( value == NULL ) {
    FPUTS( "null", fout );
    return;
  }
  FPUTC( '"', fout );
  for ( char const *p = value; *p != '\0'; ++p ) {
    if ( *p == '"' )
      FPUTS( "\\\"", fout );
    else
      FPUTC( *p, fout );
  } // for
  FPUTC( '"', fout );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
