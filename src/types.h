/*
**      ad -- ASCII dump
**      src/types.h
**
**      Copyright (C) 2019  Paul J. Lucas
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

#ifndef ad_types_H
#define ad_types_H

// local
#include "pjl_config.h"                 /* must go first */
#include "slist.h"

// standard
#include <stdbool.h>
#include <stdint.h>

#if !HAVE_CHAR16_T
typedef uint16_t char16_t;
#endif /* !HAVE_CHAR16_T */
#if !HAVE_CHAR32_T
typedef uint32_t char32_t;
#endif /* !HAVE_CHAR32_T */

///////////////////////////////////////////////////////////////////////////////

#define T_08_BITS               0x0001u               /**< 8-bit type.        */
#define T_16_BITS               0x0002u               /**< 16-bit type.       */
#define T_32_BITS               0x0004u               /**< 32-bit type.       */
#define T_64_BITS               0x0008u               /**< 64-bit type.       */

#define T_SIGNED                0x8000u               /**< Signed type.       */

// types
#define T_NONE                  0u                    /**< No type.           */

#define T_BOOL                  0x0010                /**< Boolean.           */
#define T_BOOL8   (             T_BOOL  | T_08_BITS ) /**< `bool`             */

#define T_UTF                   0x0020                /**< Unicode.           */
#define T_UTF8    (             T_UTF   | T_08_BITS ) /**< UTF-8 (multibyte). */
#define T_UTF16   (             T_UTF   | T_16_BITS ) /**< `char16_t`         */
#define T_UTF32   (             T_UTF   | T_32_BITS ) /**< `char32_t`         */

#define T_INT     (             0x0040              ) /**< Integer.           */
#define T_INT8    ( T_SIGNED |  T_INT   | T_08_BITS ) /**< `signed int8`      */
#define T_INT16   ( T_SIGNED |  T_INT   | T_16_BITS ) /**< `signed int16`     */
#define T_INT32   ( T_SIGNED |  T_INT   | T_32_BITS ) /**< `signed int32`     */
#define T_INT64   ( T_SIGNED |  T_INT   | T_64_BITS ) /**< `signed int64`     */
#define T_UINT8   (             T_INT   | T_08_BITS ) /**< `unsigned int8`    */
#define T_UINT16  (             T_INT   | T_16_BITS ) /**< `unsigned int16`   */
#define T_UINT32  (             T_INT   | T_32_BITS ) /**< `unsigned int32`   */
#define T_UINT64  (             T_INT   | T_64_BITS ) /**< `unsigned int64`   */

#define T_FLOAT                 0x0080                /**< Floating point.    */
#define T_FLOAT32 ( T_SIGNED |  T_FLOAT | T_32_BITS ) /**< `float32`          */
#define T_FLOAT64 ( T_SIGNED |  T_FLOAT | T_64_BITS ) /**< `float64`          */

#define T_STRUCT                0x0100                /**< `struct`           */
#define T_SWITCH                0x0200                /**< `switch`           */

#define T_NUMBER  (T_BOOL | T_INT | T_FLOAT)

// bit masks
#define T_MASK_BITS             0x000Fu               /**< Bits bitmask.      */
#define T_MASK_SIGN             T_SIGNED              /**< Sign bitmask.      */
#define T_MASK_TYPE             0x0FF0u               /**< Type bitmask.      */

///////////////////////////////////////////////////////////////////////////////

typedef struct  ad_char     ad_char_t;
typedef struct  ad_expr     ad_expr_t;
typedef struct  ad_float    ad_float_t;
typedef struct  ad_int      ad_int_t;
typedef enum    ad_int_base ad_int_base_t;
typedef struct  ad_struct   ad_struct_t;
typedef struct  ad_switch   ad_switch_t;
typedef struct  slist       ad_switch_cases_t;
typedef struct  ad_type     ad_type_t;
typedef struct  ad_utf      ad_utf_t;
typedef uint16_t            ad_type_id_t;
typedef unsigned short      ad_type_size_t;
typedef struct   slist      ad_type_list_t;

/**
 * Floating-point type.
 */
struct ad_float {
  ad_type_size_t  bits;
  ad_endian_t     endian;
};

/**
 * Integer base.
 */
enum ad_int_base {
  BASE_NONE,
  BASE_BIN =  2,
  BASE_OCT =  8,
  BASE_DEC = 10,
  BASE_HEX = 16
};

/**
 * Integer type.
 */
struct ad_int {
  ad_type_size_t  bits;
  ad_endian_t     endian;
  ad_int_base_t   base;
};

/**
 * struct type.
 */
struct ad_struct {
  ad_type_list_t  members;              ///< Structure members.
};

/**
 * `switch` `case` type.
 */
struct ad_switch_case {
  // value
  ad_type_list_t  case_types;           ///< All the type(s) for the case value.
};

/**
 * `switch` type.
 */
struct ad_switch {
  // field switching on
  ad_switch_cases_t cases;
};

/**
 * UTF type.
 */
struct ad_utf {
  ad_type_size_t  bits;
  ad_endian_t     endian;
};

/**
 * A type.  Every %ad_type at least has the ID that it's a type of and a size
 * in bits.
 */
struct ad_type {
  ad_type_id_t    type_id;

  union {
    // nothing needed for T_BOOL
    // nothing needed for T_ASCII
    ad_utf_t      ad_utf;
    // nothing needed fot T_UTF8
    ad_int_t      ad_int;
    ad_float_t    ad_float;
    ad_struct_t   ad_struct;
    ad_switch_t   ad_switch;
  } as;                                 ///< Union discriminator.
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Frees all the memory used by \a type.
 *
 * @param type The `ad_type` to free.  May be null.
 */
void ad_type_free( ad_type_t *type );

ad_type_t* ad_type_new( ad_type_id_t tid );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_types_H */
/* vim:set et sw=2 ts=2: */
