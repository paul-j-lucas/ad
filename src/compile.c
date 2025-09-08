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
  bool    in_switch;                    ///< True only if within `switch`.
  slist_t break_list;                   ///< TODO.
};

// local functions
NODISCARD
static bool ad_compile_let( ad_stmnt_t const*, array_t*, compile_ctx_t* ),
            ad_compile_switch( ad_stmnt_t const*, array_t*, compile_ctx_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Compile an **ad** `break` statement.
 *
 * @param in_stmnt The input statement to compile.
 * @param out_stmnts The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_break( ad_stmnt_t const *in_stmnt, array_t *out_stmnts,
                              compile_ctx_t *ctx ) {
  assert( in_stmnt != NULL );
  assert( in_stmnt->kind == AD_STMNT_BREAK );
  assert( out_stmnts != NULL );
  assert( ctx != NULL );

  if ( !ctx->in_switch ) {
    print_error( &in_stmnt->loc, "\"break\" not within \"switch\"\n" );
    return false;
  }

  slist_push_back(
    &ctx->break_list,
    array_push_back( out_stmnts, in_stmnt )
  );

  return true;
}

/**
 * Compile an **ad** declaration statement.
 *
 * @param in_stmnt The input statement to compile.
 * @param out_stmnts The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_decl( ad_stmnt_t const *in_stmnt, array_t *out_stmnts,
                             compile_ctx_t *ctx ) {
  assert( in_stmnt != NULL );
  assert( in_stmnt->kind == AD_STMNT_DECL );
  assert( out_stmnts != NULL );
  assert( ctx != NULL );

  return true;
}

/**
 * Compile an **ad** `if` statement.
 *
 * @param in_stmnt The input statement to compile.
 * @param out_stmnts The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_if( ad_stmnt_t const *in_stmnt, array_t *out_stmnts,
                           compile_ctx_t *ctx ) {
  assert( in_stmnt != NULL );
  assert( in_stmnt->kind == AD_STMNT_IF );
  assert( out_stmnts != NULL );
  assert( ctx != NULL );

  return true;
}

/**
 * TODO
 *
 * @param in_stmnts The list of input statements to compile.
 * @param out_stmnts The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statements compiled successfully.
 */
static bool ad_compile_impl( slist_t const *in_stmnts, array_t *out_stmnts,
                             compile_ctx_t *ctx ) {
  assert( in_stmnts != NULL );
  assert( out_stmnts != NULL );
  assert( ctx != NULL );

  bool ok = true;

  FOREACH_SLIST_NODE( in_stmnt_node, in_stmnts ) {
    ad_stmnt_t *const in_stmnt = in_stmnt_node->data;
    switch ( in_stmnt->kind ) {
      case AD_STMNT_BREAK:
        ok = ad_compile_break( in_stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_DECL:
        ok = ad_compile_decl( in_stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_IF:
        ok = ad_compile_if( in_stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_LET:
        ok = ad_compile_let( in_stmnt, out_stmnts, ctx );
        break;
      case AD_STMNT_SWITCH:
        ok = ad_compile_switch( in_stmnt, out_stmnts, ctx );
        break;
    } // switch
    if ( !ok )
      return false;
  } // for

  return true;
}

/**
 * Compile an **ad** `let` statement.
 *
 * @param in_stmnt The input statement to compile.
 * @param out_stmnts The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_let( ad_stmnt_t const *in_stmnt, array_t *out_stmnts,
                            compile_ctx_t *ctx ) {
  assert( in_stmnt != NULL );
  assert( in_stmnt->kind == AD_STMNT_LET );
  assert( out_stmnts != NULL );
  assert( ctx != NULL );

  return true;
}

/**
 * Compile an **ad** `switch` statement.
 *
 * @param in_stmnt The input statement to compile.
 * @param out_stmnts The array of output statements to append to.
 * @param ctx The \ref compile_ctx to use.
 * @return Returns `true` only if the statement compiled successfully.
 */
static bool ad_compile_switch( ad_stmnt_t const *in_stmnt, array_t *out_stmnts,
                               compile_ctx_t *ctx ) {
  assert( in_stmnt != NULL );
  assert( in_stmnt->kind == AD_STMNT_SWITCH );
  assert( out_stmnts != NULL );
  assert( ctx != NULL );

  compile_ctx_t switch_ctx = { .in_switch = true };

  // TODO: eval expr

  FOREACH_SWITCH_CASE( case_node, in_stmnt ) {
    ad_switch_case_t *const case_ = case_node->data;
    (void)case_;
  } // for

  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_compile( slist_t const *in_stmnts, array_t *out_stmnts ) {
  assert( in_stmnts != NULL );
  assert( out_stmnts != NULL );

  compile_ctx_t ctx = { 0 };
  return ad_compile_impl( in_stmnts, out_stmnts, &ctx );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

