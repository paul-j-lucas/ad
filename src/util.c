/*
**      ad -- ASCII dump
**      util.c
**
**      Copyright (C) 2015-2017  Paul J. Lucas
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
#include "config.h"
#define AD_UTIL_INLINE _GL_EXTERN_INLINE
#include "util.h"
#include "common.h"

// standard
#include <assert.h>
#include <ctype.h>                      /* for isspace(), isprint() */
#include <fcntl.h>                      /* for open() */
#include <stdlib.h>                     /* for exit(), strtoull(), ... */
#include <string.h>
#include <sys/stat.h>                   /* for fstat() */
#include <unistd.h>                     /* for lseek() */
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * A node for a singly linked list of pointers to memory to be freed via
 * \c atexit().
 */
struct free_node {
  void *ptr;
  struct free_node *next;
};
typedef struct free_node free_node_t;

// extern variable declarations
extern char const  *me;

// local variable definitions
static free_node_t *free_head;          // linked list of stuff to free

/////////// inline functions //////////////////////////////////////////////////

/**
 * Swaps the endianness of the given 16-bit value.
 *
 * @param n The 16-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
static inline uint16_t swap_16( uint16_t n ) {
  return  (n >> 8)
        | (n << 8);
}

/**
 * Swaps the endianness of the given 32-bit value.
 *
 * @param n The 32-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
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

////////// local functions ////////////////////////////////////////////////////

/**
 * Skips leading whitespace, if any.
 *
 * @param s The NULL-terminated string to skip whitespace for.
 * @return Returns a pointer within \a s pointing to the first non-whitespace
 * character or pointing to the NULL byte if either \a s was all whitespace or
 * empty.
 */
static char const* skip_ws( char const *s ) {
  assert( s != NULL );
  while ( isspace( *s ) )
    ++s;
  return s;
}

////////// extern funtions ////////////////////////////////////////////////////

bool ascii_any_printable( char const *s, size_t s_len ) {
  assert( s != NULL );
  for ( ; s_len; --s_len, ++s )
    if ( ascii_is_print( *s ) )
      return true;
  return false;
}

FILE* check_fopen( char const *path, char const *mode, off_t offset ) {
  assert( path != NULL );
  assert( mode != NULL );

  FILE *const file = fopen( path, mode );
  if ( !file ) {
    bool const create = strpbrk( mode, "aw" );
    PMESSAGE_EXIT( create ? EX_CANTCREAT : EX_NOINPUT,
      "\"%s\": can not open: %s\n", path, STRERROR
    );
  }
  if ( offset )
    FSEEK( file, offset, SEEK_SET );
  return file;
}

int check_open( char const *path, int oflag, off_t offset ) {
  assert( path != NULL );
  bool const create = oflag & O_CREAT;
  int const fd = create ? open( path, oflag, 0644 ) : open( path, oflag );
  if ( fd == -1 )
    PMESSAGE_EXIT( create ? EX_CANTCREAT : EX_NOINPUT,
      "\"%s\": can not open: %s\n", path, STRERROR
    );
  if ( offset )
    LSEEK( fd, offset, SEEK_SET );
  return fd;
}

void* check_realloc( void *p, size_t size ) {
  //
  // Autoconf, 5.5.1:
  //
  // realloc
  //    The C standard says a call realloc(NULL, size) is equivalent to
  //    malloc(size), but some old systems don't support this (e.g., NextStep).
  //
  if ( !size )
    size = 1;
  void *const r = p ? realloc( p, size ) : malloc( size );
  if ( unlikely( !r ) )
    perror_exit( EX_OSERR );
  return r;
}

char* check_strdup( char const *s ) {
  assert( s != NULL );
  char *const dup = strdup( s );
  if ( unlikely( !dup ) )
    perror_exit( EX_OSERR );
  return dup;
}

#ifndef HAVE_FGETLN
char* fgetln( FILE *f, size_t *len ) {
  static char *buf;
  static size_t cap;
  ssize_t const temp_len = getline( &buf, &cap, f );
  if ( unlikely( temp_len == -1 ) )
    return NULL;
  *len = (size_t)temp_len;
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
  for ( free_node_t *p = free_head; p; ) {
    free_node_t *const next = p->next;
    free( p->ptr );
    free( p );
    p = next;
  } // for
  free_head = NULL;
}

void fskip( size_t bytes_to_skip, FILE *file ) {
  assert( file != NULL );

  char    buf[ 8192 ];
  size_t  bytes_to_read = sizeof buf;

  while ( bytes_to_skip && !feof( file ) ) {
    if ( bytes_to_read > bytes_to_skip )
      bytes_to_read = bytes_to_skip;
    ssize_t const bytes_read = fread( buf, 1, bytes_to_read, file );
    if ( unlikely( ferror( file ) ) )
      PMESSAGE_EXIT( EX_IOERR, "can not read: %s\n", STRERROR );
    bytes_to_skip -= bytes_read;
  } // while
}

