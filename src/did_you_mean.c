/*
**      ad -- ASCII dump
**      src/did_you_mean.c
**
**      Copyright (C) 2020-2022  Paul J. Lucas
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
 * Defines types, constants, and functions for printing suggestions for "Did
 * you mean ...?"
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "did_you_mean.h"
#include "dam_lev.h"
#include "keyword.h"
#include "options.h"
#include "typedef.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/**
 * Used by copy_typedefs() and copy_typedef_visitor() to pass and return data.
 */
struct copy_typedef_visit_data {
  /// Pointer to a pointer to a candidate list or NULL to just get the count.
  did_you_mean_t  **pdym;

  size_t            count;              ///< The count.
};
typedef struct copy_typedef_visit_data copy_typedef_visit_data_t;

/**
 * The signature for a function passed to qsort().
 *
 * @param i_data A pointer to data.
 * @param j_data A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the data pointed to by \a i_data is less than, equal to, or
 * greater than the data pointed to by \a j_data.
 */
typedef int (*qsort_cmp_fn_t)( void const *i_data, void const *j_data );

/**
 * The edit distance must be less than or equal to this percent of a target
 * string's length in order to be considered "similar enough" to be a
 * reasonable suggestion.
 */
static double const SIMILAR_ENOUGH_PERCENT = .37;

////////// local functions ////////////////////////////////////////////////////

/**
 * Duplicates \a s prefixed by \a prefix.
 *
 * @param prefix The null-terminated prefix string to duplicate.
 * @param s The null-terminated string to duplicate.
 * @return Returns a copy of \a s prefixed by \a prefix.
 */
NODISCARD
static char* check_prefix_strdup( char const *prefix, char const *s ) {
  assert( prefix != NULL );
  assert( s != NULL );

  size_t const prefix_len = strlen( prefix );
  char *const dup_s = MALLOC( char, prefix_len + strlen( s ) + 1/*\0*/ );
  strcpy( dup_s, prefix );
  strcpy( dup_s + prefix_len, s );
  return dup_s;
}

/**
 * Copies C/C++ keywords in the current language to the candidate list pointed
 * to by \a pdym.  If \a pdym is NULL, only counts the number of keywords.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count keywords, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @param copy_types If `true` copy (or count) keywords for types; if `false`
 * copy (or count) keywords for non-types.
 * @return Returns said number of keywords.
 */
PJL_DISCARD
static size_t copy_keywords( did_you_mean_t **const pdym, bool copy_types ) {
  size_t count = 0;
  FOREACH_AD_KEYWORD( k ) {
    if ( !!k->tid == copy_types ) {
      if ( pdym != NULL )
        (*pdym)++->token = check_strdup( k->literal );
      ++count;
    }
  } // for
  return count;
}

/**
 * Copies **ad** command-line options to the candidate list pointed to by \a
 * pdym.  If \a pdym is NULL, only counts the number of options.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count options, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @return Returns said number of options.
 */
PJL_DISCARD
static size_t copy_cli_options( did_you_mean_t **pdym ) {
  size_t count = 0;
  FOREACH_CLI_OPTION( opt ) {
    if ( pdym != NULL )
      (*pdym)++->token = check_strdup( opt->name );
    ++count;
  } // for
  return count;
}

/**
 * A \ref ad_typedef visitor function to copy names of types that are only
 * valid in the current language to the candidate list pointed to
 *
 * @param tdef The ad_typedef to visit.
 * @param data A pointer to a \ref copy_typedef_visit_data.
 * @return Always returns `false`.
 */
PJL_DISCARD
static bool copy_typedef_visitor( ad_typedef_t const *tdef, void *data ) {
  assert( tdef != NULL );
  assert( data != NULL );

  copy_typedef_visit_data_t *const ctvd = data;
  if ( ctvd->pdym != NULL ) {
    char const *const name = &tdef->type->name;
    (*ctvd->pdym)++->token = check_strdup( name );
  }
  ++ctvd->count;

  return false;
}

/**
 * Counts the number of `typedef`s that are only valid in the current language.
 *
 * @param pdym A pointer to the current \ref did_you_mean pointer or NULL to
 * just count typedefs, not copy.  If not NULL, on return, the pointed-to
 * pointer is incremented.
 * @return Returns said number of `typedef`s.
 */
PJL_DISCARD
static size_t copy_typedefs( did_you_mean_t **const pdym ) {
  copy_typedef_visit_data_t ctvd = { pdym, 0 };
  ad_typedef_visit( &copy_typedef_visitor, &ctvd );
  return ctvd.count;
}

/**
 * Comparison function for two \ref did_you_mean objects.
 *
 * @param i_dym A pointer to the first \ref did_you_mean.
 * @param j_dym A pointer to the second \ref did_you_mean.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_dym is
 * less than, equal to, or greater than \a j_dym, respectively.
 */
NODISCARD
static int dym_cmp( did_you_mean_t const *i_dym, did_you_mean_t const *j_dym ) {
  int const cmp =
    STATIC_CAST( int, i_dym->dam_lev_dist ) -
    STATIC_CAST( int, j_dym->dam_lev_dist );
  return cmp != 0 ? cmp : strcmp( i_dym->token, j_dym->token );
}

