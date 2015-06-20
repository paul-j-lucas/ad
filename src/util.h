/*
**      ad -- ASCII dump
**      util.h
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

#ifndef ad_util_H
#define ad_util_H

// local
#include "common.h"
#include "config.h"

// system
#include <errno.h>
#include <stdint.h>                     // for uint8_t, ...
#include <stdio.h>
#include <string.h>                     // for str...()
#include <sys/types.h>

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

#define FSTAT(...) \
  BLOCK( if ( fstat( __VA_ARGS__ ) < 0 ) PERROR_EXIT( STAT_ERROR ); )

#define PERROR_EXIT(STATUS) BLOCK( perror( me ); exit( EXIT_##STATUS ); )

#define PRINT_ERR(...)      fprintf( stderr, __VA_ARGS__ )

#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( PRINT_ERR( "%s: " FORMAT, me, __VA_ARGS__ ); exit( EXIT_##STATUS ); )

#define PRINTF(...) \
  BLOCK( if ( printf( __VA_ARGS__ ) < 0 ) PERROR_EXIT( WRITE_ERROR ); )

#define PUTCHAR(C) \
  BLOCK( if ( putchar( C ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

#define STRINGIFY_HELPER(S) #S
#define STRINGIFY(S)        STRINGIFY_HELPER(S)

///////////////////////////////////////////////////////////////////////////////

struct free_node {
  void *p;
  struct free_node *next;
};
typedef struct free_node free_node_t;

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
 * @param pphead A pointer to the pointer to the head of the list.
 * @return Returns \a p.
 */
void* freelist_add( void *p, free_node_t **pphead );

/**
 * Frees all the memory pointed to by all the nodes in the free-list.
 *
 * @param phead A pointer to the head of the list.
 */
void freelist_free( free_node_t *phead );

/**
 * Reads and discards \a bytes_to_skip bytes.
 *
 * @param bytes_to_skip The number of bytes to skip.
 * @param file The file to read from.
 */
void fskip( size_t bytes_to_skip, FILE *file );

/**
 * Gets a byte from the given file.
 *
 * @param pbyte A pointer to the byte to receive the newly read byte.
 * @param max_bytes_to_read The maximum number of bytes to read in total.
 * @param file The file to read from.
 * @return Returns \c true if a byte was read successfully
 * and the number of bytes read does not exceed \a max_bytes_to_read.
 */
bool get_byte( uint8_t *pbyte, size_t max_bytes_to_read, FILE *file );

/**
 * Opens the given file and seeks to the given offset
 * or prints an error message and exits if there was an error.
 *
 * @param path_name The full path of the file to open.
 * @param offset The number of bytes to skip, if any.
 * @return Returns the corresponding \c FILE.
 */
FILE* open_file( char const *path_name, off_t offset );

/**
 * Parses a string into an offset.
 * Unlike \c strtoul(3):
 *  + Insists that \a s is non-negative.
 *  + May be followed by one of \c b, \c k, or \c m
 *    for 512-byte blocks, kilobytes, and megabytes, respectively.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the parsed offset.
 */
unsigned long parse_offset( char const *s );

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
 * Parses a string into an unsigned long.
 * Unlike \c strtoul(3), insists that \a s is entirely a non-negative number.
 *
 * @param s The NULL-terminated string to parse.
 * @param n A pointer to receive the parsed number.
 * @return Returns the parsed number only if \a s is entirely a non-negative
 * number or prints an error message and exits if there was an error.
 */
unsigned long parse_ul( char const *s );

/**
 * Converts a string to lower-case in-place.
 *
 * @param s The NULL-terminated string to convert.
 * @return Returns \a s.
 */
char* tolower_s( char *s );

/**
 * Gets the minimum number of bytes required to contain the given unsigned long
 * value.
 *
 * @param n The number to get the number of bytes for.
 * @return Returns the minimum number of bytes required to contain \a n.
 */
size_t ulong_len( unsigned long n );

/**
 * Rearranges the bytes in the given unsigned long such that:
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
 * @param n A pointer to the unsigned long to rearrange.
 * @param bytes The number of bytes to use; must be 1, 2, 4, or 8.
 * @param endian The endianness to use.
 */
void ulong_rearrange_bytes( unsigned long *n, size_t bytes, endian_t endian );

/**
 * Ungets the given byte to the given file.
 *
 * @param byte The byte to unget.
 * @param file The file to unget \a byte to.
 */
void unget_byte( uint8_t byte, FILE *file );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_util_H */
/* vim:set et sw=2 ts=2: */
