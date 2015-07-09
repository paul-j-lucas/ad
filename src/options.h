/*
**      ad -- ASCII dump
**      options.h
**
**      Copyright (C) 2015  Paul J. Lucas
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

// local
#include "util.h"                       /* for bool */

// system
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */
#include <stdio.h>                      /* for FILE */
#include <sys/types.h>                  /* for off_t */

///////////////////////////////////////////////////////////////////////////////

/**
 * C dump formats.
 */
enum c_fmt {
  CFMT_CONST    = 1 << 0,               // declare variables as "const"
  CFMT_STATIC   = 1 << 1,               // declare variables as "static"
  CFMT_UNSIGNED = 1 << 2,               // declare len type as "unsigned"
  CFMT_INT      = 1 << 3,               // declare len type as "int"
  CFMT_LONG     = 1 << 4,               // declare len type as "long"
  CFMT_SIZE_T   = 1 << 5                // declare len type as "size_t"
};

/**
 * Offset formats.
 */
enum offset_fmt {
  OFMT_DEC = 10,
  OFMT_HEX = 16,
  OFMT_OCT =  8
};
typedef enum offset_fmt offset_fmt_t;

////////// extern variables ///////////////////////////////////////////////////

extern FILE          *fin;              // file to read from
extern off_t          fin_offset;       // curent offset into file
extern char const    *fin_path;         // path name of input file
extern FILE          *fout;             // file to write to
extern char const    *fout_path;        // path name of output file
extern char const    *me;               // executable name from argv[0]

extern bool           opt_case_insensitive;
extern unsigned       opt_c_fmt;
extern size_t         opt_max_bytes_to_read;
extern offset_fmt_t   opt_offset_fmt;
extern bool           opt_only_matching;
extern bool           opt_only_printing;
extern bool           opt_reverse;
extern bool           opt_verbose;

extern char          *search_buf;       // not NULL-terminated when numeric
extern endian_t       search_endian;    // if searching for a number
extern size_t         search_len;       // number of bytes in search_buf
extern uint64_t       search_number;    // the number to search for

////////// extern functions ///////////////////////////////////////////////////

/**
 * Parses command-line options and sets global variables.
 *
 * @param argc The argument count from \c main().
 * @param argv The argument values from \c main().
 */
void parse_options( int argc, char *argv[] );

/**
 * Prints the usage message to standard error and exits.
 */
void usage( void );

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
