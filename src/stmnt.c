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
#include "types.h"

// standard
#include <assert.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Execution context.
 */
struct ad_exec_ctx {
  bool    in_switch;                    ///< True only if within `switch`.
};

/**
 * Statement execution return value.
 */
enum ad_exec_rv {
  EXEC_OK,                              ///< Executed successfully.
  EXEC_BREAK,                           ///< Executed successfully, but break.
  EXEC_ERROR                            ///< An error occurred.
};

typedef struct  ad_exec_ctx ad_exec_ctx_t;
typedef enum    ad_exec_rv  ad_exec_rv_t;

// local functions
NODISCARD
static ad_exec_rv_t ad_stmnt_if( ad_stmnt_t const*, ad_exec_ctx_t const* ),
                    ad_stmnt_let( ad_stmnt_t const*, ad_exec_ctx_t const* ),
                    ad_stmnt_switch( ad_stmnt_t const*, ad_exec_ctx_t const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Executes an **ad** `break` statement.
 *
 * @param stmnt The \ref ad_stmnt_break to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t ad_stmnt_break( ad_stmnt_t const *stmnt,
                                    ad_exec_ctx_t const *ctx ) {
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
static ad_exec_rv_t ad_stmnt_decl( ad_stmnt_t const *stmnt,
                                   ad_exec_ctx_t const *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_DECL );
  assert( ctx != NULL );

  // TODO

  return EXEC_OK;
}

/**
 * TODO
 *
 * @param stmnts The list of statements to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t ad_stmnt_exec_impl( ad_stmnt_list_t const *stmnts,
                                        ad_exec_ctx_t const *ctx ) {
  assert( stmnts != NULL );
  assert( ctx != NULL );

  ad_exec_rv_t rv = EXEC_OK;

  FOREACH_SLIST_NODE( stmnt_node, stmnts ) {
    ad_stmnt_t *const stmnt = stmnt_node->data;
    switch ( stmnt->kind ) {
      case AD_STMNT_BREAK:
        rv = ad_stmnt_break( stmnt, ctx );
        break;
      case AD_STMNT_DECL:
        rv = ad_stmnt_decl( stmnt, ctx );
        break;
      case AD_STMNT_IF:
        rv = ad_stmnt_if( stmnt, ctx );
        break;
      case AD_STMNT_LET:
        rv = ad_stmnt_let( stmnt, ctx );
        break;
      case AD_STMNT_SWITCH:
        rv = ad_stmnt_switch( stmnt, ctx );
        break;
    } // switch
    if ( rv == EXEC_BREAK )
      break;
  } // for

  return rv;
}

/**
 * Compile an **ad** `if` statement.
 *
 * @param stmnt The input statement to compile.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t ad_stmnt_if( ad_stmnt_t const *stmnt,
                                 ad_exec_ctx_t const *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_IF );
  assert( ctx != NULL );

  uint64_t if_val;
  if ( !ad_expr_eval_uint( stmnt->if_stmnt.expr, &if_val ) )
    return EXEC_ERROR;
  return ad_stmnt_exec_impl( &stmnt->if_stmnt.list[ if_val != 0 ], ctx );
}

/**
 * Executes an **ad** `let` statement.
 *
 * @param stmnt The \ref ad_let_stmnt to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t ad_stmnt_let( ad_stmnt_t const *stmnt,
                                  ad_exec_ctx_t const *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_LET );
  assert( ctx != NULL );

  // TODO

  return EXEC_OK;
}

/**
 * Executes an **ad** `switch` statement.
 *
 * @param stmnt The input statement to execute.
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t ad_stmnt_switch( ad_stmnt_t const *stmnt,
                                     ad_exec_ctx_t const *ctx ) {
  assert( stmnt != NULL );
  assert( stmnt->kind == AD_STMNT_SWITCH );
  assert( ctx != NULL );

  uint64_t switch_val;
  if ( !ad_expr_eval_uint( stmnt->switch_stmnt.expr, &switch_val ) )
    return EXEC_ERROR;

  FOREACH_SWITCH_CASE( case_node, stmnt ) {
    ad_switch_case_t const *const case_ = case_node->data;

    uint64_t case_val;
    if ( !ad_expr_eval_uint( case_->expr, &case_val ) )
      return EXEC_ERROR;

    if ( case_val == switch_val ) {
      ad_exec_ctx_t const switch_ctx = { .in_switch = true };
      if ( !ad_stmnt_exec_impl( &case_->stmnts, &switch_ctx ) )
        return EXEC_ERROR;
      break;
    }
  } // for

  return EXEC_OK;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_stmnt_exec( ad_stmnt_list_t const *stmnts ) {
  assert( stmnts != NULL );
  return ad_stmnt_exec_impl( stmnts, &(ad_exec_ctx_t){ 0 } );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

