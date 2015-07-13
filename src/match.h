/*
**      ad -- ASCII dump
**      match.h
**
**      Copyright (C) 2015  Paul J. Lucas
**
**      This program is free software; you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation; either version 2 of the Licence, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program; if not, write to the Free Software
**      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ad_match_H
#define ad_match_H

// system
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint8_t, uint16_t */

///////////////////////////////////////////////////////////////////////////////

typedef size_t kmp_t;                   // Knuth-Morris-Pratt prefix value

/**
 * Consructs the partial-match table used by the Knuth-Morris-Pratt (KMP)
 * string searching algorithm.
 *
 * For the small search patterns and there being no requirement for super-fast
 * performance for this application, brute-force searching would have been
 * fine.  However, KMP has the advantage of never having to back up within the
 * string being searched which is a requirement when reading from stdin.
 *
 * @param pattern The search pattern to use.
 * @param pattern_len The length of the pattern.
 * @return Returns an array containing the values comprising the partial-match
 * table.  The caller is responsible for freeing the array.
 */
kmp_t* kmp_init( char const *pattern, size_t pattern_len );

/**
 * Gets a row of bytes and whether each byte matches bytes in the search
 * buffer.
 *
 * @param row_buf A pointer to the row buffer.
 * @param row_size The size of the row to match.
 * @param match_bits A pointer to receive which bytes matched.  Note that the
 * bytes in the buffer are numbered left-to-right where as their corresponding
 * bits are numbered right-to-left.
 * @param kmps A pointer to the array of KMP values to use or NULL.
 * @param match_buf A pointer to a buffer to use while matching or NULL.
 * @return Returns the number of bytes in \a row_buf.  It should always be
 * \a row_size except on the last row in which case it will be less than
 * \a row_size.
 */
size_t match_row( uint8_t *row_buf, size_t row_size, uint16_t *match_bits,
                  kmp_t const *kmps, uint8_t *match_buf );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_match_H */
/* vim:set et sw=2 ts=2: */
