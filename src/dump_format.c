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
#include "symbol.h"
#include "util.h"
#include "ad_parser.h"                  /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <sysexits.h>

#define DUMP_FORMAT(D,...) BLOCK(               \
  FPUTNSP( (D)->indent * DUMP_INDENT, stdout ); \
  PRINTF( __VA_ARGS__ ); )

#define DUMP_INT(D,KEY,INT) \
  DUMP_KEY( (D), KEY ": %lld", STATIC_CAST( long long, (INT) ) )

#define DUMP_KEY(D,...) BLOCK(                \
  DUMP_FORMAT( (D), __VA_ARGS__ ); )

#define DUMP_STR(D,KEY,STR) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); fputs_quoted( (STR), '"', stdout ); )

/// @endcond

/**
 * @addtogroup dump-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Dump state.
 */
struct dump_state {
  unsigned indent;                      ///< Current indentation.
};
typedef struct dump_state dump_state_t;

// local functions
#if 0
NODISCARD
static bool ad_switch_exec( ad_switch_statement_t const*, dump_state_t* );
#endif

// local constants
#if 0
static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.
#endif

////////// local functions ////////////////////////////////////////////////////

#if 0
NODISCARD
static bool ad_type_match( ad_type_t const *type, dump_state_t *dump ) {
  (void)type;
  (void)dump;
  return false;
}
#endif

#if 0
/**
 * Executes and **ad** declaration.
 *
 * @param decl The \ref ad_decl to execute.
 * @return Returns `true` only if successful.
 */
NODISCARD
static bool ad_decl_exec( ad_decl_t const *decl, dump_state_t *dump ) {
  assert( decl != NULL );
  assert( dump != NULL );

  if ( decl->rep.kind == AD_REP_EXPR ) {
    // TODO
  }

  if ( ad_tid_kind( decl->type->tid ) == T_STRUCT ) {
    // decl->type->struct_t.member_list
  }

  if ( decl->match_expr != NULL ) {
    ad_expr_t match_rv_expr;
    if ( !ad_expr_eval( decl->match_expr, &match_rv_expr ) )
      return false;
    if ( ad_expr_is_zero( &match_rv_expr ) )
      return false;
  }

  // TODO
  // decl->name
  // decl->type
  // decl->rep

  return true;
}
#endif

#if 0
/**
 * Dumps \a literal.
 *
 * @param literal The \ref ad_literal_expr to dump.
 * @param dump The dump_state to use.
 */
static void ad_literal_expr_dump( ad_literal_expr_t const *literal,
                                  dump_state_t *dump ) {
  assert( literal != NULL );
  assert( dump != NULL );

  // TODO
}
#endif

#if 0
/**
 * Executes an **ad** statement.
 *
 * @param statement The \ref ad_statement to execute.
 * @return Returns `true` only if successful.
 */
NODISCARD
static bool ad_statement_exec( ad_statement_t const *statement,
                               dump_state_t *dump ) {
  assert( statement != NULL );
  assert( dump != NULL );

  switch ( statement->kind ) {
    case AD_STMNT_BREAK:
      // TODO
      break;
    case AD_STMNT_DECLARATION:
      // TODO
      if ( !ad_decl_exec( &statement->decl_s, dump ) )
        return false;
      break;
    case AD_STMNT_IF:
      if ( !ad_if_exec( &statement->if_s, dump ) )
        return false;
      break;
    case AD_STMNT_SWITCH:
      if ( !ad_switch_exec( &statement->switch_s, dump ) )
        return false;
      break;
  } // switch

  return true;
}
#endif

#if 0
/**
 * Executes an **ad** `switch` statement.
 *
 * @param switch_ The \ref ad_switch_statement to execute.
 * @return Returns `true` only if successful.
 */
NODISCARD
static bool ad_switch_exec( ad_switch_statement_t const *switch_,
                            dump_state_t *dump ) {
  assert( switch_ != NULL );
  assert( dump != NULL );

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
#endif

#if 0
/**
 * Gets a byte.
 *
 * @param pbyte A pointer to the byte to receive the newly read byte.
 * @return Returns `true` only if a byte was read successfully.
 */
NODISCARD
static bool get_byte( char8_t *pbyte ) {
  assert( pbyte != NULL );

  int const c = getchar();
  if ( unlikely( c == EOF ) ) {
    if ( unlikely( ferror( stdin ) ) ) {
      fatal_error( EX_IOERR,
        "\"%s\": read byte failed: %s\n", fin_path, STRERROR()
      );
    }
    return false;
  }

  *pbyte = STATIC_CAST( char8_t, c );
  return true;
}
#endif

#if 0
/**
 * TODO.
 *
 * @param buf TODO
 * @param size The number of bytes to read.
 * @return TODO
 */
NODISCARD
static bool get_buf( char8_t *buf, size_t size ) {
  assert( buf != NULL );

  size_t const bytes_read = fread( buf, size, 1, stdin );
  if ( unlikely( ferror( stdin ) ) ) {
    fatal_error( EX_IOERR,
      "\"%s\": read bytes failed: %s\n", fin_path, STRERROR()
    );
  }

  return true;
}
#endif

/////////// extern functions //////////////////////////////////////////////////

/**
 * Dumps a file in a specified format.
 *
 * @param format_path The path of the format file to parse.
 */
void dump_file_format( char const *format_path ) {
  assert( format_path != NULL );
  FILE *const file = fopen( format_path, "r" );
  if ( file == NULL )
    fatal_error( EX_NOINPUT, "\"%s\": %s\n", format_path, STRERROR() );

  lexer_init();
  parser_init();
  sym_table_init();

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

#if 0
  dump_state_t dump = { .indent = 0 };
  extern slist_t statement_list;
  FOREACH_SLIST_NODE( statement_node, &statement_list ) {
    if ( !ad_statement_exec( statement_node->data, &dump ) )
      break;
  } // for
#endif
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
