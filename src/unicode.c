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

// local
#include "pjl_config.h"                 /* must go first */
#define AD_UNICODE_H_INLINE _GL_EXTERN_INLINE
#include "unicode.h"
#include "util.h"

// standard
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

// extern constant definitions
char8_t const UTF8_CHAR_LEN_TABLE[] = {
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

static inline bool utf16_is_high_surrogate( char16_t u16 ) {
  return (u16 & 0xFFFFFC00u) == CP_SURROGATE_HIGH_START;
}

static inline bool utf16_is_low_surrogate( char16_t u16 ) {
  return (u16 & 0xFFFFFC00u) == CP_SURROGATE_LOW_START;
}

static inline bool utf16_is_surrogate( char16_t u16 ) {
  return u16 - CP_SURROGATE_HIGH_START < 2048u;
}

static inline char32_t utf16_surrogate_to_utf32( char16_t high, char16_t low ) {
  return STATIC_CAST( unsigned, high << 10u ) + low - 0x35FDC00u;
}

////////// extern functions ///////////////////////////////////////////////////

bool utf16_32( char16_t const *u16, size_t size16, endian_t endian,
               char32_t *u32 ) {
  assert( u16 != NULL );
  assert( u32 != NULL );

  char16_t const *const end16 = u16 + size16;
  while ( u16 < end16 ) {
    char16_t const c16 = uint16xx_host16( *u16++, endian );
    if ( likely( !utf16_is_surrogate( c16 ) ) ) {
      *u32++ = c16;
    }
    else if ( utf16_is_high_surrogate( c16 ) &&
              u16 < end16 && utf16_is_low_surrogate( *u16 ) ) {
      *u32 = utf16_surrogate_to_utf32( c16, *u16++ );
    }
    else {
      return false;
    }
  } // while
  return true;
}

size_t utf32_8( char32_t cp, char *u8 ) {
  assert( u8 != NULL );

  static unsigned const Mask1 = 0x80;
  static unsigned const Mask2 = 0xC0;
  static unsigned const Mask3 = 0xE0;
  static unsigned const Mask4 = 0xF0;

  char *const u8_orig = u8;
  if ( cp < 0x80 ) {
    // 0xxxxxxx
    *u8++ = STATIC_CAST( char, cp );
  }
  else if ( cp < 0x800 ) {
    // 110xxxxx 10xxxxxx
    *u8++ = STATIC_CAST( char, Mask2 |  (cp >>  6)         );
    *u8++ = STATIC_CAST( char, Mask1 | ( cp        & 0x3F) );
  }
  else if ( cp < 0x10000 ) {
    // 1110xxxx 10xxxxxx 10xxxxxx
    *u8++ = STATIC_CAST( char, Mask3 |  (cp >> 12)         );
    *u8++ = STATIC_CAST( char, Mask1 | ((cp >>  6) & 0x3F) );
    *u8++ = STATIC_CAST( char, Mask1 | ( cp        & 0x3F) );
  }
  else if ( cp < 0x200000 ) {
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    *u8++ = STATIC_CAST( char, Mask4 |  (cp >> 18)         );
    *u8++ = STATIC_CAST( char, Mask1 | ((cp >> 12) & 0x3F) );
    *u8++ = STATIC_CAST( char, Mask1 | ((cp >>  6) & 0x3F) );
    *u8++ = STATIC_CAST( char, Mask1 | ( cp        & 0x3F) );
  }
  else {
    return STATIC_CAST( size_t, -1 );
  }

  return STATIC_CAST( size_t, u8 - u8_orig );
}

char32_t utf8_32_impl( char const *s ) {
  assert( s != NULL );
  unsigned const len = utf8_len( STATIC_CAST( char8_t, *s ) );
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
/* vim:set et sw=2 ts=2: */
