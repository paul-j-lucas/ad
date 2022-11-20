/*
**      ad -- ASCII dump
**      src/util.h
**
**      Copyright (C) 2015-2021  Paul J. Lucas
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

#ifndef ad_util_H
#define ad_util_H

// local
#include "pjl_config.h"                 /* must go first */
#include "common.h"

// standard
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */
#include <stdio.h>                      /* for FILE */
#include <string.h>                     /* for strerror() */
#include <sys/types.h>                  /* for off_t */

_GL_INLINE_HEADER_BEGIN
#ifndef AD_UTIL_INLINE
# define AD_UTIL_INLINE _GL_INLINE
#endif /* AD_UTIL_INLINE */

///////////////////////////////////////////////////////////////////////////////

/** The fseek(3) function to use. */
#ifdef HAVE_FSEEKO
# define FSEEK_FN fseeko
#else
# define FSEEK_FN fseek
#endif /* HAVE_FSEEKO */

/**
 * Embeds the given statements into a compound statement block.
 *
 * @param ... The statement(s) to embed.
 */
#define BLOCK(...)                do { __VA_ARGS__ } while (0)

/**
 * Shorthand for printing to standard error.
 *
 * @param ... The `printf()` arguments.
 */
#define EPRINTF(...)              fprintf( stderr, __VA_ARGS__ )

/**
 * Prints an error message to standard error and exits with \a STATUS code.
 *
 * @param STATUS The status code to **exit**(3) with.
 * @param FORMAT The `printf()` format to use.
 * @param ... The `printf()` arguments.
 */
#define FATAL_ERR(STATUS,FORMAT,...) \
  BLOCK( EPRINTF( "%s: " FORMAT, me, __VA_ARGS__ ); _Exit( STATUS ); )

/**
 * Cast either from or to a pointer type &mdash; similar to C++'s
 * `reinterpret_cast`, but for pointers only.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro silences a "cast to pointer from integer of different size"
 * warning.  In C++, this would be done via `reinterpret_cast`, but it's not
 * possible to implement that in C that works for both pointers and integers.
 *
 * @sa #STATIC_CAST()
 */
#define POINTER_CAST(T,EXPR)      ((T)(uintptr_t)(EXPR))

/**
 * C version of C++'s `static_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro can't actually implement C++'s `static_cast` because
 * there's no way to do it in C.  It serves merely as a visual cue for the type
 * of cast meant.
 *
 * @sa #POINTER_CAST()
 */
#define STATIC_CAST(T,EXPR)       ((T)(EXPR))

/**
 * Shorthand for calling **strerror**(3).
 */
#define STRERROR                  strerror( errno )

#ifdef __GNUC__

/**
 * Specifies that \a EXPR is \e very likely (as in 99.99% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * magrinally better performance.
 *
 * @see http://lwn.net/Articles/255364/
 * @hideinitializer
 */
#define likely(EXPR)              __builtin_expect( !!(EXPR), 1 )

/**
 * Specifies that \a EXPR is \e very unlikely (as in .01% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * magrinally better performance.
 *
 * @see http://lwn.net/Articles/255364/
 * @hideinitializer
 */
#define unlikely(EXPR)            __builtin_expect( !!(EXPR), 0 )

#else
# define likely(EXPR)             (EXPR)
# define unlikely(EXPR)           (EXPR)
#endif /* __GNUC__ */

/**
 * Calls **fseek**(3), checks for an error, and exits if there was one.
 *
 * @param STREAM The `FILE` stream to check for an error.
 * @param OFFSET The offset to seek to.
 * @param WHENCE What \a OFFSET if relative to.
 */
#define FSEEK(STREAM,OFFSET,WHENCE) \
  perror_exit_if( FSEEK_FN( (STREAM), (OFFSET), (WHENCE) ) == -1, EX_IOERR )

/**
 * Calls **fstat**(3), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to stat.
 * @param STAT A pointer to a `struct stat` to receive the result.
 */
#define FSTAT(FD,STAT) \
  perror_exit_if( fstat( (FD), (STAT) ) < 0 , EX_IOERR )

/**
 * Calls **lseek**(3), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to seek.
 * @param OFFSET The file offset to seek to.
 * @param WHENCE Where \a OFFSET is relative to.
 */
#define LSEEK(FD,OFFSET,WHENCE) \
  perror_exit_if( lseek( (FD), (OFFSET), (WHENCE) ) == -1, EX_IOERR )

/**
 * Calls **malloc**(3) and casts the result to \a TYPE.
 *
 * @param TYPE The type to allocate.
 * @param N The number of objects of \a TYPE to allocate.
 * @return Returns a pointer to \a N uninitialized objects of \a TYPE.
 */
#define MALLOC(TYPE,N) \
  (TYPE*)check_realloc( NULL, sizeof(TYPE) * (N) )

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whethere there is at least one printable ASCII character in \a s.
 *
 * @param s The string to check.
 * @param s_len The number of characters to check.
 * @return Returns \c true only if there is at least one printable character.
 */
NODISCARD
bool ascii_any_printable( char const *s, size_t s_len );

/**
 * Checks whether the given character is an ASCII printable character.
 * (This function is needed because setlocale(3) affects what isprint(3)
 * considers printable.)
 *
 * @param c The characther to check.
 * @return Returns \c true only if \c is an ASCII printable character.
 */
NODISCARD AD_UTIL_INLINE
bool ascii_is_print( char c ) {
  return c >= ' ' && c <= '~';
}

/**
 * Opens the given file and seeks to the given offset
 * or prints an error message and exits if there was an error.
 *
 * @param path The full path of the file to open.
 * @param mode The mode to use.
 * @param offset The number of bytes to skip, if any.
 * @return Returns the corresponding \c FILE.
 */
