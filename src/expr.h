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
#include "unicode.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef AD_EXPR_INLINE
# define AD_EXPR_INLINE _GL_INLINE
#endif /* AD_EXPR_INLINE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

#define AD_EXPR_UNARY       0x0100      /**< Unary expression.    */
#define AD_EXPR_BINARY      0x0200      /**< Binary expression.   */
#define AD_EXPR_TERNARY     0x0400      /**< Ternary expression.  */

#define AD_EXPR_MASK        0x0F00      ///< Expression type bitmask.

///////////////////////////////////////////////////////////////////////////////

/**
 * Expression errors.
 */
enum ad_expr_err {
  AD_ERR_NONE,                          ///< No error.
  AD_ERR_BAD_OPERAND,                   ///< Bad operand, e.g., & for double.
  AD_ERR_DIV_0,                         ///< Divide by 0.
};

/**
 * The expression kind.
 */
enum ad_expr_kind {
  AD_EXPR_NONE,                         ///< No expression.
  AD_EXPR_ERROR,                        ///< Error expression.

  AD_EXPR_VALUE,                        ///< Constant value expression.

  // unary
  AD_EXPR_BIT_COMP = AD_EXPR_UNARY + 1, ///< Bitwise-complement expression.
  AD_EXPR_MATH_NEG,                     ///< Negation expression.
  AD_EXPR_PTR_ADDR,                     ///< Address-of expression.
  AD_EXPR_PTR_DEREF,                    ///< Dereference expression.

  // binary
  AD_EXPR_ASSIGN  = AD_EXPR_BINARY + 1, ///< Assign expression.
  AD_EXPR_CAST,                         ///< Cast expression.

  AD_EXPR_BIT_AND,                      ///< Bitwise-and expression.
  AD_EXPR_BIT_OR,                       ///< Bitwise-or expression.
  AD_EXPR_BIT_SHIFT_LEFT,               ///< Bitwise-left-shift expression.
  AD_EXPR_BIT_SHIFT_RIGHT,              ///< Bitwise-right-shift expression.
  AD_EXPR_BIT_XOR,                      ///< Bitwise-exclusive-or expression.

  AD_EXPR_LOG_AND,                      ///< Logical-and expression.
  AD_EXPR_LOG_NOT,                      ///< Logical-not expression.
  AD_EXPR_LOG_OR,                       ///< Logical-or expression.
  AD_EXPR_LOG_XOR,                      ///< Logical-exclusive-or expression.

  AD_EXPR_MATH_ADD,                     ///< Addition expression.
  AD_EXPR_MATH_SUB,                     ///< Subtraction expression.
  AD_EXPR_MATH_MUL,                     ///< Multiplication expression.
  AD_EXPR_MATH_DIV,                     ///< Division expression.
  AD_EXPR_MATH_MOD,                     ///< Modulus expression.

  AD_EXPR_REL_EQ,                       ///< Equal.
  AD_EXPR_REL_NOT_EQ,                   ///< Not equal.
  AD_EXPR_REL_GREATER,                  ///< Greater than.
  AD_EXPR_REL_GREATER_EQ,               ///< Greater than or equal to.
  AD_EXPR_REL_LESS,                     ///< Less than.
  AD_EXPR_REL_LESS_EQ,                  ///< Less than or equal to.

  // ternary
  AD_EXPR_IF_ELSE = AD_EXPR_TERNARY + 1,///< If-else ?: expression.
};

///////////////////////////////////////////////////////////////////////////////

typedef struct ad_binary_expr   ad_binary_expr_t;
typedef struct ad_expr          ad_expr_t;
typedef enum   ad_expr_err      ad_expr_err_t;
typedef enum   ad_expr_kind     ad_expr_kind_t;
typedef struct ad_ternary_expr  ad_ternary_expr_t;
typedef struct ad_unary_expr    ad_unary_expr_t;
typedef struct ad_value_expr    ad_value_expr_t;

/**
 * Unary expression; used for:
 *  + Pointer address
 *  + Pointer dereference
 *  + Bitwise complement
 *  + Negation
 */
struct ad_unary_expr {
  ad_expr_t *sub_expr;
};

/**
 * Binary expression; used for:
 *  + Addition
 *  + Bitwise and
 *  + Bitwise or
 *  + Cast
 *  + Division
 *  + Left shift
 *  + Logical and
 *  + Logical or
 *  + Modulus
 *  + Multiplication
 *  + Right shift
 *  + Subtraction
 */
struct ad_binary_expr {
#if 1
  ad_expr_t *lhs_expr;
  ad_expr_t *rhs_expr;
#else
  ad_expr_t *sub_expr[2];
#endif
};

/**
 * Ternary expression; used for:
 *  + `?:`
 */
struct ad_ternary_expr {
  ad_expr_t *cond_expr;
  ad_expr_t *sub_expr[2];
};

