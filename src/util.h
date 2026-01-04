/*
**      ad -- ASCII dump
**      src/util.h
**
**      Copyright (C) 2015-2026  Paul J. Lucas
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

/**
 * @file
 * Declares utility constants, macros, and functions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "type_traits.h"
#include "unicode.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */
#include <stdio.h>                      /* for FILE */
#include <string.h>                     /* for strerror() */
#include <sys/types.h>                  /* for off_t */
#include <sysexits.h>

/// @endcond

/**
 * @defgroup util-group Utility Macros & Functions
 * Utility macros, constants, and functions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
#define CHARIFY_0 '0'
#define CHARIFY_1 '1'
#define CHARIFY_2 '2'
#define CHARIFY_3 '3'
#define CHARIFY_4 '4'
#define CHARIFY_5 '5'
#define CHARIFY_6 '6'
#define CHARIFY_7 '7'
#define CHARIFY_8 '8'
#define CHARIFY_9 '9'
#define CHARIFY_A 'A'
#define CHARIFY_B 'B'
#define CHARIFY_C 'C'
#define CHARIFY_D 'D'
#define CHARIFY_E 'E'
#define CHARIFY_F 'F'
#define CHARIFY_G 'G'
#define CHARIFY_H 'H'
#define CHARIFY_I 'I'
#define CHARIFY_J 'J'
#define CHARIFY_K 'K'
#define CHARIFY_L 'L'
#define CHARIFY_M 'M'
#define CHARIFY_N 'N'
#define CHARIFY_O 'O'
#define CHARIFY_P 'P'
#define CHARIFY_Q 'Q'
#define CHARIFY_R 'R'
#define CHARIFY_S 'S'
#define CHARIFY_T 'T'
#define CHARIFY_U 'U'
#define CHARIFY_V 'V'
#define CHARIFY_W 'W'
#define CHARIFY_X 'X'
#define CHARIFY_Y 'Y'
#define CHARIFY_Z 'Z'
#define CHARIFY__ '_'
#define CHARIFY_a 'a'
#define CHARIFY_b 'b'
#define CHARIFY_c 'c'
#define CHARIFY_d 'd'
#define CHARIFY_e 'e'
#define CHARIFY_f 'f'
#define CHARIFY_g 'g'
#define CHARIFY_h 'h'
#define CHARIFY_i 'i'
#define CHARIFY_j 'j'
#define CHARIFY_k 'k'
#define CHARIFY_l 'l'
#define CHARIFY_m 'm'
#define CHARIFY_n 'n'
#define CHARIFY_o 'o'
#define CHARIFY_p 'p'
#define CHARIFY_q 'q'
#define CHARIFY_r 'r'
#define CHARIFY_s 's'
#define CHARIFY_t 't'
#define CHARIFY_u 'u'
#define CHARIFY_v 'v'
#define CHARIFY_w 'w'
#define CHARIFY_x 'x'
#define CHARIFY_y 'y'
#define CHARIFY_z 'z'
/// @endcond

/**
 * Gets the number of elements of the given array.
 *
 * @param ARRAY The array to get the number of elements of.
 *
 * @note \a ARRAY _must_ be a statically allocated array.
 *
 * @sa #FOREACH_ARRAY_ELEMENT()
 */
#define ARRAY_SIZE(ARRAY) (         \
  sizeof(ARRAY) / sizeof(0[ARRAY])  \
  * STATIC_ASSERT_EXPR( IS_ARRAY_EXPR(ARRAY), #ARRAY " must be an array" ))

#ifndef NDEBUG
/**
 * Asserts that this line of code is run at most once --- useful in
 * initialization functions that must be called at most once.  For example:
 *
 *      void initialize() {
 *        ASSERT_RUN_ONCE();
 *        // ...
 *      }
 */
#define ASSERT_RUN_ONCE() BLOCK(    \
  static bool UNIQUE_NAME(called);  \
  assert( !UNIQUE_NAME(called) );   \
  UNIQUE_NAME(called) = true; )
#else
#define ASSERT_RUN_ONCE()         NO_OP
#endif /* NDEBUG */

/**
 * Calls **atexit**(3) and checks for failure.
 *
 * @param FN The pointer to the function to call **atexit**(3) with.
 */
