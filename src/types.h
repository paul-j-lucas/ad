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
#ifndef AD_TYPES_H_INLINE
# define AD_TYPES_H_INLINE _GL_INLINE
#endif /* AD_TYPES_H_INLINE */

#define AD_EXPR_UNARY       0x0100      /**< Unary expression.    */
#define AD_EXPR_BINARY      0x0200      /**< Binary expression.   */
#define AD_EXPR_TERNARY     0x0400      /**< Ternary expression.  */

#define AD_EXPR_MASK        0x0F00      ///< Expression type bitmask.

///////////////////////////////////////////////////////////////////////////////

//
// The bits (right to left) are used as follows:
//
//    F-DC BA98 7654 3210
//    S--- TTTT TTZZ ENDD
//
// where:
//
//    D = endianness:
//      0 = none
//      1 = little
//      2 = big
//      3 = host
//    E = error:
//      0 = no error
//      1 = error
//    N = null terminated:
//      0 = not null terminated
//      1 = null terminated
//    S = sign:
//      0 = unsigned
//      1 = signed
//    T = type (at most one set)
//      0 = bool
//      1 = UTF
//      2 = int
//      3 = float
//      4 = struct
//    Z = size in bits:
//      0 = 8 bits
//      1 = 16 bits
//      2 = 32 bits
//      3 = 4 bits
//

#define T_END_B (ENDIAN_BIG)
#define T_END_L (ENDIAN_LITTLE)
#define T_END_H (ENDIAN_HOST)

#define T_08  (0u)
#define T_16  (1u << 4)
#define T_32  (2u << 4)
#define T_64  (3u << 4)

/** En(D)ian bitmask: `xxxx xxxx xxxx xxDD` */
#define T_MASK_ENDIAN             0x0006u

// (N)ull:    xxxx xxxx xxxx xNxx
#define T_NULL                   0x0040u              /**< Null-terminated.   */

// (E)rror:   xxxx xxxx xxxx Exxx
#define T_ERROR                  0x0080u              /**< Error type.        */

/** (T)ype bitmask: `xxTT TTTT xxxx xxxx` */
#define T_NONE                   0u                   /**< No type.           */

// (S)igned:  Sxxx xxxx xxxx xxxx
#define T_SIGNED                 0x8000u              /**< Signed type.       */

#define T_BOOL                   0x0400u              /**< Boolean.           */
#define T_BOOL8     (            T_BOOL |  8u)  /**< `bool`             */

#define T_UTF                    0x0800u        /**< Unicode.           */
#define T_UTF8      (            T_UTF  |  8u)  /**< UTF-8 (multibyte). */
#define T_UTF16HE   (            T_UTF  | 16u)  /**< UTF-16 host.       */
#define T_UTF16BE   (T_END_B | T_UTF  | 16u)  /**< UTF-16 big.        */
#define T_UTF16LE   (T_END_L | T_UTF  | 16u)  /**< UTF-16 little.     */
#define T_UTF32HE   (            T_UTF  | 32u)  /**< UTF-32 host.       */
#define T_UTF32BE   (T_END_B | T_UTF  | 32u)  /**< UTF-32 big.        */
#define T_UTF32LE   (T_END_L | T_UTF  | 32u)  /**< UTF-32 little.     */

#define T_UTF_0     (T_NULL    | T_UTF)  /**< UTF string.       */

/**< UTF-8, null-terminated string. */
#define T_UTF8_0    (T_NULL    | T_UTF   |  8u)

/** UTF-16 host-endian, null-terminated string. */
#define T_UTF16HE_0 (T_NULL    | T_UTF   | 16u)

/** UTF-16 big-endian, null-terminated string. */
#define T_UTF16BE_0 (T_END_B | T_UTF   | 16u | T_NULL)

/** UTF-16 little-endian, null-terminated string. */
#define T_UTF16LE_0 (T_END_L | T_UTF   | 16u | T_NULL)

/**< UTF-32 host-endian, null-terminated string. */
#define T_UTF32HE_0 (T_NULL    | T_UTF   | 32u)

/**< UTF-32 big-endian null-terminated string. */
#define T_UTF32BE_0 (T_END_B | T_UTF   | 32u | T_NULL)

/** UTF-32 little-endian, null-terminated string. */
#define T_UTF32LE_0 (T_END_L | T_UTF   | 32u | T_NULL)

#define T_INT       (            0x1000u      ) /**< Integer.           */
#define T_INT8      (T_SIGNED  | T_INT   |  8u) /**< `signed int8`      */
#define T_INT16     (T_SIGNED  | T_INT   | 16u) /**< `signed int16`     */
#define T_INT32     (T_SIGNED  | T_INT   | 32u) /**< `signed int32`     */
#define T_INT64     (T_SIGNED  | T_INT   | 64u) /**< `signed int64`     */
#define T_UINT8     (            T_INT   |  8u) /**< `unsigned int8`    */
#define T_UINT16    (            T_INT   | 16u) /**< `unsigned int16`   */
#define T_UINT32    (            T_INT   | 32u) /**< `unsigned int32`   */
#define T_UINT64    (            T_INT   | 64u) /**< `unsigned int64`   */

