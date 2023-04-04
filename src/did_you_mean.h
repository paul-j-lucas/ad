/*
**      ad -- ASCII dump
**      src/did_you_mean.h
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

#ifndef ad_did_you_mean_H
#define ad_did_you_mean_H

/**
 * @file
 * Declares macros, types, and functions for printing suggestions for "Did you
 * mean ...?"
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stddef.h>                     /* for size_t */

/**
 * @defgroup printing-suggestions-group Printing Suggestions
 * Macros, types, and functions for printing suggestions for "Did you mean
 * ...?"
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Data structure to hold a "Did you mean ...?" literal.
 */
struct did_you_mean {
  char const *literal;                  ///< Candidate literal.
  size_t      dam_lev_dist;             ///< Damerau-Levenshtein edit distance.
};
typedef struct did_you_mean did_you_mean_t;

/**
 * The bitwise-or of kinds of things one might have meant.
 *
 * @sa #DYM_CLI_OPTIONS
 * @sa #DYM_KEYWORDS
 * @sa #DYM_TYPES
 */
typedef unsigned dym_kind_t;

#define DYM_NONE            0u          /**< Did you mean nothing. */
#define DYM_CLI_OPTIONS     (1u << 0)   /**< Did you mean _CLI option_? */
#define DYM_KEYWORDS        (1u << 1)   /**< Did you mean _keyword_? */
#define DYM_TYPES           (1u << 2)   /**< Did you mean _type_? */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Frees all memory used by \a dym_array _including_ \a dym_array itself.
 *
 * @param dym_array The \ref did_you_mean array to free.  If NULL, does
 * nothing.
 */
void dym_free( did_you_mean_t const *dym_array );

/**
 * Creates a new array of \ref did_you_mean elements containing "Did you mean
 * ...?" literals for \a unknown_literal.
 *
 * @param kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param unknown_literal The unknown literal.
 * @return Returns a pointer to an array of elements terminated by one having a
 * NULL `literal` pointer if there are suggestions or NULL if not.  The caller
 * is responsible for calling dym_free().
 *
 * @sa dym_free()
 */
NODISCARD
did_you_mean_t const* dym_new( dym_kind_t kinds, char const *unknown_literal );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_did_you_mean_H */
/* vim:set et sw=2 ts=2: */
