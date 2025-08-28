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

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct sym_rb_visit_data {
  sym_visit_fn_t  visit_fn;             ///< Caller's visitor function.
  void           *visit_data;           ///< Caller's optional data.
};
typedef struct sym_rb_visit_data sym_rb_visit_data_t;

// extern variables
unsigned  sym_scope;

// local variables
static rb_tree_t  sym_table;

// local functions
static void synfo_free( synfo_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Red-black tree visitor function that forwards to the \ref ad_type_visit_fn_t
 * function.
 *
 * @param node_data A pointer to the node's data.
 * @param visit_data Data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
NODISCARD
static bool rb_visit_visitor( void *node_data, void *visit_data ) {
  assert( node_data != NULL );
  assert( visit_data != NULL );

  symbol_t const *const sym = node_data;
  sym_rb_visit_data_t const *const srvd = visit_data;

  return (*srvd->visit_fn)( sym, srvd->visit_data );
}

/**
 * Red-black tree visitor function for closing a scope for a symbol table.
 *
 * @param node_data A pointer to the node's data.
 * @param visit_data Not used.
 * @return Always returns `false`.
 */
NODISCARD
static bool rb_close_scope_visitor( void *node_data, void *visit_data ) {
  assert( node_data != NULL );
  (void)visit_data;

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
 * Cleans-up all memory associated with \a sym but does _not_ free \a sym
 * itself.
 *
 * @param sym The \ref symbol to clean up.  If NULL, does nothing.
 *
 * @sa sym_free()
 */
static void sym_cleanup( symbol_t *sym ) {
  if ( sym != NULL ) {
    slist_cleanup(
      &sym->synfo_list, POINTER_CAST( slist_free_fn_t, &synfo_free )
    );
    sname_cleanup( &sym->sname );
  }
}

/**
 * Frees all memory associated with \a sym.
 *
 * @param sym The \ref symbol to free.  If NULL, does nothing.
 *
 * @sa sym_cleanup()
 */
static void sym_free( symbol_t *sym ) {
  sym_cleanup( sym );
  free( sym );
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

/**
 * Frees all memory associated with \a synfo.
 *
 * @param synfo The \ref synfo to free.  If NULL, does nothing.
 */
static void synfo_free( synfo_t *synfo ) {
  if ( synfo != NULL ) {
    switch ( synfo->kind ) {
      case SYM_DECL:
        break;
      case SYM_TYPE:
        ad_type_free( synfo->type );
        break;
    } // switch
  }
}

////////// extern functions ///////////////////////////////////////////////////

synfo_t* sym_add( void *obj, sname_t const *sname, sym_kind_t kind,
                  unsigned scope ) {
  assert( obj != NULL );
  assert( sname != NULL );

  sname_t  dup_sname = sname_dup( sname );
  symbol_t tmp_sym;

  sym_init( &tmp_sym, &dup_sname );

  rb_insert_rv_t const rv_rbi =
    rb_tree_insert( &sym_table, &tmp_sym, sizeof tmp_sym );
  symbol_t *const sym = RB_DINT( rv_rbi.node );

  synfo_t *rv_synfo;
  if ( !rv_rbi.inserted ) {
    rv_synfo = slist_front( &sym->synfo_list );
    assert( rv_synfo != NULL );
    if ( rv_synfo->scope >= scope ) {
      sym_cleanup( &tmp_sym );
      return rv_synfo;
    }
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
  rb_tree_visit( &sym_table, &rb_close_scope_visitor, /*visit_data=*/NULL );
}

synfo_t* sym_find_name( char const *name ) {
  sname_t const sname = SNAME_LIT( name );
  return sym_find_sname( &sname );
}

synfo_t* sym_find_sname( sname_t const *sname ) {
  assert( sname != NULL );
  symbol_t const find_sym = { .sname = *sname };
  rb_node_t *const found_rb = rb_tree_find( &sym_table, &find_sym );
  if ( found_rb == NULL )
    return NULL;
  symbol_t *const sym = RB_DINT( found_rb );
  assert( sym != NULL );
  synfo_t *const synfo = slist_front( &sym->synfo_list );
  assert( synfo != NULL );
  return synfo;
}

void sym_table_init( void ) {
  ASSERT_RUN_ONCE();
  rb_tree_init( &sym_table, RB_DINT, POINTER_CAST( rb_cmp_fn_t, &sym_cmp ) );
  ATEXIT( &sym_table_cleanup );
}

void sym_visit( sym_visit_fn_t visit_fn, void *visit_data ) {
  assert( visit_fn != NULL );
  sym_rb_visit_data_t srvd = { visit_fn, visit_data };
  rb_tree_visit( &sym_table, &rb_visit_visitor, &srvd );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
