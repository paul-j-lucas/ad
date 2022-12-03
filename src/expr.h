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

_GL_INLINE_HEADER_BEGIN
#ifndef AD_EXPR_H_INLINE
# define AD_EXPR_H_INLINE _GL_INLINE
#endif /* AD_EXPR_H_INLINE */

/// @endcond

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
NODISCARD AD_EXPR_H_INLINE
bool ad_expr_is_value( ad_expr_t const *expr ) {
  return expr->expr_kind == AD_EXPR_VALUE;
}

/**
 * Creates a new `ad_expr`.
 *
 * @param expr_kind The kind of the expression to create.
 * @param loc A pointer to the token location data.
 * @return Returns a pointer to a new `ad_expr`.
 */
NODISCARD
ad_expr_t* ad_expr_new( ad_expr_kind_t expr_kind, ad_loc_t const *loc );

/**
 * Gets the type of \a expr.
 *
 * @param expr The expression to get the type of.
 * @return Returns said type.
 * @sa ad_expr_get_base_tid(ad_expr_t const*)
 */
NODISCARD AD_EXPR_H_INLINE
ad_tid_t ad_expr_get_tid( ad_expr_t const *expr ) {
  return ad_expr_is_value( expr ) ? expr->value.type.tid : T_NONE;
}

/**
 * Gets the base type of \a expr.  The base type of a type is the type without
 * either the sign or size.
 *
 * @param expr The expression to get the base type of.
 * @return Returns said base type.
 */
NODISCARD AD_EXPR_H_INLINE
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
