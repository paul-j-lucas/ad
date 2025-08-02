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
#include "typedef.h"
#include "ad.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct tdef_rb_visit_data {
  ad_type_visit_fn_t  visit_fn;         ///< Caller's visitor function.
  void               *v_data;           ///< Caller's optional data.
};
typedef struct tdef_rb_visit_data tdef_rb_visit_data_t;

// local variables
static rb_tree_t    typedef_set;        ///< Global set of `typedef`s.

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans up \ref ad_type data.
 *
 * @sa ad_typedefs_init()
 */
static void ad_typedefs_cleanup( void ) {
  rb_tree_cleanup( &typedef_set, POINTER_CAST( rb_free_fn_t, &ad_type_free ) );
}

/**
 * Red-black tree visitor function that forwards to the \ref ad_type_visit_fn_t
 * function.
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

  ad_type_t const *const type = node_data;
  tdef_rb_visit_data_t const *const trvd = v_data;

  return (*trvd->visit_fn)( type, trvd->v_data );
}

////////// extern functions ///////////////////////////////////////////////////

ad_type_t* ad_typedef_add( ad_type_t *type ) {
  assert( type != NULL );
  return RB_DINT( rb_tree_insert( &typedef_set, type, sizeof *type ).node );
}

ad_type_t const* ad_typedef_find_name( char const *name ) {
  assert( name != NULL );
  sname_t sname;
  if ( sname_parse( name, &sname ) ) {
    ad_type_t const *const type = ad_typedef_find_sname( &sname );
    sname_cleanup( &sname );
    return type;
  }
  return NULL;
}

ad_type_t const* ad_typedef_find_sname( sname_t const *sname ) {
  assert( sname != NULL );
  ad_type_t const type = { .sname = *sname };
  rb_node_t const *const found_rb = rb_tree_find( &typedef_set, &type );
  return found_rb != NULL ? RB_DINT( found_rb ) : NULL;
}

void ad_typedef_visit( ad_type_visit_fn_t visit_fn, void *v_data ) {
  assert( visit_fn != NULL );
  tdef_rb_visit_data_t trvd = { visit_fn, v_data };
  rb_tree_visit( &typedef_set, &rb_visitor, &trvd );
}

void ad_typedefs_init( void ) {
  ASSERT_RUN_ONCE();
  rb_tree_init(
    &typedef_set, RB_DINT, POINTER_CAST( rb_cmp_fn_t, &ad_type_cmp )
  );
  ATEXIT( &ad_typedefs_cleanup );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
