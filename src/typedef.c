/*
**      ad -- ASCII dump
**      src/typedef.c
**
**      Copyright (C) 2022  Paul J. Lucas
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
 * Defines types, data, and functions for standard types and adding and looking
 * up C/C++ `typedef` or `using` declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "common.h"
#include "options.h"
#include "red_black.h"
#include "type.h"
#include "typedef.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

/**
 * Convenience macro for specifying a \ref ad_typedef literal with an AST
 * having \a NAME.
 *
 * @param NAME The name.
 */
#define C_TYPEDEF_NAME_LIT(SNAME) \
  C_TYPEDEF_AST_LIT( &(c_ast_t const){ .name = (SNAME) } )

///////////////////////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct tdef_rb_visit_data {
  ad_typedef_visit_fn_t   visit_fn;     ///< Caller's visitor function.
  void                   *v_data;       ///< Caller's optional data.
};
typedef struct tdef_rb_visit_data tdef_rb_visit_data_t;

// local variables
static rb_tree_t    typedef_set;        ///< Global set of `typedef`s.

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans up \ref ad_typedef data.
 *
 * @sa ad_typedef_init()
 */
static void ad_typedef_cleanup( void ) {
  // There is no ad_typedef_free() function because ad_typedef_add() adds only
  // ad_typedef_t nodes pointing to pre-existing AST nodes.  The AST nodes are
  // freed independently in parser_cleanup().  Hence, this function frees only
  // the red-black tree, its nodes, and the ad_typedef_t data each node points
  // to, but not the AST nodes the ad_typedef_t data points to.
  rb_tree_cleanup( &typedef_set, &free );
}

/**
 * Comparison function for \ref ad_typedef data used by the red-black tree.
 *
 * @param i_data A pointer to data.
 * @param j_data A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the `typedef` name pointed to by \a i_data is less than, equal
 * to, or greater than the `typedef` name pointed to by \a j_data.
 */
NODISCARD
static int ad_typedef_cmp( void const *i_data, void const *j_data ) {
  assert( i_data != NULL );
  assert( j_data != NULL );
  ad_typedef_t const *const i_tdef = i_data;
  ad_typedef_t const *const j_tdef = j_data;
  return strcmp( &i_tdef->type->name, &j_tdef->type->name );
}

/**
 * Creates a new \ref ad_typedef.
 *
 * @param ast The AST of the type.
 * @return Returns said \ref ad_typedef.
 */
NODISCARD
static ad_typedef_t* ad_typedef_new( c_ast_t const *ast ) {
  assert( ast != NULL );

  ad_typedef_t *const tdef = MALLOC( ad_typedef_t, 1 );
  tdef->ast = ast;

  return tdef;
}

/**
 * Red-black tree visitor function that forwards to the \ref
 * ad_typedef_visit_fn_t function.
 *
 * @param node_data A pointer to the node's data.
 * @param v_data Data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
NODISCARD
static bool rb_visitor( void *node_data, void *v_data ) {
  assert( node_data != NULL );
  assert( v_data != NULL );

  ad_typedef_t const *const tdef = node_data;
  tdef_rb_visit_data_t const *const trvd = v_data;

  return (*trvd->visit_fn)( tdef, trvd->v_data );
}

////////// extern functions ///////////////////////////////////////////////////

ad_typedef_t const* ad_typedef_add( ad_type_t const *type ) {
  assert( ast != NULL );
  assert( ast->name != NULL );
  assert( strlen( ast->name ) > 0 );

  ad_typedef_t *const new_tdef = ad_typedef_new( type );
  rb_insert_rv_t const rv = rb_tree_insert( &typedef_set, new_tdef );
  if ( !rv.inserted ) {
    //
    // A typedef with the same name exists, so we don't need the new one.
    //
    FREE( new_tdef );
  }
  return rv.node->data;
}

ad_typedef_t const* ad_typedef_find_name( char const *name ) {
  assert( name != NULL );
  rb_node_t const *const found_rb =
    rb_tree_find( &typedef_set, &C_TYPEDEF_NAME_LIT( *name ) );
  return found_rb != NULL ? found_rb->data : NULL;
}

void ad_typedef_init( void ) {
  rb_tree_init( &typedef_set, &ad_typedef_cmp );
  perror_exit_if( atexit( &ad_typedef_cleanup ) != 0, EX_OSERR );
}

ad_typedef_t const* ad_typedef_visit( ad_typedef_visit_fn_t visit_fn,
                                      void *v_data ) {
  assert( visit_fn != NULL );
  tdef_rb_visit_data_t trvd = { visit_fn, v_data };
  rb_node_t const *const rb = rb_tree_visit( &typedef_set, &rb_visitor, &trvd );
  return rb != NULL ? rb->data : NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
