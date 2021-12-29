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
#include <float.h>
#include <math.h>
#include <stdlib.h>

/**
 * Ensures \a EXPR is `true`, otherwise sets the implicit \a rv to \a ERR and
 * returns `false`.
 *
 * @param EXPR The expression to test.
 * @param ERR The error to set if \a EXPR is `false`.
 */
#define ENSURE_ELSE(EXPR,ERR) BLOCK( \
  if ( !(EXPR) ) RETURN_ERR(ERR); )

/**
 * Evalulates an expression by:
 *  1. Declaring a variable VAR_PFX_expr of type ad_expr_t.
 *  2. Evaluating the expression into it.
 *  3. If the evaluation fails, returns false.
 *
 * @param FIELD The expression field (`unary`, `binary`, or `ternary`).
 * @param VAR_PFX The prefix of the variable to create.
 */
#define EVAL_EXPR(FIELD,VAR_PFX)                                            \
  ad_expr_t VAR_PFX##_expr;                                                 \
  if ( !ad_expr_eval( expr->as.FIELD.VAR_PFX##_expr, &(VAR_PFX##_expr) ) )  \
    return false;                                                           \
  assert( ad_expr_is_value( &VAR_PFX##_expr ) )

/**
 * Gets the base type of an expression by:
 *  1. Declaring a variable VAR_PFX_type.
 *  2. Getting the base type of VAR_PFX_expr into it.
 *
 * @param VAR_PFX The prefix of the type variable to create.
 */
#define GET_BASE_TYPE(VAR_PFX) \
  ad_type_id_t const VAR_PFX##_type = ad_expr_get_base_type( &(VAR_PFX##_expr) ); \
  if ( VAR_PFX##_type == T_ERROR ) \
    return false

/**
 * Checks that a variable VAR_PFX_type is of type \a TYPE: if not, sets the
 * implicit \a rv to \a ERR and returns `false`.
 *
 * @param VAR_PFX The prefix of the type variable to check.
 * @param TYPE The bitwise-or of types to check against.
 */
#define CHECK_TYPE(VAR_PFX,TYPE) \
  ENSURE_ELSE( (VAR_PFX##_type & (TYPE)) != T_NONE, BAD_OPERAND )

/**
 * Sets an error and returns `false`.
 *
 * @param ERR The error to set.
 */
#define RETURN_ERR(ERR) BLOCK(          \
  ad_expr_set_err( rv, AD_ERR_##ERR );  \
  return false; )

////////// local functions ////////////////////////////////////////////////////

/**
 * Compares two signed integers and returns -1, 0, or +1.
 *
 * @param i The first integer.
 * @param j The second integer.
 * @return Returns -1 if \a i &lt; \a j, 0 if \a i = \a j, or \a i &gt; \a j.
 */
static inline int int32_cmp( int32_t i, int32_t j ) {
  int32_t const temp = i - j;
  return temp < 0 ? -1 : temp > 0 ? +1 : 0;
}

/**
 * Decodes a single Unicode code-point to host-endian UTF-32.
 *
 * @param expr The expression whose Unicode code-point to decode.
 * @param cp A pointer to the code-point to be set.
 * @return Returns the Unicode code-point or #CP_INVALID.
 */
static char32_t ad_expr_utfxx_he32( ad_expr_t const *expr ) {
  char32_t cp;

  switch ( expr->as.value.type.type_id ) {
    case T_UTF8:
      return utf8_32( (char const*)&expr->as.value.as.c32 );
    case T_UTF16_BE:
      return utf16_32( &expr->as.value.as.c16, 1, AD_ENDIAN_BIG, &cp ) ?
        cp : CP_INVALID;
    case T_UTF16_LE:
      return utf16_32( &expr->as.value.as.c16, 1, AD_ENDIAN_LITTLE, &cp ) ?
        cp : CP_INVALID;
    case T_UTF32_BE:
    case T_UTF32_LE:
      return expr->as.value.as.c32;
    default:
      return CP_INVALID;
  } // switch
}

/**
 * Decodes a null-terminated Unicode string into a null-terminated UTF-8
 * string.
 *
 * @param ps8 A pointer to the pointer to receive the UTF-8 string.
 * @return Returns `true` only if the entire Unicode string TODO.
 */
static bool ad_expr_utfxx_8_0( ad_expr_t const *expr, char8_t **ps8 ) {
  switch ( expr->as.value.type.type_id ) {
    case T_UTF8_0:
      *ps8 = expr->as.value.as.s8;
      break;
    case T_UTF16_BE_0:
    case T_UTF16_LE_0:
      break;
    case T_UTF32_BE_0:
    case T_UTF32_LE_0:
      // TODO: *ps8 = expr->as.value.as.s32;
      break;
  } // switch
  return true;
}

/**
 * Compares the two Unicode code-points of two UTF expressions.
 *
 * @param lhs_expr The left-hand-side expression.
 * @param rhs_expr The right-hand-side expression.
 * @param cmp Set to -1 if \a lhs_expr &lt; \a rhs_expr, 0 if \a lhs_expr = \a
 * rhs_expr, or \a lhs_expr &gt; \a rhs_expr.
 * @return Returns `true` only if there was no error during UTF-8 decoding.
 */
static bool ad_expr_utfxx_cmp( ad_expr_t const *lhs_expr,
                               ad_expr_t const *rhs_expr,
                               int *cmp ) {
  ad_type_id_t const lhs_type = ad_expr_get_type( lhs_expr );
  ad_type_id_t const rhs_type = ad_expr_get_type( rhs_expr );

  if ( lhs_type == rhs_type ) {
    //
    // The Unicode encoding of the left- and right-hand sides matches so we can
    // compare them directly.
    //
    *cmp = int32_cmp(
      (int32_t)lhs_expr->as.value.as.c32, (int32_t)rhs_expr->as.value.as.c32
    );
    return true;
  }

  char32_t lhs_cp, rhs_cp;
  if ( unlikely( (lhs_cp = ad_expr_utfxx_he32( lhs_expr )) == CP_INVALID ) ||
       unlikely( (rhs_cp = ad_expr_utfxx_he32( rhs_expr )) == CP_INVALID ) ) {
    return false;
  }

  *cmp = int32_cmp( (int32_t)lhs_cp, (int32_t)rhs_cp );
  return true;
}

/**
 * Compares \a d1 to \a d2 for equality.
 *
 * @param d1 The first `double` to check.
 * @param d2 The second `double` to check.
 * @return Returns `true` only if \a d1 is approximately equal to \a d2.
 */
static inline bool is_fequal( double d1, double d2 ) {
  double const diff = fabs( d1 - d2 );
  return  diff <= DBL_EPSILON ||
          diff < fmax( fabs( d1 ), fabs( d2 ) ) * DBL_EPSILON;
}

/**
 * Checks if a approximately `double` is zero.
 *
 * @param d The `double` to check.
 * @return Returns `true` only if \a d is approximately zero.
 */
static inline bool is_fzero( double d ) {
  return fabs( d ) <= DBL_EPSILON;
}

static void narrow( ad_expr_t *expr ) {
  ad_type_id_t const to_type = expr->as.value.type.type_id;
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
    case T_UTF16_BE:
    case T_UTF16_LE:
      expr->as.value.as.u64 = (char16_t)expr->as.value.as.u64;
      break;
  } // switch
}

static uint64_t widen_int( ad_expr_t const *expr ) {
  assert( ad_expr_is_value( expr ) );
  assert( ad_expr_get_base_type( expr ) == T_INT );

  switch ( expr->as.value.type.type_id ) {
    case T_INT8:
    case T_INT16:
    case T_INT32:
      ;
  } // switch
  return expr->as.value.as.u64;
}

/**
 * Performs a bitwise and of two subexpressions.
 *
 * @param expr The binary expression to perform the bitwise and of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_bit_and( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_UTF );
  ad_expr_set_u( rv, lhs_expr.as.value.as.u64 & rhs_expr.as.value.as.u64 );
  return true;
}

/**
 * Performs a bitwise complement of a subexpression.
 *
 * @param expr The unary expression to perform the bitwise complement of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_bit_comp( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_BASE_TYPE( sub );
  CHECK_TYPE( sub, T_BOOL | T_INT | T_UTF );
  ad_expr_set_u( rv, ~sub_expr.as.value.as.u64 );
  return true;
}

/**
 * Performs a bitwise or of two subexpressions.
 *
 * @param expr The binary expression to perform the bitwise or of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_bit_or( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_UTF );
  ad_expr_set_u( rv, lhs_expr.as.value.as.u64 | rhs_expr.as.value.as.u64 );
  return true;
}

/**
 * Performs a bitwise shift left of two subexpressions.
 *
 * @param expr The binary expression to perform the bitwise shift left of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_bit_shift_left( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT );
  ad_expr_set_u( rv, lhs_expr.as.value.as.u64 << rhs_expr.as.value.as.u64 );
  return true;
}

/**
 * Performs a bitwise shift right of two subexpressions.
 *
 * @param expr The binary expression to perform the bitwise shift right of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_bit_shift_right( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT );
  ad_expr_set_u( rv, lhs_expr.as.value.as.u64 >> rhs_expr.as.value.as.u64 );
  return true;
}

/**
 * Performs a bitwise exclusive or of two subexpressions.
 *
 * @param expr The binary expression to perform the bitwise exclusive or of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_bit_xor( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_UTF );
  ad_expr_set_u( rv, lhs_expr.as.value.as.u64 ^ rhs_expr.as.value.as.u64 );
  return true;
}

/**
 * Performs a cast of a subexpression.
 *
 * @param expr The binary expression to perform the cast of.  (The left-hand-
 * side is the subexpression to be cast; the right-hand-side is the type to
 * cast to.)
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_cast( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  ad_expr_t *const cast_expr = expr->as.binary.rhs_expr;
  assert( cast_expr->expr_id == AD_EXPR_CAST );
  ad_type_id_t const cast_type_id = ad_expr_get_type( cast_expr );

  switch ( cast_type_id & T_MASK_TYPE ) {

    case T_BOOL:
      *rv = *expr;
      rv->as.value.type.type_id = cast_type_id;
      switch ( lhs_type ) {
        case T_BOOL:
        case T_INT:
          if ( (lhs_type & T_MASK_SIZE) < (cast_type_id & T_MASK_SIZE) )
            narrow( rv );
          break;
        case T_FLOAT:
          rv->as.value.as.u64 = rv->as.value.as.f64 != 0.0 ? 1 : 0;
          break;
      } // switch
      break;

    case T_INT:
      *rv = *expr;
      rv->as.value.type.type_id = cast_type_id;
      switch ( lhs_type ) {
        case T_BOOL:
        case T_INT:
          if ( (lhs_type & T_MASK_SIZE) < (cast_type_id & T_MASK_SIZE) )
            narrow( rv );
          break;
        case T_FLOAT:
          rv->as.value.as.i64 = (int32_t)rv->as.value.as.f64;
          break;
      } // switch
      break;

    case T_FLOAT:
      rv->as.value.type.type_id = cast_type_id;
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

/**
 * Performs an if-else and of three subexpressions.
 *
 * @param expr The ternary expression to perform the if-else of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_if_else( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( ternary, cond );
  return ad_expr_is_zero( &cond_expr ) ?
    ad_expr_eval( expr->as.ternary.false_expr, rv ) :
    ad_expr_eval( expr->as.ternary.true_expr, rv );
}

/**
 * Performs a logical and of two subexpressions.
 *
 * @param expr The binary expression to perform the logical and of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
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

/**
 * Performs a logical not of a subexpression.
 *
 * @param expr The unary expression to perform the logical not of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_log_not( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  ad_expr_set_b( rv, ad_expr_is_zero( &sub_expr ) );
  return true;
}

/**
 * Performs a logical or of two subexpressions.
 *
 * @param expr The binary expression to perform the logical or of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
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

/**
 * Performs a logical exclusive or of two subexpressions.
 *
 * @param expr The binary expression to perform the logical exclusive or of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_log_xor( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_UTF );
  ad_expr_set_b( rv,
    !ad_expr_is_zero( &lhs_expr ) ^ !ad_expr_is_zero( &rhs_expr )
  );
  return true;
}

/**
 * Performs an addition of two subexpressions.
 *
 * @param expr The binary expression to perform the addition of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_add( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_FLOAT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_FLOAT | T_UTF );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv,
            lhs_expr.as.value.as.i64 + rhs_expr.as.value.as.i64
          );
          return true;
        case T_FLOAT:
          ad_expr_set_i( rv,
            lhs_expr.as.value.as.i64 + rhs_expr.as.value.as.i64
          );
          return true;
        case T_UTF:
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 + rhs_expr.as.value.as.i64
          );
          return true;
        case T_FLOAT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 + rhs_expr.as.value.as.f64
          );
          return true;
      } // switch
      break;
    case T_UTF:
      break;
  } // switch

  return true;
}

/**
 * Performs a division of two subexpressions.
 *
 * @param expr The binary expression to perform the division of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_div( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_FLOAT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_FLOAT | T_UTF );

  ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv,
            lhs_expr.as.value.as.i64 / rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.i64 / rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 / rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 / rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    default:
      RETURN_ERR( BAD_OPERAND );
  } // switch

  return true;
}

/**
 * Performs a mod of two subexpressions.
 *
 * @param expr The binary expression to perform the mod of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_mod( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_FLOAT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_FLOAT | T_UTF );

  ENSURE_ELSE( !ad_expr_is_zero( &rhs_expr ), DIV_0 );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv,
            lhs_expr.as.value.as.i64 % rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            fmod( lhs_expr.as.value.as.i64, rhs_expr.as.value.as.f64 )
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv,
            fmod( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.i64 )
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            fmod( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.f64 )
          );
          break;
      } // switch
      break;
    default:
      RETURN_ERR( BAD_OPERAND );
  } // switch

  return true;
}

/**
 * Performs a multiplication of two subexpressions.
 *
 * @param expr The binary expression to perform the multiplication of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_mul( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_FLOAT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_FLOAT | T_UTF );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv,
            lhs_expr.as.value.as.i64 * rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.i64 * rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 * rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 * rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    default:
      RETURN_ERR( BAD_OPERAND );
  } // switch

  return true;
}

/**
 * Performs a negation of a subexpression.
 *
 * @param expr The unary expression to perform the negation of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_neg( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_BASE_TYPE( sub );
  CHECK_TYPE( sub, T_BOOL | T_INT | T_FLOAT );

  switch ( sub_type ) {
    case T_BOOL:
    case T_INT:
      ad_expr_set_i( rv, -sub_expr.as.value.as.i64 );
      break;
    case T_FLOAT:
      ad_expr_set_f( rv, -sub_expr.as.value.as.f64 );
      break;
  } // switch

  return true;
}

/**
 * Performs a subtraction of two subexpressions.
 *
 * @param expr The binary expression to perform the subtraction of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_sub( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  CHECK_TYPE( lhs, T_BOOL | T_INT | T_FLOAT | T_UTF );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  CHECK_TYPE( rhs, T_BOOL | T_INT | T_FLOAT | T_UTF );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv,
            lhs_expr.as.value.as.i64 - rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.i64 - rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 - rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv,
            lhs_expr.as.value.as.f64 - rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
  } // switch

  return true;
}

/**
 * Performs a relational equals of two subexpressions.
 *
 * @param expr The binary expression to perform the relational equals of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_rel_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 == rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            is_fequal( lhs_expr.as.value.as.i64, rhs_expr.as.value.as.f64 )
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            is_fequal( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.i64 )
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            is_fequal( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.f64 )
          );
          break;
      } // switch
      break;
    case T_UTF:
      switch ( rhs_type ) {
        case T_UTF: {
          int cmp;
          if ( !ad_expr_utfxx_cmp( &lhs_expr, &rhs_expr, &cmp ) )
            RETURN_ERR( BAD_OPERAND );
          ad_expr_set_b( rv, cmp == 0 );
          break;
        }
      } // switch
      break;
  } // switch

  return true;
}

/**
 * Performs a relational greater of two subexpressions.
 *
 * @param expr The binary expression to perform the relational greater of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_rel_greater( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 > rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 > rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 > rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 > rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
  } // switch

  return true;
}

/**
 * Performs a relational greater-equal of two subexpressions.
 *
 * @param expr The binary expression to perform the relational greater-equal of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_rel_greater_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 >= rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 >= rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 >= rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 >= rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
  } // switch

  return true;
}

/**
 * Performs a relational less of two subexpressions.
 *
 * @param expr The binary expression to perform the relational less of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_rel_less( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 < rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 < rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 < rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 < rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
  } // switch

  return true;
}

/**
 * Performs a relational less-equal of two subexpressions.
 *
 * @param expr The binary expression to perform the relational less-equal of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_rel_less_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 <= rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 <= rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 <= rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.f64 <= rhs_expr.as.value.as.f64
          );
          break;
      } // switch
      break;
  } // switch

  return true;
}

/**
 * Performs a relational not equal of two subexpressions.
 *
 * @param expr The binary expression to perform the relational not equal of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_rel_not_eq( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );

  switch ( lhs_type ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            lhs_expr.as.value.as.i64 != rhs_expr.as.value.as.i64
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            !is_fequal( lhs_expr.as.value.as.i64, rhs_expr.as.value.as.f64 )
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_type ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            !is_fequal( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.i64 )
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            !is_fequal( lhs_expr.as.value.as.f64, rhs_expr.as.value.as.f64 )
          );
          break;
      } // switch
      break;
  } // switch

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

    case AD_EXPR_BIT_AND:
      return ad_expr_bit_and( expr, rv );

    case AD_EXPR_BIT_COMP:
      return ad_expr_bit_comp( expr, rv );

    case AD_EXPR_BIT_SHIFT_LEFT:
      return ad_expr_bit_shift_left( expr, rv );

    case AD_EXPR_BIT_SHIFT_RIGHT:
      return ad_expr_bit_shift_right( expr, rv );

    case AD_EXPR_BIT_OR:
      return ad_expr_bit_or( expr, rv );

    case AD_EXPR_BIT_XOR:
      return ad_expr_bit_xor( expr, rv );

    case AD_EXPR_CAST:
      return ad_expr_cast( expr, rv );

    case AD_EXPR_DEREF:
      // TODO
      return true;

    case AD_EXPR_IF_ELSE:
      return ad_expr_if_else( expr, rv );

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
      PJL_FALLTHROUGH;
    case AD_EXPR_BINARY:
      ad_expr_free( expr->as.binary.rhs_expr );
      PJL_FALLTHROUGH;
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
        return is_fzero( expr->as.value.as.f64 );
      case T_UTF:
        return expr->as.value.as.c32 == 0;
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
  expr->as.value.type.type_id = T_BOOL8;
  expr->as.value.as.u64 = bval;
}

void ad_expr_set_f( ad_expr_t *expr, double dval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type.type_id = T_FLOAT64;
  expr->as.value.as.f64 = dval;
}

void ad_expr_set_err( ad_expr_t *expr, ad_expr_err_t err ) {
  expr->expr_id = AD_EXPR_ERROR;
  expr->as.value.type.type_id = T_ERROR;
  expr->as.value.as.err = err;
}

void ad_expr_set_i( ad_expr_t *expr, int64_t ival ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type.type_id = T_INT64;
  expr->as.value.as.i64 = ival;
}

void ad_expr_set_u( ad_expr_t *expr, uint64_t uval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.type.type_id = T_UINT64;
  expr->as.value.as.u64 = uval;
}

void ad_value_free( ad_value_expr_t *value ) {
  //
  // If the type has a null-terminated value, then the value bits are a pointer
  // that must be free'd.  (It doesn't matter which pointer type we free.)
  //
  if ( (value->type.type_id & T_NULL) != T_NONE )
    FREE( value->as.s8 );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */