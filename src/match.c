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

/**
 * @file
 * Defined functions for matching numbers or strings.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "match.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>                      /* for tolower() */
#include <stdint.h>                     /* for SIZE_MAX */
#include <stdlib.h>                     /* for exit() */
#include <sysexits.h>

/// @endcond

/**
 * @addtogroup matching-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// extern variable definitions
unsigned long       total_matches;

/// @endcond

// local variable definitions
static size_t       total_bytes_read;   ///< Total bytes read.

// local functions
static void         unget_byte( char8_t );

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets a byte.
 *
 * @param pbyte A pointer to the byte to receive the newly read byte.
 * @return Returns `true` only if a byte was read successfully.
 */
NODISCARD
static bool get_byte( char8_t *pbyte ) {
  assert( pbyte != NULL );

  if ( unlikely( total_bytes_read == opt_max_bytes ) )
    return false;

  int const c = getchar();
  if ( unlikely( c == EOF ) ) {
    if ( unlikely( ferror( stdin ) ) ) {
      fatal_error( EX_IOERR,
        "\"%s\": read byte failed: %s\n", fin_path, STRERROR()
      );
    }
    return false;
  }

  ++total_bytes_read;
  *pbyte = STATIC_CAST( char8_t, c );
  return true;
}

/**
 * Checks whether \a input_byte is either:
 *  + The byte at \a buf_pos of \ref opt_search_buf (if \ref opt_strings is
 *    `false`); or:
 *  + A printable character (if \ref opt_strings is `true`).
 *
 * @param input_byte The byte read from the input source.
 * @param buf_pos The position within \ref opt_search_buf to match against, but
 * only if \ref opt_strings is `false`.
 * @param must_be_utf8_cont
 * @parblock
 *  + If `true`, \a input_byte _must_ be a UTF-8 continuation byte;
 *  + If `false`, it _must_ be a UTF-8 start byte.
 *
 * Used only if \ref opt_utf8 is `true`.
 * @endparblock
 * @return Returns `true` only if \a input_byte matches.
 */
