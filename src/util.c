/*
**      ad -- ASCII dump
**      src/util.c
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
#define AD_UTIL_H_INLINE _GL_EXTERN_INLINE
#include "slist.h"
#include "util.h"

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

/**
 * An unsigned integer literal of \a N `0xF`s, e.g., `NF(3)` = `0xFFF`.
 *
 * @param N The number of `0xF`s of the literal in the range [1,16].
 * @return Returns said literal.
 */
#define NF(N)                     (~0ull >> ((sizeof(long long)*2 - (N)) * 4))

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

// local variable definitions
static slist_t free_later_list;         ///< List of stuff to free later.

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for fput_list() that, given a pointer to a pointer to an
 * array of pointer to `char`, returns the pointer to the associated string.
 *
 * @param ppelt A pointer to the pointer to the element to get the string of.
 * On return, it is incremented by the size of the element.
 * @return Returns said string or NULL if none.
 */
NODISCARD
static char const* fputs_list_apc_gets( void const **ppelt ) {
  char const *const *const ps = *ppelt;
  *ppelt = ps + 1;
  return *ps;
}

/**
 * Gets the regular expression error message corresponding to \a err_code.
 *
 * @param re The regex_t involved in the error.
 * @param err_code The error code.
 * @return Returns a pointer to a static buffer containing the error message.
 */
static char const* regex_error( regex_t *re, int err_code ) {
  assert( re != NULL );
  static char err_buf[ 128 ];
  (void)regerror( err_code, re, err_buf, sizeof err_buf );
  return err_buf;
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
  //
  // Autoconf, 5.5.1:
  //
  // realloc
  //    The C standard says a call realloc(NULL, size) is equivalent to
  //    malloc(size), but some old systems don't support this (e.g., NextStep).
  //
  if ( size == 0 )
    size = 1;
  void *const r = p ? realloc( p, size ) : malloc( size );
  if ( unlikely( r == NULL ) )
    perror_exit( EX_OSERR );
  return r;
}

char* check_strdup( char const *s ) {
  assert( s != NULL );
  char *const dup = strdup( s );
  if ( unlikely( dup == NULL ) )
    perror_exit( EX_OSERR );
  return dup;
}

char* check_strndup( char const *s, size_t n ) {
  if ( s == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  char *const dup_s = strndup( s, n );
  PERROR_EXIT_IF( dup_s == NULL, EX_OSERR );
  return dup_s;
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
  slist_push_back( &free_later_list, p );
  return p;
}

void free_now( void ) {
  slist_cleanup( &free_later_list, &free );
}

void fput_list( FILE *out, void const *elt,
                char const* (*gets)( void const** ) ) {
  assert( out != NULL );
  assert( elt != NULL );

  if ( gets == NULL )
    gets = &fputs_list_apc_gets;

  char const *s = (*gets)( &elt );
  for ( size_t i = 0; s != NULL; ++i ) {
    char const *const next_s = (*gets)( &elt );
    if ( i > 0 ) {
      char const *const sep = next_s != NULL ? ", " : i > 1 ? ", or " : " or ";
      FPUTS( sep, out );
    }
    FPUTS( s, out );
    s = next_s;
  } // for
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
    if ( unlikely( ferror( file ) ) )
      fatal_error( EX_IOERR, "can not read: %s\n", STRERROR() );
    bytes_to_skip -= STATIC_CAST( off_t, bytes_read );
  } // while
}

