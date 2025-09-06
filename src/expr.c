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
#define AD_EXPR_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "expr.h"
#include "print.h"
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
#define EVAL_EXPR(FIELD,VAR_PFX)                                        \
  ad_expr_t VAR_PFX##_expr;                                             \
  if ( !ad_expr_eval( expr->FIELD.VAR_PFX##_expr, &(VAR_PFX##_expr) ) ) \
    return false;                                                       \
  assert( ad_expr_is_literal( &VAR_PFX##_expr ) )

/**
 * Gets the base type of an expression by:
 *  1. Declaring a variable VAR_PFX_tid.
 *  2. Getting the base type of VAR_PFX_expr into it.
 *
 * @param VAR_PFX The prefix of the type variable to create.
 */
#define GET_BASE_TYPE(VAR_PFX)                                          \
  ad_tid_t const VAR_PFX##_tid = ad_expr_tid_base( &(VAR_PFX##_expr) ); \
  if ( VAR_PFX##_tid == T_ERROR )                                       \
    return false

/**
 * Checks that a variable VAR_PFX_tid is of type \a TYPE: if not, sets the
 * implicit \a rv to \a ERR and returns `false`.
 *
 * @param VAR_PFX The prefix of the type variable to check.
 * @param TYPE The bitwise-or of types to check against.
 */
#define CHECK_TYPE(VAR_PFX,TYPE) \
  ENSURE_ELSE( (VAR_PFX##_tid & (TYPE)) != T_NONE, BAD_OPERAND )

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
 * Decodes a single Unicode code-point to host-endian UTF-32.
 *
 * @param expr The expression whose Unicode code-point to decode.
 * @param cp A pointer to the code-point to be set.
 * @return Returns the Unicode code-point or #CP_INVALID.
 */
static char32_t ad_expr_utfxx_he32( ad_expr_t const *expr ) {
  char32_t cp;

  switch ( expr->literal.type->tid ) {
    case T_UTF8:
      return utf8c_32c( &expr->literal.c8 );
    case T_UTF16BE:
      return utf16s_32s( &expr->literal.c16, 1, ENDIAN_BIG, &cp ) ?
        cp : CP_INVALID;
    case T_UTF16LE:
      return utf16s_32s( &expr->literal.c16, 1, ENDIAN_LITTLE, &cp ) ?
        cp : CP_INVALID;
    case T_UTF32BE:
    case T_UTF32LE:
      return expr->literal.c32;
    default:
      return CP_INVALID;
  } // switch
}

/**
 * Decodes a null-terminated Unicode string into a null-terminated UTF-8
 * string.
 *
 * @param ps8 A pointer to the pointer to receive the UTF-8 string.
 * @return Returns `true` only if the entire Unicode string was decoded
 * successfully.
 */
static bool ad_expr_utfxx_8_0( ad_expr_t const *expr, char8_t **ps8 ) {
  switch ( expr->literal.type->tid ) {
    case T_UTF8_0:
      *ps8 = expr->literal.s8;
      break;
    case T_UTF16BE_0:
    case T_UTF16LE_0:
      break;
    case T_UTF32BE_0:
    case T_UTF32LE_0:
      // TODO: *ps8 = expr->literal.s32;
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
  ad_tid_t const lhs_tid = ad_expr_tid( lhs_expr );
  ad_tid_t const rhs_tid = ad_expr_tid( rhs_expr );

  if ( lhs_tid == rhs_tid ) {
    //
    // The Unicode encoding of the left- and right-hand sides matches so we can
    // compare them directly.
    //
    *cmp = !!(
      STATIC_CAST( int32_t, lhs_expr->literal.c32 ) -
      STATIC_CAST( int32_t, rhs_expr->literal.c32 )
    );
    return true;
  }

  char32_t const lhs_cp = ad_expr_utfxx_he32( lhs_expr );
  char32_t const rhs_cp = ad_expr_utfxx_he32( rhs_expr );

  if ( unlikely( lhs_cp == CP_INVALID || rhs_cp == CP_INVALID ) )
    return false;

  *cmp = !!(STATIC_CAST( int32_t, lhs_cp ) - STATIC_CAST( int32_t, rhs_cp ));
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
  ad_tid_t const to_tid = expr->literal.type->tid;
  assert( ((to_tid & T_MASK_TYPE) & T_NUMBER) != T_NONE );

  switch ( to_tid ) {
    case T_BOOL8:
      expr->literal.uval = expr->literal.uval ? 1 : 0;
      break;
    case T_FLOAT32:
      expr->literal.fval = (float)expr->literal.fval;
      break;
    case T_INT8:
      expr->literal.ival = (int8_t)expr->literal.ival;
      break;
    case T_INT16:
      expr->literal.ival = (int16_t)expr->literal.ival;
      break;
    case T_INT32:
      expr->literal.ival = (int32_t)expr->literal.ival;
      break;
    case T_INT64:
      expr->literal.ival = (int64_t)expr->literal.ival;
      break;
    case T_UINT8:
      expr->literal.uval = (uint8_t)expr->literal.uval;
      break;
    case T_UINT16:
      expr->literal.uval = (uint16_t)expr->literal.uval;
      break;
    case T_UINT32:
      expr->literal.uval = (uint32_t)expr->literal.uval;
      break;
    case T_UINT64:
      expr->literal.uval = (uint64_t)expr->literal.uval;
      break;
    case T_UTF16BE:
    case T_UTF16LE:
      expr->literal.uval = (char16_t)expr->literal.uval;
      break;
  } // switch
}

/**
 * Performs an array element access.
 *
 * @param expr The binary expression to perform the array access of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_array( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( binary, lhs );
  GET_BASE_TYPE( lhs );
  EVAL_EXPR( binary, rhs );
  GET_BASE_TYPE( rhs );
  (void)rv;
  // TODO
  return true;
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
  ad_expr_set_u( rv, lhs_expr.literal.uval & rhs_expr.literal.uval );
  return true;
}

/**
 * Performs a bitwise complement of a subexpression.
 *
 * @param expr The unary expression to perform the bitwise complement of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_bit_compl( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_BASE_TYPE( sub );
  CHECK_TYPE( sub, T_BOOL | T_INT | T_UTF );
  ad_expr_set_u( rv, ~sub_expr.literal.uval );
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
  ad_expr_set_u( rv, lhs_expr.literal.uval | rhs_expr.literal.uval );
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
  ad_expr_set_u( rv, lhs_expr.literal.uval << rhs_expr.literal.uval );
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
  ad_expr_set_u( rv, lhs_expr.literal.uval >> rhs_expr.literal.uval );
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
  ad_expr_set_u( rv, lhs_expr.literal.uval ^ rhs_expr.literal.uval );
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
  ad_expr_t *const cast_expr = expr->binary.rhs_expr;
  assert( cast_expr->expr_kind == AD_EXPR_CAST );
  ad_tid_t const cast_tid = ad_expr_tid( cast_expr );

  switch ( cast_tid & T_MASK_TYPE ) {

    case T_BOOL:
      *rv = *expr;
      rv->literal.type = cast_expr->literal.type;
      switch ( lhs_tid ) {
        case T_BOOL:
        case T_INT:
          if ( (lhs_tid & T_MASK_SIZE) < (cast_tid & T_MASK_SIZE) )
            narrow( rv );
          break;
        case T_FLOAT:
          rv->literal.uval = rv->literal.fval != 0.0 ? 1 : 0;
          break;
      } // switch
      break;

    case T_INT:
      *rv = *expr;
      rv->literal.type = cast_expr->literal.type;
      switch ( lhs_tid ) {
        case T_BOOL:
        case T_INT:
          if ( (lhs_tid & T_MASK_SIZE) < (cast_tid & T_MASK_SIZE) )
            narrow( rv );
          break;
        case T_FLOAT:
          rv->literal.ival = (int32_t)rv->literal.fval;
          break;
      } // switch
      break;

    case T_FLOAT:
      rv->literal.type = cast_expr->literal.type;
      switch ( lhs_tid & ~T_MASK_SIZE ) {
        case T_INT:
          rv->literal.fval = lhs_expr.literal.uval;
          break;
        case T_SIGNED | T_INT:
          rv->literal.fval = lhs_expr.literal.ival;
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
  return ad_expr_eval(
    ad_expr_is_zero( &cond_expr ) ?
      expr->ternary.false_expr : expr->ternary.true_expr,
    rv
  );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv, lhs_expr.literal.ival + rhs_expr.literal.ival );
          return true;
        case T_FLOAT:
          ad_expr_set_i( rv, lhs_expr.literal.ival + rhs_expr.literal.ival );
          return true;
        case T_UTF:
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv, lhs_expr.literal.fval + rhs_expr.literal.ival );
          return true;
        case T_FLOAT:
          ad_expr_set_f( rv, lhs_expr.literal.fval + rhs_expr.literal.fval );
          return true;
      } // switch
      break;
    case T_UTF:
      break;
  } // switch

  return true;
}

/**
 * Performs a post-decrement of a subexpression.
 *
 * @param expr The unary expression to perform the post-decrement of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_dec_post( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_BASE_TYPE( sub );
  CHECK_TYPE( sub, T_BOOL | T_INT );

  // TODO: verify the subexpression is an lvalue

  switch ( sub_tid ) {
    case T_BOOL:
      ad_expr_set_i( rv, sub_expr.literal.ival );
      sub_expr.literal.ival = 0;
      break;
    case T_INT:
      ad_expr_set_i( rv, sub_expr.literal.ival );
      --sub_expr.literal.ival;
      break;
  } // switch

  return true;
}

/**
 * Performs a pre-decrement of a subexpression.
 *
 * @param expr The unary expression to perform the pre-decrement of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_dec_pre( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_BASE_TYPE( sub );
  CHECK_TYPE( sub, T_BOOL | T_INT );

  // TODO: verify the subexpression is an lvalue

  switch ( sub_tid ) {
    case T_BOOL:
      ad_expr_set_i( rv, (sub_expr.literal.ival = 0) );
      break;
    case T_INT:
      ad_expr_set_i( rv, --sub_expr.literal.ival );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv, lhs_expr.literal.ival / rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, lhs_expr.literal.ival / rhs_expr.literal.fval );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv, lhs_expr.literal.fval / rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, lhs_expr.literal.fval / rhs_expr.literal.fval );
          break;
      } // switch
      break;
    default:
      RETURN_ERR( BAD_OPERAND );
  } // switch

  return true;
}

/**
 * Performs a post-increment of a subexpression.
 *
 * @param expr The unary expression to perform the post-increment of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_inc_post( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_BASE_TYPE( sub );
  CHECK_TYPE( sub, T_BOOL | T_INT );

  // TODO: verify the subexpression is an lvalue

  switch ( sub_tid ) {
    case T_BOOL:
      ad_expr_set_i( rv, sub_expr.literal.ival );
      sub_expr.literal.ival = 1;
      break;
    case T_INT:
      ad_expr_set_i( rv, sub_expr.literal.ival );
      ++sub_expr.literal.ival;
      break;
  } // switch

  return true;
}

/**
 * Performs a pre-increment of a subexpression.
 *
 * @param expr The unary expression to perform the pre-increment of.
 * @param rv A pointer to the return-value expression.
 * @return Returns `true` only if the evaluation succeeded.
 */
static bool ad_expr_math_inc_pre( ad_expr_t const *expr, ad_expr_t *rv ) {
  EVAL_EXPR( unary, sub );
  GET_BASE_TYPE( sub );
  CHECK_TYPE( sub, T_BOOL | T_INT );

  // TODO: verify the subexpression is an lvalue

  switch ( sub_tid ) {
    case T_BOOL:
      ad_expr_set_i( rv, (sub_expr.literal.ival = 1) );
      break;
    case T_INT:
      ad_expr_set_i( rv, ++sub_expr.literal.ival );
      break;
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv, lhs_expr.literal.ival % rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, fmod( lhs_expr.literal.ival, rhs_expr.literal.fval ) );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv,
            fmod( lhs_expr.literal.fval, rhs_expr.literal.ival )
          );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, fmod( lhs_expr.literal.fval, rhs_expr.literal.fval ) );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv, lhs_expr.literal.ival * rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, lhs_expr.literal.ival * rhs_expr.literal.fval );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv, lhs_expr.literal.fval * rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, lhs_expr.literal.fval * rhs_expr.literal.fval );
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

  switch ( sub_tid ) {
    case T_BOOL:
    case T_INT:
      ad_expr_set_i( rv, -sub_expr.literal.ival );
      break;
    case T_FLOAT:
      ad_expr_set_f( rv, -sub_expr.literal.fval );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_i( rv, lhs_expr.literal.ival - rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, lhs_expr.literal.ival - rhs_expr.literal.fval );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_f( rv, lhs_expr.literal.fval - rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_f( rv, lhs_expr.literal.fval - rhs_expr.literal.fval );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.ival == rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            is_fequal( lhs_expr.literal.ival, rhs_expr.literal.fval )
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            is_fequal( lhs_expr.literal.fval, rhs_expr.literal.ival )
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            is_fequal( lhs_expr.literal.fval, rhs_expr.literal.fval )
          );
          break;
      } // switch
      break;
    case T_UTF:
      switch ( rhs_tid ) {
        case T_UTF:;
          int cmp;
          if ( !ad_expr_utfxx_cmp( &lhs_expr, &rhs_expr, &cmp ) )
            RETURN_ERR( BAD_OPERAND );
          ad_expr_set_b( rv, cmp == 0 );
          break;
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.ival > rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.ival > rhs_expr.literal.fval );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.fval > rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.fval > rhs_expr.literal.fval );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.ival >= rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.ival >= rhs_expr.literal.fval );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.fval >= rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.fval >= rhs_expr.literal.fval );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.ival < rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.ival < rhs_expr.literal.fval );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.fval < rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.fval < rhs_expr.literal.fval );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.ival <= rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.ival <= rhs_expr.literal.fval );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.fval <= rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv, lhs_expr.literal.fval <= rhs_expr.literal.fval );
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

  switch ( lhs_tid ) {
    case T_BOOL:
    case T_INT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv, lhs_expr.literal.ival != rhs_expr.literal.ival );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            !is_fequal( lhs_expr.literal.ival, rhs_expr.literal.fval )
          );
          break;
      } // switch
      break;
    case T_FLOAT:
      switch ( rhs_tid ) {
        case T_BOOL:
        case T_INT:
          ad_expr_set_b( rv,
            !is_fequal( lhs_expr.literal.fval, rhs_expr.literal.ival )
          );
          break;
        case T_FLOAT:
          ad_expr_set_b( rv,
            !is_fequal( lhs_expr.literal.fval, rhs_expr.literal.fval )
          );
          break;
      } // switch
      break;
  } // switch

  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_expr_check_type( ad_expr_t const *expr ) {
  assert( expr != NULL );
  ad_tid_t cond_tid, false_tid, lhs_tid, rhs_tid;

  switch ( expr->expr_kind & AD_EXPR_MASK ) {
    case AD_EXPR_TERNARY:
      cond_tid  = ad_expr_tid_base( expr->ternary.cond_expr  );
      false_tid = ad_expr_tid_base( expr->ternary.false_expr );
      if ( (cond_tid & false_tid) == T_NONE ) {
        print_error( &expr->ternary.false_expr->loc,
          "type of \"false\" expression incompatible with \"condition\"\n"
        );
        return false;
      }
      FALLTHROUGH;

    case AD_EXPR_BINARY:
      lhs_tid = ad_expr_tid_base( expr->binary.lhs_expr );
      rhs_tid = ad_expr_tid_base( expr->binary.rhs_expr );
      if ( (lhs_tid & rhs_tid) == T_NONE ) {
        return false;
      }
      FALLTHROUGH;

    case AD_EXPR_UNARY:
      // nothing to do
      break;
  } // switch
  return true;
}