#define ATEXIT(FN)                PERROR_EXIT_IF( atexit( FN ) != 0, EX_OSERR )

/**
 * Gets a value where all bits that are less than or equal to the one bit set
 * in \a N are also set, e.g., <code>%BITS_GE(00010000)</code> = `00011111`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_LT()
 */
#define BITS_LE(N)                (BITS_LT(N) | (N))

/**
 * Gets a value where all bits that are less than the one bit set in \a N are
 * set, e.g., <code>%BITS_GT(00010000)</code> = `00001111`.
 *
 * @param N The integer.  Exactly one bit _must_ be set.
 * @return Returns said value.
 *
 * @sa #BITS_LE()
 */
#define BITS_LT(N)                ((N) - 1u)

/**
 * Gets a value comprised of \a N lower 1 bits, e.g.,
 * <code>BITS_LOWER(5)</code> = `011111`.
 *
 * @param N The integer in the range [1,63].
 * @return Returns said value.
 *
 * @sa #BITS_LE()
 * @sa #BITS_LT()
 */
#define BITS_LOWER(N)             BITS_LE( 1u << ((N) - 1u) )

/**
 * Embeds the given statements into a compound statement block.
 *
 * @param ... The statement(s) to embed.
 */
#define BLOCK(...)                do { __VA_ARGS__ } while (0)

/**
 * Macro that "char-ifies" its argument, e.g., <code>%CHARIFY(x)</code> becomes
 * `'x'`.
 *
 * @param X The unquoted character to charify.  It can be only in the set
 * `[0-9_A-Za-z]`.
 *
 * @sa #STRINGIFY()
 */
#define CHARIFY(X)                NAME2(CHARIFY_,X)

/**
 * C version of C++'s `const_cast`.
 *
 * @param T The type to cast to.
 * @param EXPR The expression to cast.
 *
 * @note This macro can't actually implement C++'s `const_cast` because there's
 * no way to do it in C.  It serves merely as a visual cue for the type of cast
 * meant.
 *
 * @sa #POINTER_CAST()
 * @sa #STATIC_CAST()
 */
#define CONST_CAST(T,EXPR)        ((T)(EXPR))

/**
 * Declares a object with an unspecified name both aligned and sized as a \a T
 * intended to be used inside a `struct` declaration to add padding.
 *
 * @param T The type of the object.
 */
#define DECL_UNUSED(T) \
  _Alignas(T) char UNIQUE_NAME(unused)[ sizeof(T) ]

/**
 * Calls **dup2**(2) and checks for failure.
 *
 * @param OLD_FD The old file descriptor to duplicate.
 * @param NEW_FD The new file descriptor to duplicate to.
 */
#define DUP2(OLD_FD,NEW_FD) \
  PERROR_EXIT_IF( dup2( (OLD_FD), (NEW_FD) ) != (NEW_FD), EX_OSERR )

/**
 * Shorthand for printing to standard error.
 *
 * @param ... The `printf()` arguments.
 *
 * @sa #FPRINTF()
 */
#define EPRINTF(...)              fprintf( stderr, __VA_ARGS__ )

/**
 * Shorthand for printing a character to standard error.
 *
 * @param C The character to print.
 *
 * @sa #EPRINTF()
 * @sa #EPUTS()
 * @sa #FPUTC()
 */
#define EPUTC(C)                  FPUTC( (C), stderr )

/**
 * Shorthand for printing a C string to standard error.
 *
 * @param S The C string to print.
 *
 * @sa #EPRINTF()
 * @sa #EPUTC()
 * @sa #FPUTS()
 */
#define EPUTS(S)                  FPUTS( (S), stderr )

/**
 * Calls **ferror**(3) and exits if there was an error on \a STREAM.
 *
 * @param STREAM The `FILE` stream to check for an error.
 *
 * @sa #PERROR_EXIT_IF()
 */
#define FERROR(STREAM) \
  PERROR_EXIT_IF( ferror( STREAM ) != 0, EX_IOERR )

/**
 * Calls **fflush(3)** on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to flush.
 *
 * @sa #PERROR_EXIT_IF()
 */
