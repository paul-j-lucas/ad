/*
**      ad -- ASCII dump
**      util.h
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

#ifndef ad_util_H
#define ad_util_H

// local
#include "common.h"
#include "config.h"

// system
#include <errno.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */
#include <stdio.h>                      /* for FILE */
#include <string.h>                     /* for strerror() */
#include <sys/types.h>                  /* for off_t */

///////////////////////////////////////////////////////////////////////////////

// define a "bool" type
#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifndef HAVE__BOOL
#   ifdef __cplusplus
typedef bool _Bool;
#   else
#     define _Bool signed char
#   endif /* __cplusplus */
# endif /* HAVE__BOOL */
# define bool   _Bool
# define false  0
# define true   1
# define __bool_true_false_are_defined 1
#endif /* HAVE_STDBOOL_H */

#define BLOCK(...)          do { __VA_ARGS__ } while (0)
#define ERROR_STR           strerror( errno )
#define PERROR_EXIT(STATUS) BLOCK( perror( me ); exit( EXIT_##STATUS ); )
#define PRINT_ERR(...)      fprintf( stderr, __VA_ARGS__ )

#define FSEEK(STREAM,OFFSET,WHENCE) \
  BLOCK( if ( fseeko( (STREAM), (OFFSET), (WHENCE) ) == -1 ) PERROR_EXIT( SEEK_ERROR ); )

#define FSTAT(FD,STAT) \
  BLOCK( if ( fstat( (FD), (STAT) ) < 0 ) PERROR_EXIT( STAT_ERROR ); )

#define MALLOC(TYPE,N) \
  (TYPE*)check_realloc( NULL, sizeof(TYPE) * (N) )

#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( PRINT_ERR( "%s: " FORMAT, me, __VA_ARGS__ ); exit( EXIT_##STATUS ); )

#define PRINTF(...) \
  BLOCK( if ( printf( __VA_ARGS__ ) < 0 ) PERROR_EXIT( WRITE_ERROR ); )

#define PUTCHAR(C) \
  BLOCK( if ( putchar( C ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

#define STRINGIFY_HELPER(S) #S
#define STRINGIFY(S)        STRINGIFY_HELPER(S)

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whethere there is at least one printable character in \a s.
 *
 * @param s The string to check.
 * @param s_len The number of characters to check.
 * @return Returns \c true only if there is at least one printable character.
 */
bool any_printable( char const *s, size_t s_len );

/**
 * Calls \c realloc(3) and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If NULL, new memory is allocated.
 * @param size The number of bytes to allocate.
 * @return Returns a pointer to the allocated memory.
 */
void* check_realloc( void *p, size_t size );

/**
 * Calls \c strdup(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The NULL-terminated string to duplicate.
 * @return Returns a copy of \a s.
 */
char* check_strdup( char const *s );

/**
 * Adds a pointer to the head of the free-list.
 *
 * @param p The pointer to add.
 * @return Returns \a p.
 */
void* freelist_add( void *p );

/**
 * Frees all the memory pointed to by all the nodes in the free-list.
 */
void freelist_free( void );

/**
 * Reads and discards \a bytes_to_skip bytes.
 *
 * @param bytes_to_skip The number of bytes to skip.
 * @param file The file to read from.
 */
void fskip( size_t bytes_to_skip, FILE *file );

/**
 * Gets the minimum number of bytes required to contain the given \c uint64_t
 * value.
 *
 * @param n The number to get the number of bytes for.
 * @return Returns the minimum number of bytes required to contain \a n.
 */
size_t int_len( uint64_t n );

/**
 * Rearranges the bytes in the given \c uint64_t such that:
 *  + The value is down-cast into the requested number of bytes.
 *  + The bytes have the requested endianness.
 *  + The bytes are shifted to start at the lowest memory address.
 *
 * For example, the value 0x1122 on a big-endian machine with 8-byte longs is
 * in memory as the bytes 00-00-00-00-00-00-11-22.
 *
 * If \a bytes were 4 and \a endian were:
 *  + big, the result in memory would be 00-00-11-22-00-00-00-00.
 *  + little, the result in memory would be 22-11-00-00-00-00-00-00.
 *
 * @param n A pointer to the \c uint64_t to rearrange.
 * @param bytes The number of bytes to use; must be 1, 2, 4, or 8.
 * @param endian The endianness to use.
 */
void int_rearrange_bytes( uint64_t *n, size_t bytes, endian_t endian );

/**
 * Opens the given file and seeks to the given offset
 * or prints an error message and exits if there was an error.
 *
 * @param path The full path of the file to open.
 * @param offset The number of bytes to skip, if any.
 * @return Returns the corresponding \c FILE.
 */
FILE* open_file( char const *path, off_t offset );

/**
 * Parses a string into an offset.
 * Unlike \c strtoull(3):
 *  + Insists that \a s is non-negative.
 *  + May be followed by one of \c b, \c k, or \c m
 *    for 512-byte blocks, kilobytes, and megabytes, respectively.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the parsed offset.
 */
uint64_t parse_offset( char const *s );

/**
 * Parses an SGR (Select Graphic Rendition) value that matches the regular
 * expression of \c n(;n)* or a semicolon-separated list of integers in the
 * range 0-255.
 *
 * See: http://en.wikipedia.org/wiki/ANSI_escape_code
 *
 * @param sgr_color The NULL-terminated allegedly SGR string to parse.
 * @return Returns \c true only only if \a s contains a valid SGR value.
 */
bool parse_sgr( char const *sgr_color );

/**
 * Parses a string into a \c uint64_t.
 * Unlike \c strtoull(3), insists that \a s is entirely a non-negative number.
 *
 * @param s The NULL-terminated string to parse.
 * @param n A pointer to receive the parsed number.
 * @return Returns the parsed number only if \a s is entirely a non-negative
 * number or prints an error message and exits if there was an error.
 */
uint64_t parse_ull( char const *s );

/**
 * Converts a string to lower-case in-place.
 *
 * @param s The NULL-terminated string to convert.
 * @return Returns \a s.
 */
char* tolower_s( char *s );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_util_H */
/* vim:set et sw=2 ts=2: */
