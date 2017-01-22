/*
**      ad -- ASCII dump
**      common.h
**
**      Copyright (C) 2015-2017  Paul J. Lucas
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

///////////////////////////////////////////////////////////////////////////////

#define EX_NO_MATCHES             1     /* no errors, but no matches either */
#define OFFSET_WIDTH              16    /* number of offset digits */
#define ROW_SIZE                  16    /* bytes dumped on a row */
#define ROW_SIZE_C                8     /* bytes dumped on a row in C */

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