#define T_FLOAT                  0x2000         /**< Floating point.    */
#define T_FLOAT32   (T_SIGNED  | T_FLOAT | 32u) /**< `float32`          */
#define T_FLOAT64   (T_SIGNED  | T_FLOAT | 64u) /**< `float64`          */

#define T_STRUCT                 0x4000u              /**< `struct`           */

#define T_SWITCH                 0x8000u

#define T_INT_LIKE  (T_BOOL | T_INT)
#define T_NUMBER    (T_BOOL | T_INT | T_FLOAT)

// bit masks
#define T_MASK_NULL               T_NULL         /**< Null bitmask.      */
#define T_MASK_SIGN               T_SIGNED       /**< Sign bitmask.      */
#define T_MASK_SIZE               0x0078u        /**< Size bitmask.      */
#define T_MASK_TYPE               0x3F00u        /**< Type bitmask.      */

///////////////////////////////////////////////////////////////////////////////

#define T_GET_SIZE(T)             (((T) & T_MASK_SIZE) + 1u)

////////// enumerations ///////////////////////////////////////////////////////

// Enumerations have to be declared before typedefs of them since ISO C doesn't
// allow forward declarations of enums.

/**
 * Enumeration value.
 *
 * @sa ad_enum
 */
struct ad_enum_value {
  char const *name;
  int64_t     value;
};

/**
 * Expression errors.
 */
enum ad_expr_err {
  AD_ERR_NONE,                          ///< No error.
  AD_ERR_BAD_OPERAND,                   ///< Bad operand, e.g., & for double.
  AD_ERR_DIV_0,                         ///< Divide by 0.
};

/**
 * The expression kind.
 */
enum ad_expr_kind {
  AD_EXPR_NONE,                         ///< No expression.
  AD_EXPR_ERROR,                        ///< Error expression.

  AD_EXPR_VALUE,                        ///< Constant value expression.

  // unary
  AD_EXPR_BIT_COMP = AD_EXPR_UNARY + 1, ///< Bitwise-complement expression.
  AD_EXPR_MATH_NEG,                     ///< Negation expression.
  AD_EXPR_PTR_ADDR,                     ///< Address-of expression.
  AD_EXPR_PTR_DEREF,                    ///< Dereference expression.

  // binary
  AD_EXPR_ASSIGN  = AD_EXPR_BINARY + 1, ///< Assign expression.
  AD_EXPR_CAST,                         ///< Cast expression.

  AD_EXPR_BIT_AND,                      ///< Bitwise-and expression.
  AD_EXPR_BIT_OR,                       ///< Bitwise-or expression.
  AD_EXPR_BIT_SHIFT_LEFT,               ///< Bitwise-left-shift expression.
  AD_EXPR_BIT_SHIFT_RIGHT,              ///< Bitwise-right-shift expression.
  AD_EXPR_BIT_XOR,                      ///< Bitwise-exclusive-or expression.

  AD_EXPR_LOG_AND,                      ///< Logical-and expression.
  AD_EXPR_LOG_NOT,                      ///< Logical-not expression.
  AD_EXPR_LOG_OR,                       ///< Logical-or expression.
  AD_EXPR_LOG_XOR,                      ///< Logical-exclusive-or expression.

  AD_EXPR_MATH_ADD,                     ///< Addition expression.
  AD_EXPR_MATH_SUB,                     ///< Subtraction expression.
  AD_EXPR_MATH_MUL,                     ///< Multiplication expression.
  AD_EXPR_MATH_DIV,                     ///< Division expression.
  AD_EXPR_MATH_MOD,                     ///< Modulus expression.

  AD_EXPR_REL_EQ,                       ///< Equal.
  AD_EXPR_REL_NOT_EQ,                   ///< Not equal.
  AD_EXPR_REL_GREATER,                  ///< Greater than.
  AD_EXPR_REL_GREATER_EQ,               ///< Greater than or equal to.
  AD_EXPR_REL_LESS,                     ///< Less than.
  AD_EXPR_REL_LESS_EQ,                  ///< Less than or equal to.

  // ternary
  AD_EXPR_IF_ELSE = AD_EXPR_TERNARY + 1,///< If-else ?: expression.
};

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
 * Repetition values.
 */
enum ad_rep_times {
  AD_REP_1,                             ///< Repeats once (no repetition).
  AD_REP_EXPR,                          ///< Repeats _expr_ times.
  AD_REP_0_1,                           ///< Repeats 0 or 1 times (optional).
  AD_REP_0_MORE,                        ///< Repeats 0 or more times.
  AD_REP_1_MORE                         ///< Repeats 1 or more times.
};

/**
 * TODO
 */
enum ad_statement_kind {
  AD_STMT_COMPOUND,
  AD_STMT_DECLARATION,
  AD_STMT_SWITCH
};

///////////////////////////////////////////////////////////////////////////////

