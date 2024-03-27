/*
**      ad -- ASCII dump
**      src/match.c
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
#include "ad.h"
#include "match.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>                      /* for tolower() */
#include <stdlib.h>                     /* for exit() */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

// extern variable definitions
unsigned long       total_matches;

// local variable definitions
static size_t       total_bytes_read;

// local functions
static void         unget_byte( char8_t );

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets a byte.
 *
 * @param pbyte A pointer to the byte to receive the newly read byte.
 * @return Returns \c true if a byte was read successfully.
 */
NODISCARD
static bool get_byte( char8_t *pbyte ) {
  if ( likely( total_bytes_read < opt_max_bytes ) ) {
    int const c = getchar();
    if ( likely( c != EOF ) ) {
      ++total_bytes_read;
      assert( pbyte != NULL );
      *pbyte = STATIC_CAST(char8_t, c);
      return true;
    }
    if ( unlikely( ferror( stdin ) ) ) {
      fatal_error( EX_IOERR,
        "\"%s\": read byte failed: %s\n", fin_path, STRERROR()
      );
    }
  }
  return false;
}

/**
 * Checks whether \a input_byte is either:
 *  + The byte at \a pos of \ref opt_search_buf (if \ref opt_strings is
 *    `false`); or:
 *  + A printable character (if \ref opt_strings is `true`).
 *
 * @param input_byte The byte read from the input source.
 * @param pos The positition within \ref opt_search_buf to match against, but
 * only if \ref opt_strings is `false`.
 * @return Returns `true` only if \a input_byte matches.
 */
NODISCARD
static bool is_match( char8_t input_byte, size_t pos ) {
  if ( opt_strings ) {
    switch ( input_byte ) {
      case '\f': return (opt_strings_opts & STRINGS_OPT_FORMFEED) != 0;
      case '\n': return (opt_strings_opts & STRINGS_OPT_NEWLINE ) != 0;
      case '\r': return (opt_strings_opts & STRINGS_OPT_RETURN  ) != 0;
      case ' ' : return (opt_strings_opts & STRINGS_OPT_SPACE   ) != 0;
      case '\t': return (opt_strings_opts & STRINGS_OPT_TAB     ) != 0;
      case '\v': return (opt_strings_opts & STRINGS_OPT_VTAB    ) != 0;
      default  : return ascii_is_graph( input_byte );
    } // switch
  }

  if ( opt_case_insensitive )
    input_byte = STATIC_CAST( char8_t, tolower( input_byte ) );

  return input_byte == STATIC_CAST( char8_t, opt_search_buf[ pos ] );
}

/**
 * Gets a byte and whether it matches one of the bytes in the search buffer.
 *
 * @param pbyte A pointer to receive the byte.
 * @param matches A pointer to receive whether the byte matches.
 * @param kmps A pointer to the array of KMP values to use.
 * @param pmatch_buf A pointer to a pointer to a buffer to use while matching.
 * It must be at least as large as the search buffer.
 * @param pmatch_len A pointer to the size of \a *pmatch_buf or NULL.
 * @return Returns \c true if a byte was read successfully.
 */
