/*
**      ad -- ASCII dump
**      common.h
**
**      Copyright (C) 2015-2017  Paul J. Lucas
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

#ifndef ad_common_H
#define ad_common_H

///////////////////////////////////////////////////////////////////////////////

#define ELIDED_SEP_CHAR           '-'
#define EX_NO_MATCHES             1     /* no errors, but no matches either */
#define GROUP_BY_DEFAULT          2     /* how many bytes to group together */
#define OFFSET_WIDTH_MIN          12    /* minimum number of offset digits */
#define OFFSET_WIDTH_MAX          16    /* maximum number of offset digits */
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
