/*
**      ad -- ASCII dump
**      src/compile.c
**
**      Copyright (C) 2025  Paul J. Lucas, et al.
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

/**
 * @file
 * Defines functions for compiling statements.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "array.h"
#include "print.h"
#include "slist.h"
#include "types.h"

// standard
#include <assert.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

typedef struct compile_ctx compile_ctx_t;

/**
 * Compile context.
 */
struct compile_ctx {
  bool  in_switch;                      ///< True only if within `switch`.
};

static bool ad_compile_switch( ad_stmnt_t const*, array_t*, compile_ctx_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Compile an **ad** `break` statement.
 *
 * @param in_statement The input statement to compile.
 * @param out_statements The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_break( ad_stmnt_t const *in_statement,
                              array_t *out_statements,
                              compile_ctx_t *ctx ) {
  assert( in_statement != NULL );
  assert( out_statements != NULL );
  assert( ctx != NULL );

  if ( !ctx->in_switch ) {
    print_error( &in_statement->loc, "\"break\" must be within \"switch\"\n" );
    return false;
  }

  return true;
}

/**
 * Compile an **ad** declaration statement.
 *
 * @param in_statement The input statement to compile.
 * @param out_statements The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_decl( ad_stmnt_t const *in_statement,
                             array_t *out_statements, compile_ctx_t *ctx ) {
  assert( in_statement != NULL );
  assert( out_statements != NULL );
  assert( ctx != NULL );

  return true;
}

/**
 * Compile an **ad** `if` statement.
 *
 * @param in_statement The input statement to compile.
 * @param out_statements The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_if( ad_stmnt_t const *in_statement,
                           array_t *out_statements, compile_ctx_t *ctx ) {
  assert( in_statement != NULL );
  assert( out_statements != NULL );
  assert( ctx != NULL );

  return true;
}

/**
 * TODO
 *
 * @param in_statements The list of input statements to compile.
 * @param out_statements The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statements compiled successfully.
 */
static bool ad_compile_impl( slist_t const *in_statements,
                             array_t *out_statements, compile_ctx_t *ctx ) {
  assert( in_statements != NULL );
  assert( out_statements != NULL );
  assert( ctx != NULL );

  bool ok = true;

  FOREACH_SLIST_NODE( in_statement_node, in_statements ) {
    ad_stmnt_t *const in_statement = in_statement_node->data;
    switch ( in_statement->kind ) {
      case AD_STMNT_BREAK:
        ok = ad_compile_break( in_statement, out_statements, ctx );
        break;
      case AD_STMNT_DECL:
        ok = ad_compile_decl( in_statement, out_statements, ctx );
        break;
      case AD_STMNT_IF:
        ok = ad_compile_if( in_statement, out_statements, ctx );
        break;
      case AD_STMNT_SWITCH:
        ok = ad_compile_switch( in_statement, out_statements, ctx );
        break;
    } // switch
    if ( !ok )
      return false;
  } // for

  return true;
}

/**
 * Compile an **ad** `switch` statement.
 *
 * @param in_statement The input statement to compile.
 * @param out_statements The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_switch( ad_stmnt_t const *in_statement,
                               array_t *out_statements, compile_ctx_t *ctx ) {
  assert( in_statement != NULL );
  assert( out_statements != NULL );
  assert( ctx != NULL );

  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_compile( slist_t const *in_statements, array_t *out_statements ) {
  assert( in_statements != NULL );
  assert( out_statements != NULL );

  compile_ctx_t ctx = { 0 };
  return ad_compile_impl( in_statements, out_statements, &ctx );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