#define FFLUSH(STREAM) \
  PERROR_EXIT_IF( fflush( STREAM ) != 0, EX_IOERR )

/**
 * Convenience macro for iterating over the elements of a static array.
 *
 * @param TYPE The type of element.
 * @param VAR The element loop variable.
 * @param ARRAY The array to iterate over.
 *
 * @note \a ARRAY _must_ be a statically allocated array.
 *
 * @sa #ARRAY_SIZE()
 */
#define FOREACH_ARRAY_ELEMENT(TYPE,VAR,ARRAY) \
  for ( TYPE const *VAR = (ARRAY); VAR < (ARRAY) + ARRAY_SIZE(ARRAY); ++VAR )

/**
 * Calls **fprintf**(3) on \a STREAM, checks for an error, and exits if there
 * was one.
 *
 * @param STREAM The `FILE` stream to print to.
 * @param ... The `fprintf()` arguments.
 *
 * @sa #EPRINTF()
 * @sa #FPUTC()
 * @sa #FPUTS()
 * @sa #PERROR_EXIT_IF()
 * @sa #PRINTF()
 * @sa #PUTC()
 * @sa #PUTS()
 */
#define FPRINTF(STREAM,...) \
  PERROR_EXIT_IF( fprintf( (STREAM), __VA_ARGS__ ) < 0, EX_IOERR )

/**
 * Calls **putc**(3), checks for an error, and exits if there was one.
 *
 * @param C The character to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #FPRINTF()
 * @sa #FPUTS()
 * @sa #PERROR_EXIT_IF()
 * @sa #PRINTF()
 * @sa #PUTC()
 */
#define FPUTC(C,STREAM) \
  PERROR_EXIT_IF( putc( (C), (STREAM) ) == EOF, EX_IOERR )

/**
 * Prints \a N spaces to \a STREAM.
 *
 * @param N The number of spaces to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #FPUTS()
 */
#define FPUTNSP(N,STREAM) \
  FPRINTF( (STREAM), "%*s", STATIC_CAST( int, (N) ), "" )

/**
 * Calls **fputs**(3), checks for an error, and exits if there was one.
 *
 * @param S The C string to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #FPRINTF()
 * @sa #FPUTC()
 * @sa #PERROR_EXIT_IF()
 * @sa #PRINTF()
 * @sa #PUTS()
 */
#define FPUTS(S,STREAM) \
  PERROR_EXIT_IF( fputs( STATIC_CAST( char const*, (S) ), (STREAM) ) == EOF, EX_IOERR )

/**
 * Prints \a N spaces to \a STREAM.
 *
 * @param N The number of spaces to print.
 * @param STREAM The `FILE` stream to print to.
 *
 * @sa #FPUTS()
 */
#define FPUTNSP(N,STREAM) \
  FPRINTF( (STREAM), "%*s", STATIC_CAST( int, (N) ), "" )

/**
 * Frees the given memory.
 *
 * @remarks This macro exists since free'ing a pointer to `const` generates a
 * warning.
 *
 * @param PTR The pointer to the memory to free.
 */
#define FREE(PTR)                 free( CONST_CAST( void*, (PTR) ) )

/**
 * Calls **fseek**(3), checks for an error, and exits if there was one.
 *
 * @param STREAM The `FILE` stream to check for an error.
 * @param OFFSET The offset to seek to.
 * @param WHENCE What \a OFFSET if relative to.
 */
#define FSEEK(STREAM,OFFSET,WHENCE) \
  PERROR_EXIT_IF( FSEEK_FN( (STREAM), (OFFSET), (WHENCE) ) == -1, EX_IOERR )

/** The fseek(3) function to use. */
#ifdef HAVE_FSEEKO
# define FSEEK_FN fseeko
#else
# define FSEEK_FN fseek
#endif /* HAVE_FSEEKO */

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
 * Calls **fseek**(3), checks for an error, and exits if there was one.
 *
 * @param STREAM The `FILE` stream to check for an error.
 * @param OFFSET The offset to seek to.
 * @param WHENCE What \a OFFSET if relative to.
 */
#define FSEEK(STREAM,OFFSET,WHENCE) \
  PERROR_EXIT_IF( FSEEK_FN( (STREAM), (OFFSET), (WHENCE) ) == -1, EX_IOERR )

