/*
**      ad -- ASCII dump
**      src/unicode.h
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

#ifndef ad_unicode_H
#define ad_unicode_H

/**
 * @file
 * Declares macros, types, and functions for working with Unicode characters.
 */

// local
#include "pjl_config.h"                 /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint*_t */
#include <string.h>                     /* for memcmp(3) */
#if HAVE_CHAR8_T || HAVE_CHAR32_T
#include <uchar.h>
#endif /* HAVE_CHAR8_T || HAVE_CHAR32_T */

#if !HAVE_CHAR8_T
typedef uint8_t char8_t;                /* borrowed from C++20 */
#endif /* !HAVE_CHAR8_T */
#if !HAVE_CHAR16_T
typedef uint16_t char16_t;              /* C11's char16_t */
#endif /* !HAVE_CHAR16_T */
#if !HAVE_CHAR32_T
typedef uint32_t char32_t;              /* C11's char32_t */
#endif /* !HAVE_CHAR32_T */

// local
#include "ad.h"

_GL_INLINE_HEADER_BEGIN
#ifndef AD_UNICODE_H_INLINE
# define AD_UNICODE_H_INLINE _GL_INLINE
#endif /* AD_UNICODE_H_INLINE */

/// @endcond

/**
 * @defgroup unicode-group Unicode
 * Macros, types, and functions for working with Unicode characters.
 * @{
 */

#if !HAVE_CHAR8_T
typedef uint8_t char8_t;                /**< Borrowed from C++20. */
#endif /* !HAVE_CHAR8_T */
#if !HAVE_CHAR32_T
typedef uint32_t char32_t;              /**< C11's `char32_t` */
#endif /* !HAVE_CHAR32_T */

///////////////////////////////////////////////////////////////////////////////

#define CP_INVALID                0x01FFFFu /**< Invalid Unicode code-point. */
#define CP_SURROGATE_HIGH_START   0x00D800u /**< Unicode surrogate high. */
#define CP_SURROGATE_HIGH_END     0x00DBFFu
#define CP_SURROGATE_LOW_START    0x00DC00u
#define CP_SURROGATE_LOW_END      0x00DFFFu /**< Unicode surrogate low. */
#define CP_VALID_MAX              0x10FFFFu /**< Maximum valid code-point. */
#define UTF8_CHAR_SIZE_MAX        4     /**< Bytes needed for UTF-8 char. */

typedef uint8_t   utf8_t[ UTF8_CHAR_SIZE_MAX ];

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whether \a cp is an ASCII character.
 *
 * @param cp The Unicode code-point to check.
 * @return Returns \c true only if \a cp is an ASCII character.
 */
AD_UNICODE_H_INLINE bool cp_is_ascii( char32_t cp ) {
  return cp <= 0x7F;
}

/**
 * Checks whether the given Unicode code-point is valid.
 *
 * @param cp_candidate The Unicode code-point candidate value to check.
 * @return Returns `true` only if \a cp_candidate is a valid code-point.
 */
NODISCARD AD_UNICODE_H_INLINE
bool cp_is_valid( unsigned long long cp_candidate ) {
  return  cp_candidate < CP_SURROGATE_HIGH_START
      || (cp_candidate > CP_SURROGATE_LOW_END && cp_candidate <= CP_VALID_MAX);
}

/**
 * Decodes UTF-16 encoded characters into their corresponding Unicode code-
 * points.
 *
 * @param u16 A pointer to the first byte of the UTF-16 encoded characters.
 * @param u16_size The number of UTF-16 characters.
 * @param u32 A pointer to receive the code-points.
 * @return Returns `true` only if the UTF-16 bytes were valid and decoded
 * successfully.
 */
NODISCARD
bool utf16_32( char16_t const *u16, size_t u16_size, endian_t endian,
               char32_t *u32 );

/**
 * Encodes a Unicode code-point into UTF-8.
 *
 * @param cp The Unicode code-point to encode.
 * @param u8 A pointer to the start of a buffer to receive the UTF-8 bytes;
 * must be at least #UTF8_CHAR_SIZE_MAX long.  No NULL byte is appended.
 * @return Returns the number of bytes comprising the code-point encoded as
 * UTF-8.
 */
PJL_DISCARD
size_t utf32_8( char32_t cp, char *u8 );

/**
 * Decodes a UTF-8 encoded character into its corresponding Unicode code-point.
 * (This inline version is optimized for the common case of ASCII.)
 *
 * @param s A pointer to the first byte of the UTF-8 encoded character.
 * @return Returns said code-point or \c CP_INVALID if the UTF-8 byte sequence
 * is invalid.
 */
NODISCARD AD_UNICODE_H_INLINE
char32_t utf8_32( char const *s ) {
  extern char32_t utf8_32_impl( char const* );
  char32_t const cp = (uint8_t)*s;
  return cp_is_ascii( cp ) ? cp : utf8_32_impl( s );
}

/*
 * Gets the number of bytes comprising a UTF-8 character.
 *
 * @param start The start byte of a UTF-8 byte sequence.
 * @return Returns the number of bytes needed for the UTF-8 character in the
 * range [1,6] or 0 if \a start is not a valid start byte.
 */
NODISCARD AD_UNICODE_H_INLINE
unsigned utf8_char_len( char8_t start ) {
  extern char8_t const UTF8_CHAR_LEN_TABLE[];
  return UTF8_CHAR_LEN_TABLE[ start ];
}

/**
 * Compares two UTF-8 characters for equality.
 *
 * @param u1 The first UTF-8 character.
 * @param u2 The second UTF-8 character.
 * @return Returns `true` only if \a u1 equals \a u2.
 */
NODISCARD AD_UNICODE_H_INLINE
bool utf8_equal( utf8_t const u1, utf8_t const u2 ) {
  extern uint8_t const UTF8_CHAR_LEN_TABLE[];
  return memcmp( u1, u2, UTF8_CHAR_LEN_TABLE[ u1[0] ] ) == 0;
}

/**
 * Checks whether the given byte is the first byte of a UTF-8 byte sequence
 * comprising an encoded character.
 *
 * @note This is _not_ equivalent to `!utf8_is_cont(c)`.
 *
 * @param c The byte to check.
 * @return Returns `true` only if the byte is the first byte of a UTF-8 byte
 * sequence comprising an encoded character.
 *
 * @sa utf8_is_cont()
 */
NODISCARD AD_UNICODE_H_INLINE
bool utf8_is_start( char8_t c ) {
  return c < 0x80 || (c >= 0xC2 && c < 0xFE);
}

/**
 * Checks whether the given byte is not the first byte of a UTF-8 byte sequence
 * comprising an encoded character.
 *
 * @note This is _not_ equivalent to `!utf8_is_start(c)`.
 *
 * @param c The byte to check.
 * @return Returns `true` only if the byte is not the first byte of a UTF-8
 * byte sequence comprising an encoded character.
 *
 * @sa utf8_is_start().
 */
NODISCARD AD_UNICODE_H_INLINE
bool utf8_is_cont( char8_t c ) {
  return c >= 0x80 && c < 0xC0;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* ad_unicode_H */
/* vim:set et sw=2 ts=2: */
