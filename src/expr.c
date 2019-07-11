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

#define EVAL_CHECK(EXPR_PTR,RESULT) BLOCK(        \
  if ( !ad_expr_eval( (EXPR_PTR), (RESULT) ) )    \
    return false;                                 \
  assert( (RESULT)->expr_id == AD_EXPR_VALUE ); )

#define TYPE_CHECK(EXPR_PTR,TYPE) BLOCK(                  \
  if ( ad_expr_get_base_type( (EXPR_PTR) ) != (TYPE) ) {  \
    ad_expr_set_err( (EXPR_PTR), ERR_BAD_OPERAND );       \
    return false;                                         \
  } )

////////// local functions ////////////////////////////////////////////////////

static bool ad_expr_add( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t augend, addend;
  EVAL_CHECK( expr->as.binary.lhs_expr, &augend );
  EVAL_CHECK( expr->as.binary.rhs_expr, &addend );
  if ( ad_expr_get_base_type( &augend ) == T_INT &&
       ad_expr_get_base_type( &addend ) == T_INT ) {
    ad_expr_set_int( rv, augend.as.value.as.i64 + addend.as.value.as.i64 );
    return true;
  }
  if ( ad_expr_get_base_type( &augend ) == T_INT &&
       ad_expr_get_base_type( &addend ) == T_FLOAT ) {
    ad_expr_set_double( rv, augend.as.value.as.i64 + addend.as.value.as.f64 );
    return true;
  }
  assert( ad_expr_get_base_type( &augend ) == T_FLOAT );
  if ( ad_expr_get_base_type( &addend ) == T_INT ) {
    ad_expr_set_double( rv, augend.as.value.as.f64 + addend.as.value.as.i64 );
    return true;
  }
  assert( addend.expr_id == T_FLOAT );
  ad_expr_set_double( rv, augend.as.value.as.f64 + addend.as.value.as.f64 );
  return true;
}

static bool ad_expr_div( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t dividend, divisor;
  EVAL_CHECK( expr->as.binary.lhs_expr, &dividend );
  EVAL_CHECK( expr->as.binary.rhs_expr, &divisor );
  ad_type_id_t const t1 = ad_expr_get_base_type( &dividend );
  ad_type_id_t const t2 = ad_expr_get_base_type( &divisor );

  if ( t1 == T_INT && t2 == T_INT ) {
    if ( ad_expr_is_zero( &divisor ) ) {
      ad_expr_set_err( rv, ERR_DIV_0 );
      return false;
    }
    ad_expr_set_int( rv, dividend.as.value.as.i64 / divisor.as.value.as.i64 );
    return true;
  }

  if ( t1 == T_INT && t2 == T_FLOAT ) {
    if ( divisor.as.value.as.f64 == 0.0 ) {
      ad_expr_set_err( rv, ERR_DIV_0 );
      return false;
    }
    ad_expr_set_double( rv,
      dividend.as.value.as.i64 / divisor.as.value.as.f64
    );
    return true;
  }

  assert( t1 == T_FLOAT );
  if ( t2 == T_INT ) {
    if ( ad_expr_is_zero( &divisor ) ) {
      ad_expr_set_err( rv, ERR_DIV_0 );
      return false;
    }
    ad_expr_set_double( rv,
      dividend.as.value.as.f64 / divisor.as.value.as.i64
    );
    return true;
  }

  assert( t2 == T_FLOAT );
  if ( divisor.as.value.as.f64 == 0.0 ) {
    ad_expr_set_err( rv, ERR_DIV_0 );
    return false;
  }
  ad_expr_set_double( rv, dividend.as.value.as.f64 / divisor.as.value.as.f64 );
  return true;
}

static bool ad_expr_mod( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t dividend, divisor;
  EVAL_CHECK( expr->as.binary.lhs_expr, &dividend );
  EVAL_CHECK( expr->as.binary.rhs_expr, &divisor );
  ad_type_id_t const t1 = ad_expr_get_base_type( &dividend );
  ad_type_id_t const t2 = ad_expr_get_base_type( &divisor );

  if ( t1 == T_INT && t2 == T_INT ) {
    if ( ad_expr_is_zero( &divisor ) ) {
      ad_expr_set_err( rv, ERR_DIV_0 );
      return false;
    }
    ad_expr_set_int( rv, dividend.as.value.as.i64 % divisor.as.value.as.i64 );
    return true;
  }

  if ( t1 == T_INT && t2 == T_FLOAT ) {
    if ( divisor.as.value.as.f64 == 0.0 ) {
      ad_expr_set_err( rv, ERR_DIV_0 );
      return false;
    }
    ad_expr_set_double( rv,
      fmod( dividend.as.value.as.i64, divisor.as.value.as.f64 )
    );
    return true;
  }

  assert( t1 == T_FLOAT );
  if ( t2 == T_INT ) {
    if ( ad_expr_is_zero( &divisor ) ) {
      ad_expr_set_err( rv, ERR_DIV_0 );
      return false;
    }
    ad_expr_set_double( rv,
      fmod( dividend.as.value.as.f64, divisor.as.value.as.i64 )
    );
    return true;
  }

  assert( t2 == T_FLOAT );
  if ( divisor.as.value.as.f64 == 0.0 ) {
    ad_expr_set_err( rv, ERR_DIV_0 );
    return false;
  }
  ad_expr_set_double( rv,
    fmod( dividend.as.value.as.f64, divisor.as.value.as.f64 )
  );
  return true;
}

