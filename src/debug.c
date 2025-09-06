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

#define DUMP_BOOL(D,KEY,BOOL) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); bool_dump( (BOOL), (D)->fout ); )

#define DUMP_EXPR(D,KEY,EXPR) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); ad_expr_dump_impl( (EXPR), (D) ); )

#define DUMP_FORMAT(D,...) BLOCK(                   \
  FPUTNSP( (D)->indent * DUMP_INDENT, (D)->fout );  \
  FPRINTF( (D)->fout, __VA_ARGS__ ); )

#define DUMP_INT(D,KEY,INT) \
  DUMP_KEY( (D), KEY ": %lld", STATIC_CAST( long long, (INT) ) )

#define DUMP_KEY(D,...) BLOCK(                \
  fput_sep( ",\n", &(D)->comma, (D)->fout );  \
  DUMP_FORMAT( (D), __VA_ARGS__ ); )

#define DUMP_LOC(D,KEY,LOC) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); ad_loc_dump( (LOC), (D)->fout ); )

#define DUMP_REP(D,KEY,REP) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); ad_rep_dump_impl( (REP), (D) ); )

#define DUMP_SNAME(D,KEY,SNAME) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); sname_dump( (SNAME), (D)->fout ); )

#define DUMP_STATEMENT(D,KEY,STATEMENT) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); ad_stmnt_dump_impl( (STATEMENT), (D) ); )

#define DUMP_STATEMENT_LIST(D,KEY,LIST) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); ad_stmnt_list_dump_impl( (LIST), (D) ); )

#define DUMP_STR(D,KEY,STR) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); fputs_quoted( (STR), '"', (D)->fout ); )

#define DUMP_TID(D,KEY,TID) BLOCK( \
  DUMP_KEY( dump, KEY ": " ); ad_tid_dump_impl( (TID), (D) ); )

#define DUMP_TYPE(D,KEY,TYPE) BLOCK( \
  DUMP_KEY( dump, KEY ": " ); ad_type_dump_impl( (TYPE), (D) ); )

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

/**
 * JSON object state.
 */
enum json_state {
  JSON_INIT      = 0u,                  ///< Initial state.
  JSON_COMMA     = (1u << 0),           ///< Previous "print a comma?" state.
  JSON_OBJ_BEGUN = (1u << 1)            ///< Has a JSON object already begun?
};
typedef enum json_state json_state_t;

// local functions
static void ad_expr_dump_impl( ad_expr_t const*, dump_state_t* );
static void ad_literal_expr_dump( ad_literal_expr_t const*, dump_state_t* );
static void ad_loc_dump( ad_loc_t const*, FILE* );
static void ad_rep_dump_impl( ad_rep_t const*, dump_state_t* );
NODISCARD
static char const* ad_stmnt_kind_name( ad_stmnt_kind_t );
static void ad_stmnt_list_dump_impl( ad_stmnt_list_t const*, dump_state_t* );
static void ad_switch_dump_impl( ad_switch_stmnt_t const*, dump_state_t* );
static void ad_tid_dump_impl( ad_tid_t, dump_state_t* );
static void ad_type_dump_impl( ad_type_t const*, dump_state_t* );
static void dump_init( dump_state_t*, unsigned, FILE* );
NODISCARD
static json_state_t json_object_begin( json_state_t, char const*,
                                       dump_state_t* );
static void json_object_end( json_state_t, dump_state_t* );

// local constants
static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.

////////// local functions ////////////////////////////////////////////////////

/**
 * Dumps \a break_stmnt in [JSON5](https://json5.org) format (for debugging).
 *
 * @param break_stmnt The \ref ad_break_stmnt to dump.
 * @param dump The dump_state to use.
 */
static void ad_break_stmnt_dump_impl( ad_break_stmnt_t const *break_stmnt,
                                      dump_state_t *dump ) {
  assert( break_stmnt != NULL );
  assert( dump != NULL );

  json_state_t const break_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_INT( dump, "goto_stmnt_idx", break_stmnt->goto_stmnt_idx );

  json_object_end( break_json, dump );
}

/**
 * Dumps \a decl_stmnt in [JSON5](https://json5.org) format (for debugging).
 *
 * @param decl_stmnt The \ref ad_decl_stmnt to dump.
 * @param dump The dump_state to use.
 */