/**
 * Calls **fstat**(3), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to stat.
 * @param STAT A pointer to a `struct stat` to receive the result.
 */
#define FSTAT(FD,STAT) \
  PERROR_EXIT_IF( fstat( (FD), (STAT) ) < 0 , EX_IOERR )

/**
 * A special-case of fatal_error() that additionally prints the file and line
 * where an internal error occurred.
 *
 * @param FORMAT The `printf()` format to use.
 * @param ... The `printf()` arguments.
 *
 * @sa fatal_error()
 * @sa #INTERNAL_ERROR()
 * @sa perror_exit()
 * @sa #PERROR_EXIT_IF()
 * @sa #UNEXPECTED_INT_VALUE()
 */
#define INTERNAL_ERROR(FORMAT,...) \
  fatal_error( EX_SOFTWARE, "%s:%d: internal error: " FORMAT, __FILE__, __LINE__, __VA_ARGS__ )

#ifdef __GNUC__

/**
 * Specifies that \a EXPR is _very_ likely (as in 99.99% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * marginally better performance.
 *
 * @param EXPR An expression that can be cast to `bool`.
 *
 * @sa #unlikely()
 * @sa [Memory part 5: What programmers can do](http://lwn.net/Articles/255364/)
 */
#define likely(EXPR)              __builtin_expect( !!(EXPR), 1 )

/**
 * Specifies that \a EXPR is _very_ unlikely (as in .01% of the time) to be
 * non-zero (true) allowing the compiler to better order code blocks for
 * marginally better performance.
 *
 * @param EXPR An expression that can be cast to `bool`.
 *
 * @sa #likely()
 * @sa [Memory part 5: What programmers can do](http://lwn.net/Articles/255364/)
 */
#define unlikely(EXPR)            __builtin_expect( !!(EXPR), 0 )

#else
# define likely(EXPR)             (EXPR)
# define unlikely(EXPR)           (EXPR)
#endif /* __GNUC__ */

/**
 * Calls **lseek**(3), checks for an error, and exits if there was one.
 *
 * @param FD The file descriptor to seek.
 * @param OFFSET The file offset to seek to.
 * @param WHENCE Where \a OFFSET is relative to.
 */
#define LSEEK(FD,OFFSET,WHENCE) \
  PERROR_EXIT_IF( lseek( (FD), (OFFSET), (WHENCE) ) == -1, EX_IOERR )

/**
 * Calls **malloc**(3) and casts the result to \a TYPE.
 *
 * @param TYPE The type to allocate.
 * @param N The number of objects of \a TYPE to allocate.
 * @return Returns a pointer to \a N uninitialized objects of \a TYPE.
 *
 * @sa check_realloc()
 * @sa #REALLOC()
 */
#define MALLOC(TYPE,N) \
  check_realloc( NULL, sizeof(TYPE) * STATIC_CAST( size_t, (N) ) )

/**
 * Gets the number of characters needed to represent the largest magnitide
 * value of the integral \a TYPE in decimal.
 *
 * @param TYPE The integral type.
 *
 * @sa https://stackoverflow.com/a/13546502/99089
 */
