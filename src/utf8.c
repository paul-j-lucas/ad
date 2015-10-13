/*
**      ad -- ASCII dump
**      utf8.c
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

// local
#include "config.h"
#define AD_UTF8_INLINE _GL_EXTERN_INLINE
#include "utf8.h"

// standard
#include <assert.h>
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_H */
#include <string.h>

////////// extern variables ///////////////////////////////////////////////////

char const utf8_len_table[] = {
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

////////// extern functions ///////////////////////////////////////////////////

bool should_utf8( utf8_when_t when ) {
  switch ( when ) {                     // handle easy cases
    case UTF8_ALWAYS: return true;
    case UTF8_NEVER : return false;
    default         : break;
  } // switch

#if defined( HAVE_SETLOCALE ) && defined( HAVE_NL_LANGINFO )
  setlocale( LC_CTYPE, "" );
  char const *const encoding =
    (char*)freelist_add( tolower_s( check_strdup( nl_langinfo( CODESET ) ) ) );
  return  strcmp( encoding, "utf8"  ) == 0 ||
          strcmp( encoding, "utf-8" ) == 0;
#else
  return false;
#endif
}

size_t utf8_encode( uint32_t codepoint, char *p ) {
  assert( p );

  static unsigned const Mask1 = 0x80;
  static unsigned const Mask2 = 0xC0;
  static unsigned const Mask3 = 0xE0;
  static unsigned const Mask4 = 0xF0;
  static unsigned const Mask5 = 0xF8;
  static unsigned const Mask6 = 0xFC;

  unsigned const n = codepoint & 0xFFFFFFFF;
  char *const p0 = p;
  if ( n < 0x80 ) {
    // 0xxxxxxx
    *p++ = (char)n;
  } else if ( n < 0x800 ) {
    // 110xxxxx 10xxxxxx
    *p++ = (char)( Mask2 |  (n >>  6)         );
    *p++ = (char)( Mask1 | ( n        & 0x3F) );
  } else if ( n < 0x10000 ) {
    // 1110xxxx 10xxxxxx 10xxxxxx
    *p++ = (char)( Mask3 |  (n >> 12)         );
    *p++ = (char)( Mask1 | ((n >>  6) & 0x3F) );
    *p++ = (char)( Mask1 | ( n        & 0x3F) );
  } else if ( n < 0x200000 ) {
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    *p++ = (char)( Mask4 |  (n >> 18)         );
    *p++ = (char)( Mask1 | ((n >> 12) & 0x3F) );
    *p++ = (char)( Mask1 | ((n >>  6) & 0x3F) );
    *p++ = (char)( Mask1 | ( n        & 0x3F) );
  } else if ( n < 0x4000000 ) {
    // 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    *p++ = (char)( Mask5 |  (n >> 24)         );
    *p++ = (char)( Mask1 | ((n >> 18) & 0x3F) );
    *p++ = (char)( Mask1 | ((n >> 12) & 0x3F) );
    *p++ = (char)( Mask1 | ((n >>  6) & 0x3F) );
    *p++ = (char)( Mask1 | ( n        & 0x3F) );
  } else if ( n < 0x8000000 ) {
    // 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    *p++ = (char)( Mask6 |  (n >> 30)         );
    *p++ = (char)( Mask1 | ((n >> 24) & 0x3F) );
    *p++ = (char)( Mask1 | ((n >> 18) & 0x3F) );
    *p++ = (char)( Mask1 | ((n >> 12) & 0x3F) );
    *p++ = (char)( Mask1 | ((n >>  6) & 0x3F) );
    *p++ = (char)( Mask1 | ( n        & 0x3F) );
  }
  return p - p0;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
