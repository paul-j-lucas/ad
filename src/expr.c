/*
**      ad -- ASCII dump
**      src/expr.c
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
#define AD_EXPR_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "expr.h"
#include "util.h"

// standard
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define ENSURE_ELSE(EXPR,ERR) BLOCK(        \
  if ( !(EXPR) ) {                          \
    ad_expr_set_err( rv, AD_ERR_ ## ERR );  \
    return false;                           \
  } )

#define EVAL_CHECK(EXPR_PTR,RESULT) BLOCK(        \
  if ( !ad_expr_eval( (EXPR_PTR), (RESULT) ) )    \
    return false;                                 \
  assert( (RESULT)->expr_id == AD_EXPR_VALUE ); )

#define TYPE_CHECK(EXPR_PTR,TYPE) \
  ENSURE_ELSE( ad_expr_get_base_type( (EXPR_PTR) ) == (TYPE), BAD_OPERAND )

////////// local functions ////////////////////////////////////////////////////

static void narrow( ad_expr_t *expr ) {
  ad_type_id_t const t = expr->as.value.type;
  assert( ((t & T_MASK_TYPE) & T_NUMBER) != T_NONE );

  switch ( t ) {
    case T_FLOAT32:
      expr->as.value.as.f64 = (float)expr->as.value.as.f64;
      break;
    case T_INT8:
      expr->as.value.as.i64 = (int8_t)expr->as.value.as.i64;
      break;
    case T_INT16:
      expr->as.value.as.i64 = (int16_t)expr->as.value.as.i64;
      break;
    case T_INT32:
      expr->as.value.as.i64 = (int32_t)expr->as.value.as.i64;
      break;
    case T_INT64:
      expr->as.value.as.i64 = (int64_t)expr->as.value.as.i64;
      break;
    case T_BOOL8:
    case T_UINT8:
      expr->as.value.as.u64 = (uint8_t)expr->as.value.as.u64;
      break;
    case T_UINT16:
      expr->as.value.as.u64 = (uint16_t)expr->as.value.as.u64;
      break;
    case T_UINT32:
      expr->as.value.as.u64 = (uint32_t)expr->as.value.as.u64;
      break;
    case T_UINT64:
      expr->as.value.as.u64 = (uint64_t)expr->as.value.as.u64;
      break;
    case T_UTF16:
      expr->as.value.as.u64 = (char16_t)expr->as.value.as.u64;
      break;
  } // switch
}

static uint64_t widen_int( ad_expr_t const *expr ) {
  assert( expr->expr_id == AD_EXPR_VALUE );
  assert( ad_expr_get_base_type( expr ) == T_INT );

  switch ( expr->as.value.type ) {
    case T_INT8:
    case T_INT16:
    case T_INT32:
      ;
  } // switch
  return expr->as.value.as.u64;
}

static bool ad_expr_add( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t lhs_expr, rhs_expr;
  EVAL_CHECK( expr->as.binary.lhs_expr, &lhs_expr );
  EVAL_CHECK( expr->as.binary.rhs_expr, &rhs_expr );
  ad_type_id_t const lhs_type = ad_expr_get_base_type( &lhs_expr );
  ad_type_id_t const rhs_type = ad_expr_get_base_type( &rhs_expr );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ad_expr_set_int( rv, lhs_expr.as.value.as.i64 + rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ad_expr_set_double( rv, lhs_expr.as.value.as.i64 + rhs_expr.as.value.as.f64 );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ad_expr_set_double( rv, lhs_expr.as.value.as.f64 + rhs_expr.as.value.as.i64 );
    return true;
  }

  assert( rhs_expr.expr_id == T_FLOAT );
  ad_expr_set_double( rv, lhs_expr.as.value.as.f64 + rhs_expr.as.value.as.f64 );
  return true;
}

static bool ad_expr_div( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t lhs_expr, rhs_expr;
  EVAL_CHECK( expr->as.binary.lhs_expr, &lhs_expr );
  EVAL_CHECK( expr->as.binary.rhs_expr, &rhs_expr );
  ad_type_id_t const lhs_type = ad_expr_get_base_type( &lhs_expr );
  ad_type_id_t const rhs_type = ad_expr_get_base_type( &rhs_expr );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_int( rv, lhs_expr.as.value.as.i64 / rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0.0, DIV_0 );
    ad_expr_set_double( rv, lhs_expr.as.value.as.i64 / rhs_expr.as.value.as.f64 );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_double( rv, lhs_expr.as.value.as.f64 / rhs_expr.as.value.as.i64 );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0.0, DIV_0 );
  ad_expr_set_double( rv, lhs_expr.as.value.as.f64 / rhs_expr.as.value.as.f64 );
  return true;
}

static bool ad_expr_mod( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t dividend, rhs_expr;
  EVAL_CHECK( expr->as.binary.lhs_expr, &dividend );
  EVAL_CHECK( expr->as.binary.rhs_expr, &rhs_expr );
  ad_type_id_t const lhs_type = ad_expr_get_base_type( &dividend );
  ad_type_id_t const rhs_type = ad_expr_get_base_type( &rhs_expr );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_int( rv, dividend.as.value.as.i64 % rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0.0, DIV_0 );
    ad_expr_set_double( rv,
      fmod( dividend.as.value.as.i64, rhs_expr.as.value.as.f64 )
    );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_double( rv,
      fmod( dividend.as.value.as.f64, rhs_expr.as.value.as.i64 )
    );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0, DIV_0 );
  ad_expr_set_double( rv,
    fmod( dividend.as.value.as.f64, rhs_expr.as.value.as.f64 )
  );
  return true;
}

static bool ad_expr_mul( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t lhs_expr, rhs_expr;
  EVAL_CHECK( expr->as.binary.lhs_expr, &lhs_expr );
  EVAL_CHECK( expr->as.binary.rhs_expr, &rhs_expr );
  ad_type_id_t const lhs_type = ad_expr_get_base_type( &lhs_expr );
  ad_type_id_t const rhs_type = ad_expr_get_base_type( &rhs_expr );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ad_expr_set_int( rv, lhs_expr.as.value.as.i64 * rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ad_expr_set_double( rv, lhs_expr.as.value.as.i64 * rhs_expr.as.value.as.f64 );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ad_expr_set_double( rv, lhs_expr.as.value.as.f64 * rhs_expr.as.value.as.i64 );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ad_expr_set_double( rv, lhs_expr.as.value.as.f64 * rhs_expr.as.value.as.f64 );
  return true;
}

static bool ad_expr_sub( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t lhs_expr, rhs_expr;
  EVAL_CHECK( expr->as.binary.lhs_expr, &lhs_expr );
  EVAL_CHECK( expr->as.binary.rhs_expr, &rhs_expr );
  ad_type_id_t const lhs_type = ad_expr_get_base_type( &lhs_expr );
  ad_type_id_t const rhs_type = ad_expr_get_base_type( &rhs_expr );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ad_expr_set_int( rv, lhs_expr.as.value.as.i64 - rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ad_expr_set_double( rv,
      lhs_expr.as.value.as.i64 - rhs_expr.as.value.as.f64
    );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ad_expr_set_double( rv,
      lhs_expr.as.value.as.f64 - rhs_expr.as.value.as.i64
    );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ad_expr_set_double( rv,
    lhs_expr.as.value.as.f64 - rhs_expr.as.value.as.f64
  );
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_expr_eval( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t temp1, temp2;

  switch ( expr->expr_id ) {
    case AD_EXPR_NONE:
      break;
    case AD_EXPR_ERROR:
    case AD_EXPR_VALUE:
      *rv = *expr;
      break;

    case AD_EXPR_MATH_ADD:
      return ad_expr_add( expr, rv );

    case AD_EXPR_BIT_AND:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_int( rv, temp1.as.value.as.i64 & temp2.as.value.as.i64 );
      return true;

    case AD_EXPR_BIT_COMP:
      EVAL_CHECK( expr->as.unary.sub_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      ad_expr_set_int( rv, ~temp1.as.value.as.i64 );
      return true;

    case AD_EXPR_BIT_OR:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_int( rv, temp1.as.value.as.i64 | temp2.as.value.as.i64 );
      return true;

    case AD_EXPR_BIT_XOR:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_int( rv, temp1.as.value.as.i64 ^ temp2.as.value.as.i64 );
      break;

    case AD_EXPR_CAST:
      // TODO
      break;

    case AD_EXPR_DEREF:
      EVAL_CHECK( expr->as.unary.sub_expr, &temp1 );
      // TODO
      return true;

    case AD_EXPR_MATH_DIV:
      return ad_expr_div( expr, rv );

    case AD_EXPR_LOG_AND:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      if ( ad_expr_is_zero( &temp1 ) ) {
        ad_expr_set_bool( rv, false );
      } else {
        EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
        ad_expr_set_bool( rv, !ad_expr_is_zero( &temp2 ) );
      }
      return true;

    case AD_EXPR_LOG_NOT:
      EVAL_CHECK( expr->as.unary.sub_expr, &temp1 );
      ad_expr_set_bool( rv, ad_expr_is_zero( &temp1 ) );
      return true;

    case AD_EXPR_LOG_OR:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      if ( !ad_expr_is_zero( &temp1 ) ) {
        ad_expr_set_bool( rv, true );
      } else {
        EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
        ad_expr_set_bool( rv, !ad_expr_is_zero( &temp2 ) );
      }
      return true;

    case AD_EXPR_LOG_XOR:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_bool( rv,
        !ad_expr_is_zero( &temp1 ) ^ !ad_expr_is_zero( &temp2 )
      );
      return true;

    case AD_EXPR_MATH_MOD:
      return ad_expr_mod( expr, rv );

    case AD_EXPR_MATH_MUL:
      return ad_expr_mul( expr, rv );

    case AD_EXPR_NEG:
      EVAL_CHECK( expr->as.unary.sub_expr, &temp1 );
      ad_type_id_t const t1 = ad_expr_get_base_type( &temp1 );
      if ( t1 == T_INT ) {
        ad_expr_set_int( rv, -temp1.as.value.as.i64 );
      } else {
        assert( t1 == T_FLOAT );
        ad_expr_set_double( rv, -temp1.as.value.as.f64 );
      }
      return true;

    case AD_EXPR_BIT_SHIFT_LEFT:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_int( rv, temp1.as.value.as.i64 << temp2.as.value.as.i64 );
      return true;

    case AD_EXPR_BIT_SHIFT_RIGHT:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_int( rv, temp1.as.value.as.i64 >> temp2.as.value.as.i64 );
      return true;

    case AD_EXPR_MATH_SUB:
      return ad_expr_sub( expr, rv );

    case AD_EXPR_IF_ELSE:
      ad_expr_eval( expr->as.ternary.cond_expr, &temp1 );
      return ad_expr_is_zero( &temp1 ) ?
        ad_expr_eval( expr->as.ternary.false_expr, rv ) :
        ad_expr_eval( expr->as.ternary.true_expr, rv );

    case AD_EXPR_REL_LESS:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      ad_expr_set_int( rv, temp1.as.value.as.i64 < temp2.as.value.as.i64 );
      return true;

    case AD_EXPR_REL_LESS_EQ:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      ad_expr_set_int( rv, temp1.as.value.as.u64 <= temp2.as.value.as.u64 );
      return true;
      
    case AD_EXPR_REL_GREATER:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      ad_expr_set_int( rv, temp1.as.value.as.u64 > temp2.as.value.as.u64 );
      return true;
      
    case AD_EXPR_REL_GREATER_EQ:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      ad_expr_set_int( rv, temp1.as.value.as.u64 >= temp2.as.value.as.u64 );
      return true;
      
    case AD_EXPR_REL_EQ:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      ad_expr_set_int( rv, temp1.as.value.as.u64 == temp2.as.value.as.u64 );
      return true;
      
    case AD_EXPR_REL_NOT_EQ:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      ad_expr_set_int( rv, temp1.as.value.as.u64 != temp2.as.value.as.u64 );
      return true;
      
  } // switch
  return false;
}

void ad_expr_free( ad_expr_t *expr ) {
  switch ( expr->expr_id & AD_EXPR_MASK ) {
    case AD_EXPR_TERNARY:
      ad_expr_free( expr->as.ternary.false_expr );
      // FALLTHROUGH
    case AD_EXPR_BINARY:
      ad_expr_free( expr->as.binary.rhs_expr );
      // FALLTHROUGH
    case AD_EXPR_UNARY:
      ad_expr_free( expr->as.unary.sub_expr );
  } // switch
  free( expr );
}

ad_expr_t* ad_expr_new( ad_expr_id_t expr_id ) {
  ad_expr_t *const e = MALLOC( ad_expr_t, 1 );
  MEM_ZERO( e );
  e->expr_id = expr_id;
  return e;
}

void ad_expr_set_bool( ad_expr_t *expr, bool bval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type = T_BOOL8;
  expr->as.value.as.u64 = bval;
}

void ad_expr_set_double( ad_expr_t *expr, double dval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type = T_FLOAT64;
  expr->as.value.as.f64 = dval;
}

void ad_expr_set_err( ad_expr_t *expr, ad_expr_err_t err ) {
  expr->expr_id = AD_EXPR_ERROR;
  expr->as.value.as.err = err;
}

void ad_expr_set_int( ad_expr_t *expr, long ival ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type = T_INT64;
  expr->as.value.as.i64 = ival;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
