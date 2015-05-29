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
#include "util.h"

/* system */
#include <ctype.h>                      /* for isprint() */
#include <errno.h>
#include <fcntl.h>                      /* for O_RDONLY */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/types.h>
#include <unistd.h>                     /* for lseek(), read(), ... */
#include <netinet/in.h>                 /* for ntohs() */

#define BUF_SIZE    16                  /* bytes displayed on a line */

typedef uint8_t char_type;
typedef uint16_t word_type;

static void skip( int, off_t );
static void usage();

char const *file_name = "<stdin>";
char const *me;                         /* executable name */

/*****************************************************************************/

int main( int argc, char *argv[] ) {
  char const *const offset_format[] = { /* offset format in printf() */
    "%016llu: ",                        /* 0: decimal */
    "%016llX: ",                        /* 1: hex */
    "%016llo: ",                        /* 2: octal */
  };
  int         offset_format_index = 1;  /* default to hex */
  int         opt;                      /* command-line option */
  char const  opts[] = "dhN:ov";
  size_t      opt_max_bytes_to_read = SIZE_MAX;
  bool        plus = false;             /* specified '+'? */

  char_type   buf[ BUF_SIZE ];
  ssize_t     bytes_read;
  size_t      bytes_to_read = BUF_SIZE;
  int         fd = STDIN_FILENO;        /* Unix file descriptor */
  off_t       offset = 0;               /* offset into file */
  size_t      total_bytes_read = 0;

  /***************************************************************************/

  me = base_name( argv[0] );

  opterr = 1;
  while ( (opt = getopt( argc, argv, opts )) != EOF ) {
    switch ( opt ) {
      case 'd': offset_format_index = 0;                          break;
      case 'h': offset_format_index = 1;                          break;
      case 'N': opt_max_bytes_to_read = check_strtoul( optarg );  break;
      case 'o': offset_format_index = 2;                          break;
      case 'v': fprintf( stderr, "%s\n", PACKAGE_STRING ); exit( EXIT_OK );
      default : usage();
    } /* switch */
  } /* while */
  argc -= optind, argv += optind - 1;

  switch ( argc ) {

    case 0:
      /*
      ** No arguments: read from stdin with no offset.
      */
      break;

    case 1:
      plus = (*argv[1] == '+');
      if ( plus ) {
        /*
        ** The argument has a leading '+' indicating that it is an offset
        ** rather than a file name and that we should read from stdin.
        ** However, we can't seek() on stdin, so read and discard offset bytes.
        */
        skip( fd, check_strtoul( argv[1] ) );
        break;
      }
      /* no break; */

    case 2:
      if ( argc == 2 ) {
        /*
        ** There really are two arguments (we didn't fall through from the
        ** above case): the 2nd argument is the offset.
        */
        offset = check_strtoul( argv[2] );

      } else if ( plus ) {
        /*
        ** There is only one argument (we fell through from the above case) and
        ** a '+' was given, hence that single argument was an offset and we
        ** should read from stdin.
        */
        break;
      }

      /*
      ** The first (and perhaps only) argument is a file name: open the file
      ** and seek to the proper offset.
      */
      file_name = argv[1];
      if ( (fd = open( file_name, O_RDONLY )) == -1 )
        PMESSAGE_EXIT( READ_OPEN,
          "\"%s\": can not open: %s\n",
          file_name, strerror( errno )
        );
      if ( lseek( fd, offset, 0 ) == -1 )
        PMESSAGE_EXIT( SEEK,
          "\"%s\": can not seek to offset %ld: %s\n",
          file_name, (long)offset, strerror( errno )
        );
      break;

    default:
      usage();
  } /* switch */

  /***************************************************************************/

  if ( bytes_to_read > opt_max_bytes_to_read )
    bytes_to_read = opt_max_bytes_to_read;

  while ( true ) {
    word_type const *pword;
    char_type const *pchar;
    ssize_t i;

    if ( total_bytes_read + bytes_to_read > opt_max_bytes_to_read )
      bytes_to_read = opt_max_bytes_to_read - total_bytes_read;

    bytes_read = read( fd, buf, bytes_to_read );
    if ( bytes_read == -1 )
      PMESSAGE_EXIT( READ_ERROR,
        "\"%s\": read failed: %s", file_name, strerror( errno )
      );
    if ( bytes_read == 0 )
      break;

    /* print offset */
    printf( offset_format[ offset_format_index ], offset );

    /* print hex part */
    for ( i = bytes_read / 2, pword = (word_type const*)buf; i; --i, ++pword )
      printf( "%04X ", ntohs( *pword ) );
    if ( bytes_read & 1 )               /* there's an odd, left-over byte */
      printf( "%02X ", (word_type)*(char_type const*)pword );

    /* print padding if we read less than BUF_SIZE bytes */
    for ( i = BUF_SIZE - bytes_read; i; --i )
      printf( "  " );
    for ( i = (BUF_SIZE - bytes_read) / 2; i; --i )
      PUTCHAR( ' ' );

    PUTCHAR( ' ' );

    /* print ASCII part */
    for ( i = 0, pchar = (char_type const*)buf; i < bytes_read; ++i, ++pchar )
      PUTCHAR( isprint( *pchar ) ? (char)*pchar : '.' );

    PUTCHAR( '\n' );

    total_bytes_read += bytes_read;
    if ( total_bytes_read >= opt_max_bytes_to_read )
      break;
    if ( bytes_read < BUF_SIZE )
      break;
    offset += bytes_read;
  } /* while */

  /***************************************************************************/

  close( fd );
  exit( EXIT_OK );
}

/*****************************************************************************/

static void skip( int fd, off_t skip_size ) {
  char buf[ 8192 ];
  ssize_t bytes_read;
  off_t read_size = sizeof( buf );

  while ( true ) {
    if ( read_size > skip_size )
      read_size = skip_size;
    if ( !read_size )
      break;
    bytes_read = read( fd, buf, read_size );
    if ( bytes_read == -1 )
      PMESSAGE_EXIT( READ_OPEN,
        "\"%s\": can not open: %s\n",
        file_name, strerror( errno )
      );
    skip_size -= bytes_read;
  } /* while */
}

static void usage() {
  fprintf( stderr, "usage: %s [-dhov] [-N length] [file] [[+]offset]\n", me );
  exit( EXIT_USAGE );
}
/* vim:set et sw=2 ts=2: */
