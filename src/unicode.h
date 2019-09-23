/*
**      ad -- ASCII dump
**      src/unicode.h
**
**      Copyright (C) 2019  Paul J. Lucas
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
 * Contains constants and types for Unicode code-points and UTF-8 byte
 * sequences as well as functions for manipulating them.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <inttypes.h>                   /* for uint32_t */
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <string.h>                     /* for memmove(3) */
#include <wctype.h>

_GL_INLINE_HEADER_BEGIN
#ifndef AD_UNICODE_INLINE
# define AD_UNICODE_INLINE _GL_INLINE
#endif /* AD_UNICODE_INLINE */

///////////////////////////////////////////////////////////////////////////////

/**
 * Unicode code-point.
 */
typedef uint32_t codepoint_t;

#define CP_BYTE_ORDER_MARK        0x00FEFFu
#define CP_EOF                    ((codepoint_t)EOF)
#define CP_INVALID                0x1FFFFFu
#define UTF8_CHAR_SIZE_MAX        6     /**< Max bytes needed for UTF-8 char. */

/**
 * UTF-8 character.
 */
typedef char utf8c_t[ UTF8_CHAR_SIZE_MAX ];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks whether \a cp is an alphabetic character.
 *
 * @param cp The Unicode code-point to check.
 * @return Returns \c true only if \a cp is an alphabetic character.
 */
AD_UNICODE_INLINE bool cp_is_alpha( codepoint_t cp ) {
  return iswalpha( cp );
}

/**
 * Checks whether \a cp is an ASCII character.
 *
 * @param cp The Unicode code-point to check.
 * @return Returns \c true only if \a cp is an ASCII character.
 */
AD_UNICODE_INLINE bool cp_is_ascii( codepoint_t cp ) {
  return cp <= 0x7F;
}

/**
 * Checks whether \a cp is a control character.
 *
 * @param cp The Unicode code-point to check.
 * @return Returns \c true only if \a cp is a control character.
 */
AD_UNICODE_INLINE bool cp_is_control( codepoint_t cp ) {
  return iswcntrl( cp );
}

/**
 * Checks whether \a cp is a space character.
 *
 * @param cp The Unicode code-point to check.
 * @return Returns \a true only if \a cp is a space character.
 */
AD_UNICODE_INLINE bool cp_is_space( codepoint_t cp ) {
  return iswspace( cp );
}

/**
 * Decodes a UTF-8 encoded character into its corresponding Unicode code-point.
 * (This inline version is optimized for the common case of ASCII.)
 *
 * @param s A pointer to the first byte of the UTF-8 encoded character.
 * @return Returns said code-point or \c CP_INVALID if the UTF-8 byte sequence
 * is invalid.
 */
AD_UNICODE_INLINE codepoint_t utf8_decode( char const *s ) {
  extern codepoint_t utf8_decode_impl( char const* );
  codepoint_t const cp = (uint8_t)*s;
  return cp_is_ascii( cp ) ? cp : utf8_decode_impl( s );
}

/**
 * Checks whether the given byte is not the first byte of a UTF-8 byte sequence
 * of an encoded character.
 *
 * @param c The byte to check.
 * @return Returns \c true only if the byte is not the first byte of a byte
 * sequence of a UTF-8 encoded character.
 */
AD_UNICODE_INLINE bool utf8_is_cont( char c ) {
  uint8_t const u = (uint8_t)c;
  return u >= 0x80 && u < 0xC0;
}

/**
 * Checks whether the given byte is the first byte of a UTF-8 byte sequence of
 * an encoded character.
 *
 * @param c The byte to check.
 * @return Returns \c true only if the byte is the first byte of a byte
 * sequence of a UTF-8 encoded character.
 */
AD_UNICODE_INLINE bool utf8_is_start( char c ) {
  uint8_t const u = (uint8_t)c;
  return u <= 0x7F || (u >= 0xC2 && u < 0xFE);
}

/**
 * Gets the number of bytes for the UTF-8 encoding of a Unicode code-point
 * given its first byte.
 *
 * @param c The first byte of the UTF-8 encoded code-point.
 * @return Returns 1-6, or 0 if \a c is invalid.
 */
AD_UNICODE_INLINE size_t utf8_len( char c ) {
  extern uint8_t const UTF8_LEN_TABLE[];
  return (size_t)UTF8_LEN_TABLE[ (uint8_t)c ];
}

/**
 * Copies all of the bytes of a UTF-8 encoded Unicode character.
 *
 * @param dest A pointer to the destination.
 * @param src A pointer to the source.
 * @return Returns the number of bytes copied.
 */
AD_UNICODE_INLINE size_t utf8_copy_char( char *dest, char const *src ) {
  size_t const len = utf8_len( src[0] );
  memmove( dest, src, len );
  return len;
}

/**
 * Given a pointer to any byte within a UTF-8 encoded string, synchronizes in
 * reverse to find the first byte of the UTF-8 character byte sequence the
 * pointer is pointing within.  (This inline version is optimized for the
 * common case of ASCII.)
 *
 * @param buf A pointer to the beginning of the buffer.
 * @param pos A pointer to any byte with the buffer.
 * @return Returns a pointer less than or equal to \a pos that points to the
 * first byte of a UTF-8 encoded character byte sequence or NULL if there is
 * none.
 */
AD_UNICODE_INLINE char const* utf8_rsync( char const *buf, char const *pos ) {
  extern char const* utf8_rsync_impl( char const*, char const* );
  codepoint_t const cp = (uint8_t)*pos;
  return cp_is_ascii( cp ) ? pos : utf8_rsync_impl( buf, pos );
}

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* ad_unicode_H */
/* vim:set et sw=2 ts=2: */
