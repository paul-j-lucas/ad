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

/**
 * Ensures \a EXPR is `true`, otherwise sets the implicit \a rv to \a ERR.
 *
 * @param EXPR The expression to test.
 * @param ERR The error to set if \a EXPR is `false`.
 */
#define ENSURE_ELSE(EXPR,ERR) BLOCK(      \
  if ( !(EXPR) ) {                        \
    ad_expr_set_err( rv, AD_ERR_##ERR );  \
    return false;                         \
  } )

/**
 * Evalulates an expression by:
 *  1. Declaring a variable VAR_PFX_expr of type ad_expr_t.
 *  2. Evaluating the expression into it.
 *  3. If the evaluation fails, returns false.
 *
 * @param FIELD The expression field (`unary`, `binary`, or `ternary`).
 * @param VAR_PFX The prefix of the variable to create.
 */
#define EVAL_EXPR(FIELD,VAR_PFX)                                          \
  ad_expr_t VAR_PFX##_expr;                                               \
  if ( !ad_expr_eval( expr->as.FIELD.VAR_PFX##_expr, &VAR_PFX##_expr ) )  \
    return false;                                                         \
  assert( ad_expr_is_value( &VAR_PFX##_expr ) )

/**
 * Gets the base type of an expression by:
 *  1. Declaring a variable VAR_PFX_type.
 *  2. Getting the base type of VAR_PFX_expr into it.
 *
 * @param VAR_PFX The prefix of the type variable to create.
 */
#define GET_TYPE(VAR_PFX) \
  ad_type_id_t const VAR_PFX##_type = ad_expr_get_base_type( &VAR_PFX##_expr )

/**
 * Checks that ...
 *
 * @param VAR_PFX The prefix of the type variable to check.
 * @param TYPE The bitwise-or of types to check against.
 */
#define CHECK_TYPE(VAR_PFX,TYPE) \
  ENSURE_ELSE( (VAR_PFX##_type & (TYPE)) != T_NONE, BAD_OPERAND )

////////// local functions ////////////////////////////////////////////////////

static void narrow( ad_expr_t *expr ) {
  ad_type_id_t const to_type = expr->as.value.type;
  assert( ((to_type & T_MASK_TYPE) & T_NUMBER) != T_NONE );

  switch ( to_type ) {
    case T_BOOL8:
      expr->as.value.as.u64 = expr->as.value.as.u64 ? 1 : 0;
      break;
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
  assert( ad_expr_is_value( expr ) );
  assert( ad_expr_get_base_type( expr ) == T_INT );

  switch ( expr->as.value.type ) {
    case T_INT8:
    case T_INT16:
    case T_INT32:
      ;
  } // switch
  return expr->as.value.as.u64;
}

static bool ad_expr_bit_and( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  CHECK_TYPE( lhs, T_INT );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );
  CHECK_TYPE( rhs, T_INT );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 & rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_bit_comp( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_TYPE( sub );
  CHECK_TYPE( sub, T_INT );
  ad_expr_set_i( rv, ~sub_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_bit_or( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  CHECK_TYPE( lhs, T_INT );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );
  CHECK_TYPE( rhs, T_INT );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 | rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_bit_shift_left( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  CHECK_TYPE( lhs, T_INT );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );
  CHECK_TYPE( rhs, T_INT );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 << rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_bit_shift_right( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  CHECK_TYPE( lhs, T_INT );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );
  CHECK_TYPE( rhs, T_INT );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 >> rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_bit_xor( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  CHECK_TYPE( lhs, T_INT );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );
  CHECK_TYPE( rhs, T_INT );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 ^ rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_cast( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  ad_expr_t *const cast_expr = expr->as.binary.rhs_expr;
  assert( cast_expr->expr_id == AD_EXPR_CAST );
  ad_type_id_t const cast_type = ad_expr_get_type( cast_expr );

  switch ( cast_type & T_MASK_TYPE ) {

    case T_BOOL:
      *rv = *expr;
      rv->as.value.type = cast_type;
      switch ( lhs_type ) {
        case T_BOOL:
        case T_INT:
          if ( (lhs_type & T_MASK_SIZE) < (cast_type & T_MASK_SIZE) )
            narrow( rv );
          break;
        case T_FLOAT:
          rv->as.value.as.u64 = rv->as.value.as.f64 ? 1 : 0;
          break;
      } // switch
      break;

    case T_INT:
      *rv = *expr;
      rv->as.value.type = cast_type;
      switch ( lhs_type ) {
        case T_BOOL:
        case T_INT:
          if ( (lhs_type & T_MASK_SIZE) < (cast_type & T_MASK_SIZE) )
            narrow( rv );
          break;
        case T_FLOAT:
          rv->as.value.as.i64 = (int32_t)rv->as.value.as.f64;
          break;
      } // switch
      break;

    case T_FLOAT:
      rv->as.value.type = cast_type;
      switch ( lhs_type & ~T_MASK_SIZE ) {
        case T_INT:
          rv->as.value.as.f64 = lhs_expr.as.value.as.u64;
          break;
        case T_SIGNED | T_INT:
          rv->as.value.as.f64 = lhs_expr.as.value.as.i64;
          break;
      } // switch
      break;
  } // switch

  return true;
}

static bool ad_expr_if_else( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( ternary, cond );
  return ad_expr_is_zero( &cond_expr ) ?
    ad_expr_eval( expr->as.ternary.false_expr, rv ) :
    ad_expr_eval( expr->as.ternary.true_expr, rv );
}

static bool ad_expr_log_and( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  if ( ad_expr_is_zero( &lhs_expr ) ) {
    ad_expr_set_b( rv, false );
  } else {
    EVAL_EXPR( binary, rhs );
    ad_expr_set_b( rv, !ad_expr_is_zero( &rhs_expr ) );
  }
  return true;
}

static bool ad_expr_log_not( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  ad_expr_set_b( rv, ad_expr_is_zero( &sub_expr ) );
  return true;
}

static bool ad_expr_log_or( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  if ( !ad_expr_is_zero( &lhs_expr ) ) {
    ad_expr_set_b( rv, true );
  } else {
    EVAL_EXPR( binary, rhs );
    ad_expr_set_b( rv, !ad_expr_is_zero( &rhs_expr ) );
  }
  return true;
}

static bool ad_expr_log_xor( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  CHECK_TYPE( lhs, T_INT );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );
  CHECK_TYPE( rhs, T_INT );
  ad_expr_set_b( rv,
    !ad_expr_is_zero( &lhs_expr ) ^ !ad_expr_is_zero( &rhs_expr )
  );
  return true;
}

static bool ad_expr_math_add( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ad_expr_set_i( rv, lhs_expr.as.value.as.i64 + rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ad_expr_set_f( rv, lhs_expr.as.value.as.i64 + rhs_expr.as.value.as.f64 );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ad_expr_set_f( rv, lhs_expr.as.value.as.f64 + rhs_expr.as.value.as.i64 );
    return true;
  }

  //assert( rhs_expr.expr_id == T_FLOAT );
  ad_expr_set_f( rv, lhs_expr.as.value.as.f64 + rhs_expr.as.value.as.f64 );
  return true;
}

static bool ad_expr_math_div( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_i( rv, lhs_expr.as.value.as.i64 / rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0.0, DIV_0 );
    ad_expr_set_f( rv, lhs_expr.as.value.as.i64 / rhs_expr.as.value.as.f64 );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_f( rv, lhs_expr.as.value.as.f64 / rhs_expr.as.value.as.i64 );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0.0, DIV_0 );
  ad_expr_set_f( rv, lhs_expr.as.value.as.f64 / rhs_expr.as.value.as.f64 );
  return true;
}

static bool ad_expr_math_mod( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_i( rv, lhs_expr.as.value.as.i64 % rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0.0, DIV_0 );
    ad_expr_set_f( rv,
      fmod( lhs_expr.as.value.as.i64, rhs_expr.as.value.as.f64 )
    );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );
    ad_expr_set_f( rv,
      fmod( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.i64 )
    );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ENSURE_ELSE( rhs_expr.as.value.as.f64 != 0, DIV_0 );
  ad_expr_set_f( rv,
    fmod( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.f64 )
  );
  return true;
}

static bool ad_expr_math_mul( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ad_expr_set_i( rv, lhs_expr.as.value.as.i64 * rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ad_expr_set_f( rv, lhs_expr.as.value.as.i64 * rhs_expr.as.value.as.f64 );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ad_expr_set_f( rv, lhs_expr.as.value.as.f64 * rhs_expr.as.value.as.i64 );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ad_expr_set_f( rv, lhs_expr.as.value.as.f64 * rhs_expr.as.value.as.f64 );
  return true;
}

static bool ad_expr_math_neg( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  ad_type_id_t const t = ad_expr_get_base_type( &sub_expr );
  if ( t == T_INT ) {
    ad_expr_set_i( rv, -sub_expr.as.value.as.i64 );
  } else {
    assert( t == T_FLOAT );
    ad_expr_set_f( rv, -sub_expr.as.value.as.f64 );
  }
  return true;
}

static bool ad_expr_math_sub( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );

  if ( lhs_type == T_INT && rhs_type == T_INT ) {
    ad_expr_set_i( rv, lhs_expr.as.value.as.i64 - rhs_expr.as.value.as.i64 );
    return true;
  }

  if ( lhs_type == T_INT && rhs_type == T_FLOAT ) {
    ad_expr_set_f( rv,
      lhs_expr.as.value.as.i64 - rhs_expr.as.value.as.f64
    );
    return true;
  }

  assert( lhs_type == T_FLOAT );
  if ( rhs_type == T_INT ) {
    ad_expr_set_f( rv,
      lhs_expr.as.value.as.f64 - rhs_expr.as.value.as.i64
    );
    return true;
  }

  assert( rhs_type == T_FLOAT );
  ad_expr_set_f( rv,
    lhs_expr.as.value.as.f64 - rhs_expr.as.value.as.f64
  );
  return true;
}

static bool ad_expr_rel_greater( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_TYPE( rhs );

  if ( lhs_type == T_INT ) {
    if ( rhs_type == T_INT ) {
      ad_expr_set_i( rv, lhs_expr.as.value.as.u64 > rhs_expr.as.value.as.u64 );
      return true;
    } else if ( rhs_type == T_FLOAT ) {
      ad_expr_set_i( rv, lhs_expr.as.value.as.u64 > rhs_expr.as.value.as.f64 );
      return true;
    }
  }

  return true;
}

static bool ad_expr_rel_greater_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  EVAL_EXPR( binary, rhs );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 >= rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_rel_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  EVAL_EXPR( binary, rhs );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 == rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_rel_less( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  EVAL_EXPR( binary, rhs );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 < rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_rel_less_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  EVAL_EXPR( binary, rhs );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 <= rhs_expr.as.value.as.u64 );
  return true;
}

static bool ad_expr_rel_not_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  EVAL_EXPR( binary, rhs );
  ad_expr_set_i( rv, lhs_expr.as.value.as.u64 != rhs_expr.as.value.as.u64 );
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_expr_eval( ad_expr_t const *expr, ad_expr_t *rv ) {
  switch ( expr->expr_id ) {
    case AD_EXPR_NONE:
      break;
    case AD_EXPR_ERROR:
    case AD_EXPR_VALUE:
      *rv = *expr;
      break;

    case AD_EXPR_IF_ELSE:
      return ad_expr_if_else( expr, rv );

    case AD_EXPR_BIT_SHIFT_LEFT:
      return ad_expr_bit_shift_left( expr, rv );

    case AD_EXPR_BIT_SHIFT_RIGHT:
      return ad_expr_bit_shift_right( expr, rv );

    case AD_EXPR_BIT_AND:
      return ad_expr_bit_and( expr, rv );

    case AD_EXPR_BIT_COMP:
      return ad_expr_bit_comp( expr, rv );

    case AD_EXPR_BIT_OR:
      return ad_expr_bit_or( expr, rv );

    case AD_EXPR_BIT_XOR:
      return ad_expr_bit_xor( expr, rv );

    case AD_EXPR_CAST:
      return ad_expr_cast( expr, rv );

    case AD_EXPR_DEREF:
      // TODO
      return true;

    case AD_EXPR_LOG_AND:
      return ad_expr_log_and( expr, rv );

    case AD_EXPR_LOG_NOT:
      return ad_expr_log_not( expr, rv );

    case AD_EXPR_LOG_OR:
      return ad_expr_log_or( expr, rv );

    case AD_EXPR_LOG_XOR:
      return ad_expr_log_xor( expr, rv );

    case AD_EXPR_MATH_DIV:
      return ad_expr_math_div( expr, rv );

    case AD_EXPR_MATH_MOD:
      return ad_expr_math_mod( expr, rv );

    case AD_EXPR_MATH_MUL:
      return ad_expr_math_mul( expr, rv );

    case AD_EXPR_MATH_NEG:
      return ad_expr_math_neg( expr, rv );

    case AD_EXPR_MATH_ADD:
      return ad_expr_math_add( expr, rv );

    case AD_EXPR_MATH_SUB:
      return ad_expr_math_sub( expr, rv );

    case AD_EXPR_REL_GREATER:
      return ad_expr_rel_greater( expr, rv );

    case AD_EXPR_REL_GREATER_EQ:
      return ad_expr_rel_greater_eq( expr, rv );

    case AD_EXPR_REL_EQ:
      return ad_expr_rel_eq( expr, rv );

    case AD_EXPR_REL_LESS:
      return ad_expr_rel_less( expr, rv );

    case AD_EXPR_REL_LESS_EQ:
      return ad_expr_rel_less_eq( expr, rv );

    case AD_EXPR_REL_NOT_EQ:
      return ad_expr_rel_not_eq( expr, rv );

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
      break;
    case AD_EXPR_VALUE:
      ad_value_free( &expr->as.value );
      break;
  } // switch
  free( expr );
}

bool ad_expr_is_zero( ad_expr_t const *expr ) {
  if ( ad_expr_is_value( expr ) ) {
    ad_type_id_t const t = ad_expr_get_base_type( expr );
    switch ( t ) {
      case T_BOOL:
      case T_INT:
        return expr->as.value.as.u64 == 0;
      case T_FLOAT:
        return expr->as.value.as.f64 == 0.0;
    } // switch
  }
  return false;
}

ad_expr_t* ad_expr_new( ad_expr_id_t expr_id ) {
  ad_expr_t *const e = MALLOC( ad_expr_t, 1 );
  MEM_ZERO( e );
  e->expr_id = expr_id;
  return e;
}

void ad_expr_set_b( ad_expr_t *expr, bool bval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type = T_BOOL8;
  expr->as.value.as.u64 = bval;
}

void ad_expr_set_f( ad_expr_t *expr, double dval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type = T_FLOAT64;
  expr->as.value.as.f64 = dval;
}

void ad_expr_set_err( ad_expr_t *expr, ad_expr_err_t err ) {
  expr->expr_id = AD_EXPR_ERROR;
  expr->as.value.as.err = err;
}

void ad_expr_set_i( ad_expr_t *expr, int64_t ival ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type = T_INT64;
  expr->as.value.as.i64 = ival;
}

void ad_value_free( ad_value_expr_t *value ) {
  if ( (value->type & T_NULL) != T_NONE )
    FREE( value->as.s8 );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