NODISCARD
static bool is_match( char8_t input_byte, size_t buf_pos,
                      bool must_be_utf8_cont ) {
  if ( opt_strings ) {
    switch ( input_byte ) {
      case '\f': return (opt_strings_opts & STRINGS_FORMFEED) != 0;
      case '\n': return (opt_strings_opts & STRINGS_LINEFEED) != 0;
      case '\r': return (opt_strings_opts & STRINGS_RETURN  ) != 0;
      case ' ' : return (opt_strings_opts & STRINGS_SPACE   ) != 0;
      case '\t': return (opt_strings_opts & STRINGS_TAB     ) != 0;
      case '\v': return (opt_strings_opts & STRINGS_VTAB    ) != 0;
      default  :
        if ( opt_utf8 ) {
          return must_be_utf8_cont ?
            utf8_is_cont( input_byte ) : utf8_is_start( input_byte );
        }
        return ascii_is_graph( input_byte );
    } // switch
  }

  if ( opt_ignore_case )
    input_byte = STATIC_CAST( char8_t, tolower( input_byte ) );

  return input_byte == STATIC_CAST( char8_t, opt_search_buf[ buf_pos ] );
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
 * @return Returns `true` if a byte was read successfully.
 */
NODISCARD
static bool match_byte( char8_t *pbyte, bool *matches, size_t const *kmps,
                        char8_t **pmatch_buf, size_t *pmatch_len ) {
  enum state {
    S_READING,                          // just reading; not matching
    S_MATCHING,                         // matching search bytes
    S_MATCHING_CONTINUE,                // matching after a mismatch
    S_MATCHED,                          // a complete match
    S_NOT_MATCHED,                      // didn't match after all
    S_DONE                              // no more input
  };
  typedef enum state state_t;

  static size_t   buf_drain;            // bytes to "drain" buf after mismatch
  static size_t   buf_matched;          // bytes in buffer matched
  static size_t   buf_pos;              // position in *pmatch_buf
  static size_t   kmp;                  // bytes partially matched
  static state_t  state = S_READING;    // current state
  static size_t   string_chars_matched; // strings(1) characters matched
  static unsigned utf8_char_bytes;      // bytes comprising UTF-8 character
  static unsigned utf8_char_bytes_left; // bytes left to match UTF-8 character

  assert( pbyte != NULL );
  assert( matches != NULL );
  assert( state != S_DONE );

  *matches = false;

  for (;;) {
    switch ( state ) {

/// @cond DOXYGEN_IGNORE
#define GOTO_STATE(S)             { state = (S); continue; }
/// @endcond

      case S_READING:
        if ( unlikely( !get_byte( pbyte ) ) )
          GOTO_STATE( S_DONE );
        if ( opt_search_len == 0 )      // user isn't searching for anything
          return true;
        if ( !is_match( *pbyte, /*buf_pos=*/0, /*must_be_utf8_cont=*/false ) )
          return true;                  // searching, but no match yet
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
        (*pmatch_buf)[0] = *pbyte;
        buf_matched = SIZE_MAX;         // assume all bytes read matched
        buf_pos = 0;
        kmp = 0;
        string_chars_matched = utf8_char_bytes = utf8_char_bytes_left = 0;
        GOTO_STATE( S_MATCHING );

      case S_MATCHING:
        ++buf_pos;
        if ( opt_strings ) {
          if ( utf8_char_bytes_left == 0 ) {
            //
            // We've matched all the bytes comprising a UTF-8 character: bump
            // the number of characers matched and reset for the next UTF-8
            // character, if any.
            //
            // We don't need to check opt_utf8 since this code also works for
            // ASCII since ASCII is a subset of UTF-8.
            //
            ++string_chars_matched;
            utf8_char_bytes = utf8_char_bytes_left = utf8_char_len( *pbyte );
          }
        }
        else if ( buf_pos == opt_search_len ) {
          //
          // For non-strings(1) searches, we've reached the end of the search
          // buffer, hence the current sequence of bytes fully matches: we can
          // now drain the match buffer and return the bytes individually to
          // the caller denoting that all matched.
          //
          ++total_matches;
          buf_drain = buf_pos;
          buf_pos = 0;
          GOTO_STATE( S_MATCHED );
        }
        FALLTHROUGH;

      case S_MATCHING_CONTINUE:
        if ( unlikely( !get_byte( pbyte ) ) ) {
          //
          // We've reached either EOF or the maximum number of bytes to read.
          //
          // + For non-strings(1) searches, there weren't enough bytes to match
          //   the search buffer.
          //
          // + For strings(1) searches, we may have already read opt_search_len
          //   characters, i.e., one last match.
          //
          // In either case, drain the match buffer and return the bytes
          // individually to the caller.
          //
          buf_drain = buf_pos;
        }
        else if ( is_match( *pbyte, buf_pos, --utf8_char_bytes_left > 0 ) ) {
          //
          // The next byte matched: keep storing bytes in the match buffer and
          // keep matching.
          //
          if ( buf_pos == *pmatch_len ) {
            *pmatch_len <<= 1;
            REALLOC( *pmatch_buf, *pmatch_len );
          }
          (*pmatch_buf)[ buf_pos ] = *pbyte;
          GOTO_STATE( S_MATCHING );     // in case we were S_MATCHING_CONTINUE
        }
        else {
          //
          // The read byte mismatches a byte in the search buffer: unget the
          // mismatched byte and now drain the bytes that occur nowhere else in
          // the search buffer (thanks to the KMP algorithm).
          //
          unget_byte( *pbyte );
          if ( kmps != NULL )
            kmp = kmps[ buf_pos ];
          buf_drain = buf_pos - kmp;
        }

        if ( opt_strings ) {
          if ( opt_utf8 ) {
            //
            // When strings(1) searching UTF-8 characters, it's possible to
            // encounter either an invalid byte or EOF while reading the bytes
            // comprising the character.
            //
            // For example, if we read the bytes E2 96 31, the E2 says we're to
            // expect 3 bytes comprising the character so the next 2 bytes must
            // be "continuation bytes" in the range 80-BF, hence 31 is invalid.
            //
            // In such a case, we've already "ungot" the invalid byte (31), but
            // what to do about E2 and 96?  Since only 1 byte of push-back is
            // guaranteed, we shouldn't rely on pushing them back.  Instead,
            // we'll "drain" them just like the valid bytes through the byte
            // before the E2, but just set *is_match to false for E2 and 96.
            // To do that, we set buf_matched.
            //
            buf_matched = buf_pos - utf8_char_bytes + utf8_char_bytes_left - 1;
          }

          //
          // For strings(1) searches, if we've matched at least opt_search_len
          // characters and either:
          //
          //  + We don't require a terminating null byte; or:
          //  + We require a terminating null byte and the byte is a null byte,
          //
          // then we've matched a string.
          //
          if ( string_chars_matched >= opt_search_len &&
              ((opt_strings_opts & STRINGS_NULL) == 0 || *pbyte == '\0') ) {
            ++total_matches;
            buf_pos = 0;
            GOTO_STATE( S_MATCHED );
          }
        }

        buf_pos = 0;
        GOTO_STATE( S_NOT_MATCHED );

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
          // go to S_MATCHING_CONTINUE.
          //
          buf_pos = kmp;
          GOTO_STATE( kmp > 0 ? S_MATCHING_CONTINUE : S_READING );
        }
        *matches = state == S_MATCHED && buf_pos <= buf_matched;
        *pbyte = (*pmatch_buf)[ buf_pos++ ];
        return true;

      case S_DONE:
        return false;

#undef GOTO_STATE

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

size_t* kmp_init( char const *pattern, size_t pattern_len ) {
  assert( pattern != NULL );

  // allocating +1 eliminates "past the end" checking
  size_t *const kmps = MALLOC( size_t, pattern_len + 1 );
  memset( kmps, 0, sizeof( size_t ) * (pattern_len + 1) );

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
                  size_t const *kmps, char8_t **pmatch_buf,
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

/** @} */

/* vim:set et sw=2 ts=2: */