char const* ad_expr_err_name( ad_expr_err_t err ) {
  switch ( err ) {
    case AD_ERR_NONE        : return "none";
    case AD_ERR_BAD_OPERAND : return "bad operand";
    case AD_ERR_DIV_0       : return "divide by zero";
  } // switch
  UNEXPECTED_INT_VALUE( err );
}

bool ad_expr_eval( ad_expr_t const *expr, ad_expr_t *rv ) {
  switch ( expr->expr_kind ) {
    case AD_EXPR_ARRAY:
      return ad_expr_array( expr, rv );

    case AD_EXPR_ASSIGN:
    case AD_EXPR_ERROR:
    case AD_EXPR_LITERAL:
    case AD_EXPR_NONE:
      *rv = *expr;
      break;

    case AD_EXPR_BIT_AND:
      return ad_expr_bit_and( expr, rv );

    case AD_EXPR_BIT_COMPL:
      return ad_expr_bit_compl( expr, rv );

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

    case AD_EXPR_COMMA:
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

    case AD_EXPR_MATH_MOD:
      return ad_expr_math_mod( expr, rv );

    case AD_EXPR_MATH_ADD:
      return ad_expr_math_add( expr, rv );

    case AD_EXPR_MATH_DEC_PRE:
      return ad_expr_math_dec_pre( expr, rv );

    case AD_EXPR_MATH_DEC_POST:
      return ad_expr_math_dec_post( expr, rv );

    case AD_EXPR_MATH_DIV:
      return ad_expr_math_div( expr, rv );

    case AD_EXPR_MATH_INC_PRE:
      return ad_expr_math_inc_pre( expr, rv );

    case AD_EXPR_MATH_INC_POST:
      return ad_expr_math_inc_post( expr, rv );

    case AD_EXPR_MATH_MUL:
      return ad_expr_math_mul( expr, rv );

    case AD_EXPR_MATH_NEG:
      return ad_expr_math_neg( expr, rv );

    case AD_EXPR_MATH_SUB:
      return ad_expr_math_sub( expr, rv );

    case AD_EXPR_NAME:
      // TODO
      return true;

    case AD_EXPR_SIZEOF:
      // TODO
      return true;

    case AD_EXPR_PTR_ADDR:
      // TODO
      return true;

    case AD_EXPR_PTR_DEREF:
      // TODO
      return true;

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

    case AD_EXPR_STRUCT_MBR_REF:
      // TODO
      return true;

    case AD_EXPR_STRUCT_MBR_DEREF:
      // TODO
      return true;
  } // switch

  return false;
}

