/*
**      ad -- ASCII dump
**      src/expr.h
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

#ifndef ad_expr_H
#define ad_expr_H

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <time.h>

_GL_INLINE_HEADER_BEGIN
#ifndef AD_EXPR_INLINE
# define AD_EXPR_INLINE _GL_INLINE
#endif /* AD_EXPR_INLINE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

#define AD_EXPR_UNARY   0x0100
#define AD_EXPR_BINARY  0x0200
#define AD_EXPR_TERNARY 0x0400

#define AD_EXPR_MASK    0x0F00

///////////////////////////////////////////////////////////////////////////////

/**
 * Expression errors.
 */
enum ad_expr_err {
  ERR_NONE,                             ///< No error.
  ERR_BAD_OPERAND,                      ///< Bad operand, e.g., & for double.
  ERR_DIV_0,                            ///< Divide by 0.
};
typedef enum ad_expr_err ad_expr_err_t;

/**
 * The expression ID.
 */
enum ad_expr_id {
  AD_EXPR_NONE,
  AD_EXPR_ERROR,                        ///< Error expression.

  AD_EXPR_VALUE,                        ///< Constant value expression.

  // unary
  AD_EXPR_DEREF   = AD_EXPR_UNARY + 1,  ///< Dereference expression.
  AD_EXPR_NEG,                          ///< Negation expression.

  // binary
  AD_EXPR_ADD     = AD_EXPR_BINARY + 1, ///< Addition expression.
  AD_EXPR_CAST,                         ///< Cast expression.
  AD_EXPR_SUB,                          ///< Subtraction expression.
  AD_EXPR_MUL,                          ///< Multiplication expression.
  AD_EXPR_DIV,                          ///< Division expression.
  AD_EXPR_MOD,                          ///< Modulus expression.
  AD_EXPR_BIT_AND,                      ///< Bitwise-and expression.
  AD_EXPR_BIT_COMP,                     ///< Bitwise-complement expression.
  AD_EXPR_BIT_OR,                       ///< Bitwise-or expression.
  AD_EXPR_BIT_XOR,                      ///< Bitwise-exclusive-or expression.
  AD_EXPR_LOG_AND,                      ///< Logical-and expression.
  AD_EXPR_LOG_NOT,                      ///< Logical-not expression.
  AD_EXPR_LOG_OR,                       ///< Logical-or expression.
  AD_EXPR_LOG_XOR,                      ///< Logical-exclusive-or expression.
  AD_EXPR_SHIFT_LEFT,                   ///< Bitwise-left-shift expression.
  AD_EXPR_SHIFT_RIGHT,                  ///< Bitwise-right-shift expression.

  // ternary
  AD_EXPR_IF_ELSE = AD_EXPR_TERNARY + 1,///< If-else ?: expression.
};

///////////////////////////////////////////////////////////////////////////////

typedef struct ad_binary_expr   ad_binary_expr_t;
typedef struct ad_expr          ad_expr_t;
typedef enum   ad_expr_id       ad_expr_id_t;
typedef struct ad_ternary_expr  ad_ternary_expr_t;
typedef struct ad_unary_expr    ad_unary_expr_t;
typedef struct ad_value_expr    ad_value_expr_t;

/**
 * Unary expression; used for:
 *  + Dereference
 *  + Negation
 */
struct ad_unary_expr {
  ad_expr_t *sub_expr;
};

/**
 * Binary expression; used for:
 *  + Addition
 *  + Bitwise and
 *  + Bitwise complement
 *  + Bitwise or
 *  + Division
 *  + Left shift
 *  + Logical exclusive or
 *  + Logical or
 *  + Modulus
 *  + Multiplication
 *  + Right shift
 *  + Subtraction
 */
struct ad_binary_expr {
  ad_expr_t *lhs_expr;
  ad_expr_t *rhs_expr;
};

/**
 * Ternary expression; used for:
 *  + If-else
 */
struct ad_ternary_expr {
  ad_expr_t *cond_expr;
  ad_expr_t *true_expr;
  ad_expr_t *false_expr;
};

/**
 * Constant value expression.
 */
struct ad_value_expr {
  ad_type_id_t    type;                 ///< The type of the value.
  union {
    int64_t       i64;
    uint64_t      u64;
    double        f64;
    ad_expr_err_t err;
    ad_type_t     type;                 ///< For casting.
  } as;
};

/**
 * An expression.
 */
struct ad_expr {
  ad_expr_id_t  expr_id;

  union {
    ad_unary_expr_t   unary;
    ad_binary_expr_t  binary;
    ad_ternary_expr_t ternary;
    ad_value_expr_t   value;
  } as;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Evalulates an expression.
 *
 * @param expr The expression to evaluate.
 * @param rv The evaluated expression's value.
 * @return Returns `true` only i the evaluation succeeded.
 */
bool ad_expr_eval( ad_expr_t const *expr, ad_expr_t *rv );

/**
 * Frees an expression.
 *
 * @param expr The expression to free.
 */
void ad_expr_free( ad_expr_t *expr );

AD_EXPR_INLINE ad_type_id_t ad_expr_get_type( ad_expr_t const *expr ) {
  return expr->expr_id == AD_EXPR_VALUE ? expr->as.value.type : T_NONE;
}

AD_EXPR_INLINE ad_type_id_t ad_expr_get_base_type( ad_expr_t const *expr ) {
  return ad_expr_get_type( expr ) & T_MASK_TYPE;
}

AD_EXPR_INLINE bool ad_expr_is_zero( ad_expr_t const *expr ) {
  return expr->as.value.as.u64 == 0;
}

AD_EXPR_INLINE void ad_expr_set_bool( ad_expr_t *expr, bool bval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.as.u64 = bval;
}

AD_EXPR_INLINE void ad_expr_set_double( ad_expr_t *expr, double dval ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.as.f64 = dval;
}

AD_EXPR_INLINE void ad_expr_set_err( ad_expr_t *expr, ad_expr_err_t err ) {
  expr->expr_id = AD_EXPR_ERROR;
  expr->as.value.as.err = err;
}

AD_EXPR_INLINE void ad_expr_set_int( ad_expr_t *expr, long ival ) {
  expr->expr_id = AD_EXPR_VALUE;
  expr->as.value.as.i64 = ival;
}

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* ad_expr_H */
/* vim:set et sw=2 ts=2: */