NODISCARD
static bool match_byte( char8_t *pbyte, bool *matches, kmp_t const *kmps,
                        char8_t **pmatch_buf, size_t *pmatch_len ) {
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

  char8_t byte;

  assert( pbyte != NULL );
  assert( matches != NULL );
  assert( state != S_DONE );

  *matches = false;

  for (;;) {
    switch ( state ) {

#define GOTO_STATE(POS,S)         { buf_pos = (POS); state = (S); continue; }
#define RETURN(BYTE)              BLOCK( *pbyte = (BYTE); return true; )

      case S_READING:
        if ( unlikely( !get_byte( &byte ) ) )
          GOTO_STATE( 0, S_DONE );
        if ( opt_search_len == 0 )      // user isn't searching for anything
          RETURN( byte );
        if ( !is_match( byte, 0 ) )
          RETURN( byte );               // searching, but no match yet
        //
        // For non-strings(1) searches, the read byte matches the first byte of
        // opt_search_buf: start storing bytes in the match buffer and try to
        // match the rest of opt_search_buf;
        //
        // For strings(1) searches, the read byte is the start of a string:
        // start storing bytes in the match buffer and try to match at least
        // opt_search_len total string characters.
        //
        // While matching, we can't return to the caller since we don't know
        // whether the current byte sequence will either fully match
        // opt_search_buf (for non-strings(1) searches) or will match at least
        // opt_search_len total string characters possibly null terminated (for
        // strings(1) searches).
        //
        (*pmatch_buf)[0] = byte;
        kmp = 0;
        GOTO_STATE( 0, S_MATCHING );

      case S_MATCHING:
        ++buf_pos;
        if ( !opt_strings && buf_pos == opt_search_len ) {
          //
          // For non-strings(1) matches, we've reached the end of the search
          // buffer, hence the current sequence of bytes fully matches: we can
          // now drain the match buffer and return the bytes individually to
          // the caller denoting that all matched.
          //
          ++total_matches;
          buf_drain = buf_pos;
          GOTO_STATE( 0, S_MATCHED );
        }
        FALLTHROUGH;
      case S_MATCHING_CONT:
        if ( unlikely( !get_byte( &byte ) ) ) {
          //
          // We've reached EOF and there weren't enough bytes to match the
          // search buffer: just drain the match buffer and return the bytes
          // individually to the caller denoting that none matched.
          //
          buf_drain = buf_pos;
          GOTO_STATE( 0, S_NOT_MATCHED );
        }
        if ( is_match( byte, buf_pos ) ) {
          //
          // The next byte matched: keep storing bytes in the match buffer and
          // keep matching.
          //
          if ( buf_pos == *pmatch_len ) {
            *pmatch_len <<= 1;
            REALLOC( *pmatch_buf, *pmatch_len );
          }
          (*pmatch_buf)[ buf_pos ] = byte;
          GOTO_STATE( buf_pos, S_MATCHING ); // in case we were S_MATCHING_CONT
        }

        //
        // The read byte mismatches a byte in the search buffer: unget the
        // mismatched byte and now drain the bytes that occur nowhere else in
        // the search buffer (thanks to the KMP algorithm).
        //
        unget_byte( byte );
        if ( kmps != NULL )
          kmp = kmps[ buf_pos ];
        buf_drain = buf_pos - kmp;

        //
        // However, for strings(1) matches, if we've matched at least
        // opt_search_len bytes and either:
        //
        //  + We don't require a terminating null byte; or:
        //  + We require a terminating null byte and the byte is a null byte,
        //
        // then we've matched a string.
        //
        if ( opt_strings && buf_pos >= opt_search_len &&
             ((opt_strings_opts & STRINGS_OPT_NULL) == 0 || byte == '\0') ) {
          ++total_matches;
          GOTO_STATE( 0, S_MATCHED );
        }

        GOTO_STATE( 0, S_NOT_MATCHED );

      case S_MATCHED:
      case S_NOT_MATCHED:
        //
        // Drain the match buffer returning each byte to the caller along with
        // whether it matched.
        //
        if ( buf_pos == buf_drain ) {
          //
          // We've drained all the bytes: if kmp > 0, it means we already have
          // a partial match further along in the read bytes so we don't have
          // to re-read them and re-compare them since they will match; hence,
          // go to S_MATCHING_CONT.
          //
          GOTO_STATE( kmp, kmp > 0 ? S_MATCHING_CONT : S_READING );
        }
        *matches = state == S_MATCHED;
        RETURN( (*pmatch_buf)[ buf_pos++ ] );

      case S_DONE:
        return false;

#undef GOTO_STATE
#undef RETURN

    } // switch
  } // for
}

/**
 * Ungets the given byte.
 *
 * @param byte The byte to unget.
 */
static void unget_byte( char8_t byte ) {
  if ( unlikely( ungetc( byte, stdin ) == EOF ) ) {
    fatal_error( EX_IOERR,
      "\"%s\": unget byte failed: %s\n", fin_path, STRERROR()
    );
  }
  --total_bytes_read;
}

////////// extern functions ///////////////////////////////////////////////////

kmp_t* kmp_init( char const *pattern, size_t pattern_len ) {
  assert( pattern != NULL );

  // allocating +1 eliminates "past the end" checking
  kmp_t *const kmps = MALLOC( kmp_t, pattern_len + 1 );
  memset( kmps, 0, sizeof( kmp_t ) * (pattern_len + 1) );

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

size_t match_row( char8_t *row_buf, size_t row_len, match_bits_t *match_bits,
                  kmp_t const *kmps, char8_t **pmatch_buf,
                  size_t *pmatch_len ) {
  assert( row_buf != NULL );
  assert( row_len <= row_bytes );
  assert( match_bits != NULL );
  *match_bits = 0;

  size_t buf_len;
  for ( buf_len = 0; buf_len < row_len; ++buf_len ) {
    bool matches;
    if ( !match_byte( row_buf + buf_len, &matches, kmps,
                      pmatch_buf, pmatch_len ) ) {
      break;
    }
    if ( matches )
      *match_bits |= 1u << buf_len;
  } // for
  return buf_len;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
