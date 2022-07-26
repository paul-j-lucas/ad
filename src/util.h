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

// standard
#include <errno.h>
#include <inttypes.h>                   /* for uint64_t */
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
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
 * Explicit C version of C++'s `const_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa REINTERPRET_CAST
 * @sa STATIC_CAST
 */
#define CONST_CAST(T,EXPR)        ((T)(uintptr_t)(EXPR))

/**
 * Shorthand for printing to standard error.
 *
 * @param ... The `printf()` arguments.
 */
#define EPRINTF(...)              fprintf( stderr, __VA_ARGS__ )

/**
 * Calls **putc**(3), checks for an error, and exits if there was one.
 *
 * @param C The character to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #EPUTC()
 * @sa #FPRINTF()
 * @sa #FPUTS()
 * @sa perror_exit_if()
 */
#define FPUTC(C,STREAM) \
  perror_exit_if( putc( (C), (STREAM) ) == EOF, EX_IOERR )

/**
 * Calls **fputs**(3), checks for an error, and exits if there was one.
 *
 * @param S The C string to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #EPUTS()
 * @sa #FPRINTF()
 * @sa #FPUTC()
 * @sa perror_exit_if()
 */
#define FPUTS(S,STREAM) \
  perror_exit_if( fputs( (S), (STREAM) ) == EOF, EX_IOERR )

/**
 * Frees the given memory.
 *
 * @param PTR The pointer to the memory to free.
 *
 * @remarks
 * This macro exists since free'ing a pointer-to const generates a warning.
 */
#define FREE(PTR)                 free( CONST_CAST( void*, (PTR) ) )

/**
 * Zeros the memory pointed to by \a PTR.  The number of bytes to zero is given
 * by `sizeof *(PTR)`.
 *
 * @param PTR The pointer to the start of memory to zero.  \a PTR must be a
 * pointer.  If it's an array, it'll generate a compile-time error.
 */
#ifdef HAVE___TYPEOF__
#define MEM_ZERO(PTR) BLOCK(                                            \
  /* "error: array initializer must be an initializer list" if array */ \
  __typeof__(PTR) _tmp __attribute__((unused)) = 0;                     \
  memset( (PTR), 0, sizeof *(PTR) ); )
#else
#define MEM_ZERO(PTR)             memset( (PTR), 0, sizeof *(PTR) )
#endif /* HAVE___TYPEOF__ */

/**
 * Explicit C version of C++'s `reinterpret_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa CONST_CAST
 * @sa STATIC_CAST
 */
#define REINTERPRET_CAST(T,EXPR)  ((T)(EXPR))

/**
 * Explicit C version of C++'s `static_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @sa CONST_CAST
 * @sa REINTERPRET_CAST
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

#define FSEEK(STREAM,OFFSET,WHENCE) BLOCK( \
	if ( unlikely( FSEEK_FN( (STREAM), (OFFSET), (WHENCE) ) == -1 ) ) perror_exit( EX_IOERR ); )

/**
 * Calls **fstat**(3), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to stat.
 * @param STAT A pointer to a `struct stat` to receive the result.
 */
#define FSTAT(FD,STAT) BLOCK( \
	if ( unlikely( fstat( (FD), (STAT) ) < 0 ) ) perror_exit( EX_IOERR ); )

/**
 * Calls **lseek**(3), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to seek.
 * @param OFFSET The file offset to seek to.
 * @param WHENCE Where \a OFFSET is relative to.
 */
#define LSEEK(FD,OFFSET,WHENCE) BLOCK( \
	if ( unlikely( lseek( (FD), (OFFSET), (WHENCE) ) == -1 ) ) perror_exit( EX_IOERR ); )

/**
 * Calls **malloc**(3) and casts the result to \a TYPE.
 *
 * @param TYPE The type to allocate.
 * @param N The number of objects of \a TYPE to allocate.
 * @return Returns a pointer to \a N uninitialized objects of \a TYPE.
 */
#define MALLOC(TYPE,N) \
  (TYPE*)check_realloc( NULL, sizeof(TYPE) * (N) )

/**
 * Prints an error message to standard error and exits with \a STATUS code.
 *
 * @param STATUS The status code to **exit**(3) with.
 * @param FORMAT The `printf()` format to use.
 * @param ... The `printf()` arguments.
 */
#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( EPRINTF( "%s: " FORMAT, me, __VA_ARGS__ ); exit( STATUS ); )

///////////////////////////////////////////////////////////////////////////////

/**
 * Endian-ness.
 */
