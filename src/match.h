/*
**      ad -- ASCII dump
**      src/match.h
**
**      Copyright (C) 2015-2024  Paul J. Lucas
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

#ifndef ad_match_H
#define ad_match_H

/**
 * @file
 * Declares types and functions for matching numbers or strings.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "unicode.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint32_t */

/// @endcond

/**
 * @defgroup matching-group Matching Numbers or Strings
 * Types, variables, and functions for matching numbers or strings.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

typedef uint32_t  match_bits_t;         ///< Bit _i_ means byte _i_ matches.

// extern variables
extern unsigned long total_matches;     ///< Total number of matches.

/**
 * Consructs the partial-match table used by the Knuth-Morris-Pratt (KMP)
 * string searching algorithm.
 *
 * @remarks For the small search patterns and there being no requirement for
 * super-fast performance for this application, brute-force searching would
 * have been fine.  However, KMP has the advantage of never having to back up
 * within the string being searched which is a requirement when reading from
 * stdin.
 *
 * @param pattern The search pattern to use.
 * @param pattern_len The length of \a pattern.
 * @return Returns an array containing the values comprising the partial-match
 * table.  The caller is responsible for freeing the array.
 */
NODISCARD
size_t* kmp_new( char const *pattern, size_t pattern_len );

/**
 * Gets a row of bytes and whether each byte matches bytes in the search
 * buffer.
 *
 * @param row_buf A pointer to the row buffer.
 * @param row_len The length of \a row_buf.
 * @param match_bits A pointer to receive which bytes matched.  Note that the
 * bytes in the buffer are numbered left-to-right whereas their corresponding
 * bits are numbered right-to-left.
 * @param kmps A pointer to the array of KMP values to use or NULL.
 * @param pmatch_buf A pointer to a pointer to a buffer to use while matching
 * or NULL.
 * @param pmatch_len A pointer to the size of \a *pmatch_buf or NULL.
 * @return Returns the number of bytes in \a row_buf.  It should always be \a
 * row_len except on the last row in which case it will be less than \a
 * row_len.
 */
NODISCARD
size_t match_row( char8_t *row_buf, size_t row_len,
                  match_bits_t *match_bits, size_t const *kmps,
                  char8_t **pmatch_buf, size_t *pmatch_len );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_match_H */
/* vim:set et sw=2 ts=2: */