bool ad_expr_eval_uint( ad_expr_t const *expr, uint64_t *rv ) {
  assert( expr != NULL );
  assert( rv != NULL );

  ad_expr_t rv_expr;
  if ( !ad_expr_eval( expr, &rv_expr ) )
    return false;
  assert( rv_expr.expr_kind == AD_EXPR_LITERAL );
  switch ( ad_tid_kind( rv_expr.literal.type->tid ) ) {
    case T_NONE:
    case T_ERROR:
      return false;
    case T_BOOL:
    case T_ENUM:
    case T_INT:
      *rv = rv_expr.literal.uval;
      return true;
    case T_UTF:
      if ( ad_tid_is_null_term( rv_expr.literal.type->tid ) )
        return false;
      *rv = STATIC_CAST( uint64_t, rv_expr.literal.fval );
      return true;
    case T_FLOAT:
      print_error( &expr->loc,
        "expression must be integral\n"
      );
      return false;
    case T_STRUCT:
    case T_UNION:
    case T_TYPEDEF:
      return false;
  } // switch
}

void ad_expr_free( ad_expr_t *expr ) {
  if ( expr == NULL )
    return;
  switch ( expr->expr_kind & AD_EXPR_MASK ) {
    case AD_EXPR_LITERAL:
      ad_literal_free( &expr->literal );
      break;
    case AD_EXPR_NAME:
      FREE( expr->name );
      break;
    case AD_EXPR_TERNARY:
      ad_expr_free( expr->ternary.false_expr );
      FALLTHROUGH;
    case AD_EXPR_BINARY:
      ad_expr_free( expr->binary.rhs_expr );
      FALLTHROUGH;
    case AD_EXPR_UNARY:
      ad_expr_free( expr->unary.sub_expr );
      break;
  } // switch
  free( expr );
}

