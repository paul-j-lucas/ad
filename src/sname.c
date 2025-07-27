/*
**      ad -- ASCII dump
**      src/sname.c
**
**      Copyright (C) 2019-2024  Paul J. Lucas
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
 * Defines functions for dealing with "sname" (scoped name) objects, e.g.,
 * `S::T::x`.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define SNAME_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "sname.h"
#include "keyword.h"
#include "literals.h"
#include "options.h"
#include "strbuf.h"
#include "typedef.h"
#include "types.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <stdlib.h>                     /* for free(3) */

/**
 * @addtogroup sname-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for sname_full_name() and sname_scope_name() that writes the
 * scope names from outermost to innermost separated by `::` into a buffer.
 *
 * @param sbuf The buffer to write into.
 * @param sname The scoped name to write.
 * @param end_scope The scope to stop before or NULL for all scopes.
 * @return If not NULL, returns \a sbuf&ndash;>str; otherwise returns the empty
 * string.
 */
NODISCARD
static char const* sname_name_impl( strbuf_t *sbuf, sname_t const *sname,
                                    slist_node_t const *end_scope ) {
  assert( sbuf != NULL );
  assert( sname != NULL );

  strbuf_reset( sbuf );
  bool colon2 = false;

  FOREACH_SNAME_SCOPE_UNTIL( scope, sname, end_scope ) {
    if ( true_or_set( &colon2 ) )
      strbuf_putsn( sbuf, "::", 2 );
    sname_scope_t const *const data = sname_scope_data( scope );
    strbuf_puts( sbuf, data->name );
  } // for

  return empty_if_null( sbuf->str );
}

////////// extern functions ///////////////////////////////////////////////////

int sname_scope_cmp( sname_scope_t const *i_data,
                     sname_scope_t const *j_data ) {
  assert( i_data != NULL );
  assert( j_data != NULL );
  return strcmp( i_data->name, j_data->name );
}

sname_scope_t* sname_scope_dup( sname_scope_t const *src ) {
  if ( src == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  sname_scope_t *const dst = MALLOC( sname_scope_t, 1 );
  dst->name = check_strdup( src->name );
  return dst;
}

void sname_scope_free( sname_scope_t *data ) {
  if ( data != NULL ) {
    FREE( data->name );
    free( data );
  }
}

void sname_append_name( sname_t *sname, char const *name ) {
  assert( sname != NULL );
  assert( name != NULL );
  sname_scope_t *const data = MALLOC( sname_scope_t, 1 );
  data->name = name;
  slist_push_back( sname, data );
}

void sname_cleanup( sname_t *sname ) {
  slist_cleanup( sname, POINTER_CAST( slist_free_fn_t, &sname_scope_free ) );
}

int sname_cmp_name( sname_t const *sname, char const *name ) {
  assert( sname != NULL );
  assert( name != NULL );

  SNAME_VAR_INIT_NAME( name_sname, name );
  return sname_cmp( sname, &name_sname );
}

void sname_free( sname_t *sname ) {
  sname_cleanup( sname );
  free( sname );
}

char const* sname_full_name( sname_t const *sname ) {
  static strbuf_t sbuf;
  return sname != NULL ?
    sname_name_impl( &sbuf, sname, /*end_scope=*/NULL ) : "";
}

void sname_list_cleanup( slist_t *list ) {
  slist_cleanup( list, POINTER_CAST( slist_free_fn_t, &sname_free ) );
}

char const* sname_local_name( sname_t const *sname ) {
  if ( sname != NULL ) {
    sname_scope_t const *const local_data = slist_back( sname );
    if ( local_data != NULL )
      return local_data->name;
  }
  return "";                            // LCOV_EXCL_LINE
}

size_t sname_parse( char const *s, sname_t *rv_sname ) {
  assert( s != NULL );
  assert( rv_sname != NULL );

  sname_t temp_sname;
  sname_init( &temp_sname );

  char const *const s_orig = s;
  char const *end;
  char const *prev_end = s_orig;
  char const *prev_name = "";

  while ( (end = parse_identifier( s )) != NULL ) {
    char *const name = check_strndup( s, STATIC_CAST( size_t, end - s ) );

    // Ensure that the name is NOT a keyword.
    ad_keyword_t const *const k = ad_keyword_find( name );
    if ( k != NULL ) {
      FREE( name );
      // k->literal is set to L_* so == is OK
      if ( sname_empty( &temp_sname ) )
        return 0;
      goto done;
    }
    sname_append_name( &temp_sname, name );

    prev_end = end;
    SKIP_WS( end );
    if ( *end == '\0' ) {
      if ( strcmp( name, prev_name ) != 0 )
        goto error;
      goto done;
    }
    if ( strncmp( end, "::", 2 ) != 0 )
      break;
    s = end + 2;
    SKIP_WS( s );
    prev_name = name;
  } // while

error:
  sname_cleanup( &temp_sname );
  return 0;

done:
  *rv_sname = temp_sname;
  return STATIC_CAST( size_t, prev_end - s_orig );
}

void sname_pop_back( sname_t *sname ) {
  assert( sname != NULL );
  switch ( sname_count( sname ) ) {
    case 0:
      break;
    case 1:
      sname_cleanup( sname );
      break;
    default:
      sname_scope_free( slist_pop_back( sname ) );
  } // switch
}

char const* sname_scope_name( sname_t const *sname ) {
  static strbuf_t sbuf;
  return sname != NULL ? sname_name_impl( &sbuf, sname, sname->tail ) : "";
}

void sname_set( sname_t *dst_sname, sname_t *src_sname ) {
  if ( dst_sname != src_sname ) {
    sname_cleanup( dst_sname );
    sname_append_sname( dst_sname, src_sname );
  }
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
