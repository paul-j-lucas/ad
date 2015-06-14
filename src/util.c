/*
**      ad -- ASCII dump
**      util.c
**
**      Copyright (C) 1996-2015  Paul J. Lucas
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

/* local */
#include "common.h"
#include "config.h"
#include "util.h"

/* system */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>                     /* for exit(), strtoul(), ... */
#include <string.h>                     /* for str...() */
#include <sys/types.h>

/*****************************************************************************/

extern char const*  me;
extern char const*  path_name;

static size_t       total_bytes_read;

/* local functions */
static char const*  skip_ws( char const *s );

/*****************************************************************************/

/**
 * Flips the endianness of the given 16-bit value.
 *
 * @param n The 16-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
static inline uint16_t swap_16( uint16_t n ) {
  return  (n >> 8)
        | (n << 8);
}

/**
 * Flips the endianness of the given 32-bit value.
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

#if SIZEOF_UNSIGNED_LONG == 8
/**
 * Flips the endianness of the given 64-bit value.
 *
 * @param n The 64-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
static inline uint64_t swap_64( uint64_t n ) {
  return  ( n                         >> 56)
        | ((n & 0x00FF000000000000ul) >> 40)
        | ((n & 0x0000FF0000000000ul) >> 24)
        | ((n & 0x000000FF00000000ul) >>  8)
        | ((n & 0x00000000FF000000ul) <<  8)
        | ((n & 0x0000000000FF0000ul) << 24)
        | ((n & 0x000000000000FF00ul) << 40)
        | ( n                         << 56);
}
#endif /* SIZEOF_UNSIGNED_LONG */

/*****************************************************************************/

void* check_realloc( void *p, size_t size ) {
  void *r;
  /*
   * Autoconf, 5.5.1:
   *
   * realloc
   *    The C standard says a call realloc(NULL, size) is equivalent to
   *    malloc(size), but some old systems don't support this (e.g., NextStep).
   */
  if ( !size )
    size = 1;
  r = p ? realloc( p, size ) : malloc( size );
  if ( !r )
    PERROR_EXIT( OUT_OF_MEMORY );
  return r;
}

void* freelist_add( void *p, free_node_t **pphead ) {
  free_node_t *new_node;
  assert( pphead );
  new_node = check_realloc( NULL, sizeof( free_node_t ) );
  new_node->p = p;
  new_node->next = *pphead ? *pphead : NULL;
  *pphead = new_node;
  return p;
}

void freelist_free( free_node_t *phead ) {
  while ( phead ) {
    free_node_t *const next = phead->next;
    free( phead->p );
    free( phead );
    phead = next;
  } /* while */
}

void fskip( size_t bytes_to_skip, FILE *file ) {
  char    buf[ 8192 ];
  ssize_t bytes_read;
  size_t  bytes_to_read = sizeof( buf );

  while ( bytes_to_skip && !feof( file ) ) {
    if ( bytes_to_read > bytes_to_skip )
      bytes_to_read = bytes_to_skip;
    bytes_read = fread( buf, 1, bytes_to_read, file );
    if ( ferror( file ) )
      PMESSAGE_EXIT( READ_ERROR,
        "\"%s\": can not read: %s\n",
        path_name, ERROR_STR
      );
    bytes_to_skip -= bytes_read;
  } /* while */
}

bool get_byte( uint8_t *pbyte, size_t max_bytes_to_read, FILE *file ) {
  if ( total_bytes_read < max_bytes_to_read ) {
    int const c = getc( file );
    if ( c != EOF ) {
      ++total_bytes_read;
      assert( pbyte );
      *pbyte = (uint8_t)c;
      return true;
    }
    if ( ferror( file ) )
      PMESSAGE_EXIT( READ_ERROR,
        "\"%s\": read byte failed: %s", path_name, ERROR_STR
      );
  }
  return false;
}

/**
 * Skips leading whitespace, if any.
 *
 * @param s The NULL-terminated string to skip whitespace for.
 * @return Returns a pointer within \a s pointing to the first non-whitespace
 * character or pointing to the NULL byte if either \a was all whitespace or
 * empty.
 */
char const* skip_ws( char const *s ) {
  assert( s );
  while ( *s && isspace( *s ) )
    ++s;
  return s;
}