/**
 * Constant value expression.
 */
struct ad_value_expr {
  ad_type_t       type;                 ///< The type of the value.
  union {
    // Integer.
    int64_t       i64;                  ///< i8, i16, i32, i64
    uint64_t      u64;                  ///< u8, u16, u32, u64

    // Floating-point.
    double        f64;                  ///< f32, f64

    // UTF characters.
    char8_t       c8;                   ///< UTF-8 character.
    char16_t      c16;                  ///< UTF-16 character.
    char32_t      c32;                  ///< UTF-32 character.

    // UTF strings.
    char8_t      *s8;                   ///< UTF-8 string.
    char16_t     *s16;                  ///< UTF-16 string.
    char32_t     *s32;                  ///< UTF-32 string.

    // Miscellaneous.
    ad_type_t     cast_type;
    ad_expr_err_t err;
  } as;                                 ///< Union discriminator.
};

/**
 * An expression.
 */
struct ad_expr {
  ad_expr_kind_t  expr_kind;            ///< Expression kind.

  union {
    ad_unary_expr_t   unary;            ///< Unary expression.
    ad_binary_expr_t  binary;           ///< Binary expression.
    ad_ternary_expr_t ternary;          ///< Ternary expression.
    ad_value_expr_t   value;            ///< Value expression.
  } as;                                 ///< Union discriminator.
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Evalulates an expression.
 *
 * @param expr The expression to evaluate.
 * @param rv The evaluated expression's value.
 * @return Returns `true` only if the evaluation succeeded.
 */
NODISCARD
bool ad_expr_eval( ad_expr_t const *expr, ad_expr_t *rv );

/**
 * Frees an expression.
 *
 * @param expr The expression to free.
 */
void ad_expr_free( ad_expr_t *expr );

/**
 * Gets whether \a expr is a value.
 *
 * @parm expr The expression to check.
 * @return Returns `true` only if \a expr is a value.
 */
NODISCARD AD_EXPR_INLINE
bool ad_expr_is_value( ad_expr_t const *expr ) {
  return expr->expr_kind == AD_EXPR_VALUE;
}

/**
 * Creates a new `ad_expr`.
 *
 * @param expr_kind The kind of the expression to create.
 * @return Returns a pointer to a new `ad_expr`.
 */
NODISCARD
ad_expr_t* ad_expr_new( ad_expr_kind_t expr_kind );

/**
 * Gets the type of \a expr.
 *
 * @param expr The expression to get the type of.
 * @return Returns said type.
 * @sa ad_expr_get_base_tid(ad_expr_t const*)
 */
NODISCARD AD_EXPR_INLINE
ad_tid_t ad_expr_get_tid( ad_expr_t const *expr ) {
  return ad_expr_is_value( expr ) ? expr->as.value.type.tid : T_NONE;
}

/**
 * Gets the base type of \a expr.  The base type of a type is the type without
 * either the sign or size.
 *
 * @param expr The expression to get the base type of.
 * @return Returns said base type.
 */
NODISCARD AD_EXPR_INLINE
ad_tid_t ad_expr_get_base_tid( ad_expr_t const *expr ) {
  return ad_expr_get_tid( expr ) & T_MASK_TYPE;
}

/**
 * Gets whether \a expr is zero.
 *
 * @param expr The expresion to check.
 * @return Returns `true` only of \a expr is zero.
 */
NODISCARD
bool ad_expr_is_zero( ad_expr_t const *expr );

/**
 * Sets the type of \a expr to #T_BOOL and its value to \a bval.
 *
 * @param expr The expression to set.
 * @param bval The boolean value.
 */
void ad_expr_set_b( ad_expr_t *expr, bool bval );

/**
 * Sets the type of \a expr to #T_ERR and its value to \a err.
 *
 * @param expr The expression to set.
 * @param err The error value.
 */
void ad_expr_set_err( ad_expr_t *expr, ad_expr_err_t err );

/**
 * Sets the type of \a expr to #T_FLOAT and its value to \a fval.
 *
 * @param expr The expression to set.
 * @param fval The floating-point value.
 */
void ad_expr_set_f( ad_expr_t *expr, double fval );

/**
 * Sets the type of \a expr to #T_INT and its value to \a ival.
 *
 * @param expr The expression to set.
 * @param ival The integer value.
 */
void ad_expr_set_i( ad_expr_t *expr, int64_t ival );

/**
 * Sets the type of \a expr to #T_INT and its value to \a uval.
 *
 * @param expr The expression to set.
 * @param uval The integer value.
 */
void ad_expr_set_u( ad_expr_t *expr, uint64_t ival );

/**
 * Frees the memory associated with \a value.
 *
 * @param value The value to free.
 */
void ad_value_free( ad_value_expr_t *value );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* ad_expr_H */
/* vim:set et sw=2 ts=2: */
