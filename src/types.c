/*
**      ad -- ASCII dump
**      src/types.c
**
**      Copyright (C) 2019  Paul J. Lucas
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
/// @cond DOXYGEN_IGNORE
#define AD_TYPES_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "expr.h"
#include "literals.h"
#include "slist.h"
#include "types.h"

// standard
#include <assert.h>
#include <inttypes.h>

///////////////////////////////////////////////////////////////////////////////

static slist_node_t TB_BOOL_SNAME_NODE = {
  NULL, &(sname_scope_t){ .name = "bool" }
};

ad_type_t const TB_BOOL8 = {
  .sname = {
    &TB_BOOL_SNAME_NODE,
    &TB_BOOL_SNAME_NODE,
    1
  },
  .tid = T_BOOL8,
  .int_t = { .printf_fmt = "%d" }
};

static slist_node_t TB_FLOAT_SNAME_NODE = {
  NULL, &(sname_scope_t){ .name = "float" }
};

ad_type_t const TB_FLOAT64 = {
  .sname = {
    &TB_FLOAT_SNAME_NODE,
    &TB_FLOAT_SNAME_NODE,
    1
  },
  .tid = T_FLOAT64 | T_END_H,
  .int_t = { .printf_fmt = "%f" }
};

static slist_node_t TB_INT64_SNAME_NODE = {
  NULL, &(sname_scope_t){ .name = "int64" }
};

ad_type_t const TB_INT64 = {
  .sname = {
    &TB_INT64_SNAME_NODE,
    &TB_INT64_SNAME_NODE,
    1
  },
  .tid = T_INT64 | T_END_H,
  .int_t = { .printf_fmt = PRId64 }
};

static slist_node_t TB_UINT64_SNAME_NODE = {
  NULL, &(sname_scope_t){ .name = "uint64" }
};

ad_type_t const TB_UINT64 = {
  .sname = {
    &TB_UINT64_SNAME_NODE,
    &TB_UINT64_SNAME_NODE,
    1
  },
  .tid = T_UINT64 | T_END_H,
  .int_t = { .printf_fmt = PRIu64 }
};

static slist_node_t TB_UTF_SNAME_NODE = {
  NULL, &(sname_scope_t){ .name = "utf" }
};

ad_type_t const TB_UTF8_0 = {
  .sname = {
    &TB_UTF_SNAME_NODE,
    &TB_UTF_SNAME_NODE,
    1
  },
  .tid = T_UTF8_0,
  .int_t = { .printf_fmt = "%s" }
};

////////// extern functions ///////////////////////////////////////////////////

void ad_enum_value_free( ad_enum_value_t *value ) {
  if ( value != NULL ) {
    FREE( value->name );
    free( value );
  }
}

char const* ad_rep_kind_name( ad_rep_kind_t kind ) {
  switch ( kind ) {
    case AD_REP_1     : return "1";
    case AD_REP_EXPR  : return "expr";
    case AD_REP_0_1   : return "?";
    case AD_REP_0_MORE: return "*";
    case AD_REP_1_MORE: return "+";
  } // switch
  UNEXPECTED_INT_VALUE( kind );
}

void ad_stmnt_free( ad_stmnt_t *stmnt ) {
  if ( stmnt == NULL )
    return;

  switch ( stmnt->kind ) {
    case AD_STMNT_BREAK:
      // nothing to do
      break;

    case AD_STMNT_DECL:
      FREE( stmnt->decl_stmnt.name );
      // stmnt->decl_stmnt.type points to a type in a synfo in the symbol table
      FREE( stmnt->decl_stmnt.printf_fmt );
      break;

    case AD_STMNT_IF:
      ad_expr_free( stmnt->if_stmnt.expr );
      ad_stmnt_list_cleanup( &stmnt->if_stmnt.list[ true  ] );
      ad_stmnt_list_cleanup( &stmnt->if_stmnt.list[ false ] );
      break;

    case AD_STMNT_LET:
      FREE( stmnt->let_stmnt.name );
      ad_expr_free( stmnt->let_stmnt.expr );
      break;

    case AD_STMNT_SWITCH:
      ad_expr_free( stmnt->switch_stmnt.expr );
      FOREACH_SWITCH_CASE( case_node, stmnt ) {
        ad_switch_case_t *const switch_case = case_node->data;
        ad_expr_free( switch_case->expr );
        ad_stmnt_list_cleanup( &switch_case->stmnts );
      } // for
      break;
  } // switch

  free( stmnt );
}

void ad_stmnt_list_cleanup( ad_stmnt_list_t *stmnts ) {
  slist_cleanup( stmnts, POINTER_CAST( slist_free_fn_t, &ad_stmnt_free ) );
}

char const* ad_tid_kind_name( ad_tid_kind_t kind ) {
  switch ( kind ) {
    case T_NONE   : return "none";
    case T_BOOL   : return L_bool;
    case T_ENUM   : return L_enum;
    case T_ERROR  : return "error";
    case T_FLOAT  : return L_float;
    case T_INT    : return L_int;
    case T_STRUCT : return L_struct;
    case T_TYPEDEF: return L_typedef;
    case T_UNION  : return L_union;
    case T_UTF    : return L_utf;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
}

int ad_type_cmp( ad_type_t const *i_type, ad_type_t const *j_type ) {
  assert( i_type != NULL );
  assert( j_type != NULL );
  return sname_cmp( &i_type->sname, &j_type->sname );
}

bool ad_type_equal( ad_type_t const *i_type, ad_type_t const *j_type ) {
  if ( i_type == j_type )
    return true;
  if ( i_type == NULL || j_type == NULL )
    return false;

  // ...

  return true;
}

void ad_type_free( ad_type_t *type ) {
  if ( type == NULL )
    return;
  ad_expr_free( type->endian_expr );
  ad_expr_free( type->size_expr );

  switch ( ad_tid_kind( type->tid ) ) {
    case T_BOOL:
    case T_FLOAT:
    case T_INT:
    case T_UTF:
      FREE( type->fmt_t.printf_fmt );
      FALLTHROUGH;
    case T_ENUM:
      slist_cleanup(
        &type->enum_t.value_list,
        POINTER_CAST( slist_free_fn_t, &ad_enum_value_free )
      );
      break;
    case T_STRUCT:
    case T_UNION:
      slist_cleanup(
        &type->union_t.member_list,
        POINTER_CAST( slist_free_fn_t, &ad_stmnt_free )
      );
      break;
    case T_ERROR:
    case T_NONE:
    case T_TYPEDEF:
      // nothing to do
      break;
  } // switch

  sname_cleanup( &type->sname );
  free( type );
}

ad_type_t* ad_type_new( ad_tid_t tid ) {
  assert( tid != T_NONE );

  ad_type_t *const type = MALLOC( ad_type_t, 1 );
  *type = (ad_type_t){ 0 };

  return type;
}

unsigned ad_type_size( ad_type_t const *t ) {
  unsigned const bits = ad_tid_size( t->tid );
  if ( bits != 0 )
    return bits;
  assert( ad_expr_is_literal( t->size_expr ) );
  return STATIC_CAST( unsigned, t->size_expr->literal.uval );
}

ad_type_t const* ad_type_untypedef( ad_type_t const *type ) {
  for (;;) {
    assert( type != NULL );
    if ( (type->tid & T_MASK_TYPE) != T_TYPEDEF )
      return type;
    type = type->typedef_t.type;
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
