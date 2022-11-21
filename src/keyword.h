/*
**      ad -- ASCII dump
**      src/keyword.h
**
**      Copyright (C) 2021  Paul J. Lucas
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

#ifndef ad_keyword_H
#define ad_keyword_H

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/**
 * @file
 * Declares types and functions for looking up **ad** keyword information.
 */

/**
 * Convenience macro for iterating over all **ad** keywords.
 *
 * @param VAR The keyword loop variable.
 *
 * @sa ad_keyword_next()
 */
#define FOREACH_AD_KEYWORD(VAR) \
  for ( ad_keyword_t const *VAR = NULL; (VAR = ad_keyword_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

/**
 * ad keyword info.
 */
struct ad_keyword {
  char const *literal;                  ///< C string literal of the keyword.
  int         yy_token_id;              ///< Bison token number.
  ad_tid_t    tid;                      ///< Type the keyword maps to, if any.
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Finds a ad keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or null if not found.
 */
ad_keyword_t const* ad_keyword_find( char const *s );

/**
 * Iterates to the next ad keyword.
 *
 * @param k A pointer to the current keyword. For the first iteration, NULL
 * should be passed.
 * @return Returns the next **ad** keyword or NULL for none.
 *
 * @sa #FOREACH_C_KEYWORD()
 */
NODISCARD
ad_keyword_t const* ad_keyword_next( ad_keyword_t const *k );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_keyword_H */
/* vim:set et sw=2 ts=2: */
