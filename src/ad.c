/*
**      ad -- ASCII dump
**      ad.c: implementation
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
#include "config.h"
#include "sgr_color.h"

/* system */
#include <assert.h>
#include <ctype.h>                      /* for isprint() */
#include <errno.h>
#include <libgen.h>                     /* for basename() */
#include <stdio.h>
#include <stdlib.h>                     /* for exit(), strtoul(), ... */
#include <string.h>                     /* for str...() */
#include <sys/types.h>
#include <unistd.h>                     /* for lseek(), read(), ... */

/* define a "bool" type */
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

/* exit(3) status codes */
#define EXIT_OK             0
#define EXIT_USAGE          1
#define EXIT_OUT_OF_MEMORY  3
#define EXIT_READ_OPEN      10
#define EXIT_READ_ERROR     11
#define EXIT_WRITE_ERROR    13
#define EXIT_SEEK_ERROR     20

#define BLOCK(...)          do { __VA_ARGS__ } while (0)

#define ERROR_STR           strerror( errno )

#define PERROR_EXIT(STATUS) \
  BLOCK( perror( me ); exit( EXIT_##STATUS ); )

#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( fprintf( stderr, "%s: " FORMAT, me, __VA_ARGS__ ); exit( EXIT_##STATUS ); )

#define PRINTF(...) \
  BLOCK( if ( printf( __VA_ARGS__ ) < 0 ) PERROR_EXIT( WRITE_ERROR ); )

#define PUTCHAR(C) \
  BLOCK( if ( putchar( C ) == EOF ) PERROR_EXIT( WRITE_ERROR ); )

/*****************************************************************************/

#define BUF_SIZE      16                /* bytes displayed on a line */

typedef enum {
  ENDIAN_UNSPECIFIED,
  ENDIAN_BIG,
  ENDIAN_LITTLE
} endian_t;

typedef int kmp_value;

typedef enum {
  OFMT_DEC,
  OFMT_HEX,
  OFMT_OCT
} offset_fmt_t;

FILE*         file;                     /* file to read from */
char const*   me;                       /* executable name */
off_t         offset;                   /* curent offset into file */
bool          opt_case_insensitive = false;
size_t        opt_max_bytes_to_read = SIZE_MAX;
offset_fmt_t  opt_offset_fmt = OFMT_HEX;
char const*   path_name = "<stdin>";
size_t        total_bytes_read;

char*         search_buf;               /* not NULL-terminated when numeric */
endian_t      search_endian;            /* if searching for a number */
size_t        search_len;               /* number of bytes in search_buf */
char const*   search_match_color = SGR_BG_COLOR_BRIGHT_RED;
unsigned long search_number;            /* the number to search for */

/* local functions */
static void*          check_realloc( void*, size_t );
static void           dump();
static void           fskip( size_t, FILE* );
static bool           get_byte( uint8_t* );
static kmp_value*     kmp_init( char const*, size_t );
static bool           match_byte( uint8_t*, bool*, kmp_value const*, uint8_t* );
static FILE*          open_file( char const*, off_t );
static bool           parse_ul( char const*, unsigned long* );
static unsigned long  parse_ul_or_exit( char const* );
static unsigned long  parse_offset( char const* );
static void           parse_options( int, char*[] );
static size_t         prep_search_number( unsigned long* );
static void           set_match_color();
static char*          tolower_s( char* );
static void           usage();

/* inline functions **********************************************************/

static inline size_t byte_len( unsigned long n ) {
  if ( n < 0x10000 )
    return n < 0x100 ? 1 : 2;
  else
    return n < 0x100000000L ? 4 : 8;
}

static inline char const* skip_ws( char const *s ) {
  assert( s );
  while ( *s && isspace( *s ) )
    ++s;
  return s;
}

static inline uint16_t swap_16( uint16_t n ) {
  return  (n >> 8)
        | (n << 8);
}

static inline uint32_t swap_32( uint32_t n ) {
  return  ( n                >> 24)
        | ((n & 0x00FF0000u) >>  8)
        | ((n & 0x0000FF00u) <<  8)
        | ( n                << 24);
}

#if SIZEOF_UNSIGNED_LONG == 8
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

int main( int argc, char *argv[] ) {
  me = basename( argv[0] );
  parse_options( argc, argv );

  if ( search_buf )
    search_len = strlen( search_buf );
  else if ( search_endian ) {
    search_len = prep_search_number( &search_number );
    search_buf = (char*)&search_number;
  }

  if ( search_buf )
    set_match_color();
  dump();
  fclose( file );
  exit( EXIT_OK );
}

/*****************************************************************************/

#define COLOR_ON_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_START_COLOR( search_match_color ); )
#define COLOR_OFF_IF(EXPR) \
  BLOCK( if ( EXPR ) SGR_END_COLOR(); )

#define COLOR_ON()  COLOR_ON_IF( matches )
#define COLOR_OFF() COLOR_OFF_IF( matches )

/**
 * Calls \c realloc(3) and checks for failure.
 * If reallocation fails, prints an error message and exits.
 *
 * @param p The pointer to reallocate.  If NULL, new memory is allocated.
 * @param size The number of bytes to allocate.
 * @return Returns a pointer to the allocated memory.
 */
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

static void dump() {
  static char const *const offset_fmt_printf[] = {
    "%016llu: ",                        /* decimal */
    "%016llX: ",                        /* hex */
    "%016llo: ",                        /* octal */
  };

  uint8_t buf[ BUF_SIZE ];              /* store bytes to print ASCII later */
  size_t buf_pos = 0;
  uint16_t color_bits = 0;              /* bit set = print in color */
  bool done = false;
  bool matches_prev = false;
  size_t i;

  kmp_value const *kmp_values;
  uint8_t *match_buf;                   /* not NULL-terminated */

  if ( search_len ) {
    kmp_values = kmp_init( search_buf, search_len );
    match_buf = check_realloc( NULL, search_len );
  } else {
    kmp_values = NULL;
    match_buf = NULL;
  }

  while ( !done ) {
    bool matches;
    done = !match_byte( buf + buf_pos, &matches, kmp_values, match_buf );
    if ( done ) {
      COLOR_OFF_IF( matches_prev );
      if ( buf_pos ) {                  /* print padding */
        for ( i = BUF_SIZE - buf_pos; i; --i )
          PRINTF( "  " );
        for ( i = (BUF_SIZE - buf_pos) / 2; i; --i )
          PUTCHAR( ' ' );
        goto ascii;                     /* print final ASCII part */
      }
    } else {
      if ( !buf_pos ) {                 /* print offset */
        COLOR_OFF();
        PRINTF( offset_fmt_printf[ opt_offset_fmt ], offset );
        COLOR_ON();
      } else if ( buf_pos % 2 == 0 ) {  /* print separator between columns */
        COLOR_OFF_IF( matches_prev );
        PUTCHAR( ' ' );
        COLOR_ON_IF( matches_prev );
      }
      ++offset;

      if ( matches ) {                  /* print hex part */
        COLOR_ON_IF( matches != matches_prev );
        color_bits |= 1 << buf_pos;
      } else {
        COLOR_OFF_IF( matches != matches_prev );
      }
      PRINTF( "%02X", (unsigned)buf[ buf_pos ] );
      matches_prev = matches;

      if ( ++buf_pos == BUF_SIZE ) {    /* print ASCII part */
ascii:  COLOR_OFF();
        matches_prev = false;
        PRINTF( "  " );
        for ( i = 0; i < buf_pos; ++i ) {
          matches = color_bits & (1 << i);
          if ( matches )
            COLOR_ON_IF( matches != matches_prev );
          else
            COLOR_OFF_IF( matches != matches_prev );
          PUTCHAR( isprint( buf[i] ) ? buf[i] : '.' );
          matches_prev = matches;
        } /* for */
        COLOR_OFF();
        PUTCHAR( '\n' );
        COLOR_ON();
        buf_pos = 0;
        color_bits = 0;
      }
    }
  } /* while */

  free( (void*)kmp_values );
  free( match_buf );
}

static void fskip( size_t bytes_to_skip, FILE *file ) {
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

static bool get_byte( uint8_t *pbyte ) {
  if ( total_bytes_read < opt_max_bytes_to_read ) {
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

static inline void unget_byte( uint8_t byte ) {
  if ( ungetc( byte, file ) == EOF )
    PMESSAGE_EXIT( READ_ERROR,
      "\"%s\": unget byte failed: %s", path_name, ERROR_STR
    );
  --total_bytes_read;
}

static kmp_value* kmp_init( char const *pattern, size_t pattern_len ) {
  kmp_value *kmp_values;
  size_t i, j = 0;

  assert( pattern );
  kmp_values = check_realloc( NULL, pattern_len * sizeof( kmp_value ) );

  kmp_values[0] = -1;
  for ( i = 1; i < pattern_len; ) {
    if ( pattern[i] == pattern[j] )
      kmp_values[++i] = ++j;
    else if ( j > 0 )
      j = kmp_values[j-1];
    else
      kmp_values[++i] = 0;
  } /* for */
  return kmp_values;
}

static bool match_byte( uint8_t *pbyte, bool *matches,
                        kmp_value const *kmp_values, uint8_t *buf ) {
  typedef enum {
    S_READING,                          /* just reading; not matching */
    S_MATCHING,                         /* matching search string */
    S_MATCHING_2,
    S_MATCHED,                          /* a complete match */
    S_NOT_MATCHED,                      /* didn't match after all */
    S_DONE                              /* no more input */
  } state_t;

  static size_t buf_pos;
  static state_t state = S_READING;
  static size_t buf_drain;
  static size_t kmp;

  uint8_t byte_copy;

  assert( pbyte );
  assert( matches );
  assert( state != S_DONE );

  *matches = false;

  for ( ;; ) {
    switch ( state ) {

#define GOTO_STATE(S) { buf_pos = 0; state = (S); continue; }

      case S_READING:
        if ( !get_byte( pbyte ) )
          GOTO_STATE( S_DONE );
        if ( !search_len )
          return true;
        byte_copy = opt_case_insensitive ? tolower( *pbyte ) : *pbyte;
        if ( byte_copy != search_buf[0] )
          return true;
        buf[ 0 ] = *pbyte;
        kmp = 0;
        GOTO_STATE( S_MATCHING );

      case S_MATCHING:
        if ( ++buf_pos == search_len ) {
          *matches = true;
          buf_drain = buf_pos;
          GOTO_STATE( S_MATCHED );
        }
      case S_MATCHING_2:
        if ( !get_byte( pbyte ) ) {
          buf_drain = buf_pos;
          GOTO_STATE( S_NOT_MATCHED );
        }
        byte_copy = opt_case_insensitive ? tolower( *pbyte ) : *pbyte;
        if ( byte_copy == search_buf[ buf_pos ] ) {
          buf[ buf_pos ] = *pbyte;
          state = S_MATCHING;
          break;
        }
        unget_byte( *pbyte );
        kmp = kmp_values[ buf_pos ];
        buf_drain = buf_pos - kmp;
        GOTO_STATE( S_NOT_MATCHED );

      case S_MATCHED:
      case S_NOT_MATCHED:
        if ( buf_pos == buf_drain ) {
          buf_pos = kmp ? kmp - 1 : 0;
          state = kmp ? S_MATCHING : S_READING;
          continue;
        }
        *pbyte = buf[ buf_pos++ ];
        *matches = state == S_MATCHED;
        return true;

      case S_DONE:
        return false;

#undef GOTO_STATE

    } /* switch */
  } /* for */
}

static FILE* open_file( char const *path_name, off_t offset ) {
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

/**
 * Parses a string into an unsigned long offset.
 * Unlike \c strtoul(3):
 *  + Insists that \a s is non-negative.
 *  + May be followed by one of \c b, \c k, or \c m
 *    for 512-byte blocks, kilobytes, and megabytes, respectively.
 *
 * @param s The NULL-terminated string to parse.
 * @return Returns the parsed offset.
 */
static unsigned long parse_offset( char const *s ) {
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

/**
 * Parses a string into an unsigned long.
 * Unlike \c strtoul(3), insists that \a s is entirely a non-negative number.
 *
 * @param s The NULL-terminated string to parse.
 * @param n A pointer to receive the parsed number.
 * @return Returns \c true only if \a s is entirely a non-negative number.
 */
static bool parse_ul( char const *s, unsigned long *n ) {
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

static unsigned long parse_ul_or_exit( char const *s ) {
  unsigned long n;
  if ( !parse_ul( s, &n ) )
    PMESSAGE_EXIT( USAGE, "\"%s\": invalid integer\n", s );
  return n;
}

static void parse_options( int argc, char *argv[] ) {
  int         opt;                      /* command-line option */
  char const  opts[] = "de:E:hij:N:os:v";

  opterr = 1;
  while ( (opt = getopt( argc, argv, opts )) != EOF ) {
    switch ( opt ) {
      case 'd': opt_offset_fmt = OFMT_DEC;                      break;
      case 'e': search_number = parse_ul_or_exit( optarg );
                search_endian = ENDIAN_LITTLE;                  break;
      case 'E': search_number = parse_ul_or_exit( optarg );
                search_endian = ENDIAN_BIG;                     break;
      case 'h': opt_offset_fmt = OFMT_HEX;                      break;
      case 'i': opt_case_insensitive = true;                    break;
      case 'j': offset += parse_offset( optarg );               break;
      case 'N': opt_max_bytes_to_read = parse_offset( optarg ); break;
      case 'o': opt_offset_fmt = OFMT_OCT;                      break;
      case 's': search_buf = optarg;                            break;
      case 'v': fprintf( stderr, "%s\n", PACKAGE_STRING );      exit( EXIT_OK );
      default : usage();
    } /* switch */
  } /* while */
  argc -= optind, argv += optind - 1;

  if ( search_endian && search_buf )
    PMESSAGE_EXIT( USAGE,
      "-[%c%c] and -%c: options are mutually exclusive\n", 'e', 'E', 's'
    );

  if ( opt_case_insensitive ) {
    if ( !search_buf )
      PMESSAGE_EXIT( USAGE,
        "-%c: option requires -%c option to be given also\n", 'i', 's'
      );
    tolower_s( search_buf );
  }

  switch ( argc ) {
    case 0:                             /* read from stdin with no offset */
      file = fdopen( STDIN_FILENO, "r" );
      break;

    case 1:                             /* offset OR file */
      if ( *argv[1] == '+' ) {
        file = fdopen( STDIN_FILENO, "r" );
        fskip( parse_offset( argv[1] ), file );
      } else {
        path_name = argv[1];
        file = open_file( path_name, offset );
      }
      break;

    case 2:                             /* offset & file OR file & offset */
      if ( *argv[1] == '+' ) {
        if ( *argv[2] == '+' )
          PMESSAGE_EXIT( USAGE,
            "'%c': can not specify for more than one argument\n", '+'
          );
        offset += parse_offset( argv[1] );
        path_name = argv[2];
      } else {
        path_name = argv[1];
        offset += parse_offset( argv[2] );
      }
      file = open_file( path_name, offset );
      break;

    default:
      usage();
  } /* switch */

  if ( !opt_max_bytes_to_read )
    exit( EXIT_OK );
}

static size_t prep_search_number( unsigned long *n ) {
  size_t const len = byte_len( *n );
  switch ( search_endian ) {
#ifdef WORDS_BIGENDIAN

    case ENDIAN_BIG:
      /* move bytes to start of buffer */
      *n <<= (sizeof( unsigned long ) - len) * 8;
      break;

    case ENDIAN_LITTLE:
      switch ( len ) {
        case 1: /* do nothing */    break;
        case 2: *n = swap_16( *n ); break;
        case 4: *n = swap_32( *n ); break;
#if SIZEOF_UNSIGNED_LONG == 8
        case 8: *n = swap_64( *n ); break;
#endif /* SIZEOF_UNSIGNED_LONG */
      } /* switch */
      break;

#else /* words are little endian */

    case ENDIAN_BIG:
      switch ( len ) {
        case 1: /* do nothing */    break;
        case 2: *n = swap_16( *n ); break;
        case 4: *n = swap_32( *n ); break;
#if SIZEOF_UNSIGNED_LONG == 8
        case 8: *n = swap_64( *n ); break;
#endif /* SIZEOF_UNSIGNED_LONG */
      } /* switch */
      break;

    case ENDIAN_LITTLE:
      /* do nothing */
      break;
#endif /* WORDS_BIGENDIAN */

    case ENDIAN_UNSPECIFIED:            /* here only to suppress warning */
      /* do nothing */
      break;
  } /* switch */
  return len;
}

static void set_match_color() {
  static char const *const env_vars[] = {
    "AD_COLOR",
    "GREP_COLOR",
    0
  };

  char const *const *env_var;
  for ( env_var = env_vars; *env_var; ++env_var ) {
    char const *const color = getenv( *env_var );
    unsigned long dont_care;            /* only care if it will parse */
    if ( color && parse_ul( color, &dont_care ) ) {
      search_match_color = color;
      break;
    }
  } /* for */
}

/**
 * Converts a string to lower-case in-place.
 *
 * @param s The NULL-terminated string to convert.
 * @return Returns \a s.
 */
static char* tolower_s( char *s ) {
  assert( s );
  for ( ; *s; ++s )
    *s = tolower( *s );
  return s;
}

static void usage() {
  fprintf( stderr,
"usage: %s [-dhiov] [-j offset] [-N length] [[-i] -s search] [file] [[+]offset]\n"
"       %s [-dhiov] [-j offset] [-N length] [[-i] -s search] [+offset] [file]\n"
    , me, me
  );
  exit( EXIT_USAGE );
}
/* vim:set et sw=2 ts=2: */
