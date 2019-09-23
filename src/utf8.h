/*
**      ad -- ASCII dump
**      src/utf8.h
**
**      Copyright (C) 2015-2018  Paul J. Lucas
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

#ifndef ad_utf8_H
#define ad_utf8_H

// local
#include "pjl_config.h"                 /* must go first */
#include "util.h"

// standard
#include <inttypes.h>                   /* for uint32_t */
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

_GL_INLINE_HEADER_BEGIN
#ifndef AD_UTF8_INLINE
# define AD_UTF8_INLINE _GL_INLINE
#endif /* AD_UTF8_INLINE */

///////////////////////////////////////////////////////////////////////////////

#define CP_SURROGATE_HIGH_START   0x00D800
#define CP_SURROGATE_LOW_END      0x00DFFF
#define CP_VALID_MAX              0x10FFFF
#define UTF8_LEN_MAX              6       /* max bytes needed for UTF-8 char */
#define UTF8_PAD_CHAR_DEFAULT     "\xE2\x96\xA1" /* U+25A1: "white square" */

/**
 * When to dump in UTF-8.
 */
enum utf8_when {
  UTF8_NEVER,                           // never dump in UTF-8
  UTF8_ENCODING,                        // dump in UTF-8 only if encoding is
  UTF8_ALWAYS                           // always dump in UTF-8
};
typedef enum utf8_when utf8_when_t;

#define UTF8_WHEN_DEFAULT         UTF8_NEVER

typedef uint32_t codepoint_t;

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whether the given Unicode code-point is valid.
 *
 * @param codepoint The Unicode code-point to check.
 * @return Returns \c true only if \a codepoint is valid.
 */
AD_UTF8_INLINE bool cp_is_valid( uint64_t codepoint ) {
  return  codepoint < CP_SURROGATE_HIGH_START
      || (codepoint > CP_SURROGATE_LOW_END && codepoint <= CP_VALID_MAX);
}

/**
 * Determines whether we should dump in UTF-8.
 *
 * @param when The UTF-8 when value.
 * @return Returns \c true only if we should do UTF-8.
 */
bool should_utf8( utf8_when_t when );

/**
 * Encodes a Unicode codepoint into UTF-8.
 *
 * @param codepoint The Unicode code-point to encode.
 * @param utf8_buf A pointer to the start of a buffer to receive the UTF-8
 * bytes; must be at least \c UTF8_LEN_MAX long.  No NULL byte is appended.
 * @return Returns the number of bytes comprising the codepoint encoded as
 * UTF-8.
 */
size_t utf8_encode( codepoint_t codepoint, char *utf8_buf );

/**
 * Checks whether the given byte is the first byte of a UTF-8 byte sequence
 * comprising an encoded character.  Note that this is not equivalent to
 * !utf8_is_cont(c).
 *
 * @param c The byte to check.
 * @return Returns \c true only if the byte is the first byte of a UTF-8 byte
 * sequence comprising an encoded character.
 */
AD_UTF8_INLINE bool utf8_is_start( char c ) {
  unsigned char const u = c;
  return u < 0x80 || (u >= 0xC2 && u < 0xFE);
}

/**
 * Checks whether the given byte is not the first byte of a UTF-8 byte sequence
 * comprising an encoded character.  Note that this is not equivalent to
 * !utf8_is_start(c).
 *
 * @param c The byte to check.
 * @return Returns \c true only if the byte is not the first byte of a UTF-8
 * byte sequence comprising an encoded character.
 */
AD_UTF8_INLINE bool utf8_is_cont( char c ) {
  unsigned char const u = c;
  return u >= 0x80 && u < 0xC0;
}

/**
 * Gets the length of a UTF-8 character.
 *
 * @param start The start byte of a UTF-8 byte sequence.
 * @return Returns the number of bytes needed for the UTF-8 character in the
 * range [1,6] or 0 if \a start is not a valid start byte.
 */
AD_UTF8_INLINE size_t utf8_len( char start ) {
  extern char const UTF8_LEN_TABLE[];
  return STATIC_CAST(
    size_t, UTF8_LEN_TABLE[ STATIC_CAST(unsigned char, start) ]
  );
}

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* ad_utf8_H */
/* vim:set et sw=2 ts=2: */
