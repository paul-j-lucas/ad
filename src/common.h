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

///////////////////////////////////////////////////////////////////////////////

#define EXIT_OK             0           /* no errors */
#define EXIT_NO_MATCHES     1           /* no errors, but no matches either */
#define EXIT_USAGE          2           /* command-line usage error */
#define EXIT_OUT_OF_MEMORY  3
#define EXIT_READ_OPEN      10          /* error opening file for reading */
#define EXIT_READ_ERROR     11          /* error reading */
#define EXIT_WRITE_ERROR    13          /* error writing */
#define EXIT_SEEK_ERROR     20          /* error seeking */
#define EXIT_STAT_ERROR     21          /* error stat'ing */

#define ROW_BUF_SIZE        16          /* bytes displayed in a row */

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
