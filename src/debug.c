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

#define DUMP_FORMAT(D,...) BLOCK(                   \
  FPUTNSP( (D)->indent * DUMP_INDENT, (D)->fout );  \
  FPRINTF( (D)->fout, __VA_ARGS__ ); )

#define DUMP_KEY(D,...) BLOCK( \
  fput_sep( ",\n", &(D)->comma, (D)->fout ); DUMP_FORMAT( (D), __VA_ARGS__ ); )

#define DUMP_KEY(D,...) BLOCK(                \
  fput_sep( ",\n", &(D)->comma, (D)->fout );  \
  DUMP_FORMAT( (D), __VA_ARGS__ ); )

#define DUMP_LOC(D,KEY,LOC) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); ad_loc_dump( (LOC), (D)->fout ); )

#define DUMP_STR(D,KEY,STR) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); fputs_quoted( (STR), '"', (D)->fout ); )

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Dump state.
 */
struct dump_state {
  FILE     *fout;                       ///< File to dump to.
  unsigned  indent;                     ///< Current indentation.
  bool      comma;                      ///< Print a comma?
};
typedef struct dump_state dump_state_t;

// local functions
static void ad_loc_dump( ad_loc_t const*, FILE* );
static void dump_init( dump_state_t*, unsigned, FILE* );

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
 * @param dump The dump_state to use.
 *
 * @sa ad_expr_list_dump()
 */
static void ad_expr_dump_impl( ad_expr_t const *expr, char const *key,
                               dump_state_t *dump ) {
  assert( expr != NULL );
  assert( dump != NULL );
  bool const has_key = key != NULL && key[0] != '\0';

  if ( has_key )
    DUMP_FORMAT( dump, "%s = ", key );

  if ( expr == NULL ) {
    FPUTS( "null", dump->fout );
    return;
  }

  if ( has_key )
    FPUTS( "{\n", dump->fout );
  else
    DUMP_FORMAT( dump, "{\n" );

  ++dump->indent;

  //DUMP_SNAME( "sname", &ast->sname );
  FPUTS( ",\n", dump->fout );
  DUMP_STR( dump, "kind", ad_expr_kind_name( expr->expr_kind ) );
  FPUTS( ",\n", dump->fout );
  DUMP_LOC( dump, "loc", &expr->loc );
  FPUTS( ",\n", dump->fout );

  switch ( expr->expr_kind ) {
    case AD_EXPR_NONE:
      FPUTS( "\"none\"", dump->fout );
      break;
    case AD_EXPR_ERROR:
      FPUTS( "\"error\"", dump->fout );
      break;

    case AD_EXPR_LITERAL:
      switch ( ad_type_tid( expr->literal.type ) ) {
        case T_NONE:
          FPUTS( "\"none\"", dump->fout );
          break;
        case T_ERROR:
          FPRINTF( dump->fout, "\"%s\"", ad_expr_err_name( expr->literal.err ) );
          break;
        case T_BOOL:
          FPRINTF( dump->fout, "%u", !!expr->literal.u8 );
          break;
        case T_UTF:
          // TODO
          break;
        case T_INT:
          FPRINTF( dump->fout, "%lld", expr->literal.i64 );
          break;
        case T_FLOAT:
          FPRINTF( dump->fout, "%f", expr->literal.f64 );
          break;
        case T_ENUM:
          // TODO
          break;
        case T_STRUCT:
          // TODO
          break;
        case T_TYPEDEF:
          // TODO
          break;
      } // switch
      break;

    case AD_EXPR_NAME:
      FPRINTF( dump->fout, "\"%s\"", expr->name );
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
      ad_expr_dump_impl( expr->unary.sub_expr, "sub_expr", dump );
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
      ad_expr_dump_impl( expr->binary.lhs_expr, "lhs_expr", dump );
      FPUTS( ",\n", dump->fout );
      ad_expr_dump_impl( expr->binary.rhs_expr, "rhs_expr", dump );
      break;

    case AD_EXPR_STRUCT_MBR_REF:
    case AD_EXPR_STRUCT_MBR_DEREF:
      // TODO
      break;

    // ternary
    case AD_EXPR_IF_ELSE:
      ad_expr_dump_impl( expr->ternary.cond_expr, "cond_expr", dump );
      FPUTS( ",\n", dump->fout );
      ad_expr_dump_impl( expr->ternary.sub_expr[0], "sub_expr[0]", dump );
      FPUTS( ",\n", dump->fout );
      ad_expr_dump_impl( expr->ternary.sub_expr[1], "sub_expr[1]", dump );
      break;
  } // switch

  FPUTC( '\n', dump->fout );
  --dump->indent;
  DUMP_FORMAT( dump, "}" );
}

/**
 * Dumps \a loc in [JSON5](https://json5.org) format (for debugging).
 *
 * @param loc The location to dump.
 * @param fout The `FILE` to dump to.
 */
static void ad_loc_dump( ad_loc_t const *loc, FILE *fout ) {
  assert( loc != NULL );
  assert( fout != NULL );

  FPUTS( "{ ", fout );

  if ( loc->first_line > 1 )
    FPRINTF( fout, "first_line: " PRI_ad_loc_num_t ", ", loc->first_line );

  FPRINTF( fout, "first_column: " PRI_ad_loc_num_t, loc->first_column );

  if ( loc->last_line != loc->first_line )
    FPRINTF( fout, ", last_line: " PRI_ad_loc_num_t, loc->last_line );

  if ( loc->last_column != loc->first_column )
    FPRINTF( fout, ", last_column: " PRI_ad_loc_num_t, loc->last_column );

  FPUTS( " }", fout );
}

/**
 * Initializes a dump_state.
 *
 * @param dump The dump_state to initialize.
 * @param indent The current indent.
 * @param fout The `FILE` to dump to.
 */
static void dump_init( dump_state_t *dump, unsigned indent, FILE *fout ) {
  assert( dump != NULL );
  assert( fout != NULL );

  *dump = (dump_state_t){
    .indent = indent,
    .fout = fout
  };
}

////////// extern functions ///////////////////////////////////////////////////

void bool_dump( bool value, FILE *fout ) {
  assert( fout != NULL );
  FPUTS( value ? "true" : "false", fout );
}

void ad_expr_dump( ad_expr_t const *expr, char const *key, FILE *fout ) {
  dump_state_t dump;
  dump_init( &dump, 1, fout );
  ad_expr_dump_impl( expr, key, &dump );
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

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
