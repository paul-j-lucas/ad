/*
**      ad -- ASCII dump
**      src/symbol.c
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
 * Defines functions for manipulating a symbol table.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define SYMBOL_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "red_black.h"
#include "lexer.h"
#include "symbol.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// extern variables
unsigned  sym_scope;

// local variables
static rb_tree_t  sym_table;

// local functions
static void sym_free( symbol_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Red-black tree visitor function for closing a scope for a symbol table.
 *
 * @param node_data A pointer to the node's data.
 * @param v_data Not used.
 * @return Always returns `false`.
 */
NODISCARD
static bool rb_close_scope_visitor( void *node_data, void *v_data ) {
  assert( node_data != NULL );
  (void)v_data;

  symbol_t *const sym = node_data;

  for ( slist_t *const synfo_list = &sym->synfo_list;
        !slist_empty( synfo_list ); ) {
    synfo_t *const synfo = slist_front( synfo_list );
    if ( synfo->scope != sym_scope )
      break;
    free( slist_pop_front( synfo_list ) );
  } // for

  return false;
}

/**
 * Frees all memory associated with \a sym.
 *
 * @param sym The \ref symbol to free.  If NULL, does nothing.
 */
static void sym_free( symbol_t *sym ) {
  if ( sym != NULL ) {
    slist_cleanup( &sym->synfo_list, &free );
    sname_cleanup( &sym->sname );
    free( sym );
  }
}

/**
 * Comparison function for two \ref symbol.
 *
 * @param i_sym A pointer to the first \ref symbol.
 * @param j_sym A pointer to the second \ref symbol.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the symbol name pointed to by \a i_sym is less than, equal to, or
 * greater than the `typedef` name pointed to by \a j_sym.
 */
NODISCARD
static int sym_cmp( symbol_t const *i_sym, symbol_t const *j_sym ) {
  return sname_cmp( &i_sym->sname, &j_sym->sname );
}

/**
 * Initializes \a sym.
 *
 * @param sym The \ref symbol to initialize.
 * @param sname The scoped name for \a sym.
 */
static void sym_init( symbol_t *sym, sname_t *sname ) {
  assert( sym != NULL );
  assert( sname != NULL );
  *sym = (symbol_t){ .sname = sname_move( sname ) };
}

/**
 * Cleans up symbol table data.
 *
 * @sa sym_table_init()
 */
static void sym_table_cleanup( void ) {
  rb_tree_cleanup( &sym_table, POINTER_CAST( rb_free_fn_t, &sym_free ) );
}

////////// extern functions ///////////////////////////////////////////////////

synfo_t* sym_add( void *obj, sname_t *sname, sym_kind_t kind, unsigned scope ) {
  assert( obj != NULL );
  assert( sname != NULL );

  synfo_t *rv_synfo;
  symbol_t tmp_sym;
  sym_init( &tmp_sym, sname );

  rb_insert_rv_t const rbi_rv =
    rb_tree_insert( &sym_table, &tmp_sym, sizeof tmp_sym );
  symbol_t *const sym = RB_DINT( rbi_rv.node );

  if ( !rbi_rv.inserted ) {
    rv_synfo = slist_front( &sym->synfo_list );
    assert( rv_synfo != NULL );
    if ( rv_synfo->scope == scope )
      return rv_synfo;
  }

  rv_synfo = MALLOC( synfo_t, 1 );
  *rv_synfo = (synfo_t){
    .first_loc = lexer_loc(),
    .kind = kind,
    .scope = scope,
    .obj = obj
  };
  slist_push_front( &sym->synfo_list, rv_synfo );

  return rv_synfo;
}

void sym_close_scope( void ) {
  assert( sym_scope > 0 );
  rb_tree_visit( &sym_table, &rb_close_scope_visitor, /*v_data=*/NULL );
}

symbol_t* sym_find_name( char const *name ) {
  SNAME_VAR_INIT_NAME( sname, name );
  return sym_find_sname( &sname );
}

symbol_t* sym_find_sname( sname_t const *sname ) {
  assert( sname != NULL );
  symbol_t const sym = { .sname = *sname };
  rb_node_t *const found_rb = rb_tree_find( &sym_table, &sym );
  return found_rb != NULL ? RB_DPTR( found_rb ) : NULL;
}

void sym_table_init( void ) {
  ASSERT_RUN_ONCE();
  rb_tree_init( &sym_table, RB_DPTR, POINTER_CAST( rb_cmp_fn_t, &sym_cmp ) );
  ATEXIT( &sym_table_cleanup );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