static void ad_decl_stmnt_dump_impl( ad_decl_stmnt_t const *decl_stmnt,
                                     dump_state_t *dump ) {
  assert( decl_stmnt != NULL );
  assert( dump != NULL );

  json_state_t const decl_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_STR( dump, "name", decl_stmnt->name );
  DUMP_KEY( dump, "align: %u", decl_stmnt->align );
  DUMP_TYPE( dump, "type", decl_stmnt->type );
  DUMP_REP( dump, "rep", &decl_stmnt->rep );
  if ( decl_stmnt->if_expr != NULL )
    DUMP_EXPR( dump, "if_expr", decl_stmnt->if_expr );
  if ( decl_stmnt->requires_expr != NULL )
    DUMP_EXPR( dump, "requires_expr", decl_stmnt->requires_expr );
  DUMP_STR( dump, "printf_fmt", decl_stmnt->printf_fmt );

  json_object_end( decl_json, dump );
}

/**
 * Helper function for ad_expr_dump().
 *
 * @param expr The expression to dump.  If NULL and \a key is not NULL, dumps
 * only \a key followed by `=&nbsp;NULL`.
 * @param indent The current indent.
 * @param dump The dump_state to use.
 *
 * @sa ad_expr_list_dump()
 */
static void ad_expr_dump_impl( ad_expr_t const *expr, dump_state_t *dump ) {
  assert( dump != NULL );

  if ( expr == NULL ) {
    FPUTS( "null", dump->fout );
    return;
  }

  json_state_t const expr_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_KEY( dump,
    "kind: { value: 0x%X, string: \"%s\" }",
    expr->expr_kind, ad_expr_kind_name( expr->expr_kind )
  );
  DUMP_LOC( dump, "loc", &expr->loc );

  json_state_t kind_json;

  switch ( expr->expr_kind ) {
    case AD_EXPR_NONE:
    case AD_EXPR_ERROR:
      break;

    case AD_EXPR_LITERAL:
      ad_literal_expr_dump( &expr->literal, dump );
      break;

    case AD_EXPR_NAME:
      DUMP_STR( dump, "name", expr->name );
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
      kind_json = json_object_begin( JSON_INIT, "unary", dump );
      DUMP_EXPR( dump, "sub_expr", expr->unary.sub_expr );
      json_object_end( kind_json, dump );
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
    case AD_EXPR_COMMA:
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
    case AD_EXPR_STRUCT_MBR_REF:
    case AD_EXPR_STRUCT_MBR_DEREF:
      kind_json = json_object_begin( JSON_INIT, "binary", dump );
      DUMP_EXPR( dump, "lhs_expr", expr->binary.lhs_expr );
      DUMP_EXPR( dump, "rhs_expr", expr->binary.rhs_expr );
      json_object_end( kind_json, dump );
      break;

    // ternary
    case AD_EXPR_IF_ELSE:
      kind_json = json_object_begin( JSON_INIT, "ternary", dump );
      DUMP_EXPR( dump, "cond_expr", expr->ternary.cond_expr );
      DUMP_EXPR( dump, "true_expr[0]", expr->ternary.true_expr );
      DUMP_EXPR( dump, "false_expr[1]", expr->ternary.false_expr );
      json_object_end( kind_json, dump );
      break;
  } // switch

  json_object_end( expr_json, dump );
}

/**
 * Dumps \a if_stmnt in [JSON5](https://json5.org) format (for debugging).
 *
 * @param if_stmnt The \ref ad_if_stmnt to dump.
 * @param dump The dump_state to use.
 */
static void ad_if_dump_impl( ad_if_stmnt_t const *if_stmnt,
                             dump_state_t *dump ) {
  assert( if_stmnt != NULL );
  assert( dump != NULL );

  json_state_t const if_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_EXPR( dump, "expr", if_stmnt->expr );
  DUMP_STATEMENT_LIST( dump, "if-true", &if_stmnt->if_list );
  DUMP_STATEMENT_LIST( dump, "if-false", &if_stmnt->else_list );

  FPUTC( '\n', dump->fout );
  DUMP_FORMAT( dump, "]" );

  json_object_end( if_json, dump );
}

/**
 * Dumps \a literal in [JSON5](https://json5.org) format (for debugging).
 *
 * @param literal The \ref ad_literal_expr to dump.
 * @param dump The dump_state to use.
 */