/**
 * Frees memory used by \a dym.
 *
 * @param dym A pointer to the first \ref did_you_mean to free and continuing
 * until `token` is NULL.
 */
static void dym_free_tokens( did_you_mean_t const *dym ) {
  assert( dym != NULL );
  while ( dym->token != NULL )
    FREE( dym++->token );
}

/**
 * Gets whether \a dam_lev_dist is "similar enough" to be a candidate.
 *
 * Using a Damerau-Levenshtein edit distance alone to implement "Did you mean
 * ...?" can yield poor results if you just always use the results with the
 * least distance.  For example, given a source string of "fixed" and the best
 * target string of "float", it's probably safe to assume that because "fixed"
 * is so different from "float" that there's no way "float" was meant.  It
 * would be better to offer _no_ suggestions than not-even-close suggestions.
 *
 * Hence, you need a heuristic to know whether a least edit distance is
 * "similar enough" to the target string even to bother offering suggestions.
 * This can be done by checking whether the distance is less than or equal to
 * some percentage of the target string's length in order to be considered
 * "similar enough" to be a reasonable suggestion.
 *
 * @param dam_lev_dist A Damerau-Levenshtein edit distance.
 * @param percent The edit distance must be less than or equal to this percent
 * of \a target_len in order to be considered "similar enough" to be a
 * reasonable suggestion.
 * @param target_len The length of the target string.
 * @return Returns `true` only if \a dam_lev_dist is "similar enough."
 */
NODISCARD
static bool is_similar_enough( size_t dam_lev_dist, double percent,
                               size_t target_len ) {
  assert( percent > 0 && percent < 1 );
  return dam_lev_dist <=
    STATIC_CAST( size_t, STATIC_CAST( double, target_len ) * percent + 0.5 );
}

////////// extern functions ///////////////////////////////////////////////////

void dym_free( did_you_mean_t const *dym_array ) {
  if ( dym_array != NULL ) {
    dym_free_tokens( dym_array );
    FREE( dym_array );
  }
}

did_you_mean_t const* dym_new( dym_kind_t kinds, char const *unknown_token ) {
  if ( kinds == DYM_NONE )
    return NULL;
  assert( unknown_token != NULL );

  // Pre-flight to calculate array size; the order here doesn't matter.
  size_t const dym_size =
    ((kinds & DYM_CLI_OPTIONS) != DYM_NONE ?
      copy_cli_options( /*pdym=*/NULL ) : 0) +
    ((kinds & DYM_KEYWORDS) != DYM_NONE ?
      copy_keywords( /*pdym=*/NULL, /*copy_types=*/false ) : 0) +
    ((kinds & DYM_TYPES) != DYM_NONE ?
      copy_keywords( /*pdym=*/NULL, /*copy_types=*/true ) +
      copy_typedefs( /*pdym=*/NULL ) : 0);

  if ( dym_size == 0 )
    return NULL;

  did_you_mean_t *const dym_array = MALLOC( did_you_mean_t, dym_size + 1 );
  did_you_mean_t *dym = dym_array;

  // The order here doesn't matter either.
  if ( (kinds & DYM_CLI_OPTIONS) != DYM_NONE ) {
    copy_cli_options( &dym );
  }
  if ( (kinds & DYM_KEYWORDS) != DYM_NONE ) {
    copy_keywords( &dym, /*copy_types=*/false );
  }
  if ( (kinds & DYM_TYPES) != DYM_NONE ) {
    copy_keywords( &dym, /*copy_types=*/true );
    copy_typedefs( &dym );
  }
  MEM_ZERO( dym );                      // one past last is zero'd

  /*
   * Adapted from the code:
   * <https://github.com/git/git/blob/3a0b884caba2752da0af626fb2de7d597c844e8b/help.c#L516>
   */

  // calculate Damerau-Levenshtein edit distance for all candidates
  for ( dym = dym_array; dym->token != NULL; ++dym )
    dym->dam_lev_dist = dam_lev_dist( unknown_token, dym->token );

  // sort by Damerau-Levenshtein distance
  qsort(
    dym_array, dym_size, sizeof( did_you_mean_t ),
    POINTER_CAST( qsort_cmp_fn_t, &dym_cmp )
  );

  size_t const best_dist = dym_array->dam_lev_dist;
  if ( best_dist == 0 ) {
    //
    // This means unknown_token was an exact match for a token which means we
    // shouldn't suggest it for itself.
    //
    goto none;
  }

  size_t const best_len = strlen( dym_array->token );
  if ( !is_similar_enough( best_dist, SIMILAR_ENOUGH_PERCENT, best_len ) )
    goto none;

  // include all candidates that have the same distance
  for ( dym = dym_array;
        (++dym)->token != NULL && dym->dam_lev_dist == best_dist; )
    ;

  //
  // Free tokens past the best ones and set the one past the last to NULL to
  // mark the end.
  //
  dym_free_tokens( dym );
  dym->token = NULL;

  return dym_array;

none:
  dym_free( dym_array );
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
