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

/*
**      Options:
**
**          -d  Output offset in decimal.
**          -o  Output offset in octal.
**          -h  Output offset in hexadecimal (default).
**          -v  Print version to stderr and exit.
*/

/* local */
#include "util.h"

/* system */
#include <ctype.h>                      /* for isprint() */
#include <fcntl.h>                      /* for O_RDONLY */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/types.h>
#include <unistd.h>                     /* for lseek(), read(), ... */
#include <netinet/in.h>                 /* for htons() */

#define BUF_SIZE    16                  /* bytes displayed on a line */

static void skip( int, off_t );
static void usage();

char const *me;                         /* executable name */

/*****************************************************************************/

int main( int argc, char *argv[] ) {
  extern char *optarg;
  extern int  optind, opterr;
  int         opt;                      /* command-line option */

  char const *const format_spec[] = {   /* offset format in printf() */
    "%016llu: ",                        /* 0: decimal */
    "%016llX: ",                        /* 1: hex */
    "%016llo: ",                        /* 2: octal */
  };
  char const  opts[] = "dhov";          /* dho in same order as above */
  int         which_format = strchr( opts, 'h' ) - opts;
  bool        plus = false;             /* specified '+'? */

  int         fd = 0;                   /* Unix file descriptor */
  off_t       offset = 0;               /* offset into file */
  uint8_t     buf[ BUF_SIZE ];
  ssize_t     bytes_read;

  /***************************************************************************/

  me = base_name( argv[0] );

  opterr = 1;
  while ( (opt = getopt( argc, argv, opts )) != EOF ) {
    switch ( opt ) {
      case 'd':
      case 'o':
      case 'h':
        which_format = strchr( opts, opt ) - opts;
        break;
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
        skip( fd, check_atoul( argv[1] ) );
        break;
      }
      /* no break; */

    case 2:
      if ( argc == 2 ) {
        /*
        ** There really are two arguments (we didn't fall through from the
        ** above case): the 2nd argument is the offset.
        */
        offset = check_atoul( argv[2] );

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
      if ( (fd = open( argv[1], O_RDONLY )) == -1 )
        PMESSAGE_EXIT( READ_OPEN, "\"%s\": can not open", argv[1] );
      if ( lseek( fd, offset, 0 ) == -1 )
        PMESSAGE_EXIT( SEEK,
          "\"%s\": can not seek to %ld", argv[1], (long)offset
        );
      break;

    default:
      usage();
  } /* switch */

  /***************************************************************************/

  while ( (bytes_read = read( fd, buf, BUF_SIZE )) > 0 ) {
    uint16_t const *pword = (uint16_t*)buf;
    uint8_t  const *pchar = (uint8_t*)buf;
    ssize_t i;

    /* print offset */
    printf( format_spec[ which_format ], offset );

    /* print hex part */
    for ( i = bytes_read / 2; i; --i, ++pword )
      printf( "%04X ", htons( *pword ) );
    if ( bytes_read & 1 ) {
      /* there's an odd, left-over byte */
      printf( "%02X ", (uint16_t)*(uint8_t*)pword );
    }

    /* print padding if we read less than BUF_SIZE bytes */
    for ( i = BUF_SIZE - bytes_read; i; --i )
      printf( "  " );
    for ( i = (BUF_SIZE - bytes_read) / 2; i; --i )
      PUTCHAR( ' ' );

    /* separator */
    PUTCHAR( ' ' );

    /* print ascii part */
    for ( i = 0; i < bytes_read; ++i, ++pchar )
      PUTCHAR( (char)( isprint( *pchar ) ? *pchar : '.' ) );

    PUTCHAR( '\n' );

    if ( bytes_read < BUF_SIZE )
      break;

    offset += BUF_SIZE;
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
      break;
    skip_size -= bytes_read;
  } /* while */
}

static void usage() {
  fprintf( stderr, "usage: %s [-dhov] [file] [ [+]offset ]\n", me );
  exit( EXIT_USAGE );
}
/* vim:set et sw=2 ts=2: */
