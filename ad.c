#define VERSION "1.1"
/*
**      ad -- ASCII dump
**      ad.c: implementation
**
**      Copyright (C) 1996  Paul J. Lucas
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

#include <ctype.h>                      /* for isprint() */
#include <fcntl.h>                      /* for O_RDONLY */
#include <stdio.h>
#include <stdlib.h>                     /* for exit() */
#include <string.h>                     /* for str...() */
#include <sys/types.h>
#include <unistd.h>                     /* for lseek(), read(), ... */

#define BUF_SIZE    16                  /* bytes displayed on a line */

#if !( defined(__STDC__) || defined(__cplusplus) )
#   define KNR_C
#endif

#ifdef KNR_C
#   define const                       /* throw away const */
#endif

typedef unsigned short ad_uint16;

/*************************/
    ad_uint16
#ifdef KNR_C
flip_int16( i ) ad_uint16 i; {
#else
flip_int16( ad_uint16 i ) {
#endif
    return (i << 8) & 0xFF00 |
           (i >> 8) & 0x00FF;
}

/*************************/
    int
#ifdef KNR_C
main( argc, argv ) int argc; char *argv[]; {
#else
main( int argc, char *argv[] ) {
#endif
    char const  *me;                    /* our name */
    extern char *optarg;
    extern int  optind, opterr;
    int         opt;                    /* command-line option */

    char const *const format_spec[] = { /* offset format in printf() */
        "%016llu: ",                    /* 0: decimal */
        "%016llX: ",                    /* 1: hex */
        "%016llo: ",                    /* 2: octal */
    };
    char const  options[] = "dhov";     /* dho in same order as above */
    int         which_format = strchr( options, 'h' ) - options;
    int         plus = 0;               /* specified '+'? */

    int         fd = 0;                 /* Unix file descriptor */
    off_t       offset = 0L;            /* offset into file */
    u_char      buf[ BUF_SIZE ];
    size_t      bytes_read;
    int         little_endian;
    int         i;

    /* strip pathname from argv[0] */
    if ( me = strrchr( argv[0], '/' ) )
        ++me;
    else
        me = argv[0];

    /* ---------- determine endianness ---------- */

    union {
        ad_uint16 s;
        u_char    c[ sizeof( short ) ];
    } u;
    for ( i = 0; i < sizeof( ad_uint16 ); ++i )
        u.c[i] = i + 1;
    little_endian = u.s == 0x0201;

    /* ---------- process command line ---------- */

    opterr = 1;
    while ( ( opt = getopt( argc, argv, options ) ) != EOF )
        switch ( opt ) {
            case 'd':
            case 'o':
            case 'h':
                which_format = strchr( options, opt ) - options;
                break;
            case 'v': goto version;
            case '?': goto usage;
        }
    argc -= optind, argv += optind - 1;

    switch ( argc ) {

        case 0:
            /*
            ** No arguments: read from stdin with no offset.
            */
            break;

        case 1:
            if ( plus = *argv[1] == '+' ) {
                /*
                ** The argument has a leading '+' indicating that it is an
                ** offset rather than a file name and that we should read from
                ** stdin.  However, we can't seek() on stdin, so read and
                ** discard offset bytes.
                */
                size_t size = BUF_SIZE;
                offset = strtol( argv[1], (char**)0, 0 );
                for ( ;; ) {
                    if ( size > offset )
                        size = offset;
                    if ( !size )
                        break;
                    bytes_read = read( fd, buf, size );
                    if ( bytes_read == -1 )
                        break;
                    offset -= bytes_read;
                }
                break;
            }
            /* no break; */

        case 2:
            if ( argc == 2 ) {
                /*
                ** There really are two arguments (we didn't fall through from
                ** the above case): the 2nd argument is the offset.
                */
                offset = strtol( argv[2], (char**)0, 0 );

            } else if ( plus ) {
                /*
                ** There is only one argument (we fell through from the above
                ** case) and a '+' was given, hence that single argument was an
                ** offset and we should read from stdin.
                */
                break;
            }

            /*
            ** The first (and perhaps only) argument is a file name: open the
            ** file and seek to the proper offset.
            */
            if ( ( fd = open( argv[1], O_RDONLY ) ) == -1 ) {
                fprintf( stderr, "%s: can not open %s\n", me, argv[1] );
                exit( 2 );
            }
            if ( lseek( fd, offset, 0 ) == -1 ) {
                fprintf( stderr, "%s: can not seek to %ld\n", me, offset );
                exit( 3 );
            }
            break;

        default:
            goto usage;
    }

    /* ---------- main processing ---------- */

    while ( ( bytes_read = read( fd, buf, BUF_SIZE ) ) > 0 ) {

        register ad_uint16 const *pword = (ad_uint16*)buf;
        register u_char    const *pchar = (u_char*)buf;
        register int i;

        /* print offset */
        printf( format_spec[ which_format ], offset );

        /* print hex part */
        for ( i = bytes_read / 2; i; --i, ++pword )
            printf( "%04X ", little_endian ? flip_int16( *pword ) : *pword );
        if ( bytes_read & 1 ) {
            /* there's an odd, left-over byte */
            printf( "%02X ", (ad_uint16)*(u_char*)pword );
        }

        /* print padding if we read less than BUF_SIZE bytes */
        for ( i = BUF_SIZE - bytes_read; i; --i )
            printf( "  " );
        for ( i = (BUF_SIZE - bytes_read) / 2; i; --i )
            putchar( ' ' );

        /* separator */
        putchar( ' ' );

        /* print ascii part */
        for ( i = 0; i < bytes_read; ++i, ++pchar )
            putchar( (char)( isprint( *pchar ) ? *pchar : '.' ) );

        putchar( '\n' );

        if ( bytes_read < BUF_SIZE )
            break;

        offset += BUF_SIZE;
    }

    /* ---------- done ---------- */

    close( fd );
    exit( 0 );

usage:
    fprintf( stderr, "usage: %s [-dhov] [ file ] [ [+]offset ]\n", me );
    exit( 1 );

version:
    fprintf( stderr, "%s: version %s\n", me, VERSION );
    exit( 0 );
}
/* vim:set et sw=4 ts=4: */