char const* ad_expr_kind_name( ad_expr_kind_t kind ) {
  switch ( kind ) {
    case AD_EXPR_ARRAY            : return "[]";
    case AD_EXPR_ASSIGN           : return "=";
    case AD_EXPR_BIT_AND          : return "bitand";
    case AD_EXPR_BIT_COMPL        : return "compl";
    case AD_EXPR_BIT_OR           : return "bitor";
    case AD_EXPR_BIT_SHIFT_LEFT   : return "<<";
    case AD_EXPR_BIT_SHIFT_RIGHT  : return ">>";
    case AD_EXPR_BIT_XOR          : return "bitxor";
    case AD_EXPR_CAST             : return "cast";
    case AD_EXPR_COMMA            : return ",";
    case AD_EXPR_ERROR            : return "error";
    case AD_EXPR_IF_ELSE          : return "?:";
    case AD_EXPR_LITERAL          : return "literal";
    case AD_EXPR_LOG_AND          : return "&&";
    case AD_EXPR_LOG_NOT          : return "!";
    case AD_EXPR_LOG_OR           : return "||";
    case AD_EXPR_LOG_XOR          : return "xor";
    case AD_EXPR_MATH_ADD         : return "add";
    case AD_EXPR_MATH_DEC_POST    : return "post--";
    case AD_EXPR_MATH_DEC_PRE     : return "--pre";
    case AD_EXPR_MATH_DIV         : return "div";
    case AD_EXPR_MATH_INC_POST    : return "post++";
    case AD_EXPR_MATH_INC_PRE     : return "++pre";
    case AD_EXPR_MATH_MOD         : return "mod";
    case AD_EXPR_MATH_MUL         : return "mul";
    case AD_EXPR_MATH_NEG         : return "neg";
    case AD_EXPR_MATH_SUB         : return "sub";
    case AD_EXPR_NAME             : return "name";
    case AD_EXPR_NONE             : return "none";
    case AD_EXPR_PTR_ADDR         : return "addr";
    case AD_EXPR_PTR_DEREF        : return "deref";
    case AD_EXPR_REL_EQ           : return "==";
    case AD_EXPR_REL_GREATER      : return ">";
    case AD_EXPR_REL_GREATER_EQ   : return ">=";
    case AD_EXPR_REL_LESS         : return "<";
    case AD_EXPR_REL_LESS_EQ      : return "<=";
    case AD_EXPR_REL_NOT_EQ       : return "!=";
    case AD_EXPR_SIZEOF           : return "sizeof";
    case AD_EXPR_STRUCT_MBR_REF   : return ".";
    case AD_EXPR_STRUCT_MBR_DEREF : return "->";
  } // switch

  UNEXPECTED_INT_VALUE( kind );
  return NULL;
}