FILE* open_file( char const *path_name, off_t offset ) {
  FILE *file;
  assert( path_name );
  if ( (file = fopen( path_name, "r" )) == NULL )
    PMESSAGE_EXIT( READ_OPEN,
      "\"%s\": can not open: %s\n",
      path_name, ERROR_STR
    );
  if ( offset && fseek( file, offset, SEEK_SET ) == -1 )
    PMESSAGE_EXIT( SEEK_ERROR,
      "\"%s\": can not seek to offset %ld: %s\n",
      path_name, (long)offset, ERROR_STR
    );
  return file;
}

unsigned long parse_offset( char const *s ) {
  char *end = NULL;
  unsigned long n;
  assert( s );

  s = skip_ws( s );
  if ( !*s || *s == '-' )               /* strtoul(3) wrongly allows '-' */
    return false;

  errno = 0;
  n = strtoul( s, &end, 0 );
  if ( end == s || errno == ERANGE )
    goto error;
  if ( end[0] ) {                       /* possibly 'b', 'k', or 'm' */
    if ( end[1] )                       /* not a single char */
      goto error;
    switch ( end[0] ) {
      case 'b': n *=         512; break;
      case 'k': n *=        1024; break;
      case 'm': n *= 1024 * 1024; break;
      default : goto error;
    } /* switch */
  }
  return n;
error:
  PMESSAGE_EXIT( USAGE, "\"%s\": invalid offset\n", s );
}

unsigned long parse_ul( char const *s ) {
  unsigned long n;
  if ( !parse_ul_impl( s, &n ) )
    PMESSAGE_EXIT( USAGE, "\"%s\": invalid integer\n", s );
  return n;
}

bool parse_ul_impl( char const *s, unsigned long *n ) {
  unsigned long temp;
  char *end = NULL;

  s = skip_ws( s );
  if ( !*s || *s == '-' )               /* strtoul(3) wrongly allows '-' */
    return false;

  errno = 0;
  temp = strtoul( s, &end, 0 );
  if ( end == s || *end || errno == ERANGE )
    return false;

  assert( n );
  *n = temp;
  return true;
}

char* tolower_s( char *s ) {
  assert( s );
  for ( ; *s; ++s )
    *s = tolower( *s );
  return s;
}

size_t ulong_len( unsigned long n ) {
  if ( n < 0x10000 )
    return n < 0x100 ? 1 : 2;
#if SIZEOF_UNSIGNED_LONG == 8
    return n < 0x100000000L ? 4 : 8;
#else
    return 4;
#endif /* SIZEOF_UNSIGNED_LONG */
}

void ulong_rearrange_bytes( unsigned long *n, size_t bytes, endian_t endian ) {
#ifndef NDEBUG
  switch ( bytes ) {
    case 1:
    case 2:
    case 4:
#if SIZEOF_UNSIGNED_LONG == 8
    case 8:
#endif /* SIZEOF_UNSIGNED_LONG */
      break;
    default:
      assert( true );
  } /* switch */
#endif /* NDEBUG */

  switch ( endian ) {
#ifdef WORDS_BIGENDIAN

    case ENDIAN_BIG:
      /* move bytes to start of buffer */
      *n <<= (sizeof( unsigned long ) - bytes) * 8;
      break;

    case ENDIAN_LITTLE:
      switch ( bytes ) {
        case 1 : /* do nothing */     break;
        case 2 : *n = swap_16( *n );  break;
        case 4 : *n = swap_32( *n );  break;
#if SIZEOF_UNSIGNED_LONG == 8
        case 8 : *n = swap_64( *n );  break;
#endif /* SIZEOF_UNSIGNED_LONG */
      } /* switch */
      /* post-swap, bytes are at start of buffer */
      break;

#else /* machine words are little endian */

    case ENDIAN_BIG:
      switch ( bytes ) {
        case 1 : /* do nothing */     break;
        case 2 : *n = swap_16( *n );  break;
        case 4 : *n = swap_32( *n );  break;
#if SIZEOF_UNSIGNED_LONG == 8
        case 8 : *n = swap_64( *n );  break;
#endif /* SIZEOF_UNSIGNED_LONG */
      } /* switch */
      break;

    case ENDIAN_LITTLE:
      /* do nothing */
      break;

#endif /* WORDS_BIGENDIAN */

    default:
      assert( true );
  } /* switch */
}

void unget_byte( uint8_t byte, FILE *file ) {
  if ( ungetc( byte, file ) == EOF )
    PMESSAGE_EXIT( READ_ERROR,
      "\"%s\": unget byte failed: %s", path_name, ERROR_STR
    );
  --total_bytes_read;
}

/*****************************************************************************/
/* vim:set et sw=2 ts=2: */
