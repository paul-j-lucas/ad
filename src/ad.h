/*
**      ad -- ASCII dump
**      src/ad.h
**
**      Copyright (C) 2015-2021  Paul J. Lucas
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

#ifndef ad_ad_H
#define ad_ad_H

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stddef.h>                     /* for size_t */
#include <stdint.h>                     /* for uint64_t */
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

#define ELIDED_SEP_CHAR           '-'
#define EX_NO_MATCHES             1     /**< No errors, but no matches. */
#define GROUP_BY_DEFAULT          2     /**< Bytes to group together. */
#define OFFSET_WIDTH_MIN          12    /**< Minimum offset digits. */
#define OFFSET_WIDTH_MAX          16    /**< Maximum offset digits. */
#define ROW_BYTES_DEFAULT         16    /**< Default bytes dumped on a row. */
#define ROW_BYTES_C               8     /**< Bytes dumped on a row in C. */
#define ROW_BYTES_MAX             32    /**< Maximum bytes dumped on a row. */

/**
 * Byte endian order.
 */
enum endian {
  ENDIAN_NONE,                          ///< No-endian order.
  ENDIAN_LITTLE,                        ///< Little-endian order.
  ENDIAN_BIG,                           ///< Big-endian order.
  ENDIAN_HOST                           ///< Host-endian order.
};
typedef enum endian endian_t;

extern FILE        *fin;                ///< Input file.
extern off_t        fin_offset;         ///< Curent offset into fin.
extern char const  *fin_path;           ///< Path name of fin.
extern FILE        *fout;               ///< Output file.
extern char const  *fout_path;          ///< Path name of fout.
extern char const  *me;                 ///< Executable name from argv[0].
extern size_t       row_bytes;          ///< Bytes dumped on a row.
extern char        *search_buf;         ///< Not NULL-terminated when numeric.
extern endian_t     search_endian;      ///< Numeric search endianness.
extern size_t       search_len;         ///< Number of bytes in search_buf.
extern uint64_t     search_number;      ///< The number to search for.

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_ad_H */
/* vim:set et sw=2 ts=2: */