char* identify( char const *s ) {
  assert( s != NULL );
  assert( *s );

  size_t const s_len = strlen( s );
  char *const ident = MALLOC( char, s_len + 1 /*NULL*/ + 1 /* leading _ */ );
  char *p = ident;

  // first char is a special case: must be alpha or _
  bool substitute_ = !(isalpha( *s ) || *s == '_');
  *p++ = substitute_ ? '_' : *s++;

  // remaining chars must be alphanum or _
  for ( ; *s; ++s ) {
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

size_t int_len( uint64_t n ) {
  if ( n <= 0xFFFFFFFFu )
    return n <= 0xFFFFu ? (n <= 0xFFu ? 1 : 2) : (n <= 0xFFFFFFu ? 3 : 4);
  return n <= 0xFFFFFFFFFFFFul ?
    (n <= 0xFFFFFFFFFFul ? 5 : 6) : (n <= 0xFFFFFFFFFFFFFFul ? 7 : 8);
}

void int_rearrange_bytes( uint64_t *n, size_t bytes, endian_t endian ) {
  assert( n != NULL );
  assert( bytes <= 8 );

  switch ( endian ) {
#ifdef WORDS_BIGENDIAN

    case ENDIAN_LITTLE:
      switch ( bytes ) {
        case 1: /* do nothing */    break;
        case 2: *n = swap_16( *n ); break;
        case 3: *n <<= 8;        // FALLTHROUGH
        case 4: *n = swap_32( *n ); break;
        case 5: *n <<= 8;        // FALLTHROUGH
        case 6: *n <<= 8;        // FALLTHROUGH
        case 7: *n <<= 8;        // FALLTHROUGH
        case 8: *n = swap_64( *n ); break;
      } // switch
      // FALLTHROUGH

    case ENDIAN_BIG:
      // move bytes to start of buffer
      *n <<= (sizeof( uint64_t ) - bytes) * 8;
      break;

#else /* machine words are little endian */

    case ENDIAN_BIG:
      switch ( bytes ) {
        case 1: /* do nothing */               break;
        case 2: *n = swap_16( *n );            break;
        case 3: *n = swap_32( *n ); *n >>=  8; break;
        case 4: *n = swap_32( *n );            break;
        case 5: *n = swap_64( *n ); *n >>= 24; break;
        case 6: *n = swap_64( *n ); *n >>= 16; break;
        case 7: *n = swap_64( *n ); *n >>=  8; break;
        case 8: *n = swap_64( *n );            break;
      } // switch
      break;

    case ENDIAN_LITTLE:
      // do nothing
      break;

#endif /* WORDS_BIGENDIAN */

    default:
      assert( true );
  } // switch
}

bool is_file( int fd ) {
  struct stat fd_stat;
  FSTAT( fd, &fd_stat );
  return S_ISREG( fd_stat.st_mode );
}

uint64_t parse_offset( char const *s ) {
  s = skip_ws( s );
  if ( !*s || *s == '-' )               // strtoull(3) wrongly allows '-'
    goto error;

  { // local scope
    char *end = NULL;
    errno = 0;
    uint64_t n = strtoull( s, &end, 0 );
    if ( unlikely( errno || end == s ) )
      goto error;
    if ( end[0] ) {                     // possibly 'b', 'k', or 'm'
      if ( end[1] )                     // not a single char
        goto error;
      switch ( end[0] ) {
        case 'b': n *=         512; break;
        case 'k': n *=        1024; break;
        case 'm': n *= 1024 * 1024; break;
        default : goto error;
      } // switch
    }
    return n;
  } // local scope

error:
  PMESSAGE_EXIT( EX_USAGE, "\"%s\": invalid offset\n", s );
}

bool parse_sgr( char const *sgr_color ) {
  if ( !sgr_color )
    return false;
  for ( ;; ) {
    if ( !isdigit( *sgr_color ) )
      return false;
    char *end;
    errno = 0;
    uint64_t const n = strtoull( sgr_color, &end, 10 );
    if ( errno || n > 255 )
      return false;
    switch ( *end ) {
      case '\0':
        return true;
      case ';':
        sgr_color = end + 1;
        continue;
      default:
        return false;
    } // switch
  } // for
}

uint64_t parse_ull( char const *s ) {
  s = skip_ws( s );
  if ( *s && *s != '-' ) {              // strtoull(3) wrongly allows '-'
    char *end = NULL;
    errno = 0;
    uint64_t const n = strtoull( s, &end, 0 );
    if ( likely( !errno && !*end ) )
      return n;
  }
  PMESSAGE_EXIT( EX_USAGE, "\"%s\": invalid integer\n", s );
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
  if ( ascii_is_print( c ) )
    buf[0] = c, buf[1] = '\0';
  else
    snprintf( buf, sizeof buf, "\\x%02X", (unsigned)c );
  return buf;
}

char* tolower_s( char *s ) {
  assert( s != NULL );
  for ( char *t = s; *t; ++t )
    *t = tolower( *t );
  return s;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
