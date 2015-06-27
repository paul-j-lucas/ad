/*
**      ad -- ASCII dump
**      match.c
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
#include "match.h"
#include "util.h"

// system
#include <assert.h>
#include <ctype.h>                      /* for tolower() */
#include <stdlib.h>                     /* for exit() */

////////// extern variables ///////////////////////////////////////////////////

extern FILE        *fin;
extern char const  *fin_path;
extern char const  *me;
extern bool         opt_case_insensitive;
extern size_t       opt_max_bytes_to_read;
extern char        *search_buf;
extern size_t       search_len;

////////// local variables ////////////////////////////////////////////////////

static size_t       total_bytes_read;

////////// local functions ////////////////////////////////////////////////////

static bool         get_byte( uint8_t*, FILE* );
static bool         match_byte( uint8_t*, bool*, kmp_t const*, uint8_t* );
static void         unget_byte( uint8_t, FILE* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets a byte from the given file.
 *
 * @param pbyte A pointer to the byte to receive the newly read byte.
 * @param file The file to read from.
 * @return Returns \c true if a byte was read successfully.
 */
static bool get_byte( uint8_t *pbyte, FILE *file ) {
  if ( total_bytes_read < opt_max_bytes_to_read ) {
    int const c = getc( file );
    if ( c != EOF ) {
      ++total_bytes_read;
      assert( pbyte );
      *pbyte = (uint8_t)c;
      return true;
    }
    if ( ferror( file ) )
      PMESSAGE_EXIT( READ_ERROR,
        "\"%s\": read byte failed: %s", fin_path, ERROR_STR
      );
  }
  return false;
}

/**
 * Gets a byte and whether it matches one of the bytes in the search buffer.
 *
 * @param pbyte A pointer to receive the byte.
 * @param matches A pointer to receive whether the byte matches.
 * @param kmps A pointer to the array of KMP values to use.
 * @param match_buf A pointer to a buffer to use while matching.
 * It must be at least as large as the search buffer.
 * @return Returns \c true if a byte was read successfully.
 */
static bool match_byte( uint8_t *pbyte, bool *matches, kmp_t const *kmps,
                        uint8_t *match_buf ) {
  enum state {
    S_READING,                          // just reading; not matching
    S_MATCHING,                         // matching search bytes
    S_MATCHING_CONT,                    // matching after a mismatch
    S_MATCHED,                          // a complete match
    S_NOT_MATCHED,                      // didn't match after all
    S_DONE                              // no more input
  };
  typedef enum state state_t;

  static size_t buf_pos;
  static size_t buf_drain;              // bytes to "drain" buf after mismatch
  static kmp_t kmp;
  static state_t state = S_READING;

  uint8_t byte;

  assert( pbyte );
  assert( matches );
  assert( state != S_DONE );

  *matches = false;

  for ( ;; ) {
    switch ( state ) {

#define GOTO_STATE(S)       { buf_pos = 0; state = (S); continue; }
#define RETURN(BYTE)        BLOCK( *pbyte = (BYTE); return true; )

#define MAYBE_NO_CASE(BYTE) \
  ( opt_case_insensitive ? (uint8_t)tolower( (char)(BYTE) ) : (BYTE) )

      case S_READING:
        if ( !get_byte( &byte, fin ) )
          GOTO_STATE( S_DONE );
        if ( !search_len )              // user isn't searching for anything
          RETURN( byte );
        if ( MAYBE_NO_CASE( byte ) != (uint8_t)search_buf[0] )
          RETURN( byte );               // searching, but no match yet
        //
        // The read byte matches the first byte of the search buffer: start
        // storing bytes in the match buffer and try to match the rest of the
        // search buffer.  While matching, we can not return to the caller
        // since we won't know whether the current sequence of bytes will fully
        // match the search buffer until we reach its end.
        //
        match_buf[0] = byte;
        kmp = 0;
        GOTO_STATE( S_MATCHING );

      case S_MATCHING:
        if ( ++buf_pos == search_len ) {
          //
          // We've reached the end of the serch buffer, hence the current
          // sequence of bytes fully matches: we can now drain the match buffer
          // and return the bytes individually to the caller denoting that all
          // matched.
          //
          buf_drain = buf_pos;
          GOTO_STATE( S_MATCHED );
        }
        // no break;
      case S_MATCHING_CONT:
        if ( !get_byte( &byte, fin ) ) {
          //
          // We've reached EOF and there weren't enough bytes to match the
          // search buffer: just drain the match buffer and return the bytes
          // individually to the caller denoting that none matched.
          //
          buf_drain = buf_pos;
          GOTO_STATE( S_NOT_MATCHED );
        }
        if ( MAYBE_NO_CASE( byte ) == (uint8_t)search_buf[ buf_pos ] ) {
          //
          // The next byte matched: keep storing bytes in the match buffer and
          // keep matching.
          //
          match_buf[ buf_pos ] = byte;
          state = S_MATCHING;           // in case we were S_MATCHING_CONT
          continue;
        }
        //
        // The read byte mismatches a byte in the search buffer: unget the
        // mismatched byte and now drain the bytes that occur nowhere else in
        // the search buffer (thanks to the KMP algorithm).
        //
        unget_byte( byte, fin );
        kmp = kmps[ buf_pos ];
        buf_drain = buf_pos - kmp;
        GOTO_STATE( S_NOT_MATCHED );

      case S_MATCHED:
      case S_NOT_MATCHED:
        //
        // Drain the match buffer returning each byte to the caller along with
        // whether it matched.
        //
        if ( buf_pos == buf_drain ) {
          //
          // We've drained all the bytes: if kmp != 0, it means we already have
          // a partial match further along in the read bytes so we don't have
          // to re-read them and re-compare them since they will match; hence,
          // go to S_MATCHING_CONT.
          //
          buf_pos = kmp;
          state = buf_pos ? S_MATCHING_CONT : S_READING;
          continue;
        }
        *matches = state == S_MATCHED;
        RETURN( match_buf[ buf_pos++ ] );

      case S_DONE:
        return false;

#undef GOTO_STATE
#undef MAYBE_NO_CASE
#undef RETURN

    } // switch
  } // for
}

/**
 * Ungets the given byte to the given file.
 *
 * @param byte The byte to unget.
 * @param file The file to unget \a byte to.
 */
static void unget_byte( uint8_t byte, FILE *file ) {
  if ( ungetc( byte, file ) == EOF )
    PMESSAGE_EXIT( READ_ERROR,
      "\"%s\": unget byte failed: %s", fin_path, ERROR_STR
    );
  --total_bytes_read;
}

////////// extern functions ///////////////////////////////////////////////////

kmp_t* kmp_init( char const *pattern, size_t pattern_len ) {
  assert( pattern );

  // allocating +1 eliminates "past the end" checking
  kmp_t *const kmps = MALLOC( kmp_t, pattern_len + 1 );
  memset( kmps, 0, sizeof( kmp_t ) * pattern_len + 1 );

  for ( size_t i = 1, j = 0; i < pattern_len; ) {
    assert( j < pattern_len );
    if ( pattern[i] == pattern[j] )
      kmps[++i] = ++j;
    else if ( j > 0 )
      j = kmps[j-1];
    else
      kmps[++i] = 0;
  } // for
  return kmps;
}

size_t match_row( uint8_t *row_buf, uint16_t *match_bits, kmp_t const *kmps,
                  uint8_t *match_buf ) {
  assert( match_bits );
  *match_bits = 0;

  size_t buf_len;
  for ( buf_len = 0; buf_len < ROW_BUF_SIZE; ++buf_len ) {
    bool matches;
    if ( !match_byte( row_buf + buf_len, &matches, kmps, match_buf ) )
      break;
    if ( matches )
      *match_bits |= 1 << buf_len;
  } // for
  return buf_len;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
