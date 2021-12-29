/*
**      ad -- ASCII dump
**      src/slist.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas
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
 * Defines functions to manipulate a singly-linked-list data structure.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define AD_SLIST_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "slist.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdint.h>

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

void slist_cleanup( slist_t *list, slist_free_fn_t free_fn ) {
  if ( list != NULL ) {
    slist_node_t *curr = list->head, *next;

    if ( free_fn == NULL ) {            // avoid repeated check in loop
      for ( ; curr != NULL; curr = next ) {
        next = curr->next;
        free( curr );
      } // for
    }
    else {
      for ( ; curr != NULL; curr = next ) {
        (*free_fn)( curr->data );
        next = curr->next;
        free( curr );
      } // for
    }

    slist_init( list );
  }
}

int slist_cmp( slist_t const *i_list, slist_t const *j_list,
               slist_cmp_fn_t cmp_fn ) {
  assert( i_list != NULL );
  assert( j_list != NULL );

  if ( i_list == j_list )
    return 0;

  slist_node_t const *i_node = i_list->head, *j_node = j_list->head;

  if ( cmp_fn == NULL ) {               // avoid repeated check in loop
    for ( ; i_node != NULL && j_node != NULL;
          i_node = i_node->next, j_node = j_node->next ) {
      int const cmp = STATIC_CAST( int,
        REINTERPRET_CAST( intptr_t, i_node->data ) -
        REINTERPRET_CAST( intptr_t, j_node->data )
      );
      if ( cmp != 0 )
        return cmp;
    } // for
  }
  else {
    for ( ; i_node != NULL && j_node != NULL;
          i_node = i_node->next, j_node = j_node->next ) {
      int const cmp = (*cmp_fn)( i_node->data, j_node->data );
      if ( cmp != 0 )
        return cmp;
    } // for
  }

  return i_node == NULL ? (j_node == NULL ? 0 : -1) : 1;
}

slist_t slist_dup( slist_t const *src_list, ssize_t n,
                   slist_dup_fn_t dup_fn ) {
  slist_t dst_list;
  slist_init( &dst_list );

  if ( src_list != NULL && n != 0 ) {
    size_t un = STATIC_CAST( size_t, n );
    if ( dup_fn == NULL ) {             // avoid repeated check in loop
      FOREACH_SLIST_NODE( src_node, src_list ) {
        if ( un-- == 0 )
          break;
        slist_push_tail( &dst_list, src_node->data );
      } // for
    }
    else {
      FOREACH_SLIST_NODE( src_node, src_list ) {
        if ( un-- == 0 )
          break;
        void *const dst_data = (*dup_fn)( src_node->data );
        slist_push_tail( &dst_list, dst_data );
      } // for
    }
  }

  return dst_list;
}

void* slist_peek_at( slist_t const *list, size_t offset ) {
  assert( list != NULL );
  if ( offset >= list->len )
    return NULL;

  slist_node_t *p;

  if ( offset == list->len - 1 ) {
    p = list->tail;
  } else {
    for ( p = list->head; offset-- > 0; p = p->next )
      ;
  }

  assert( p != NULL );
  return p->data;
}

void* slist_pop_head( slist_t *list ) {
  assert( list != NULL );
  if ( list->head != NULL ) {
    void *const data = list->head->data;
    slist_node_t *const next_node = list->head->next;
    FREE( list->head );
    list->head = next_node;
    if ( list->head == NULL )
      list->tail = NULL;
    --list->len;
    return data;
  }
  return NULL;
}

void slist_push_head( slist_t *list, void *data ) {
  assert( list != NULL );
  slist_node_t *const new_head_node = MALLOC( slist_node_t, 1 );
  new_head_node->data = data;
  new_head_node->next = list->head;
  list->head = new_head_node;
  if ( list->tail == NULL )
    list->tail = new_head_node;
  ++list->len;
}

void slist_push_list_head( slist_t *dst_list, slist_t *src_list ) {
  assert( dst_list != NULL );
  assert( src_list != NULL );
  if ( dst_list->head == NULL ) {
    assert( dst_list->tail == NULL );
    dst_list->head = src_list->head;
    dst_list->tail = src_list->tail;
  }
  else if ( src_list->head != NULL ) {
    assert( src_list->tail != NULL );
    src_list->tail->next = dst_list->head;
    dst_list->head = src_list->head;
  }
  dst_list->len += src_list->len;
  slist_init( src_list );
}

void slist_push_list_tail( slist_t *dst_list, slist_t *src_list ) {
  assert( dst_list != NULL );
  assert( src_list != NULL );
  if ( dst_list->head == NULL ) {
    assert( dst_list->tail == NULL );
    dst_list->head = src_list->head;
    dst_list->tail = src_list->tail;
  }
  else if ( src_list->head != NULL ) {
    assert( src_list->tail != NULL );
    assert( dst_list->tail->next == NULL );
    dst_list->tail->next = src_list->head;
    dst_list->tail = src_list->tail;
  }
  dst_list->len += src_list->len;
  slist_init( src_list );
}

void slist_push_tail( slist_t *list, void *data ) {
  assert( list != NULL );
  slist_node_t *const new_tail_node = MALLOC( slist_node_t, 1 );
  new_tail_node->data = data;
  new_tail_node->next = NULL;

  if ( list->head == NULL ) {
    assert( list->tail == NULL );
    list->head = new_tail_node;
  } else {
    assert( list->tail != NULL );
    assert( list->tail->next == NULL );
    list->tail->next = new_tail_node;
  }
  list->tail = new_tail_node;
  ++list->len;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */