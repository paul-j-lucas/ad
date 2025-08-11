/*
**      ad -- ASCII dump
**      src/util.c
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

/**
 * @file
 * Defines utility functions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#define AD_UTIL_H_INLINE _GL_EXTERN_INLINE
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>                      /* for isalnum(), isalpha() */
#include <fcntl.h>                      /* for open() */
#include <stdarg.h>
#include <stdlib.h>                     /* for exit(), strtoull(), ... */
#include <string.h>
#include <sys/stat.h>                   /* for fstat() */
#include <sysexits.h>
#include <unistd.h>                     /* for lseek() */

/// @endcond

/**
 * @addtogroup util-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * A node for a singly linked list of pointers to memory to be freed via
 * `atexit()`.
 */
struct free_node {
  void *ptr;
  struct free_node *next;
};
typedef struct free_node free_node_t;

// local variable definitions
static free_node_t *free_head;          ///< List of stuff to free later.

/////////// inline functions //////////////////////////////////////////////////

/**
 * Swaps the endianness of the given 16-bit value.
 *
 * @param n The 16-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
NODISCARD
static inline uint16_t swap_16( uint16_t n ) {
  return (uint16_t)((n >> 8)
                  | (n << 8));
}

/**
 * Swaps the endianness of the given 32-bit value.
 *
 * @param n The 32-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
NODISCARD
static inline uint32_t swap_32( uint32_t n ) {
  return  ( n                >> 24)
        | ((n & 0x00FF0000u) >>  8)
        | ((n & 0x0000FF00u) <<  8)
        | ( n                << 24);
}

/**
 * Swaps the endianness of the given 64-bit value.
 *
 * @param n The 64-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
NODISCARD
static inline uint64_t swap_64( uint64_t n ) {
  return  ( n                          >> 56)
        | ((n & 0x00FF000000000000ull) >> 40)
        | ((n & 0x0000FF0000000000ull) >> 24)
        | ((n & 0x000000FF00000000ull) >>  8)
        | ((n & 0x00000000FF000000ull) <<  8)
        | ((n & 0x0000000000FF0000ull) << 24)
        | ((n & 0x000000000000FF00ull) << 40)
        | ( n                          << 56);
}

////////// extern funtions ////////////////////////////////////////////////////

bool ascii_any_printable( char const *s, size_t s_len ) {
  assert( s != NULL );
  for ( ; s_len > 0; --s_len, ++s ) {
    if ( ascii_is_print( *s ) )
      return true;
  } // for
  return false;
}

char const* base_name( char const *path_name ) {
  assert( path_name != NULL );
  char const *const slash = strrchr( path_name, '/' );
  if ( slash != NULL )
    return slash[1] != '\0' ? slash + 1 : slash;
  return path_name;
}

void* check_realloc( void *p, size_t size ) {
  assert( size > 0 );
  p = p != NULL ? realloc( p, size ) : malloc( size );
  PERROR_EXIT_IF( p == NULL, EX_OSERR );
  return p;
}

char* check_strdup( char const *s ) {
  assert( s != NULL );
  char *const dup = strdup( s );
  if ( unlikely( dup == NULL ) )
    perror_exit( EX_OSERR );
  return dup;
}

void fatal_error( int status, char const *format, ... ) {
  EPRINTF( "%s: ", me );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  _Exit( status );
}

bool fd_is_file( int fd ) {
  struct stat fd_stat;
  FSTAT( fd, &fd_stat );
  return S_ISREG( fd_stat.st_mode );
}

#ifndef HAVE_FGETLN
char* fgetln( FILE *fin, size_t *len ) {
  assert( fin != NULL );
  assert( len != NULL );

  static char *buf;
  static size_t cap;
  ssize_t const temp_len = getline( &buf, &cap, fin );
  if ( unlikely( temp_len == -1 ) )
    return NULL;
  *len = STATIC_CAST( size_t, temp_len );
  return buf;
}
#endif /* HAVE_FGETLN */

void* free_later( void *p ) {
  assert( p != NULL );
  free_node_t *const new_node = MALLOC( free_node_t, 1 );
  new_node->ptr = p;
  new_node->next = free_head;
  free_head = new_node;
  return p;
}

void free_now( void ) {
  for ( free_node_t *p = free_head; p != NULL; ) {
    free_node_t *const next = p->next;
    free( p->ptr );
    free( p );
    p = next;
  } // for
  free_head = NULL;
}

