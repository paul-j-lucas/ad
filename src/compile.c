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
  compile_ctx_t  *parent_ctx;           ///< Parent context, if any.
  bool            in_switch;            ///< True only if within `switch`.
};

////////// local functions ////////////////////////////////////////////////////

static bool ad_compile_statement( ad_stmnt_t *const in_statement,
                                  array_t *out_statements,
                                  compile_ctx_t *ctx ) {
  assert( in_statement != NULL );
  assert( out_statements != NULL );
  assert( ctx != NULL );

  switch ( in_statement->kind ) {
    case AD_STMNT_BREAK:
      if ( !ctx->in_switch ) {
        print_error( &in_statement->loc,
          "\"break\" must be within \"switch\"\n"
        );
        return false;
      }
      break;
    case AD_STMNT_DECLARATION:
      break;
    case AD_STMNT_IF:
      break;
    case AD_STMNT_SWITCH:
      break;
  } // switch
}

static bool ad_compile_impl( slist_t const *in_statements,
                             array_t *out_statements, compile_ctx_t *ctx ) {
  assert( in_statements != NULL );
  assert( out_statements != NULL );
  assert( ctx != NULL );

  FOREACH_SLIST_NODE( in_statement_node, in_statements ) {
    ad_stmnt_t *const in_statement = in_statement_node->data;
    ad_compile_statement( in_statement, out_statements, ctx );
  } // for

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