static bool ad_expr_mul( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t multiplier, multiplicand;
  EVAL_CHECK( expr->as.binary.lhs_expr, &multiplier );
  EVAL_CHECK( expr->as.binary.rhs_expr, &multiplicand );
  ad_type_id_t const t1 = ad_expr_get_base_type( &multiplier );
  ad_type_id_t const t2 = ad_expr_get_base_type( &multiplicand );

  if ( t1 == T_INT && t2 == T_INT ) {
    ad_expr_set_int( rv,
      multiplier.as.value.as.i64 * multiplicand.as.value.as.i64
    );
    return true;
  }

  if ( t1 == T_INT && t2 == T_FLOAT ) {
    ad_expr_set_double( rv,
      multiplier.as.value.as.i64 * multiplicand.as.value.as.f64
    );
    return true;
  }

  assert( t1 == T_FLOAT );
  if ( t2 == T_INT ) {
    ad_expr_set_double( rv,
      multiplier.as.value.as.f64 * multiplicand.as.value.as.i64
    );
    return true;
  }

  assert( t2 == T_FLOAT );
  ad_expr_set_double( rv,
    multiplier.as.value.as.f64 * multiplicand.as.value.as.f64
  );
  return true;
}

static bool ad_expr_sub( ad_expr_t const *expr, ad_expr_t *rv ) {
  ad_expr_t minuend, subtrahend;
  EVAL_CHECK( expr->as.binary.lhs_expr, &minuend );
  EVAL_CHECK( expr->as.binary.rhs_expr, &subtrahend );
  if ( ad_expr_get_base_type( &minuend ) == T_INT &&
       ad_expr_get_base_type( &subtrahend ) == T_INT ) {
    ad_expr_set_int( rv, minuend.as.value.as.i64 - subtrahend.as.value.as.i64 );
    return true;
  }
  if ( ad_expr_get_base_type( &minuend ) == T_INT &&
       ad_expr_get_base_type( &subtrahend ) == T_FLOAT ) {
    ad_expr_set_double( rv,
      minuend.as.value.as.i64 - subtrahend.as.value.as.f64
    );
    return true;
  }
  assert( ad_expr_get_base_type( &minuend ) == T_FLOAT );
  if ( ad_expr_get_base_type( &subtrahend ) == T_INT ) {
    ad_expr_set_double( rv,
      minuend.as.value.as.f64 - subtrahend.as.value.as.i64
    );
    return true;
  }
  assert( subtrahend.expr_id == T_FLOAT );
  ad_expr_set_double( rv,
    minuend.as.value.as.f64 - subtrahend.as.value.as.f64
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

    case AD_EXPR_ADD:
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

    case AD_EXPR_DIV:
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
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      ad_expr_set_bool( rv,
        !ad_expr_is_zero( &temp1 ) ^ !ad_expr_is_zero( &temp2 )
      );
      return true;

    case AD_EXPR_MOD:
      return ad_expr_mod( expr, rv );

    case AD_EXPR_MUL:
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

    case AD_EXPR_SHIFT_LEFT:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_int( rv, temp1.as.value.as.i64 << temp2.as.value.as.i64 );
      return true;

    case AD_EXPR_SHIFT_RIGHT:
      EVAL_CHECK( expr->as.binary.lhs_expr, &temp1 );
      TYPE_CHECK( &temp1, T_INT );
      EVAL_CHECK( expr->as.binary.rhs_expr, &temp2 );
      TYPE_CHECK( &temp2, T_INT );
      ad_expr_set_int( rv, temp1.as.value.as.i64 >> temp2.as.value.as.i64 );
      return true;

    case AD_EXPR_SUB:
      return ad_expr_sub( expr, rv );

    case AD_EXPR_IF_ELSE:
      ad_expr_eval( expr->as.ternary.cond_expr, &temp1 );
      return ad_expr_is_zero( &temp1 ) ?
        ad_expr_eval( expr->as.ternary.false_expr, rv ) :
        ad_expr_eval( expr->as.ternary.true_expr, rv );
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

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