static void ad_literal_expr_dump( ad_literal_expr_t const *literal,
                                  dump_state_t *dump ) {
  assert( literal != NULL );
  assert( dump != NULL );

  json_state_t const expr_json =
    json_object_begin( JSON_INIT, "literal", dump );

  ad_tid_t const tid_base = literal->type->tid & T_MASK_TYPE;
  char8_t *u8s;

  DUMP_TID( dump, "tid", literal->type->tid );

  switch ( tid_base ) {
    case T_NONE:
      break;
    case T_BOOL:
      DUMP_BOOL( dump, "bool", !!literal->uval );
      break;
    case T_FLOAT:
      DUMP_KEY( dump, "float: %f", literal->fval );
      break;
    case T_INT:
      if ( ad_tid_is_signed( literal->type->tid ) )
        DUMP_INT( dump, "int", literal->ival );
      else
        DUMP_KEY( dump, "uint: %llu", (unsigned long long)literal->uval );
      break;
    case T_UTF:
      if ( (literal->type->tid & T_MASK_NULL) != 0 ) {
        switch ( ad_type_size( literal->type ) ) {
          case 8:
            DUMP_STR( dump, "s", literal->s );
            break;
          case 16:
            u8s = utf16s_8s( literal->s16, SIZE_MAX, ENDIAN_HOST );
            if ( u8s == NULL ) {        // conversion failed
              DUMP_KEY( dump, "s16: " );
              fputs16( literal->s16, dump->fout );
            } else {
              DUMP_STR( dump, "s16", STATIC_CAST( char const*, u8s ) );
              free( u8s );
            }
            break;
          case 32:
            u8s = utf32s_8s( literal->s32, SIZE_MAX );
            if ( u8s == NULL ) {        // conversion failed
              DUMP_KEY( dump, "s32: " );
              fputs32( literal->s32, dump->fout );
            } else {
              DUMP_STR( dump, "s32", STATIC_CAST( char const*, u8s ) );
              free( u8s );
            }
            break;
        } // switch
      }
      else {
        switch ( ad_type_size( literal->type ) ) {
          case 8:
            DUMP_KEY( dump, "c8: '%c'", literal->c8 );
            break;
          case 16:
            // TODO: literal->c16
            break;
          case 32:
            DUMP_KEY( dump, "c32: '%04X'", literal->c32 );
            break;
        } // switch
      }
      break;
    case T_ENUM:
    case T_STRUCT:
    case T_TYPEDEF:
      UNEXPECTED_INT_VALUE( tid_base );
  } // switch

  json_object_end( expr_json, dump );
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
    FPRINTF( fout, "first_line: %d, ", loc->first_line );

  FPRINTF( fout, "first_column: %d", loc->first_column );

  if ( loc->last_line != loc->first_line )
    FPRINTF( fout, ", last_line: %d", loc->last_line );

  if ( loc->last_column != loc->first_column )
    FPRINTF( fout, ", last_column: %d", loc->last_column );

  FPUTS( " }", fout );
}

/**
 * Dumps \a rep in [JSON5](https://json5.org) format (for debugging).
 *
 * @param rep The \ref ad_rep to dump.
 * @param dump The dump_state to use.
 */
static void ad_rep_dump_impl( ad_rep_t const *rep, dump_state_t *dump ) {
  assert( rep != NULL );
  assert( dump != NULL );

  json_state_t const rep_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_KEY( dump,
    "kind: { value: 0x%X, string: \"%s\" }",
    rep->kind, ad_rep_kind_name( rep->kind )
  );

  if ( rep->expr != NULL )
    DUMP_EXPR( dump, "expr", rep->expr );

  json_object_end( rep_json, dump );
}

/**
 * Dumps \a statment in [JSON5](https://json5.org) format (for debugging).
 *
 * @param statement The \ref ad_stmnt to dump.
 * @param dump The dump_state to use.
 */
static void ad_stmnt_dump_impl( ad_stmnt_t const *statement,
                                dump_state_t *dump ) {
  assert( statement != NULL );
  assert( dump != NULL );

  json_state_t const statement_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_KEY( dump,
    "kind: { value: 0x%X, string: \"%s\" }",
    statement->kind, ad_stmnt_kind_name( statement->kind )
  );
  DUMP_LOC( dump, "loc", &statement->loc );

  switch ( statement->kind ) {
    case AD_STMNT_BREAK:
      DUMP_KEY( dump, "break: " );
      ad_break_stmnt_dump_impl( &statement->break_stmnt, dump );
      break;
    case AD_STMNT_DECLARATION:
      DUMP_KEY( dump, "declaration: " );
      ad_decl_stmnt_dump_impl( &statement->decl_stmnt, dump );
      break;
    case AD_STMNT_IF:
      DUMP_KEY( dump, "if: " );
      ad_if_dump_impl( &statement->if_stmnt, dump );
      break;
    case AD_STMNT_SWITCH:
      DUMP_KEY( dump, "switch: " );
      ad_switch_dump_impl( &statement->switch_stmnt, dump );
      break;
  } // switch

  json_object_end( statement_json, dump );
}

