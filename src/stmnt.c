/*
**      ad -- ASCII dump
**      src/stmnt.c
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
 * Defines functions for executing statements.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "expr.h"
#include "print.h"
#include "slist.h"
#include "types.h"

// standard
#include <assert.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Compile context.
 */
struct ad_exec_ctx {
  bool    in_switch;                    ///< True only if within `switch`.
};

/**
 * Statement execution return value.
 */
enum ad_exec_rv {
  EXEC_ERROR,                           ///< An error occurred.
  EXEC_BREAK,                           ///< Break.
  EXEC_CONTINUE                         ///< Continue to next statement.
};

typedef struct  ad_exec_ctx ad_exec_ctx_t;
typedef enum    ad_exec_rv  ad_exec_rv_t;

// local functions
NODISCARD
static bool ad_let_exec( ad_stmnt_t const*, ad_exec_ctx_t* ),
            ad_switch_exec( ad_stmnt_t const*, ad_exec_ctx_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Executes an **ad** `break` statement.
 *
 * @param stmnt The \ref ad_stmnt_break to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv ad_stmnt_break( ad_stmnt_t const *stmnt,
                                  ad_exec_ctx_t *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_BREAK );
  assert( ctx != NULL );

  if ( !ctx->in_switch ) {
    print_error( &stmnt->loc, "\"break\" not within \"switch\"\n" );
    return EXEC_ERROR;
  }

  return EXEC_BREAK;
}

/**
 * Executes an **ad** declaration statement.
 *
 * @param stmnt The \ref ad_stmnt_decl to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv ad_stmnt_decl( ad_stmnt_t const *stmnt, ad_exec_ctx_t *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_DECL );
  assert( out_stmnts != NULL );
  assert( ctx != NULL );

  return EXEC_CONTINUE;
}

/**
 * Compile an **ad** `if` statement.
 *
 * @param stmnt The input statement to compile.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv ad_stmnt_if( ad_stmnt_t const *stmnt, ad_exec_ctx_t *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_IF );
  assert( ctx != NULL );

  uint64_t val;
  if ( !ad_expr_eval_uint( stmnt->if_stmnt.expr, &val ) )
    return EXEC_ERROR;

  ad_stmnt_list_t *const stmnts = val != 0 ?
    &stmnt->if_stmnt.if_list :
    &stmnt->if_stmnt.else_list;

  return ad_stmnt_exec_impl( stmnts, ctx );
}

/**
 * TODO
 *
 * @param stmnts The list of input statements to compile.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns `true` only if the statements compiled successfully.
 */
static ad_exec_rv ad_stmnt_exec_impl( ad_stmnt_list_t const *stmnts,
                                      ad_exec_ctx_t *ctx ) {
  assert( stmnts != NULL );
  assert( ctx != NULL );

  ad_exec_rv rv = EXEC_CONTINUE;

  FOREACH_SLIST_NODE( stmnt_node, stmnts ) {
    ad_stmnt_t *const stmnt = stmnt_node->data;
    switch ( stmnt->kind ) {
      case AD_STMNT_BREAK:
        rv = ad_stmnt_break( stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_DECL:
        rv = ad_stmnt_decl( stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_IF:
        rv = ad_stmnt_if( stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_LET:
        rv = ad_stmnt_let( stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_SWITCH:
        rv = ad_stmnt_switch( stmnt, out_stmnts, ctx );
        break;
    } // switch
    if ( rv == EXEC_BREAK )
      break;
  } // for

  return rv;
}

/**
 * Executes an **ad** `let` statement.
 *
 * @param stmnt The \ref ad_let_stmnt to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns `true` only if the statement executed successfully.
 */
static bool ad_stmnt_let( ad_stmnt_t const *stmnt, ad_exec_ctx_t *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_LET );
  assert( ctx != NULL );

  return true;
}

/**
 * Executes an **ad** `switch` statement.
 *
 * @param stmnt The input statement to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_stmnt_switch( ad_stmnt_t const *stmnt, ad_exec_ctx_t *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_SWITCH );
  assert( ctx != NULL );

  ad_exec_ctx_t switch_ctx = { .in_switch = true };

  // TODO: eval expr

  FOREACH_SWITCH_CASE( case_node, stmnt ) {
    ad_switch_case_t *const case_ = case_node->data;
    (void)case_;
  } // for

  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_stmnt_exec( ad_stmnt_list_t const *stmnts ) {
  assert( stmnts != NULL );

  ad_exec_ctx_t ctx = { 0 };
  return ad_stmnt_exec_impl( stmnts, &ctx );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