NODISCARD
FILE* check_fopen( char const *path, char const *mode, off_t offset );

/**
 * Opens the given file and seeks to the given offset
 * or prints an error message and exits if there was an error.
 *
 * @param path The full path of the file to open.
 * @param oflag The open flags to use.
 * @param offset The number of bytes to skip, if any.
 * @return Returns the corresponding file descriptor.
 */
NODISCARD
int check_open( char const *path, int oflag, off_t offset );

/**
 * Calls \c realloc(3) and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If NULL, new memory is allocated.
 * @param size The number of bytes to allocate.
 * @return Returns a pointer to the allocated memory.
 */
NODISCARD
void* check_realloc( void *p, size_t size );

/**
 * Calls \c strdup(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The NULL-terminated string to duplicate.
 * @return Returns a copy of \a s.
 */
NODISCARD
char* check_strdup( char const *s );

#ifndef HAVE_FGETLN
/**
 * Gets a line from a stream.
 *
 * @param f The FILE to get the line from.
 * @param len A pointer to receive the length of the line,
 * including the newline.
 * @return Returns a pointer to the line that is \e not NULL-terminated;
 * or NULL upon EOF or error.
 */
NODISCARD
char* fgetln( FILE *f, size_t *len );
#endif /* HAVE_FGETLN */

/**
 * Adds a pointer to the head of the free-later-list.
 *
 * @param p The pointer to add.
 * @return Returns \a p.
 */
NODISCARD
void* free_later( void *p );

/**
 * Frees all the memory pointed to by all the nodes in the free-later-list.
 */
void free_now( void );

/**
 * Reads and discards \a bytes_to_skip bytes.
 * If an error occurs, prints an error message and exits.
 *
 * @param bytes_to_skip The number of bytes to skip.
 * @param file The file to read from.
 */
void fskip( size_t bytes_to_skip, FILE *file );

/**
 * Converts a string into one that is a valid identifier in C such that:
 *  + Non-valid identifier characters [^A-Za-z_0-9] are replaced with '_'.
 *  + Multiple consecutive '_' are coalesced into a single '_'
 *    (except multiple consecutive '_' that are in \a s to begin with).
 *  + If \a s begins with a digit, then '_' is prepended.
 *
 * @param s The string to create an identifier from.
 * @return Returns \a s converted to a valid identifier in C.
 * The caller is responsible for freeing the string.
 */
NODISCARD
char* identify( char const *s );

/**
 * Gets the minimum number of bytes required to contain the given \c uint64_t
 * value.
 *
 * @param n The number to get the number of bytes for.
 * @return Returns the minimum number of bytes required to contain \a n
 * in the range [1,8].
 */
NODISCARD
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
 * @param bytes The number of bytes to use; must be 1-8.
 * @param endian The endianness to use.
 */
void int_rearrange_bytes( uint64_t *n, size_t bytes, endian_t endian );

/**
 * Checks whether the given file descriptor refers to a regular file.
 *
 * @param fd The file descriptor to check.
 * @return Returns \c true only if \a fd refers to a regular file.
 */
NODISCARD
bool is_file( int fd );

/**
 * Parses a string into an offset.
 * Unlike \c strtoull(3):
 *  + Insists that \a s is non-negative.
 *  + May be followed by one of \c b, \c k, or \c m
 *    for 512-byte blocks, kilobytes, and megabytes, respectively.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the parsed offset only if \a s is a non-negative number or
 * prints an error message and exits if there was an error.
 */
NODISCARD
unsigned long long parse_offset( char const *s );

/**
 * Parses a string into an <code>unsigned long long</code>.
 * Unlike \c strtoull(3), insists that \a s is entirely a non-negative number.
 *
 * @param s The NULL-terminated string to parse.
 * @param n A pointer to receive the parsed number.
 * @return Returns the parsed number only if \a s is entirely a non-negative
 * number or prints an error message and exits if there was an error.
 */
NODISCARD
unsigned long long parse_ull( char const *s );

/**
 * Prints an error message for \c errno to standard error and exits.
 *
 * @param status The exit status code.
 *
 * @sa #FATAL_ERR()
 * @sa perror_exit_if()
 */
void perror_exit( int status );

/**
 * If \a expr is `true`, prints an error message for `errno` to standard error
 * and exits.
 *
 * @param expr The expression.
 * @param status The exit status code.
 *
 * @sa #FATAL_ERR()
 * @sa perror_exit()
 */
AD_UTIL_INLINE
void perror_exit_if( bool expr, int status ) {
  if ( unlikely( expr ) )
    perror_exit( status );
}

/**
 * Gets a printable version of the given character:
 *  + For characters for which isprint(3) returns non-zero,
 *    the printable version is a single character string of itself.
 *  + For the special-case characters of \0, \a, \b, \f, \n, \r, \t, and \v,
 *    the printable version is a two character string of a backslash followed
 *    by the letter.
 *  + For all other characters, the printable version is a four-character
 *    string of a backslash followed by an 'x' and the two-character
 *    hexedecimal value of che characters ASCII code.
 *
 * @param c The character to get the printable form of.
 * @return Returns a NULL-terminated string that is a printable version of
 * \a c.  Note that the result is a pointer to static storage, hence subsequent
 * calls will overwrite the returned value.  As such, this function is not
 * thread-safe.
 */
NODISCARD
char const* printable_char( char c );

/**
 * Converts a string to lower-case in-place.
 *
 * @param s The NULL-terminated string to convert.
 * @return Returns \a s.
 */
PJL_DISCARD
char* tolower_s( char *s );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* ad_util_H */
/* vim:set et sw=2 ts=2: */
