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

/* system */
#include <assert.h>
#include <ctype.h>                      /* for isprint() */
#include <errno.h>
#include <fcntl.h>                      /* for O_RDONLY */
#include <netinet/in.h>                 /* for ntohs() */
#include <stdio.h>
#include <stdlib.h>                     /* for exit(), strtoul(), ... */
#include <string.h>                     /* for str...() */
#include <sys/types.h>
#include <unistd.h>                     /* for lseek(), read(), ... */

#define BUF_SIZE    16                  /* bytes displayed on a line */

/* exit(3) status codes */
#define EXIT_OK             0
#define EXIT_USAGE          1
#define EXIT_READ_OPEN      10
#define EXIT_READ_ERROR     11
#define EXIT_SEEK_ERROR     12

#define BLOCK(...) \
  do { __VA_ARGS__ } while (0)

#define PMESSAGE_EXIT(STATUS,FORMAT,...) \
  BLOCK( fprintf( stderr, "%s: " FORMAT, me, __VA_ARGS__ ); exit( EXIT_##STATUS ); )

typedef enum {
  OFMT_DEC,
  OFMT_HEX,
  OFMT_OCT
} OFFSET_FMT;

static char const*    base_name( char const* );
static void           dump( off_t, OFFSET_FMT, char const[], size_t );
static int            open_file( char const*, off_t );
static unsigned long  parse_number( char const* );
static void           skip_stdin( off_t );
static void           usage();

char const *me;                         /* executable name */

/*****************************************************************************/

int main( int argc, char *argv[] ) {
  OFFSET_FMT  offset_fmt = OFMT_HEX;
  int         opt;                      /* command-line option */
  char const  opts[] = "dhj:N:ov";
  size_t      opt_max_bytes_to_read = SIZE_MAX;

  char        buf[ BUF_SIZE ];
  ssize_t     bytes_read;
  size_t      bytes_to_read = BUF_SIZE;
  int         fd = STDIN_FILENO;        /* Unix file descriptor */
  off_t       offset = 0;               /* offset into file */
  char const *path_name;
  size_t      total_bytes_read = 0;

  /***************************************************************************/

  me = base_name( argv[0] );

  opterr = 1;
  while ( (opt = getopt( argc, argv, opts )) != EOF ) {
    switch ( opt ) {
      case 'd': offset_fmt = OFMT_DEC;                          break;
      case 'h': offset_fmt = OFMT_HEX;                          break;
      case 'j': offset += parse_number( optarg );               break;
      case 'N': opt_max_bytes_to_read = parse_number( optarg ); break;
      case 'o': offset_fmt = OFMT_OCT;                          break;
      case 'v': fprintf( stderr, "%s\n", PACKAGE_STRING );      exit( EXIT_OK );
      default : usage();
    } /* switch */
  } /* while */
  argc -= optind, argv += optind - 1;

  switch ( argc ) {
    case 0:                             /* read from stdin with no offset */
      break;

    case 1:                             /* offset OR file */
      if ( *argv[1] == '+' ) {
        path_name = "<stdin>";
        skip_stdin( parse_number( argv[1] ) );
      } else {
        path_name = argv[1];
        fd = open_file( path_name, offset );
      }
      break;

    case 2:                             /* offset & file OR file & offset */
      if ( *argv[1] == '+' ) {
        if ( *argv[2] == '+' )
          PMESSAGE_EXIT( USAGE,
            "'%c': can not specify for more than one argument\n", '+'
          );
        offset += parse_number( argv[1] );
        path_name = argv[2];
      } else {
        path_name = argv[1];
        offset += parse_number( argv[2] );
      }
      fd = open_file( path_name, offset );
      break;

    default:
      usage();
  } /* switch */

  /***************************************************************************/

  if ( bytes_to_read > opt_max_bytes_to_read )
    bytes_to_read = opt_max_bytes_to_read;

  for ( ;; ) {
    if ( total_bytes_read + bytes_to_read > opt_max_bytes_to_read )
      bytes_to_read = opt_max_bytes_to_read - total_bytes_read;

    bytes_read = read( fd, buf, bytes_to_read );
    if ( bytes_read == -1 )
      PMESSAGE_EXIT( READ_ERROR,
        "\"%s\": read failed: %s", path_name, strerror( errno )
      );
    if ( bytes_read == 0 )
      break;

    dump( offset, offset_fmt, buf, bytes_read );

    total_bytes_read += bytes_read;
    if ( total_bytes_read >= opt_max_bytes_to_read )
      break;
    if ( bytes_read < BUF_SIZE )
      break;
    offset += bytes_read;
  } /* for */

  /***************************************************************************/

  close( fd );
  exit( EXIT_OK );
}

/*****************************************************************************/

static char const* base_name( char const *path_name ) {
  assert( path_name );
  char const *const slash = strrchr( path_name, '/' );
  if ( slash )
    return slash[1] ? slash + 1 : slash;
  return path_name;
}

static void dump( off_t offset, OFFSET_FMT offset_fmt, char const buf[],
                  size_t bytes_read ) {
  static char const *const offset_fmt_printf[] = {
    "%016llu: ",                        /* decimal */
    "%016llX: ",                        /* hex */
    "%016llo: ",                        /* octal */
  };

  uint16_t const *pword;
  char const *pchar;
  size_t i;

  /* print offset */
  printf( offset_fmt_printf[ offset_fmt ], offset );

  /* print hex part */
  for ( i = bytes_read / 2, pword = (uint16_t const*)buf; i; --i, ++pword )
    printf( "%04X ", ntohs( *pword ) );
  if ( bytes_read & 1 )               /* there's an odd, left-over byte */
    printf( "%02X ", (uint16_t)*(uint8_t const*)pword );

  /* print padding if we read less than BUF_SIZE bytes */
  for ( i = BUF_SIZE - bytes_read; i; --i )
    printf( "  " );
  for ( i = (BUF_SIZE - bytes_read) / 2 + 1; i; --i )
    putchar( ' ' );

  /* print ASCII part */
  for ( pchar = buf; pchar < buf + bytes_read; ++pchar )
    putchar( isprint( *pchar ) ? *pchar : '.' );

  putchar( '\n' );
}

static int open_file( char const *path_name, off_t offset ) {
  int const fd = open( path_name, O_RDONLY );
  if ( fd == -1 )
    PMESSAGE_EXIT( READ_OPEN,
      "\"%s\": can not open: %s\n",
      path_name, strerror( errno )
    );
  if ( offset && lseek( fd, offset, 0 ) == -1 )
    PMESSAGE_EXIT( SEEK_ERROR,
      "\"%s\": can not seek to offset %ld: %s\n",
      path_name, (long)offset, strerror( errno )
    );
  return fd;
}

unsigned long parse_number( char const *s ) {
  assert( s );
  char *end = NULL;
  errno = 0;
  unsigned long n = strtoul( s, &end, 0 );
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
  PMESSAGE_EXIT( USAGE, "\"%s\": invalid integer\n", s );
}

static void skip_stdin( off_t bytes_to_skip ) {
  char buf[ 8192 ];
  ssize_t bytes_read;
  off_t bytes_to_read = sizeof( buf );

  for ( ;; ) {
    if ( bytes_to_read > bytes_to_skip )
      bytes_to_read = bytes_to_skip;
    if ( !bytes_to_read )
      break;
    bytes_read = read( STDIN_FILENO, buf, bytes_to_read );
    if ( bytes_read == -1 )
      PMESSAGE_EXIT( READ_OPEN,
        "\"<stdin>\": can not read: %s\n",
        strerror( errno )
      );
    bytes_to_skip -= bytes_read;
  } /* for */
}

static void usage() {
  fprintf( stderr,
"usage: %s [-dhov] [-j offset[b|k|m]] [-N length] [file] [[+]offset[b|k|m]]\n"
"       %s [-dhov] [-j offset[b|k|m]] [-N length] [+offset[b|k|m]] [file]\n"
    , me, me
  );
  exit( EXIT_USAGE );
}
/* vim:set et sw=2 ts=2: */