bool ad_expr_is_zero( ad_expr_t const *expr ) {
  if ( ad_expr_is_literal( expr ) ) {
    ad_tid_t const tid = ad_expr_tid_base( expr );
    switch ( tid ) {
      case T_BOOL:
      case T_INT:
        return expr->literal.uval == 0;
      case T_FLOAT:
        return is_fzero( expr->literal.fval );
      case T_UTF:
        return expr->literal.c32 == 0;
    } // switch
  }
  return false;
}

ad_expr_t* ad_expr_new( ad_expr_kind_t expr_kind, ad_loc_t const *loc ) {
  assert( loc != NULL );

  ad_expr_t *const e = MALLOC( ad_expr_t, 1 );
  *e = (ad_expr_t){
    .expr_kind = expr_kind,
    .loc = *loc
  };
  return e;
}

void ad_expr_set_b( ad_expr_t *expr, bool bval ) {
  expr->expr_kind = AD_EXPR_LITERAL;
  expr->literal.type = &TB_BOOL8;
  expr->literal.uval = bval;
}

void ad_expr_set_f( ad_expr_t *expr, double dval ) {
  expr->expr_kind = AD_EXPR_LITERAL;
  expr->literal.type = &TB_FLOAT64;
  expr->literal.fval = dval;
}

void ad_expr_set_err( ad_expr_t *expr, ad_expr_err_t err ) {
  expr->expr_kind = AD_EXPR_ERROR;
  expr->err = err;
}

void ad_expr_set_i( ad_expr_t *expr, int64_t ival ) {
  expr->expr_kind = AD_EXPR_LITERAL;
  expr->literal.type = &TB_INT64;
  expr->literal.ival = ival;
}

void ad_expr_set_u( ad_expr_t *expr, uint64_t uval ) {
  expr->expr_kind = AD_EXPR_LITERAL;
  expr->literal.type = &TB_UINT64;
  expr->literal.uval = uval;
}

void ad_literal_free( ad_literal_expr_t *value ) {
  //
  // If the type has a null-terminated value, then the value bits are a pointer
  // that must be free'd.  (It doesn't matter which pointer type we free.)
  //
  if ( (value->type->tid & T_NULL) != T_NONE )
    free( value->s );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
