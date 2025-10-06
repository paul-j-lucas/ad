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

/// @endcond

///////////////////////////////////////////////////////////////////////////////

bool ad_expr_check_type( ad_expr_t const *expr );

/**
 * Gets the name of \a err.
 *
 * @param err The \ref ad_expr_err to get the name for.
 * @return Returns said name.
 */
NODISCARD
char const* ad_expr_err_name( ad_expr_err_t err );

/**
 * Evalulates an expression.
 *
 * @param expr The expression to evaluate.
 * @param rv Receives the evaluated expression's value.
 * @return Returns `true` only if the evaluation succeeded.
 *
 * @sa ad_expr_eval_uint()
 */
NODISCARD
bool ad_expr_eval( ad_expr_t const *expr, ad_expr_t *rv );

/**
 * Evaluates an expression that is expected to be unsigned integral.
 *
 * @param expr The expression to evaluate.
 * @param rv Receives the evaluated expression's value.
 * @return Returns `true` only if the evaluation succeeded.
 *
 * @sa ad_expr_eval()
 */
NODISCARD
bool ad_expr_eval_uint( ad_expr_t const *expr, uint64_t *rv );

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
NODISCARD 
inline bool ad_expr_is_literal( ad_expr_t const *expr ) {
  return expr->expr_kind == AD_EXPR_LITERAL;
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
 * Gets whether \a expr is zero.
 *
 * @param expr The expresion to check.
 * @return Returns `true` only of \a expr is zero.
 */
NODISCARD
bool ad_expr_is_zero( ad_expr_t const *expr );

/**
 * Gets the name \a kind.
 *
 * @param kind The \ref ad_expr_kind to get the name for.
 * @return Returns said name.
 */
NODISCARD
char const* ad_expr_kind_name( ad_expr_kind_t kind );

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
 * Gets the type of \a expr.
 *
 * @param expr The expression to get the type of.
 * @return Returns said type.
 *
 * @sa ad_expr_base_tid()
 */
NODISCARD
inline ad_tid_t ad_expr_tid( ad_expr_t const *expr ) {
  return ad_expr_is_literal( expr ) ? expr->literal.type->tid : T_NONE;
}

/**
 * Gets the base type of \a expr.  The base type of a type is the type without
 * either the sign or size.
 *
 * @param expr The expression to get the base type of.
 * @return Returns said base type.
 */
NODISCARD
inline ad_tid_t ad_expr_tid_base( ad_expr_t const *expr ) {
  return ad_expr_tid( expr ) & T_MASK_TYPE;
}

/**
 * Frees the memory associated with \a value.
 *
 * @param value The literal value to free.
 */
void ad_literal_free( ad_literal_expr_t *value );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_expr_H */
/* vim:set et sw=2 ts=2: */
