/*
**      ad -- ASCII dump
**      src/unicode.c
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

/**
 * @file
 * Defines functions for working with Unicode characters.
 */

// local
#include "pjl_config.h"                 /* must go first */
#define AD_UNICODE_H_INLINE _GL_EXTERN_INLINE
#include "unicode.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

/**
 * @addtogroup matching-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

// extern constant definitions
char8_t const UTF8C_LEN_TABLE[] = {
  /*      0 1 2 3 4 5 6 7 8 9 A B C D E F */
  /* 0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 1 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 2 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 3 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 4 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 5 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 6 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 7 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  /* 8 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // continuation bytes
  /* 9 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //        |
  /* A */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //        |
  /* B */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //        |
  /* C */ 0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  // C0 & C1 are overlong ASCII
  /* D */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  /* E */ 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  /* F */ 4,4,4,4,4,4,4,4,5,5,5,5,6,6,0,0
};

////////// inline functions ///////////////////////////////////////////////////

static inline bool utf16_is_high_surrogate( char16_t u16c ) {
  return (u16c & 0xFFFFFC00u) == CP_SURROGATE_HIGH_START;
}

static inline bool utf16_is_low_surrogate( char16_t u16c ) {
  return (u16c & 0xFFFFFC00u) == CP_SURROGATE_LOW_START;
}

static inline bool utf16_is_surrogate( char16_t u16c ) {
  return u16c - CP_SURROGATE_HIGH_START < 2048u;
}

static inline char32_t utf16_surrogate_to_utf32( char16_t high, char16_t low ) {
  return STATIC_CAST( unsigned, high << 10u ) + low - 0x35FDC00u;
}

////////// extern functions ///////////////////////////////////////////////////

bool utf16s_32s( char16_t const *u16s, size_t u16_size, endian_t endian,
                 char32_t *u32s ) {
  assert( u16s != NULL );
  assert( u32s != NULL );

  char16_t const *const u16_end = u16s + u16_size;
  while ( u16s < u16_end ) {
    char16_t const u16c = uint16xx_host16( *u16s++, endian );
    if ( likely( !utf16_is_surrogate( u16c ) ) ) {
      *u32s++ = u16c;
    }
    else if ( utf16_is_high_surrogate( u16c ) &&
              u16s < u16_end && utf16_is_low_surrogate( *u16s ) ) {
      *u32s = utf16_surrogate_to_utf32( u16c, *u16s++ );
    }
    else {
      return false;
    }
  } // while
  return true;
}

unsigned utf32c_8c( char32_t cp, char8_t *u8c ) {
  assert( u8c != NULL );

  static unsigned const Mask1 = 0x80;
  static unsigned const Mask2 = 0xC0;
  static unsigned const Mask3 = 0xE0;
  static unsigned const Mask4 = 0xF0;

  char8_t *const u8c_orig = u8c;
  if ( cp < 0x80 ) {
    // 0xxxxxxx
    *u8c++ = STATIC_CAST( char8_t, cp );
  }
  else if ( cp < 0x800 ) {
    // 110xxxxx 10xxxxxx
    *u8c++ = STATIC_CAST( char8_t, Mask2 |  (cp >>  6)         );
    *u8c++ = STATIC_CAST( char8_t, Mask1 | ( cp        & 0x3F) );
  }
  else if ( cp < 0x10000 ) {
    // 1110xxxx 10xxxxxx 10xxxxxx
    *u8c++ = STATIC_CAST( char8_t, Mask3 |  (cp >> 12)         );
    *u8c++ = STATIC_CAST( char8_t, Mask1 | ((cp >>  6) & 0x3F) );
    *u8c++ = STATIC_CAST( char8_t, Mask1 | ( cp        & 0x3F) );
  }
  else if ( cp < 0x200000 ) {
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    *u8c++ = STATIC_CAST( char8_t, Mask4 |  (cp >> 18)         );
    *u8c++ = STATIC_CAST( char8_t, Mask1 | ((cp >> 12) & 0x3F) );
    *u8c++ = STATIC_CAST( char8_t, Mask1 | ((cp >>  6) & 0x3F) );
    *u8c++ = STATIC_CAST( char8_t, Mask1 | ( cp        & 0x3F) );
  }
  else {
    return STATIC_CAST( unsigned, -1 );
  }

  return STATIC_CAST( unsigned, u8c - u8c_orig );
}

char8_t* utf32s_8s( char32_t const *cps ) {
  strbuf_t sbuf;
  strbuf_init( &sbuf );
  utf8_t u8c;
  for ( ; *cps != 0; ++cps )
    strbuf_putsn( &sbuf, POINTER_CAST( char*, u8c ), utf32c_8c( *cps, u8c ) );
  return POINTER_CAST( char8_t*, strbuf_take( &sbuf ) );
}

char32_t utf8c_32c_impl( char const *s ) {
  assert( s != NULL );
  unsigned const len = utf8c_len( STATIC_CAST( char8_t, *s ) );
  assert( len >= 1 );

  char32_t cp = 0;
  uint8_t const *u8 = POINTER_CAST( uint8_t const*, s );

  switch ( len ) {
    case 4: cp += *u8++; cp <<= 6; FALLTHROUGH;
    case 3: cp += *u8++; cp <<= 6; FALLTHROUGH;
    case 2: cp += *u8++; cp <<= 6; FALLTHROUGH;
    case 1: cp += *u8;
  } // switch

  static char32_t const OFFSET_TABLE[] = {
    0, // unused
    0x0, 0x3080, 0xE2080, 0x3C82080, 0xFA082080, 0x82082080
  };
  cp -= OFFSET_TABLE[ len ];
  return cp_is_valid( cp ) ? cp : CP_INVALID;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