/**
 * Dumps \a list in [JSON5](https://json5.org) format (for debugging).
 *
 * @param list The list of \ref ad_stmnt to dump.
 * @param dump The dump_state to use.
 */
static void ad_stmnt_list_dump_impl( ad_stmnt_list_t const *list,
                                     dump_state_t *dump ) {
  assert( list != NULL );
  assert( dump != NULL );

  if ( slist_empty( list ) ) {
    FPUTS( "[]", dump->fout );
    return;
  }
  FPUTS( "[\n", dump->fout );

  dump_state_t list_dump;
  dump_init( &list_dump, dump->indent + 1, dump->fout );
  FOREACH_SLIST_NODE( statement_node, list ) {
    DUMP_KEY( &list_dump, "%s", "" );
    ad_stmnt_dump_impl( statement_node->data, &list_dump );
  } // for

  FPUTC( '\n', dump->fout );
  DUMP_FORMAT( dump, "]" );
}

/**
 * Gets the name of \a kind.
 *
 * @param kind The \ref ad_stmnt_kind to get the name for.
 * @return Returns said name.
 */
static char const* ad_stmnt_kind_name( ad_stmnt_kind_t kind ) {
  switch ( kind ) {
    case AD_STMNT_BREAK       : return L_break;
    case AD_STMNT_DECLARATION : return "declaration";
    case AD_STMNT_IF          : return L_if;
    case AD_STMNT_SWITCH      : return L_switch;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
  return NULL;
}

/**
 * Dumps \a switch_stmnt in [JSON5](https://json5.org) format (for debugging).
 *
 * @param switch_stmnt The \ref ad_switch_stmnt to dump.
 * @param dump The dump_state to use.
 */
static void ad_switch_dump_impl( ad_switch_stmnt_t const *switch_stmnt,
                                 dump_state_t *dump ) {
  assert( switch_stmnt != NULL );
  assert( dump != NULL );

  json_state_t const switch_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_EXPR( dump, "expr", switch_stmnt->expr );
  DUMP_KEY( dump, "cases: " );

  if ( slist_empty( &switch_stmnt->case_list ) ) {
    FPUTS( "[]", dump->fout );
  }
  else {
    FPUTS( "[\n", dump->fout );
    dump_state_t list_dump;
    dump_init( &list_dump, dump->indent + 1, dump->fout );
    FOREACH_SLIST_NODE( case_node, &switch_stmnt->case_list ) {
      ad_switch_case_t const *const case_s = case_node->data;
      json_state_t const case_json =
        json_object_begin( JSON_INIT, /*key=*/NULL, &list_dump );
      DUMP_EXPR( &list_dump, "expr", case_s->expr );
      DUMP_STATEMENT_LIST( &list_dump, "statements", &case_s->statement_list );
      json_object_end( case_json, &list_dump );
    } // for
    FPUTC( '\n', dump->fout );
    DUMP_FORMAT( dump, "]" );
  }

  json_object_end( switch_json, dump );
}

/**
 * Dumps \a tid in [JSON5](https://json5.org) format (for debugging).
 *
 * @param tid The \ref ad_tid_t to dump.
 * @param dump The dump_state to use.
 */
static void ad_tid_dump_impl( ad_tid_t tid, dump_state_t *dump ) {
  assert( dump != NULL );

  json_state_t const tid_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  ad_tid_kind_t const kind = ad_tid_kind( tid );
  unsigned const size = ad_tid_size( tid );

  DUMP_KEY( dump,
    "kind: { value: 0x%X, string: \"%s\" }",
    kind, ad_tid_kind_name( kind )
  );

  if ( size > 0 )
    DUMP_KEY( dump, "size: %u", ad_tid_size( tid ) );

  if ( kind == T_INT )
    DUMP_BOOL( dump, "signed", ad_tid_is_signed( tid ) );

  if ( size > 8 )
    DUMP_STR( dump, "endian", endian_name( ad_tid_endian( tid ) ) );

  json_object_end( tid_json, dump );
}

/**
 * Dumps \a type in [JSON5](https://json5.org) format (for debugging).
 *
 * @param type The \ref ad_type to dump.
 * @param dump The dump_state to use.
 */
static void ad_type_dump_impl( ad_type_t const *type, dump_state_t *dump ) {
  assert( type != NULL );
  assert( dump != NULL );

  json_state_t const type_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_SNAME( dump, "sname", &type->sname );
  DUMP_TID( dump, "tid", type->tid );
  DUMP_LOC( dump, "loc", &type->loc );

  ad_tid_kind_t const kind = ad_tid_kind( type->tid );

  if ( kind >= T_BOOL ) {
    if ( type->size_expr != NULL )
      DUMP_EXPR( dump, "size_expr", type->size_expr );
    if ( type->endian_expr != NULL )
      DUMP_EXPR( dump, "endian_expr", type->endian_expr );
  }
  DUMP_REP( dump, "rep", &type->rep );

  switch ( kind ) {
    case T_NONE:
    case T_ERROR:
      break;

    case T_BOOL:;
      json_state_t const bool_json =
        json_object_begin( JSON_INIT, L_bool, dump );
      DUMP_STR( dump, "printf_fmt", type->bool_t.printf_fmt );
      json_object_end( bool_json, dump );
      break;

    case T_ENUM:;
      json_state_t const enum_json =
        json_object_begin( JSON_INIT, L_enum, dump );
      DUMP_STR( dump, "printf_fmt", type->enum_t.printf_fmt );
      DUMP_KEY( dump, "value_list" );
      if ( slist_empty( &type->enum_t.value_list ) ) {
        FPUTS( "[]", dump->fout );
      }
      else {
        FPUTS( "[\n", dump->fout );
        dump_state_t list_dump;
        dump_init( &list_dump, dump->indent + 1, dump->fout );
        FOREACH_SLIST_NODE( value_node, &type->enum_t.value_list ) {
          ad_enum_value_t const *const value = value_node->data;
          json_state_t const enum_value_json =
            json_object_begin( JSON_INIT, /*key=*/NULL, &list_dump );
          DUMP_STR( dump, "name", value->name );
          DUMP_INT( dump, "value", value->value );
          DUMP_STR( dump, "printf_fmt", value->printf_fmt );
          json_object_end( enum_value_json, &list_dump );
        } // for
        FPUTC( '\n', dump->fout );
        DUMP_FORMAT( dump, "]" );
      }
      json_object_end( enum_json, dump );
      break;

    case T_FLOAT:;
      json_state_t const float_json =
        json_object_begin( JSON_INIT, L_float, dump );
      DUMP_STR( dump, "printf_fmt", type->float_t.printf_fmt );
      json_object_end( float_json, dump );
      break;

    case T_INT:;
      json_state_t const int_json =
        json_object_begin( JSON_INIT, L_int, dump );
      DUMP_STR( dump, "printf_fmt", type->int_t.printf_fmt );
      json_object_end( int_json, dump );
      break;

    case T_STRUCT:
    case T_UNION:;
      json_state_t const struct_json = json_object_begin(
        JSON_INIT, kind == T_STRUCT ? L_struct : L_union, dump
      );
      DUMP_KEY( dump, "member_list: " );
      if ( slist_empty( &type->struct_t.member_list ) ) {
        FPUTS( "[]", dump->fout );
      }
      else {
        FPUTS( "[\n", dump->fout );
        dump_state_t list_dump;
        dump_init( &list_dump, dump->indent + 1, dump->fout );
        FOREACH_SLIST_NODE( member_node, &type->struct_t.member_list ) {
          DUMP_KEY( &list_dump, "%s", "" );
          ad_stmnt_dump_impl( member_node->data, &list_dump );
        }
        FPUTC( '\n', dump->fout );
        DUMP_FORMAT( dump, "]" );
      }
      json_object_end( struct_json, dump );
      break;

    case T_TYPEDEF:;
      json_state_t const typedef_json =
        json_object_begin( JSON_INIT, L_typedef, dump );
      DUMP_TYPE( dump, "type", type->typedef_t.type );
      json_object_end( typedef_json, dump );
      break;

    case T_UTF:;
      json_state_t const utf_json =
        json_object_begin( JSON_INIT, L_utf, dump );
      DUMP_STR( dump, "printf_fmt", type->utf_t.printf_fmt );
      json_object_end( utf_json, dump );
      break;
  } // switch

  json_object_end( type_json, dump );
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

/**
 * Dumps the beginning of a JSON object.
 *
 * @param json The \ref json_state to use.  If not equal to #JSON_INIT, does
 * nothing.  It allows json_object_begin() to be to be called even inside a
 * `switch` statement and the previous `case` (that also called
 * json_object_begin()) falls through and not begin a new JSON object when that
 * happens allowing common code in the second case to be shared with the first
 * and not be duplicated.  For example, given:
 * ```cpp
 *  case C1:
 *    json = json_object_begin( JSON_INIT, "K1", dump );
 *    // Do stuff unique to C1.
 *    FALLTHROUGH;
 *  case C2:
 *    json = json_object_begin( json, "K2", dump ); // Passing 'json' here.
 *    // Do stuff common to C1 and C2.
 *    json_object_end( json, dump );
 *    break;
 * ```
 * There are two cases:
 * 1. `case C2` is entered: a JSON object will be begun having the key `K2`.
 *    (There is nothing special about this case.)
 * 2. `case C1` is entered: a JSON object will be begun having the key `K1`.
 *    When the case falls through into `case C2`, a second JSON object will
 *    _not_ be begun: the call to the second <code>%json_object_begin()</code>
 *    will do nothing.
 * @param key The key for the JSON object; may be NULL.  If neither NULL nor
 * empty, dumps \a key followed by `: `.
 * @param dump The dump_state to use.
 * @return Returns a new \ref json_state that must eventually be passed to
 * json_object_end().
 *
 * @sa json_object_end()
 */
NODISCARD
static json_state_t json_object_begin( json_state_t json, char const *key,
                                       dump_state_t *dump ) {
  assert( dump != NULL );

  if ( json == JSON_INIT ) {
    key = null_if_empty( key );
    if ( key != NULL )
      DUMP_KEY( dump, "%s: ", key );
    FPUTS( "{\n", dump->fout );
    json = JSON_OBJ_BEGUN;
    if ( dump->comma ) {
      json |= JSON_COMMA;
      dump->comma = false;
    }
    ++dump->indent;
  }
  return json;
}

/**
 * Dumps the end of a JSON object.
 *
 * @param json The \ref json_state returned from json_object_begin().
 * @param dump The dump_state to use.
 *
 * @sa json_object_begin()
 */
static void json_object_end( json_state_t json, dump_state_t *dump ) {
  assert( json != JSON_INIT );
  assert( dump != NULL );

  FPUTC( '\n', dump->fout );
  dump->comma = !!(json & JSON_COMMA);
  --dump->indent;
  DUMP_FORMAT( dump, "}" );
}

////////// extern functions ///////////////////////////////////////////////////

void bool_dump( bool value, FILE *fout ) {
  assert( fout != NULL );
  FPUTS( value ? "true" : "false", fout );
}

void ad_expr_dump( ad_expr_t const *expr, FILE *fout ) {
  dump_state_t dump;
  dump_init( &dump, /*indent=*/1, fout );
  ad_expr_dump_impl( expr, &dump );
}

void ad_rep_dump( ad_rep_t const *rep, FILE *fout ) {
  dump_state_t dump;
  dump_init( &dump, /*indent=*/1, fout );
  ad_rep_dump_impl( rep, &dump );
}

void ad_stmnt_dump( ad_stmnt_t const *statement, FILE *fout ) {
  dump_state_t dump;
  dump_init( &dump, /*indent=*/1, fout );
  ad_stmnt_dump_impl( statement, &dump );
}

void ad_tid_dump( ad_tid_t tid, FILE *fout ) {
  dump_state_t dump;
  dump_init( &dump, /*indent=*/1, fout );
  ad_tid_dump_impl( tid, &dump );
}

void ad_type_dump( ad_type_t const *type, FILE *fout ) {
  dump_state_t dump;
  dump_init( &dump, /*indent=*/1, fout );
  ad_type_dump_impl( type, &dump );
}

char const* endian_name( endian_t e ) {
  switch ( e ) {
    case ENDIAN_NONE  : return "none";
    case ENDIAN_LITTLE: return "little";
    case ENDIAN_BIG   : return "big";
    case ENDIAN_HOST  : return "host";
  } // switch
  UNEXPECTED_INT_VALUE( e );
  return NULL;
}

void sname_dump( sname_t const *sname, FILE *fout ) {
  assert( sname != NULL );
  assert( fout != NULL );

  FPUTS( sname_empty( sname ) ? "null" : sname_full_name( sname ), fout );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