#define MAX_DEC_INT_DIGITS(TYPE)                                \
  (((sizeof(TYPE) * CHAR_BIT * 1233) >> 12)                     \
    + STATIC_ASSERT_EXPR( IS_INTEGRAL_TYPE(TYPE),               \
                          #TYPE " must be an integral type " )  \
    + IS_SIGNED_TYPE(TYPE))

/**
 * Concatenate \a A and \a B together to form a single token.
 *
 * @remarks This macro is needed instead of simply using `##` when either
 * argument needs to be expanded first, e.g., `__LINE__`.
 *
 * @param A The first name.
 * @param B The second name.
 */
#define NAME2(A,B)                NAME2_HELPER(A,B)

/// @cond DOXYGEN_IGNORE
#define NAME2_HELPER(A,B)         A ## B
/// @endcond

/**
 * No-operation statement.
 *
 * @remarks This is useful for do-nothing statements.
 */
#define NO_OP                     ((void)0)

/**
 * If \a EXPR is `true`, prints an error message for `errno` to standard error
 * and exits with status \a STATUS.
 *
 * @param EXPR The expression.
 * @param STATUS The exit status code.
 *
 * @sa fatal_error()
 * @sa #INTERNAL_ERROR()
 * @sa perror_exit()
 */
#define PERROR_EXIT_IF( EXPR, STATUS ) \
  BLOCK( if ( unlikely( EXPR ) ) perror_exit( STATUS ); )

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
 * @sa #CONST_CAST()
 * @sa #STATIC_CAST()
 */
#define POINTER_CAST(T,EXPR)      ((T)(uintptr_t)(EXPR))

/**
 * Calls #FPRINTF() with `stdout`.
 *
 * @param ... The `fprintf()` arguments.
 *
 * @sa #EPRINTF()
 * @sa #FPRINTF()
 * @sa #PUTC()
 * @sa #PUTS()
 */
#define PRINTF(...)               FPRINTF( stdout, __VA_ARGS__ )

/**
 * Calls #FPUTC() with `stdout`.
 *
 * @param C The character to print.
 *
 * @sa #FPUTC()
 * @sa #PRINTF()
 */
#define PUTC(C)                   FPUTC( (C), stdout )

/**
 * Calls #FPUTS() with `stdout`.
 *
 * @param S The C string to print.
 *
 * @note Unlike **puts**(3), does _not_ print a newline.
 *
 * @sa #FPUTS()
 * @sa #PRINTF()
 */
#define PUTS(S)                   FPUTS( (S), stdout )

/**
 * Convenience macro for calling check_realloc().
 *
 * @param PTR The pointer to memory to reallocate.  It is set to the newly
 * reallocated memory.
 * @param N The number of objects to reallocate.
 *
 * @sa check_realloc()
 * @sa #MALLOC()
 */
#define REALLOC(PTR,N) \
  ((PTR) = check_realloc( (PTR), sizeof(*(PTR)) * (N) ))

/**
 * Advances \a S over all \a CHARS.
 *
 * @param S The string pointer to advance.
 * @param CHARS A string containing the characters to skip over.
 * @return Returns the updated \a S.
 *
 * @sa #SKIP_WS()
 */
#define SKIP_CHARS(S,CHARS)       ((S) += strspn( (S), (CHARS) ))

/**
 * Advances \a S over all whitespace.
 *
 * @param S The string pointer to advance.
 * @return Returns the updated \a S.
 *
 * @sa #SKIP_CHARS()
 */
#define SKIP_WS(S)                SKIP_CHARS( (S), WS_CHARS )

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
 * @sa #CONST_CAST()
 * @sa #POINTER_CAST()
 */
#define STATIC_CAST(T,EXPR)       ((T)(EXPR))

/**
 * Shorthand for calling **strerror**(3).
 */
#define STRERROR()                strerror( errno )

/**
 * Macro that "string-ifies" its argument, e.g., <code>%STRINGIFY(x)</code>
 * becomes `"x"`.
 *
 * @param X The unquoted string to stringify.
 *
 * @note This macro is sometimes necessary in cases where it's mixed with uses
 * of `##` by forcing re-scanning for token substitution.
 *
 * @sa #CHARIFY()
 */
#define STRINGIFY(X)              STRINGIFY_HELPER(X)

/// @cond DOXYGEN_IGNORE
#define STRINGIFY_HELPER(X)       #X
/// @endcond

/**
 * Gets the length of \a S.
 *
 * @param S The C string literal to get the length of.
 * @return Returns said length.
 */
#define STRLITLEN(S) \
  (ARRAY_SIZE( (S) ) \
   - STATIC_ASSERT_EXPR( IS_C_STR_EXPR( (S) ), #S " must be a C string literal" ))

/**
 * A special-case of #INTERNAL_ERROR() that prints an unexpected integer value.
 *
 * @param EXPR The expression having the unexpected value.
 *
 * @sa fatal_error()
 * @sa #INTERNAL_ERROR()
 * @sa perror_exit()
 * @sa #PERROR_EXIT_IF()
 */
#define UNEXPECTED_INT_VALUE(EXPR) \
  INTERNAL_ERROR( "%lld (0x%llX): unexpected value for " #EXPR "\n", (long long)(EXPR), (unsigned long long)(EXPR) )

/**
 * Synthesises a name prefixed by \a PREFIX unique to the line on which it's
 * used.
 *
 * @param PREFIX The prefix of the synthesized name.
 *
 * @warning All uses for a given \a PREFIX that refer to the same name _must_
 * be on the same line.  This is not a problem within macro definitions, but
 * won't work outside of them since there's no way to refer to a previously
 * used unique name.
 */
#define UNIQUE_NAME(PREFIX)       NAME2(NAME2(PREFIX,_),__LINE__)

/**
 * Whitespace characters.
 */
#define WS_CHARS                  " \f\n\r\t\v"

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whether there is at least one printable ASCII character in \a s.
 *
 * @param s The string to check.
 * @param s_len The number of characters to check.
 * @return Returns `true` only if there is at least one printable character.
 */
NODISCARD
bool ascii_any_printable( char const *s, size_t s_len );

/**
 * Checks whether the given character is an ASCII printable character _not_
 * including space.
 *
 * @remarks This function is needed because **setlocale**(3) affects what
 * **isgraph**(3) considers printable.
 *
 * @param c The characther to check.
 * @return Returns `true` only if \a c is an ASCII printable character _not_
 * including space.
 *
 * @sa ascii_is_print()
 */
NODISCARD
inline bool ascii_is_graph( char8_t c ) {
  return c >= '!' && c <= '~';
}

/**
 * Checks whether the given character is an ASCII printable character including
 * space.
 *
 * @remarks This function is needed because **setlocale**(3) affects what
 * **isprint**(3) considers printable.
 *
 * @param c The characther to check.
 * @return Returns `true` only if \a c is an ASCII printable character
 * including space.
 *
 * @sa ascii_is_graph()
 */
NODISCARD
inline bool ascii_is_print( char c ) {
  return c >= ' ' && c <= '~';
}

/**
 * Extracts the base portion of a path-name.
 * Unlike **basename**(3):
 *  + Trailing `/` characters are not deleted.
 *  + \a path_name is never modified (hence can therefore be `const`).
 *  + Returns a pointer within \a path_name (hence is multi-call safe).
 *
 * @param path_name The path-name to extract the base portion of.
 * @return Returns a pointer to the last component of \a path_name.
 * If \a path_name consists entirely of `/` characters, a pointer to the string
 * `/` is returned.
 */
NODISCARD
char const* base_name( char const *path_name );

/**
 * Calls **dup2**(2) and checks for failure.
 *
 * @param old_fd The old file descriptor to duplicate.
 * @param new_fd The new file descriptor to duplicate to.
 */
void check_dup2( int old_fd, int new_fd );

/**
 * Calls **realloc(3)** and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If NULL, new memory is allocated.
 * @param size The number of bytes to allocate.
 * @return Returns a pointer to the allocated memory.
 *
 * @sa #MALLOC()
 * @sa #REALLOC()
 */
NODISCARD
void* check_realloc( void *p, size_t size );

/**
 * Calls **strdup(3)** and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The NULL-terminated string to duplicate.
 * @return Returns a copy of \a s.
 *
 * @sa check_strndup()
 */
NODISCARD
char* check_strdup( char const *s );

/**
 * Calls **strndup**(3) and checks for failure.
 * If memory allocation fails, prints an error message and exits.
 *
 * @param s The null-terminated string to duplicate or NULL.
 * @param n The number of characters of \a s to duplicate.
 * @return Returns a copy of \a n characters of \a s or NULL if \a s is NULL.
 *
 * @sa check_strdup()
 */
NODISCARD
char* check_strndup( char const *s, size_t n );

/**
 * Checks whether \a s is null: if so, returns the empty string.
 *
 * @param s The pointer to check.
 * @return If \a s is null, returns the empty string; otherwise returns \a s.
 *
 * @sa null_if_empty()
 */
NODISCARD
inline char const* empty_if_null( char const *s ) {
  return s == NULL ? "" : s;
}

/**
 * Prints an error message to standard error and exits with \a status code.
 *
 * @param status The status code to exit with.
 * @param format The `printf()` format string literal to use.
 * @param ... The `printf()` arguments.
 *
 * @sa #INTERNAL_ERROR()
 * @sa perror_exit()
 * @sa #PERROR_EXIT_IF()
 * @sa #UNEXPECTED_INT_VALUE()
 */
PJL_PRINTF_LIKE_FUNC(2)
_Noreturn void fatal_error( int status, char const *format, ... );

/**
 * Checks whether the given file descriptor refers to a regular file.
 *
 * @param fd The file descriptor to check.
 * @return Returns `true` only if \a fd refers to a regular file.
 */
NODISCARD
bool fd_is_file( int fd );

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
 * remain valid at least until after the _next_ call to fput_list(), that is
 * upon return, the previously returned string pointer must still be valid
 * also.
 */
void fput_list( FILE *out, void const *elt,
                char const* (*gets)( void const **ppelt ) );

/**
 * Prints \a u16s as a quoted sqeuence of escaped UTF-16 characters.
 *
 * @param u16s The UTF-16 string to print.
 * @param fout The `FILE` to print to.
 */
void fputs16( char16_t const *u16s, FILE *fout );

/**
 * Prints \a u32s as a quoted sqeuence of escaped UTF-32 characters.
 *
 * @param u32s The UTF-32 string to print.
 * @param fout The `FILE` to print to.
 */
void fputs32( char32_t const *u32s, FILE *fout );

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
 * Prints \a s as a quoted string with escaped characters.
 *
 * @param s The string to put.  If NULL, prints `null` (unquoted).
 * @param quote The quote character to use, either <tt>'</tt> or <tt>"</tt>.
 * @param fout The `FILE` to print to.
 *
 * @sa strbuf_puts_quoted()
 */
void fputs_quoted( char const *s, char quote, FILE *fout );

/**
 * Skips over \a bytes_to_skip bytes.
 * If an error occurs, prints an error message and exits.
 *
 * @param bytes_to_skip The number of bytes to skip.  Must not be negative.
 * @param file The file to read from.
 */
void fskip( off_t bytes_to_skip, FILE *file );

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
 * Rearranges the bytes in the given `uint64_t` such that:
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
 * @param n A pointer to the `uint64_t` to rearrange.
 * @param bytes The number of bytes to use; must be 1-8.
 * @param endian The endianness to use.
 */
void int_rearrange_bytes( uint64_t *n, size_t bytes, endian_t endian );

/**
 * Checks whether \a c is an identifier character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphanumeric or `_`
 * character.
 *
 * @sa is_ident_first()
 */
NODISCARD
inline bool is_ident( char c ) {
  return isalnum( c ) || c == '_';
}

/**
 * Checks whether \a c is an identifier first character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is either an alphabetic or `_`
 * character.
 *
 * @sa is_ident()
 */
NODISCARD
inline bool is_ident_first( char c ) {
  return isalpha( c ) || c == '_';
}

/**
 * Checks whether \a s is null, an empty string, or consists only of
 * whitespace.
 *
 * @param s The null-terminated string to check.
 * @return If \a s is either null or the empty string, returns NULL; otherwise
 * returns a pointer to the first non-whitespace character in \a s.
 *
 * @sa empty_if_null()
 */
NODISCARD
inline char const* null_if_empty( char const *s ) {
  return s != NULL && *SKIP_WS( s ) == '\0' ? NULL : s;
}

/**
 * Parses an identifier.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns a pointer to the first character that is not an identifier
 * character only if an identifier was parsed or NULL if an identifier was not
 * parsed.
 */
NODISCARD
char const* parse_identifier( char const *s );

/**
 * Parses a string into an <code>unsigned long long</code>.
 *
 * @remarks Unlike **strtoull(3)**, insists that \a s is entirely a non-
 * negative number.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the parsed number only if \a s is entirely a non-negative
 * number or prints an error message and exits if there was an error.
 */
NODISCARD
unsigned long long parse_ull( char const *s );

/**
 * Prints an error message for `errno` to standard error and exits.
 *
 * @param status The exit status code.
 *
 * @sa fatal_error()
 * @sa #INTERNAL_ERROR()
 * @sa #PERROR_EXIT_IF()
 */
void perror_exit( int status );

/**
 * Gets a printable version of the given character:
 *  + For characters for which **isprint**(3) returns non-zero, the printable
 *    version is a single character string of itself.
 *  + For the special-case characters of `\0`, `\a`, `\b`, `\f`, `\n`, `\r`,
 *    `\t`, and `\v`, the printable version is a two character string of a
 *    backslash followed by the letter.
 *  + For all other characters, the printable version is a four-character
 *    string of a backslash followed by an `'x'` and the two-character
 *    hexedecimal value of che characters ASCII code.
 *
 * @param c The character to get the printable form of.
 * @return Returns a NULL-terminated string that is a printable version of \a
 * c.  Note that the result is a pointer to static storage, hence subsequent
 * calls will overwrite the returned value.  As such, this function is not
 * thread-safe.
 */
NODISCARD
char const* printable_char( char c );

/**
 * Gets a pointer with \a value stored in its lower \a nbits bits.
 *
 * @param p The pointer to store \a value into.
 * @param nbits The number of low bits of \a p to use for \a value.
 * @param value The value to add.  It must not require more than \a nbits bits.
 * @return Returns \a p with the low \a nbits set to \a value.
 *
 * @sa ptr_without_bits()
 */
NODISCARD
void* ptr_with_bits( void *p, unsigned nbits, uintptr_t value );

/**
 * Gets a pointer without its lower \a nbits bits.
 *
 * @param p The pointer to remove the low \a nbits bits from.
 * @param nbits The number of low bits to remove.
 * @param pvalue A pointer to receive the low \a nbits bits of \a p.  May be
 * NULL.
 * @return Returns \a p with the low \a nbits zeroed.
 *
 * @sa ptr_with_bits()
 */
NODISCARD
void* ptr_without_bits( void *p, unsigned nbits, uintptr_t *pvalue );

/**
 * Decrements \a *s_len as if to trim whitespace, if any, from the end of \a s.
 *
 * @param s The null-terminated string to trim.
 * @param s_len A pointer to the length of \a s.
 */
void str_rtrim_len( char const *s, size_t *s_len );

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
 * Checks \a flag: if `false`, sets it to `true`.
 *
 * @param flag A pointer to the Boolean flag to be tested and, if `false`, sets
 * it to `true`.
 * @return Returns `true` only if \a flag was `true` initially.
 *
 * @sa false_set()
 * @sa true_clear()
 */
NODISCARD
inline bool true_or_set( bool *flag ) {
  return *flag || !(*flag = true);
}

/**
 * Possibly prints the list separator \a sep based on \a sep_flag.
 *
 * @param sep The separator to print.
 * @param sep_flag If `true`, prints \a sep; if `false`, prints nothing, but
 * sets it flag to `true`.  The flag should be `false` initially.
 * @param sout The `FILE` to print to.
 */
inline void fput_sep( char const *sep, bool *sep_flag, FILE *sout ) {
  if ( true_or_set( sep_flag ) )
    FPUTS( sep, sout );
}

/**
 * Converts a big-endian 16-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @return Returns \a n converted to the host's representation.
 */
NODISCARD
inline uint16_t uint16be_16he( uint16_t n ) {
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
NODISCARD
inline uint16_t uint16le_16he( uint16_t n ) {
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
NODISCARD
inline uint16_t uint16xx_16he( uint16_t n, endian_t endian ) {
  return endian == ENDIAN_LITTLE ? uint16le_16he( n ) : uint16be_16he( n );
}

/**
 * Converts a big-endian 32-bit integer to the host's representation.
 *
 * @param n The integer to convert.
 * @return Returns \a n converted to the host's representation.
 */
NODISCARD
inline uint32_t uint32be_32he( uint32_t n ) {
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
NODISCARD
inline uint32_t uint32le_32he( uint32_t n ) {
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
NODISCARD
inline uint32_t uint32xx_32he( uint32_t n, endian_t endian ) {
  return endian == ENDIAN_LITTLE ? uint32le_32he( n ) : uint32be_32he( n );
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

/** @} */

#endif /* ad_util_H */
/* vim:set et sw=2 ts=2: */