void fputs_quoted( char const *s, char quote, FILE *fout ) {
  assert( quote == '\'' || quote == '"' );
  assert( fout != NULL );

  if ( s == NULL ) {
    FPUTS( "null", fout );
    return;
  }

  bool in_quote = false;
  char const other_quote = quote == '\'' ? '"' : '\'';

  FPUTC( quote, fout );
  for ( char prev = '\0'; *s != '\0'; prev = *s++ ) {
    switch ( *s ) {
      case '\b': FPUTS( "\\b", fout ); continue;
      case '\f': FPUTS( "\\f", fout ); continue;
      case '\n': FPUTS( "\\n", fout ); continue;
      case '\r': FPUTS( "\\r", fout ); continue;
      case '\t': FPUTS( "\\t", fout ); continue;
      case '\v': FPUTS( "\\v", fout ); continue;
      case '\\':
        if ( in_quote ) {
          if ( prev != '\\' )
            FPUTS( "\\\\", fout );
          continue;
        }
        break;
    } // switch

    if ( prev != '\\' ) {
      if ( *s == quote ) {
        FPUTC( '\\', fout );
        in_quote = !in_quote;
      }
      else if ( *s == other_quote ) {
        in_quote = !in_quote;
      }
    }

    FPUTC( *s, fout );
  } // for
  FPUTC( quote, fout );
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

size_t int_len( uint64_t n ) {
  return n <= NF(8) ?
    (n <= NF( 4) ? (n <= NF( 2) ? 1 : 2) : (n <= NF( 6) ? 3 : 4)) :
    (n <= NF(12) ? (n <= NF(10) ? 5 : 6) : (n <= NF(14) ? 7 : 8));
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

char const* parse_identifier( char const *s ) {
  assert( s != NULL );
  if ( !is_ident_first( s[0] ) )
    return NULL;
  while ( is_ident( *++s ) )
    ;
  return s;
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

void str_rtrim_len( char const *s, size_t *s_len ) {
  assert( s != NULL );
  assert( s_len != NULL );

  while ( *s_len > 0 && strchr( WS_CHARS, s[ *s_len - 1 ] ) != NULL )
    --*s_len;
}

uint16_t swap_16( uint16_t n ) {
  return STATIC_CAST( uint16_t,
    (n >> 8)
  | (n << 8)
  );
}

uint32_t swap_32( uint32_t n ) {
  return  ( n                >> 24)
        | ((n & 0x00FF0000u) >>  8)
        | ((n & 0x0000FF00u) <<  8)
        | ( n                << 24);
}

uint64_t swap_64( uint64_t n ) {
  return  ( n                          >> 56)
        | ((n & 0x00FF000000000000ull) >> 40)
        | ((n & 0x0000FF0000000000ull) >> 24)
        | ((n & 0x000000FF00000000ull) >>  8)
        | ((n & 0x00000000FF000000ull) <<  8)
        | ((n & 0x0000000000FF0000ull) << 24)
        | ((n & 0x000000000000FF00ull) << 40)
        | ( n                          << 56);
}

void regex_compile( regex_t *re, char const *pattern ) {
  assert( re != NULL );
  assert( pattern != NULL );
  int const rv = regcomp( re, pattern, REG_EXTENDED );
  if ( rv != 0 ) {
    fatal_error( EX_DATAERR,
      "\"%s\": invalid regular expression (%d): %s\n",
      pattern, rv, regex_error( re, rv )
    );
  }
}

bool regex_match( regex_t *re, char const *s, size_t offset, size_t *range ) {
  assert( re != NULL );
  assert( s != NULL );

  char const *const so = s + offset;
  regmatch_t match[ re->re_nsub + 1 ];

  int const err_code = regexec( re, so, re->re_nsub + 1, match, /*eflags=*/0 );

  if ( err_code == REG_NOMATCH )
    return false;
  if ( err_code < 0 ) {
    fatal_error( EX_SOFTWARE,
      "regular expression error (%d): %s\n",
      err_code, regex_error( re, err_code )
    );
  }

  if ( range != NULL ) {
    range[0] = (size_t)match[0].rm_so + offset;
    range[1] = (size_t)match[0].rm_eo + offset;
  }
  return true;
}

char* tolower_s( char *s ) {
  assert( s != NULL );
  for ( char *t = s; *t != '\0'; ++t )
    *t = STATIC_CAST( char, tolower( *t ) );
  return s;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