void fskip( off_t bytes_to_skip, FILE *file ) {
  assert( bytes_to_skip >= 0 );
  assert( file != NULL );

  if ( bytes_to_skip == 0 )
    return;

  if ( fd_is_file( fileno( file ) ) ) {
    if ( FSEEK_FN( file, bytes_to_skip, SEEK_CUR ) == 0 )
      return;
    clearerr( file );                   // fall back to reading bytes
  }

  char    buf[ 8192 ];
  size_t  bytes_to_read = sizeof buf;

  while ( bytes_to_skip > 0 && !feof( file ) ) {
    if ( bytes_to_read > STATIC_CAST( size_t, bytes_to_skip ) )
      bytes_to_read = STATIC_CAST( size_t, bytes_to_skip );
    size_t const bytes_read = fread( buf, 1, bytes_to_read, file );
    if ( unlikely( ferror( file ) != 0 ) )
      fatal_error( EX_IOERR, "can not read: %s\n", STRERROR() );
    bytes_to_skip -= STATIC_CAST( off_t, bytes_read );
  } // while
}

char* identify( char const *s ) {
  assert( s != NULL );
  assert( s[0] != '\0' );

  size_t const s_len = strlen( s );
  char *const ident = MALLOC( char, s_len + 1 /*NULL*/ + 1 /* leading _ */ );
  char *p = ident;

  // first char is a special case: must be alpha or _
  bool substitute_ = !(isalpha( *s ) || *s == '_');
  *p++ = substitute_ ? '_' : *s++;

  // remaining chars must be alphanum or _
  for ( ; *s != '\0'; ++s ) {
    if ( isalnum( *s ) || *s == '_' ) {
      *p++ = *s;
      substitute_ = false;
    } else if ( !substitute_ ) {        // don't create __ (double underscore)
      *p++ = '_';
      substitute_ = true;
    }
  } // for

  *p = '\0';
  return ident;
}

void int_rearrange_bytes( uint64_t *n, size_t bytes, endian_t endian ) {
  assert( n != NULL );
  assert( bytes >= 1 && bytes <= 8 );

  switch ( endian ) {
    case ENDIAN_NONE:
      break;
#ifdef WORDS_BIGENDIAN

    case ENDIAN_LITTLE:
      switch ( bytes ) {
        case 1: /* do nothing */              break;
        case 2: *n = swap_16( (uint16_t)*n ); break;
        case 3: *n <<= 8;                     FALLTHROUGH;
        case 4: *n = swap_32( (uint32_t)*n ); break;
        case 5: *n <<= 8;                     FALLTHROUGH;
        case 6: *n <<= 8;                     FALLTHROUGH;
        case 7: *n <<= 8;                     FALLTHROUGH;
        case 8: *n = swap_64( *n );           break;
      } // switch
      FALLTHROUGH;
    case ENDIAN_BIG:
    case ENDIAN_HOST:
      // move bytes to start of buffer
      *n <<= (sizeof( uint64_t ) - bytes) * 8;
      return;

#else /* machine words are little endian */

    case ENDIAN_BIG:
      switch ( bytes ) {
        case 1: /* do nothing */                          break;
        case 2: *n = swap_16( (uint16_t)*n );             break;
        case 3: *n = swap_32( (uint32_t)*n ); *n >>=  8;  break;
        case 4: *n = swap_32( (uint32_t)*n );             break;
        case 5: *n = swap_64( *n );           *n >>= 24;  break;
        case 6: *n = swap_64( *n );           *n >>= 16;  break;
        case 7: *n = swap_64( *n );           *n >>=  8;  break;
        case 8: *n = swap_64( *n );                       break;
      } // switch
      FALLTHROUGH;
    case ENDIAN_LITTLE:
    case ENDIAN_HOST:
      // do nothing
      return;

#endif /* WORDS_BIGENDIAN */
  } // switch
  UNEXPECTED_INT_VALUE( endian );
}

unsigned long long parse_ull( char const *s ) {
  SKIP_WS( s );
  if ( likely( s[0] != '\0' || s[0] != '-' ) ) {
    char *end = NULL;
    errno = 0;
    unsigned long long const n = strtoull( s, &end, 0 );
    if ( likely( errno == 0 && *end == '\0' ) )
      return n;
  }
  fatal_error( EX_USAGE, "\"%s\": invalid integer\n", s );
}

void perror_exit( int status ) {
  perror( me );
  exit( status );
}

char const* printable_char( char c ) {
  switch( c ) {
    case '\0': return "\\0";
    case '\a': return "\\a";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    case '\v': return "\\v";
  } // switch

  static char buf[5];                   // \xHH + NULL
  if ( ascii_is_print( c ) ) {
    buf[0] = c; buf[1] = '\0';
  } else {
    snprintf( buf, sizeof buf, "\\x%02X", STATIC_CAST(unsigned char, c) );
  }
  return buf;
}

char* tolower_s( char *s ) {
  assert( s != NULL );
  for ( char *t = s; *t != '\0'; ++t )
    *t = STATIC_CAST( char, tolower( *t ) );
  return s;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