typedef struct  ad_binary_expr        ad_binary_expr_t;
typedef unsigned                      ad_bits_t;
typedef struct  ad_compound_statement ad_compound_statement_t;
typedef struct  ad_declaration        ad_declaration_t;
typedef struct  ad_char               ad_char_t;
typedef struct  ad_enum               ad_enum_t;
typedef struct  ad_enum_value         ad_enum_value_t;
typedef struct  ad_expr               ad_expr_t;
typedef enum    ad_expr_err           ad_expr_err_t;
typedef enum    ad_expr_kind          ad_expr_kind_t;
typedef struct  ad_field              ad_field_t;
typedef struct  ad_int                ad_int_t;
typedef enum    ad_int_base           ad_int_base_t;
typedef struct  ad_keyword            ad_keyword_t;
typedef struct  ad_loc                ad_loc_t;
typedef struct  ad_rep                ad_rep_t;
typedef enum    ad_rep_times          ad_rep_times_t;
typedef struct  ad_statement          ad_statement_t;
typedef enum    ad_statement_kind     ad_statement_kind_t;
typedef struct  ad_struct             ad_struct_t;
typedef struct  ad_switch             ad_switch_t;
typedef struct  ad_switch_statement   ad_switch_statement_t;
typedef struct  ad_switch_case        ad_switch_case_t;
typedef struct  slist                 ad_switch_cases_t;
typedef struct  ad_ternary_expr       ad_ternary_expr_t;
typedef struct  ad_type               ad_type_t;
typedef uint16_t                      ad_tid_t;
typedef unsigned short                ad_type_size_t;
typedef struct  slist                 ad_type_list_t;
typedef struct  ad_typedef            ad_typedef_t;
typedef struct  ad_unary_expr         ad_unary_expr_t;
typedef struct  ad_value_expr         ad_value_expr_t;
typedef struct  print_params          print_params_t;

typedef ad_loc_t YYLTYPE;               ///< Source location type for Bison.
/// @cond DOXYGEN_IGNORE
#define YYLTYPE_IS_DECLARED       1
#define YYLTYPE_IS_TRIVIAL        1
/// @endcond

/**
 * Enumeration type.
 */
struct ad_enum {
  ad_bits_t       bits;                 ///< Value number of bits.
  endian_t        endian;               ///< Endianness of values.
  ad_int_base_t   base;                 ///< Base of values.
  slist_t         values;               ///< List of ad_enum_value.
};

/**
 * Integer type.
 */
struct ad_int {
  ad_bits_t       bits;                 ///< Value number of bits.
  endian_t        endian;               ///< Endianness of value.
  ad_int_base_t   base;                 ///< Base of value.
};

/**
 * The source location used by Bison.
 */
struct ad_loc {
  //
  // These should be either unsigned or size_t, but Bison generates code that
  // tests these for >= 0 which is always true for unsigned types so it
  // generates warnings; hence these are kept as int to eliminate the warnings.
  //
  int first_line;                       ///< First line of location range.
  int first_column;                     ///< First column of location range.
  int last_line;                        ///< Last line of location range.
  int last_column;                      ///< Last column of location range.
};

/**
 * Repetition.
 */
struct ad_rep {
  ad_rep_times_t  times;
  ad_expr_t      *expr;                 ///< Used only if times == AD_REP_EXPR
};

/**
 * A data field.
 */
struct ad_field {
  char const *name;
  ad_type_t  *type;
  ad_rep_t    rep;
};

struct ad_compound_statement {
  slist_t statements;
};

struct ad_declaration {
  int PLACEHOLDER;
};

struct ad_switch_statement {
  int PLACEHOLDER;
};

struct ad_statement {
  union {
    ad_compound_statement_t compound;   ///< Compound statement.
    ad_declaration_t        declaration;///< Declaration.
    ad_switch_statement_t   st_switch;  ///< `switch` statement.
  };
  ad_statement_kind_t kind;
};

/**
 * struct type.
 */
struct ad_struct {
  char const     *name;
  ad_type_list_t  members;              ///< Structure members.
};

/**
 * `switch` `case` type.
 */
struct ad_switch_case {
  ad_expr_t      *expr;
  ad_type_list_t  case_types;           ///< All the type(s) for the case value.
};

/**
 * `switch` type.
 */
struct ad_switch {
  ad_expr_t        *expr;
  ad_switch_cases_t cases;
};

/**
 * TODO
 *
 * @param VAR The `slist_node` loop variable.
 */
#define FOREACH_CASE(VAR,SWITCH) \
  FOREACH_SLIST_NODE( VAR, (SWITCH)->cases )

/**
 * A type.  Every %ad_type at least has the ID that it's a type of and a size
 * in bits.
 */
struct ad_type {
  ad_tid_t        tid;

  union {
    ad_int_t      ad_int;
    ad_enum_t     ad_enum;
    ad_struct_t   ad_struct;
    ad_switch_t   ad_switch;
  };
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
NODISCARD
ad_type_t* ad_type_new( ad_tid_t tid );

/**
 * Gets the size (in bits) of the type represented by \a tid.
 *
 * @param tid The ID of the type to get the size of.
 * @return Returns said size.
 */
NODISCARD AD_TYPES_H_INLINE
size_t ad_type_size( ad_tid_t tid ) {
  return tid & T_MASK_SIZE;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_types_H */
/* vim:set et sw=2 ts=2: */