enum ad_endian {
  AD_ENDIAN_NONE,
  AD_ENDIAN_LITTLE,
  AD_ENDIAN_BIG
};
typedef enum ad_endian ad_endian_t;

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
 * Prints a zero-or-more element list of strings where for:
 *
 *  + A zero-element list, nothing is printed;
 *  + A one-element list, the string for the element is printed;
 *  + A two-element list, the strings for the elements are printed separated by
 *    `or`;
 *  + A three-or-more element list, the strings for the first N-1 elements are
 *    printed separated by `,` and the N-1st and Nth elements are separated by
 *    `, or`.
 *
 * @param out The `FILE` to print to.
 * @param elt A pointer to the first element to print.
 * @param gets A pointer to a function to call to get the string for the
 * element `**ppelt`: if the function returns NULL, it signals the end of the
 * list; otherwise, the function returns the string for the element and must
 * increment `*ppelt` to the next element.  If \a gets is NULL, it is assumed
 * that \a elt points to the first element of an array of `char*` and that the
 * array ends with NULL.
 *
 * @warning The string pointer returned by \a gets for a given element _must_
 * remain valid at least until after the _next_ call to fprint_list(), that is
 * upon return, the previously returned string pointer must still be valid
 * also.
 */
void fprint_list( FILE *out, void const *elt,
                  char const* (*gets)( void const **ppelt ) );

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
void int_rearrange_bytes( uint64_t *n, size_t bytes, ad_endian_t endian );

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
 * Parses an SGR (Select Graphic Rendition) value that matches the regular
 * expression of \c n(;n)* or a semicolon-separated list of integers in the
 * range 0-255.
 *
 * See: http://en.wikipedia.org/wiki/ANSI_escape_code
 *
 * @param sgr_color The NULL-terminated allegedly SGR string to parse.
 * @return Returns \c true only only if \a s contains a valid SGR value.
 */
NODISCARD
bool parse_sgr( char const *sgr_color );

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
 * @sa #INTERNAL_ERR()
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
 * Compiles a regular expression pattern.
 *
 * @param re A pointer to the wregex_t to compile to.
 * @param pattern The regular expression pattern to compile.
 */
void regex_compile( regex_t *re, char const *pattern );

/**
 * Frees all memory used by a wregex_t.
 *
 * @param re A pointer to the wregex_t to free.
 */
AD_UTIL_INLINE
void regex_free( regex_t *re ) {
  regfree( re );
}

/**
 * Attempts to match \a s against the previously compiled regular expression
 * pattern in \a re.
 *
 * @param re A pointer to the wregex_t to match against.
 * @param s The string to match.
 * @param offset The offset into \a s to start.
 * @param range A pointer to an array of size 2 to receive the beginning
 * position and one past the end position of the match -- set only if not NULL
 * and there was a match.
 * @return Returns \c true only if there was a match.
 */
NODISCARD
bool regex_match( regex_t *re, char const *s, size_t offset, size_t *range );

/**
 * Swaps the endianness of the given 16-bit value.
 *
 * @param n The 16-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
NODISCARD
uint16_t swap_16( uint16_t n );

/**
 * Swaps the endianness of the given 32-bit value.
 *
 * @param n The 32-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
NODISCARD
uint32_t swap_32( uint32_t n );

/**
 * Swaps the endianness of the given 64-bit value.
 *
 * @param n The 64-bit value to swap.
 * @return Returns the value with the endianness flipped.
 */
NODISCARD
uint64_t swap_64( uint64_t n );

/**
 * Converts a big-endian 16-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @return Returns \a n converted to the host's representation.
 */
AD_UTIL_INLINE uint16_t uint16be_host16( uint16_t n ) {
#ifdef WORDS_BIGENDIAN
  return n;
#else /* machine words are little endian */
  return swap_16( n );
#endif /* WORDS_BIGENDIAN */
}

/**
 * Converts a little-endian 16-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @return Returns \a n converted to the host's representation.
 */
AD_UTIL_INLINE uint16_t uint16le_host16( uint16_t n ) {
#ifdef WORDS_BIGENDIAN
  return swap_16( n );
#else /* machine words are little endian */
  return n;
#endif /* WORDS_BIGENDIAN */
}

/**
 * Converts a specified endian 16-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @param endian The endianness of \a n.
 * @Returns \a n converted to the host's representation.
 */
AD_UTIL_INLINE uint16_t uint16xx_host16( uint16_t n, ad_endian_t endian ) {
  return endian == AD_ENDIAN_LITTLE ?
    uint16le_host16( n ) : uint16be_host16( n );
}

/**
 * Converts a big-endian 32-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @return Returns \a n converted to the host's representation.
 */
AD_UTIL_INLINE uint32_t uint32be_host32( uint32_t n ) {
#ifdef WORDS_BIGENDIAN
  return n;
#else /* machine words are little endian */
  return swap_32( n );
#endif /* WORDS_BIGENDIAN */
}

/**
 * Converts a little-endian 32-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @return Returns \a n converted to the host's representation.
 */
AD_UTIL_INLINE uint32_t uint32le_host32( uint32_t n ) {
#ifdef WORDS_BIGENDIAN
  return swap_32( n );
#else /* machine words are little endian */
  return n;
#endif /* WORDS_BIGENDIAN */
}

/**
 * Converts a specified endian 32-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @param endian The endianness of \a n.
 * @Returns \a n converted to the host's representation.
 */
AD_UTIL_INLINE uint32_t uint32xx_host32( uint32_t n, ad_endian_t endian ) {
  return endian == AD_ENDIAN_LITTLE ?
    uint32le_host32( n ) : uint32be_host32( n );
}

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
