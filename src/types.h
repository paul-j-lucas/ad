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

_GL_INLINE_HEADER_BEGIN
#ifndef AD_TYPES_INLINE
# define AD_TYPES_INLINE _GL_INLINE
#endif /* AD_TYPES_INLINE */

///////////////////////////////////////////////////////////////////////////////

//
// The bits are used as follows:
//
//    S-TT TTTT EZZZ ZDDN
//    F-DC BA98 7654 3210
//
// where:
//
//    D = Endianness: 0 = host, 1 = little, 2 = big
//    E = Error: 1 = error
//    N = null terminated: 0 = no, 1 = yes
//    S = sign: 0 = unsigned, 1 = signed
//    T = type (at most one set)
//    Z = size in bits (at most one set)
//

#define T_08_BITS               0x0008u               /**< 8-bit type.        */
#define T_16_BITS               0x0010u               /**< 16-bit type.       */
#define T_32_BITS               0x0020u               /**< 32-bit type.       */
#define T_64_BITS               0x0040u               /**< 64-bit type.       */

#define T_SIGNED                0x8000u               /**< Signed type.       */
#define T_END_HOST              0x0000u               /**< Host endian.*      */
#define T_END_LITTLE            0x0020u               /**< Little-endian.     */
#define T_END_BIG               0x0040u               /**< Big-endian.        */
#define T_NULL                  0x0001u               /**< Null-terminated.   */

// types
#define T_NONE                  0u                    /**< No type.           */
#define T_ERROR                 0x0080u               /**< Error type.        */

#define T_BOOL                  0x0100                /**< Boolean.           */
#define T_BOOL8   (             T_BOOL  | T_08_BITS) /**< `bool`             */

#define T_UTF                   0x0200               /**< Unicode.           */
#define T_UTF8     (            T_UTF   | T_08_BITS) /**< UTF-8 (multibyte). */
#define T_UTF16_H  (            T_UTF   | T_16_BITS) /**< UTF-16 host.       */
#define T_UTF16_BE (T_END_BIG | T_UTF   | T_16_BITS) /**< UTF-16 big.        */
#define T_UTF16_LE (T_END_LITTLE | T_UTF| T_16_BITS) /**< UTF-16 little.  */
#define T_UTF32_H  (            T_UTF   | T_32_BITS) /**< UTF-32 host.       */
#define T_UTF32_BE (T_END_BIG | T_UTF   | T_32_BITS) /**< UTF-32 big.        */
#define T_UTF32_LE (T_END_LITTLE | T_UTF| T_32_BITS) /**< UTF-32 little.  */

#define T_UTF_0    (T_NULL    | T_UTF              ) /**< UTF string.        */
#define T_UTF8_0   (T_NULL    | T_UTF   | T_08_BITS) /**< UTF-8 string.      */

/** UTF-16, big-endian, null-terminated string. */
#define T_UTF16_H_0               (T_NULL | T_UTF | T_16_BITS)

/** UTF-16 big-endian, null-terminated string. */
#define T_UTF16_BE_0              (T_END_BIG | T_NULL | T_UTF | T_16_BITS)

/** UTF-16 little-endian, null-terminated string. */
#define T_UTF16_LE_0              (T_END_LITTLE | T_NULL | T_UTF | T_16_BITS)

/**< UTF-32 host-endian, null-terminated string. */
#define T_UTF32_H_0               (T_NULL | T_UTF | T_32_BITS)

/**< UTF-32 big-endian null-terminated string. */
#define T_UTF32_BE_0              (T_END_BIG | T_NULL | T_UTF | T_32_BITS)

/** UTF-32 little-endian, null-terminated string. */
#define T_UTF32_LE_0              (T_END_LITTLE | T_NULL | T_UTF | T_32_BITS)

#define T_INT       (            0x0400             ) /**< Integer.           */
#define T_INT8      (T_SIGNED  | T_INT   | T_08_BITS) /**< `signed int8`      */
#define T_INT16     (T_SIGNED  | T_INT   | T_16_BITS) /**< `signed int16`     */
#define T_INT32     (T_SIGNED  | T_INT   | T_32_BITS) /**< `signed int32`     */
#define T_INT64     (T_SIGNED  | T_INT   | T_64_BITS) /**< `signed int64`     */
#define T_UINT8     (            T_INT   | T_08_BITS) /**< `unsigned int8`    */
#define T_UINT16    (            T_INT   | T_16_BITS) /**< `unsigned int16`   */
#define T_UINT32    (            T_INT   | T_32_BITS) /**< `unsigned int32`   */
#define T_UINT64    (            T_INT   | T_64_BITS) /**< `unsigned int64`   */

#define T_FLOAT                 0x0800               /**< Floating point.    */
#define T_FLOAT32  (T_SIGNED |  T_FLOAT | T_32_BITS) /**< `float32`          */
#define T_FLOAT64  (T_SIGNED |  T_FLOAT | T_64_BITS) /**< `float64`          */

#define T_STRUCT                0x1000                /**< `struct`           */
#define T_SWITCH                0x2000                /**< `switch`           */

#define T_INT_LIKE  (T_BOOL | T_INT)
#define T_NUMBER    (T_BOOL | T_INT | T_FLOAT)

// bit masks
#define T_MASK_ENDIAN           0x0006u               /**< Endian bitmask.    */
#define T_MASK_NULL             T_NULL                /**< Null bitmask.      */
#define T_MASK_SIGN             T_SIGNED              /**< Sign bitmask.      */
#define T_MASK_SIZE             0x0078u               /**< Size bitmask.      */
#define T_MASK_TYPE             0x3F00u               /**< Type bitmask.      */

///////////////////////////////////////////////////////////////////////////////

typedef struct  ad_char     ad_char_t;
typedef struct  ad_expr     ad_expr_t;
typedef struct  ad_int      ad_int_t;
typedef enum    ad_int_base ad_int_base_t;
typedef struct  ad_struct   ad_struct_t;
typedef struct  ad_switch   ad_switch_t;
typedef struct  slist       ad_switch_cases_t;
typedef struct  ad_type     ad_type_t;
typedef uint16_t            ad_type_id_t;
typedef unsigned short      ad_type_size_t;
typedef struct   slist      ad_type_list_t;

/**
 * Integer base (for printing).
 */
enum ad_int_base {
  AD_BASE_NONE,                         ///< No base.
  AD_BASE_BIN =  2,                     ///< Binary.
  AD_BASE_OCT =  8,                     ///< Octal.
  AD_BASE_DEC = 10,                     ///< Decimal.
  AD_BASE_HEX = 16                      ///< Hexadecimal.
};

/**
 * Integer type.
 */
struct ad_int {
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
 * A type.  Every %ad_type at least has the ID that it's a type of and a size
 * in bits.
 */
struct ad_type {
  ad_type_id_t    type_id;

  union {
    ad_endian_t   endian;
    ad_int_t      ad_int;
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

/**
 * Creates a new `ad_type`.
 *
 * @param tid The ID of the type to create.
 */
AD_WARN_UNUSED_RESULT
ad_type_t* ad_type_new( ad_type_id_t tid );

/**
 * Gets the size (in bits) of the type represented by \a tid.
 *
 * @param tid The ID of the type to get the size of.
 * @return Returns said size.
 */
AD_WARN_UNUSED_RESULT AD_TYPES_INLINE
size_t ad_type_size( ad_type_id_t tid ) {
  return tid & T_MASK_SIZE;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_types_H */
/* vim:set et sw=2 ts=2: */
