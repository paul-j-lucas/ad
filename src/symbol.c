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
#include "symbol.h"
#include "red_black.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// local variables
static unsigned   curr_scope;
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
    if ( synfo->scope != curr_scope )
      break;
    free( slist_pop_front( synfo_list ) );
  } // for

  return false;
}

/**
 * Cleans up symbol table data.
 *
 * @sa sym_init()
 */
static void sym_cleanup( void ) {
  rb_tree_cleanup( &sym_table, POINTER_CAST( rb_free_fn_t, &sym_free ) );
}

/**
 * Frees all memory associated with \a sym.
 *
 * @param sym The \ref symbol to free.  If NULL, does nothing.
 */
static void sym_free( symbol_t *sym ) {
  if ( sym != NULL ) {
    slist_cleanup( &sym->synfo_list, &free );
    FREE( sym->name );
  }
}

/**
 * Comparison function for two \ref symbol.
 *
 * @param i_tdef A pointer to the first \ref symbol.
 * @param j_tdef A pointer to the second \ref symbol.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the symbol name pointed to by \a i_sym is less than, equal to, or
 * greater than the `typedef` name pointed to by \a j_sym.
 */
NODISCARD
static int sym_cmp( symbol_t const *i_sym, symbol_t const *j_sym ) {
  return strcmp( i_sym->name, j_sym->name );
}

////////// extern functions ///////////////////////////////////////////////////

void sym_close_scope( void ) {
  assert( curr_scope > 0 );
  rb_tree_visit( &sym_table, &rb_close_scope_visitor, /*v_data=*/NULL );
}

symbol_t* sym_find_name( char const *name ) {
  rb_node_t *const rb = rb_tree_find( &sym_table, name );
  return rb != NULL ? rb->data : NULL;
}

void sym_init( void ) {
  ASSERT_RUN_ONCE();
  rb_tree_init( &sym_table, POINTER_CAST( rb_cmp_fn_t, &sym_cmp ) );
  ATEXIT( &sym_cleanup );
}

void sym_open_scope( void ) {
  ++curr_scope;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
