/*
**      ad -- ASCII dump
**      common.h
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

#ifndef ad_common_H
#define ad_common_H

// local
#include "config.h"

// standard
#ifdef HAVE_SYSEXITS_H
# include <sysexits.h>
#endif /* HAVE_SYSEXITS_H */

///////////////////////////////////////////////////////////////////////////////

#define EX_NO_MATCHES       1           /* no errors, but no matches either */
#ifndef HAVE_SYSEXITS_H
# define EX_OK              0           /* success */
# define EX_USAGE           64          /* command-line usage error */
# define EX_DATAERR         65          /* invalid dump format for -r option */
# define EX_NOINPUT         66          /* opening file error */
# define EX_OSERR           71          /* system error (e.g., can't fork) */
# define EX_CANTCREAT       73          /* creating file error */
# define EX_IOERR           74          /* input/output error */
#endif /* HAVE_SYSEXITS_H */

#define OFFSET_WIDTH        16          /* number of offset digits */
#define ROW_SIZE            16          /* bytes dumped on a row */
#define ROW_SIZE_C          8           /* bytes dumped on a row in C */

/**
 * The endian order for numeric searches.
 */
enum endian {
  ENDIAN_UNSPECIFIED,
  ENDIAN_BIG,
  ENDIAN_LITTLE
};
typedef enum endian endian_t;

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_common_H */
/* vim:set et sw=2 ts=2: */
