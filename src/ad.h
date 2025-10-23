/*
**      ad -- ASCII dump
**      src/ad.h
**
**      Copyright (C) 2015-2025  Paul J. Lucas
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

/**
 * @file
 * Declares miscellaneous macros, types, and global variables.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stddef.h>                     /* for size_t */
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * **ad** primary author.
 */
#define AD_AUTHOR                 "Paul J. Lucas"

/**
 * **ad** latest copyright year.
 */
#define AD_COPYRIGHT_YEAR         "2025"

/**
 * **ad** license.
 *
 * @sa #AD_LICENSE_URL
 */
#define AD_LICENSE                "GPLv3+: GNU GPL version 3 or later"

/**
 * **ad** license URL.
 *
 * @sa #AD_LICENSE
 */
#define AD_LICENSE_URL            "https://gnu.org/licenses/gpl.html"

#define ELIDED_SEP_CHAR           '-'   /**< Elided row separator character. */
#define EX_NO_MATCHES             1     /**< Exit status for no matches. */
#define GROUP_BY_DEFAULT          2     /**< Bytes to group together. */
#define OFFSET_WIDTH_MIN          12    /**< Minimum offset digits. */
#define OFFSET_WIDTH_MAX          16    /**< Maximum offset digits. */
#define ROW_BYTES_DEFAULT         16    /**< Default bytes dumped on a row. */
#define ROW_BYTES_C               8     /**< Bytes dumped on a row in C. */
#define ROW_BYTES_MAX             32    /**< Maximum bytes dumped on a row. */
#define STRINGS_LEN_DEFAULT       4     /**< Default **strings**(1) length. */

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

extern off_t        fin_offset;         ///< Current input file offset.
extern char const  *fin_path;           ///< Input file path name.
extern char const  *prog_name;          ///< Program name.
extern unsigned     row_bytes;          ///< Bytes dumped on a row.

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_ad_H */
/* vim:set et sw=2 ts=2: */
