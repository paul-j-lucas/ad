/*
**      ad -- ASCII dump
**      src/dump_format.c
**
**      Copyright (C) 2024  Paul J. Lucas
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
 * Defines types and functions for dumping a file in a specified format.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "expr.h"
#include "lexer.h"
#include "options.h"
#include "typedef.h"
#include "util.h"
#include "ad_parser.h"                  /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <sysexits.h>

/// @endcond

/**
 * @addtogroup dump-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

// local functions
NODISCARD
static bool ad_switch_exec( ad_switch_statement_t const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Executes and **ad** declaration.
 *
 * @param decl The \ref ad_decl to execute.
 * @return Returns `true` only if successful.
 */
NODISCARD
static bool ad_decl_exec( ad_decl_t const *decl ) {
  assert( decl != NULL );

  // TODO

  return true;
}

/**
 * Executes an **ad** statement.
 *
 * @param statement The \ref ad_statement to execute.
 * @return Returns `true` only if successful.
 */
NODISCARD
static bool ad_statement_exec( ad_statement_t const *statement ) {
  assert( statement != NULL );

  switch ( statement->kind ) {
    case AD_STMNT_BREAK:
      // TODO
      break;
    case AD_STMNT_DECLARATION:
      // TODO
      if ( !ad_decl_exec( &statement->decl_s ) )
        return false;
      break;
    case AD_STMNT_SWITCH:
      if ( !ad_switch_exec( &statement->switch_s ) )
        return false;
      break;
  } // switch

  return true;
}

/**
 * Executes an **ad** `switch` statement.
 *
 * @param switch_ The \ref ad_switch_statement to execute.
 * @return Returns `true` only if successful.
 */
NODISCARD
static bool ad_switch_exec( ad_switch_statement_t const *switch_ ) {
  assert( switch_ != NULL );

  ad_expr_t switch_rv_expr;
  if ( !ad_expr_eval( switch_->expr, &switch_rv_expr ) )
    return false;
  FOREACH_SLIST_NODE( case_expr_node, &switch_->case_list ) {
    ad_expr_t const *const case_expr = case_expr_node->data;
    ad_expr_t case_rv_expr;
    if ( !ad_expr_eval( case_expr, &case_rv_expr ) )
      return false;
    // TODO
  } // for

  return true;
}

/////////// extern functions //////////////////////////////////////////////////

/**
 * Dumps a file in a specified format.
 */
void dump_file_format( void ) {
  FILE *const file = fopen( opt_format_path, "r" );
  PERROR_EXIT_IF( file == NULL, EX_NOINPUT );
  lexer_init();
  ad_typedefs_init();
  yyrestart( file );
  int const rv = yyparse();
  fclose( file );
  if ( unlikely( rv == 2 ) ) {
    //
    // Bison has already printed "memory exhausted" via yyerror() that doesn't
    // print a newline, so print one now.
    //
    EPUTC( '\n' );
    _Exit( EX_SOFTWARE );
  }

  if ( rv != 0 )
    exit( EX_DATAERR );

  extern slist_t statement_list;
  FOREACH_SLIST_NODE( statement_node, &statement_list ) {
    if ( !ad_statement_exec( statement_node->data ) )
      break;
  } // for

  parser_cleanup();
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
