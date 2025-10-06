/*
**      ad -- ASCII dump
**      src/unicode.h
**
**      Copyright (C) 2015-2025  Paul J. Lucas
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
#include "ad.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
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

/**
 * Casts an ordinary C string to a UTF-8 string.
 *
 * @param S The C string to cast.
 * @return Returns said UTF-8 string.
 */
#define UTF8_STR(S)               ((char8_t const*)(S))

/** A small array large enough to contain any UTF-8 encoded character. */
typedef uint8_t utf8c_t[ UTF8_CHAR_SIZE_MAX ];

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whether \a cp is an ASCII character.
 *
 * @param cp The Unicode code-point to check.
 * @return Returns \c true only if \a cp is an ASCII character.
 */
NODISCARD
inline bool cp_is_ascii( char32_t cp ) {
  return cp <= 0x7F;
}

/**
 * Checks whether the given Unicode code-point is valid.
 *
 * @param cp_candidate The Unicode code-point candidate value to check.
 * @return Returns `true` only if \a cp_candidate is a valid code-point.
 */
NODISCARD
inline bool cp_is_valid( unsigned long long cp_candidate ) {
  return  cp_candidate < CP_SURROGATE_HIGH_START
      || (cp_candidate > CP_SURROGATE_LOW_END && cp_candidate <= CP_VALID_MAX);
}

/**
 * Decodes a UTF-16 encoded string into its corresponding UTF-32 string.
 *
 * @param u16s A pointer to the UTF-16 encoded string.
 * @param u16_len The length of \a u16s.  Decoding stops when either this
 * number of characters has been decoded or a null byte is encountered.
 * @param endian The endianness of \a u16s.
 * @param u32s A pointer to receive the code-points.
 * @return Returns `true` only if the UTF-16 bytes were valid and decoded
 * successfully.
 *
 * @sa utf16s_8s()
 */
NODISCARD
bool utf16s_32s( char16_t const *u16s, size_t u16_len, endian_t endian,
                 char32_t *u32s );

/**
 * Transcodes a UTF-16 encoded string into its corresponding UTF-8 encoded
 * string.
 *
 * @param u16s The UTF-16 string.
 * @param u16_len The lenth of \a u16s.  Decoding stops when either this
 * number of characters has been decoded or a null byte is encountered.
 * @param endian The endianness of \a u16s.
 * @return Returns said UTF-8 string or NULL if \a u16s contains any invalid
 * bytes.
 *
 * @sa utf16s_32s()
 */
NODISCARD
char8_t* utf16s_8s( char16_t const *u16s, size_t u16_len, endian_t endian );

/**
 * Encodes a Unicode code-point into UTF-8.
 *
 * @param cp The Unicode code-point to encode.
 * @param u8c A pointer to the start of a buffer to receive the UTF-8 bytes;
 * must be at least #UTF8_CHAR_SIZE_MAX long.  No NULL byte is appended.
 * @return Returns `true` only if \a cp is valid.
 *
 * @sa utf32s_8s()
 * @sa utf8c_32c()
 */
PJL_DISCARD
bool utf32c_8c( char32_t cp, char8_t u8c[static UTF8_CHAR_SIZE_MAX] );

/**
 * Encodes a Unicode string into UTF-8.
 *
 * @param u32s The Unicode string to encode.
 * @param u32_len The length of \a u32s.  Encoding stops when either this
 * number of characters has been encoded or a null byte is encountered.
 * @return Returns the UTF-8 encoded string. The caller is responsible for
 * free'ing it.
 *
 * @sa utf32c_8c()
 */
NODISCARD
char8_t* utf32s_8s( char32_t const *u32s, size_t u32_len );

/**
 * Decodes a UTF-8 encoded character into its corresponding Unicode code-point.
 * (This inline version is optimized for the common case of ASCII.)
 *
 * @param u8s A pointer to the first byte of the UTF-8 encoded character.
 * @return Returns said code-point or \c CP_INVALID if the UTF-8 byte sequence
 * is invalid.
 *
 * @sa utf32c_8c()
 */
NODISCARD
inline char32_t utf8c_32c( char8_t const u8c[static UTF8_CHAR_SIZE_MAX] ) {
  extern char32_t utf8c_32c_impl( char8_t const[static UTF8_CHAR_SIZE_MAX] );
  return cp_is_ascii( *u8c ) ? *u8c : utf8c_32c_impl( u8c );
}

/*
 * Gets the number of bytes comprising a UTF-8 character.
 *
 * @param start The start byte of a UTF-8 byte sequence.
 * @return Returns the number of bytes needed for the UTF-8 character in the
 * range [1,6] or 0 if \a start is not a valid start byte.
 */
NODISCARD
inline unsigned utf8c_len( char8_t start ) {
  extern char8_t const UTF8C_LEN_TABLE[];
  return UTF8C_LEN_TABLE[ start ];
}

/**
 * Compares two UTF-8 characters for equality.
 *
 * @param u8c_i The first UTF-8 character.
 * @param u8c_j The second UTF-8 character.
 * @return Returns `true` only if \a u8c_i equals \a u8c_j.
 */
NODISCARD
inline bool utf8c_equal( utf8c_t u8c_i, utf8c_t u8c_j ) {
  extern uint8_t const UTF8C_LEN_TABLE[];
  return memcmp( u8c_i, u8c_j, UTF8C_LEN_TABLE[ u8c_i[0] ] ) == 0;
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
NODISCARD
inline bool utf8_is_start( char8_t c ) {
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
NODISCARD
inline bool utf8_is_cont( char8_t c ) {
  return c >= 0x80 && c < 0xC0;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_unicode_H */
/* vim:set et sw=2 ts=2: */
