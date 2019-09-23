/*
**      ad -- ASCII dump
**      src/unicode.c
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

// local
#include "pjl_config.h"                 /* must go first */
#define AD_UNICODE_INLINE _GL_EXTERN_INLINE
#include "unicode.h"

// standard
#include <assert.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

static codepoint_t const CP_SURROGATE_HIGH_START  = 0x00D800u;
static codepoint_t const CP_SURROGATE_LOW_END     = 0x00DFFFu;
static codepoint_t const CP_VALID_MAX             = 0x10FFFFu;

uint8_t const UTF8_LEN_TABLE[] = {
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

/**
 * Checks whether a given Unicode code-point is valid.
 *
 * @param cp The Unicode code-point to check.
 * @return Returns \c true only if valid.
 */
static inline bool cp_is_valid( codepoint_t cp ) {
  return  cp < CP_SURROGATE_HIGH_START
      || (cp > CP_SURROGATE_LOW_END && cp <= CP_VALID_MAX);
}

////////// extern functions ///////////////////////////////////////////////////

codepoint_t utf8_decode_impl( char const *s ) {
  assert( s != NULL );
  size_t const len = utf8_len( *s );
  assert( len >= 1 );

  codepoint_t cp = 0;
  uint8_t const *u = (uint8_t const*)s;

  switch ( len ) {
    case 6: cp += *u++; cp <<= 6; // FALLTHROUGH
    case 5: cp += *u++; cp <<= 6; // FALLTHROUGH
    case 4: cp += *u++; cp <<= 6; // FALLTHROUGH
    case 3: cp += *u++; cp <<= 6; // FALLTHROUGH
    case 2: cp += *u++; cp <<= 6; // FALLTHROUGH
    case 1: cp += *u;
  } // switch

  static codepoint_t const OFFSET_TABLE[] = {
    0, // unused
    0x0, 0x3080, 0xE2080, 0x3C82080, 0xFA082080, 0x82082080
  };
  cp -= OFFSET_TABLE[ len ];
  return cp_is_valid( cp ) ? cp : CP_INVALID;
}

char const* utf8_rsync_impl( char const *buf, char const *pos ) {
  while ( pos > buf && utf8_is_cont( *pos ) )
    --pos;
  return utf8_is_start( *pos ) ? pos : NULL;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
