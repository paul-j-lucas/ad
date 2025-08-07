/*
**      PJL Library
**      src/slist_test.c
**
**      Copyright (C) 2021-2025  Paul J. Lucas
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

// local
#include "pjl_config.h"                 /* must go first */
#include "slist.h"
#include "util.h"
#include "unit_test.h"

// standard
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

struct counter {
  unsigned i;
  unsigned n;
};
typedef struct counter counter_t;

// local variables
static char const  *str_equal_to;

////////// local functions ////////////////////////////////////////////////////

static bool slist_node_str_equal( slist_node_t *node, void *data ) {
  (void)data;
  char const *const s = node->data;
  return strcmp( s, str_equal_to ) == 0;
}

static bool slist_bool( slist_node_t *node, void *data ) {
  (void)node;
  return (uintptr_t)data != 0;
}

static bool slist_counter( slist_node_t *node, void *data ) {
  (void)node;
  counter_t *const c = data;
  return c->i++ == c->n;
};

////////// test functions /////////////////////////////////////////////////////

static bool test_slist_cmp( void ) {
  TEST_FUNC_BEGIN();
  slist_t list, list2;
  slist_init( &list );
  slist_init( &list2 );

  // test 2 empty lists
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) == 0 );

  // test empty and non-empty lists
  slist_push_back( &list2, (void*)"A" );
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) != 0 );
  slist_cleanup( &list2, /*free_fn=*/NULL );

  // test non-empty and empty lists
  slist_push_back( &list, (void*)"A" );
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) != 0 );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // test matching 1,2,3-element lists
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list2, (void*)"A" );
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) == 0 );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list2, (void*)"B" );
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) == 0 );
  slist_push_back( &list, (void*)"C" );
  slist_push_back( &list2, (void*)"C" );
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) == 0 );
  slist_cleanup( &list, /*free_fn=*/NULL );
  slist_cleanup( &list2, /*free_fn=*/NULL );

  // test 1-element non-matching lists
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) < 0 );
  slist_cleanup( &list, /*free_fn=*/NULL );
  slist_cleanup( &list2, /*free_fn=*/NULL );

  // test 1-element and 2-element lists
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list2, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  TEST( slist_cmp( &list, &list2, POINTER_CAST( slist_cmp_fn_t, &strcmp ) ) < 0 );

  slist_cleanup( &list, /*free_fn=*/NULL );
  slist_cleanup( &list2, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_dup( void ) {
  TEST_FUNC_BEGIN();
  slist_t list, list2;
  slist_init( &list );
  slist_init( &list2 );
  char const X[] = "X";
  char *p;

  // empty list
  list2 = slist_dup( &list, -1, /*dup_fn=*/NULL );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );

  // 1-element list
  slist_push_back( &list, (void*)&X );
  list2 = slist_dup( &list, -1, /*dup_fn=*/NULL );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 1 );
  if ( TEST( (p = slist_front( &list2 )) != NULL ) ) {
    TEST( *p == 'X' );
    TEST( p == X );
  }
  slist_cleanup( &list, /*free_fn=*/NULL );
  slist_cleanup( &list2, /*free_fn=*/NULL );

  // 2-element list
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  list2 = slist_dup( &list, -1, /*dup_fn=*/NULL );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 2 );
  if ( TEST( (p = slist_front( &list2 )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list2 )) != NULL ) )
    TEST( *p == 'B' );
  slist_cleanup( &list, /*free_fn=*/NULL );
  slist_cleanup( &list2, /*free_fn=*/NULL );

  // 3-element list
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  list2 = slist_dup( &list, -1, /*dup_fn=*/NULL );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 3 );
  if ( TEST( (p = slist_front( &list2 )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list2, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list2 )) != NULL ) )
    TEST( *p == 'C' );
  slist_cleanup( &list, /*free_fn=*/NULL );
  slist_cleanup( &list2, /*free_fn=*/NULL );

  // check data_dup_fn
  slist_push_back( &list, (void*)&X );
  list2 = slist_dup( &list, -1, POINTER_CAST( slist_dup_fn_t, &strdup ) );
  TEST( !slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 1 );
  if ( TEST( (p = slist_front( &list2 )) != NULL ) ) {
    TEST( *p == 'X' );
    if ( TEST( p != X ) )
      free( p );
  }

  slist_cleanup( &list, /*free_fn=*/NULL );
  slist_cleanup( &list2, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_at( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  if ( TEST( (p = slist_at( &list, 0 )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  TEST( slist_at( &list, 3 ) == NULL );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_atr( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  if ( TEST( (p = slist_atr( &list, 0 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_atr( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_atr( &list, 2 )) != NULL ) )
    TEST( *p == 'A' );
  TEST( slist_at( &list, 4 ) == NULL );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_free_if( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  // test match list[0] with list->len == 1
  slist_push_back( &list, (void*)"A" );
  str_equal_to = "A";
  slist_free_if( &list, &slist_node_str_equal, /*data=*/NULL );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );

  // test match list[0] and list[1] with list->len == 2
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"A" );
  slist_free_if( &list, &slist_node_str_equal, /*data=*/NULL );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );

  // test match list[0] with list->len == 2
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal, /*data=*/NULL );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // test match list[0] and list[1] with list->len == 3
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal, /*data=*/NULL );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // test match list[1] with list->len == 3
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  str_equal_to = "B";
  slist_free_if( &list, &slist_node_str_equal, /*data=*/NULL );
  TEST( slist_len( &list ) == 2 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // test match list[1] with list->len == 2
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal, /*data=*/NULL );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'A' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // test match list[1] and list[2] with list->len == 3
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"B" );
  slist_free_if( &list, &slist_node_str_equal, /*data=*/NULL );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'A' );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_insert_if( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  // test not inserting into an empty list
  slist_insert_if( &list, (void*)"X", &slist_bool, (void*)false );
  TEST( slist_len( &list ) == 0 );
  TEST( slist_front( &list ) == NULL );
  TEST( slist_back( &list ) == NULL );

  // test inserting into an empty list
  slist_insert_if( &list, (void*)"C", &slist_bool, (void*)true );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'C' );
  TEST( slist_front( &list ) == slist_back( &list ) );

  // test inserting at position 0
  slist_insert_if( &list, (void*)"A", &slist_bool, (void*)true );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_front( &list ) != slist_back( &list ) );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );

  // test inserting at position 1
  counter_t c = { .n = 1 };
  slist_insert_if( &list, (void*)"B", &slist_counter, &c );
  TEST( slist_len( &list ) == 3 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'C' );

  // test inserting at the end
  c = (counter_t){ .n = 3 };
  slist_insert_if( &list, (void*)"D", &slist_counter, &c );
  TEST( slist_len( &list ) == 4 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'D' );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_pop_front( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  TEST( slist_pop_front( &list ) == NULL );

  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  if ( TEST( (p = slist_pop_front( &list )) != NULL ) ) {
    TEST( *p == 'A' );
    TEST( slist_len( &list ) == 1 );
    TEST( !slist_empty( &list ) );
  }
  if ( TEST( (p = slist_pop_front( &list )) != NULL ) ) {
    TEST( *p == 'B' );
    TEST( slist_len( &list ) == 0 );
    TEST( slist_empty( &list ) );
  }
  TEST( slist_pop_front( &list ) == NULL );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_push_front( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_front( &list, (void*)"B" );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'B' );

  slist_push_front( &list, (void*)"A" );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_push_list_front( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_t list2;
  slist_init( &list2 );

  // empty lists
  slist_push_list_front( &list, &list2 );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );

  // empty first, 1-element second
  slist_push_back( &list2, (void*)"A" );
  slist_push_list_front( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // empty first, 2-element second
  slist_push_back( &list2, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  slist_push_list_front( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // empty first, 3-element second
  slist_push_back( &list2, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  slist_push_back( &list2, (void*)"C" );
  slist_push_list_front( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 1-element first, 1-element second
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list2, (void*)"A" );
  slist_push_list_front( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 2-element first, 1-element second
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  slist_push_back( &list2, (void*)"A" );
  slist_push_list_front( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 3-element first, 1-element second
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  slist_push_back( &list, (void*)"D" );
  slist_push_back( &list2, (void*)"A" );
  slist_push_list_front( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 4 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'D' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 3-element first, 2-element second
  slist_push_back( &list, (void*)"C" );
  slist_push_back( &list, (void*)"D" );
  slist_push_back( &list, (void*)"E" );
  slist_push_back( &list2, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  slist_push_list_front( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 5 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_at( &list, 3 )) != NULL ) )
    TEST( *p == 'D' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'E' );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_push_list_back( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_t list2;
  slist_init( &list2 );

  // empty lists
  slist_push_list_back( &list, &list2 );
  TEST( slist_empty( &list ) );
  TEST( slist_len( &list ) == 0 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );

  // empty first, 1-element second
  slist_push_back( &list2, (void*)"A" );
  slist_push_list_back( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // empty first, 2-element second
  slist_push_back( &list2, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  slist_push_list_back( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // empty first, 3-element second
  slist_push_back( &list2, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  slist_push_back( &list2, (void*)"C" );
  slist_push_list_back( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 1-element first, 1-element second
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list2, (void*)"B" );
  slist_push_list_back( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 2 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 2-element first, 1-element second
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list2, (void*)"C" );
  slist_push_list_back( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 3 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'C' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 3-element first, 1-element second
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  slist_push_back( &list2, (void*)"D" );
  slist_push_list_back( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 4 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'D' );
  slist_cleanup( &list, /*free_fn=*/NULL );

  // 3-element first, 2-element second
  slist_push_back( &list, (void*)"A" );
  slist_push_back( &list, (void*)"B" );
  slist_push_back( &list, (void*)"C" );
  slist_push_back( &list2, (void*)"D" );
  slist_push_back( &list2, (void*)"E" );
  slist_push_list_back( &list, &list2 );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 5 );
  TEST( slist_empty( &list2 ) );
  TEST( slist_len( &list2 ) == 0 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_at( &list, 1 )) != NULL ) )
    TEST( *p == 'B' );
  if ( TEST( (p = slist_at( &list, 2 )) != NULL ) )
    TEST( *p == 'C' );
  if ( TEST( (p = slist_at( &list, 3 )) != NULL ) )
    TEST( *p == 'D' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'E' );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

static bool test_slist_push_back( void ) {
  TEST_FUNC_BEGIN();
  slist_t list;
  slist_init( &list );
  char *p;

  slist_push_back( &list, (void*)"A" );
  TEST( !slist_empty( &list ) );
  TEST( slist_len( &list ) == 1 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'A' );

  slist_push_back( &list, (void*)"B" );
  TEST( slist_len( &list ) == 2 );
  if ( TEST( (p = slist_front( &list )) != NULL ) )
    TEST( *p == 'A' );
  if ( TEST( (p = slist_back( &list )) != NULL ) )
    TEST( *p == 'B' );

  slist_cleanup( &list, /*free_fn=*/NULL );
  TEST_FUNC_END();
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *const argv[] ) {
  test_prog_init( argc, argv );

  if ( test_slist_push_front() && test_slist_push_back() ) {
    test_slist_at();
    test_slist_atr();
    test_slist_cmp();
    test_slist_dup();
    test_slist_free_if();
    test_slist_insert_if();
    test_slist_push_list_front();
    test_slist_push_list_back();
    test_slist_pop_front();
  }
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
