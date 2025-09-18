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
#include "filebuf.h"
#include "options.h"
#include "print.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Execution context.
 */
struct ad_exec_ctx {
  filebuf_t fbuf;                       ///< File buffer.
  size_t    total_bytes_read;           ///< Total bytes read.
  unsigned  indent;                     ///< Current indentation.
  bool      in_switch;                  ///< True only if within `switch`.
};

/**
 * Statement execution return value.
 */
enum ad_exec_rv {
  EXEC_OK,                              ///< Executed successfully.
  EXEC_BREAK,                           ///< Executed successfully, but break.
  EXEC_NO_MATCH,                        ///< Declaration doesn't match.
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
 * Attempts to match bytes read against an \ref ad_enum_type.
 *
 * @param type TODO
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t match_enum( ad_type_t const *type,
                                ad_exec_ctx_t const *ctx ) {
  assert( type != NULL );
  assert( ctx != NULL );

  return EXEC_OK;
}

/**
 * Attempts to match bytes read against an \ref ad_int_type.
 *
 * @param type TODO
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t match_int( ad_type_t const *type,
                               ad_exec_ctx_t const *ctx ) {
  assert( type != NULL );
  assert( ctx != NULL );

  return EXEC_OK;
}

/**
 * Attempts to match bytes read against an \ref ad_struct_type.
 *
 * @param type TODO
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t match_struct( ad_type_t const *type,
                                  ad_exec_ctx_t const *ctx ) {
  assert( type != NULL );
  assert( ctx != NULL );

  return EXEC_OK;
}

/**
 * Attempts to match bytes read against an \ref ad_union_type.
 *
 * @param type TODO
 * @param ctx The \ref ad_exec_ctx to use.
 * @return Returns \ref ad_exec_rv.
 */
static ad_exec_rv_t match_union( ad_type_t const *type,
                                 ad_exec_ctx_t const *ctx ) {
  assert( type != NULL );
  assert( ctx != NULL );

  return EXEC_OK;
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

  if ( ctx->total_bytes_read >= opt_max_bytes )
    return EXEC_BREAK;

  ad_decl_stmnt_t const *const decl_stmnt = &stmnt->decl_stmnt;

  switch ( ad_tid_kind( decl_stmnt->type->tid ) ) {
    case T_BOOL:
    case T_ENUM:
      return match_enum( decl_stmnt->type, ctx );
    case T_FLOAT:
    case T_INT:
      return match_int( stmnt->decl_stmnt.type, ctx );
    case T_UTF:
      // TODO
      break;
    case T_STRUCT:
      return match_struct( stmnt->decl_stmnt.type, ctx );
    case T_UNION:
      return match_union( stmnt->decl_stmnt.type, ctx );
    case T_ERROR:
    case T_NONE:
    case T_TYPEDEF:
      unreachable();
  } // switch

  if ( unlikely( ferror( stdin ) != 0 ) ) {
    fatal_error( EX_IOERR,
      "\"%s\": read byte failed: %s\n", fin_path, STRERROR()
    );
  }

  return EXEC_OK;
}

/**
 * Executes a series of statements.
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
 * Executes an **ad** `if` statement.
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

  ad_expr_t if_expr;
  if ( !ad_expr_eval( stmnt->if_stmnt.expr, &if_expr ) )
    return EXEC_ERROR;
  bool const is_zero = ad_expr_is_zero( &if_expr );
  return ad_stmnt_exec_impl( &stmnt->if_stmnt.list[ is_zero ], ctx );
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
    ad_switch_case_t const *const switch_case = case_node->data;

    uint64_t case_val;
    if ( !ad_expr_eval_uint( switch_case->expr, &case_val ) )
      return EXEC_ERROR;

    if ( case_val == switch_val ) {
      ad_exec_ctx_t const switch_ctx = { .in_switch = true };
      if ( !ad_stmnt_exec_impl( &switch_case->stmnts, &switch_ctx ) )
        return EXEC_ERROR;
      break;
    }
  } // for

  return EXEC_OK;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_stmnt_exec( ad_stmnt_list_t const *stmnts ) {
  assert( stmnts != NULL );
#if 0
  ad_exec_ctx_t ctx = { 0 };
  filebuf_init( &ctx.fbuf, stdin );
  return ad_stmnt_exec_impl( stmnts, &ctx );
#else
  return true;
#endif
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

