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
#include "unicode.h"

// standard
#include <stdbool.h>
#include <stdint.h>

_GL_INLINE_HEADER_BEGIN
#ifndef AD_TYPES_H_INLINE
# define AD_TYPES_H_INLINE _GL_INLINE
#endif /* AD_TYPES_H_INLINE */

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
//      4 = enum
//      5 = struct
//    Z = size in bits:
//      0 = 8 bits
//      1 = 16 bits
//      2 = 32 bits
//      3 = 64 bits
//

#define T_END_B                 ENDIAN_BIG
#define T_END_L                 ENDIAN_LITTLE
#define T_END_H                 ENDIAN_HOST

#define T_08                    (0u)
#define T_16                    (1u << 4)
#define T_32                    (2u << 4)
#define T_64                    (3u << 4)

/** En(D)ian bitmask: `xxxx xxxx xxxx xxDD` */
#define T_MASK_ENDIAN           0x0006u

/** (E)rror bitmask: `xxxx xxxx xxxx Exxx` */
#define T_MASK_ERROR            0x0008u

/** (N)ull bitmask: `xxxx xxxx xxxx xNxx` */
#define T_MASK_NULL             0x0010u

/** (S)igned: `Sxxx xxxx xxxx xxxx` */
#define T_MASK_SIGN             0x8000u

/** Si(Z)e: `xxxx xxxx xxZZ xxxx` */
#define T_MASK_SIZE             0x0030u

/** (T)ype bitmask: `xxxx TTTT TTxx xxxx` */
#define T_MASK_TYPE             0x0FC0u

#define T_ERROR                 T_MASK_ERROR
#define T_NONE                  0u              /**< No type.           */
#define T_SIGNED                T_MASK_SIGN     /**< Signed type.       */

#define T_BOOL                  0x0040u         /**< Boolean.           */
#define T_BOOL8     (           T_BOOL | T_08)  /**< `bool`             */

#define T_UTF                   0x0080u         /**< Unicode.           */
#define T_UTF8      (           T_UTF  | T_08)  /**< UTF-8 (multibyte). */
#define T_UTF16HE   (           T_UTF  | T_16)  /**< UTF-16 host.       */
#define T_UTF16BE   (T_END_B  | T_UTF  | T_16)  /**< UTF-16 big.        */
#define T_UTF16LE   (T_END_L  | T_UTF  | T_16)  /**< UTF-16 little.     */
#define T_UTF32HE   (           T_UTF  | T_32)  /**< UTF-32 host.       */
#define T_UTF32BE   (T_END_B  | T_UTF  | T_32)  /**< UTF-32 big.        */
#define T_UTF32LE   (T_END_L  | T_UTF  | T_32)  /**< UTF-32 little.     */

#define T_NULL                  T_MASK_NULL     /**< Null-terminated.   */
#define T_UTF_0     (T_NULL   | T_UTF)          /**< UTF string.       */

/**< UTF-8, null-terminated string. */
#define T_UTF8_0    (T_UTF_0           |  T_08)

/** UTF-16 host-endian, null-terminated string. */
#define T_UTF16HE_0 (T_UTF_0           | T_16)

/** UTF-16 big-endian, null-terminated string. */
#define T_UTF16BE_0 (T_END_B  | T_UTF  | T_16 | T_NULL)

/** UTF-16 little-endian, null-terminated string. */
#define T_UTF16LE_0 (T_END_L  | T_UTF  | T_16 | T_NULL)

/**< UTF-32 host-endian, null-terminated string. */
#define T_UTF32HE_0 (T_UTF_0           | T_32)

/**< UTF-32 big-endian null-terminated string. */
#define T_UTF32BE_0 (T_END_B  | T_UTF  | T_32 | T_NULL)

/** UTF-32 little-endian, null-terminated string. */
#define T_UTF32LE_0 (T_END_L  | T_UTF  | T_32 | T_NULL)

#define T_INT       (            0x0100u    )   /**< Integer.           */
#define T_INT8      (T_SIGNED | T_INT  | T_08)  /**< `signed int8`      */
#define T_INT16     (T_SIGNED | T_INT  | T_16)  /**< `signed int16`     */
#define T_INT32     (T_SIGNED | T_INT  | T_32)  /**< `signed int32`     */
#define T_INT64     (T_SIGNED | T_INT  | T_64)  /**< `signed int64`     */
#define T_UINT8     (           T_INT  | T_08)  /**< `unsigned int8`    */
#define T_UINT16    (           T_INT  | T_16)  /**< `unsigned int16`   */
#define T_UINT32    (           T_INT  | T_32)  /**< `unsigned int32`   */
#define T_UINT64    (           T_INT  | T_64)  /**< `unsigned int64`   */

#define T_FLOAT                 0x0200          /**< Floating point.    */
#define T_FLOAT32   (T_SIGNED | T_FLOAT | T_32) /**< `float32`          */
#define T_FLOAT64   (T_SIGNED | T_FLOAT | T_64) /**< `float64`          */

#define T_ENUM                  0x0400u         /**< `enum`             */

#define T_STRUCT                0x0800u         /**< `struct`           */

#define T_INT_LIKE  (           T_BOOL | T_INT)
#define T_NUMBER    (           T_BOOL | T_INT | T_FLOAT)

///////////////////////////////////////////////////////////////////////////////

#define AD_EXPR_UNARY       0x0100      /**< Unary expression.    */
#define AD_EXPR_BINARY      0x0200      /**< Binary expression.   */
#define AD_EXPR_TERNARY     0x0400      /**< Ternary expression.  */

#define AD_EXPR_MASK        0x0F00      /**< Expression type bitmask. */

#define T_GET_SIZE(T)             (8u << (((T) & T_MASK_SIZE) >> 4))

/**
 * Decimal print conversion specifier for \ref ad_loc_num_t.
 */
#define PRI_ad_loc_num_t          "%u"

////////// enumerations ///////////////////////////////////////////////////////

// Enumerations have to be declared before typedefs of them since ISO C doesn't
// allow forward declarations of enums.

/**
 * **ad** debug mode.
 */
enum ad_debug {
  AD_DEBUG_NO            = 0u,          ///< Do not print debug output.
  AD_DEBUG_YES           = (1u << 0),   ///< Print JSON5 debug output.

  /**
   * Include `unique_id` values in debug output.
   *
   * @note May be used _only_ in combination with #AD_DEBUG_YES.
   */
  AD_DEBUG_OPT_EXPR_UNIQUE_ID = (1u << 1)
};

/**
 * Enumeration value.
 *
 * @sa ad_enum_type
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
  AD_EXPR_BIT_COMPL = AD_EXPR_UNARY + 1,///< Bitwise-complement expression.
  AD_EXPR_MATH_NEG,                     ///< Negation expression.
  AD_EXPR_PTR_ADDR,                     ///< Address-of expression.
  AD_EXPR_PTR_DEREF,                    ///< Dereference expression.
  AD_EXPR_MATH_DEC_POST,                ///< Post-decrement.
  AD_EXPR_MATH_DEC_PRE,                 ///< Pre-decrement.
  AD_EXPR_MATH_INC_POST,                ///< Post-incremeent.
  AD_EXPR_MATH_INC_PRE,                 ///< Pre-incremeent.
  AD_EXPR_SIZEOF,                       ///< `sizeof`.

  // binary
  AD_EXPR_ASSIGN  = AD_EXPR_BINARY + 1, ///< Assign expression.
  AD_EXPR_ARRAY,                        ///< Array expression.
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
 * Kinds of statements in the **ad** language.
 */
enum ad_statement_kind {
  /**
   * A single declaration statement.
   */
  AD_ST_DECLARATION,

  /**
   * A compound statement, i.e. zero or more statements between `{` `}`.
   */
  AD_ST_COMPOUND,

  /**
   * A `switch` statement.
   */
  AD_ST_SWITCH
};

////////// typedefs ///////////////////////////////////////////////////////////

typedef struct  ad_binary_expr        ad_binary_expr_t;
typedef unsigned                      ad_bits_t;
typedef struct  ad_compound_statement ad_compound_statement_t;
typedef enum    ad_debug              ad_debug_t;
typedef struct  ad_declaration        ad_declaration_t;
typedef struct  ad_char               ad_char_t;
typedef struct  ad_enum_type          ad_enum_type_t;
typedef struct  ad_enum_value         ad_enum_value_t;
typedef struct  ad_expr               ad_expr_t;
typedef enum    ad_expr_err           ad_expr_err_t;
typedef enum    ad_expr_kind          ad_expr_kind_t;
typedef struct  ad_field              ad_field_t;
typedef struct  ad_int_type           ad_int_type_t;
typedef enum    ad_int_base           ad_int_base_t;
typedef struct  ad_keyword            ad_keyword_t;
typedef struct  ad_loc                ad_loc_t;

/**
 * Underlying source location numeric type for \ref ad_loc.
 *
 * @remarks This should be an unsigned type, but Flex & Bison generate code
 * that assumes it's signed.  Making it unsigned generates warnings; hence this
 * is kept as signed to prevent the warnings.
 */
typedef short                         ad_loc_num_t;

typedef struct  ad_rep                ad_rep_t;
typedef enum    ad_rep_times          ad_rep_times_t;
typedef struct  ad_statement          ad_statement_t;
typedef enum    ad_statement_kind     ad_statement_kind_t;
typedef struct  ad_struct_type        ad_struct_type_t;
typedef struct  ad_switch_statement   ad_switch_statement_t;
typedef struct  ad_switch_case        ad_switch_case_t;
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

///////////////////////////////////////////////////////////////////////////////

/**
 * The source location used by Flex & Bison.
 */
struct ad_loc {
  ad_loc_num_t first_line;              ///< First line of location range.
  ad_loc_num_t first_column;            ///< First column of location range.
  ad_loc_num_t last_line;               ///< Last line of location range.
  ad_loc_num_t last_column;             ///< Last column of location range.
};

/**
 * Repetition.
 */
struct ad_rep {
  ad_rep_times_t  times;
  ad_expr_t      *expr;                 ///< Used only if times == AD_REP_EXPR
};

////////// ad types ///////////////////////////////////////////////////////////

/**
 * Enumeration type.
 */
struct ad_enum_type {
  ad_bits_t       bits;                 ///< Value number of bits.
  endian_t        endian;               ///< Endianness of values.
  ad_int_base_t   base;                 ///< Base of values.
  slist_t         values;               ///< List of ad_enum_value.
};

/**
 * Integer type.
 */
struct ad_int_type {
  ad_bits_t       bits;                 ///< Value number of bits.
  endian_t        endian;               ///< Endianness of value.
  ad_int_base_t   base;                 ///< Base of value.
};

/**
 * struct type.
 */
struct ad_struct_type {
  char const     *name;
  ad_type_list_t  members;              ///< Structure members.
};

/**
 * A type.  Every %ad_type at least has the ID that it's a type of.
 */
struct ad_type {
  ad_tid_t        tid;
  ad_expr_t      *size_expr;
  ad_expr_t      *endian_expr;

  union {
    ad_int_type_t     ad_int;
    ad_enum_type_t    ad_enum;
    ad_struct_type_t  ad_struct;
  };
};

////////// ad statements //////////////////////////////////////////////////////

/**
 * Compound statement.
 */
struct ad_compound_statement {
  slist_t statements;
};

/**
 * Declaration.
 */
struct ad_declaration {
  char const *name;
};

/**
 * An individual `case` for an \ref ad_switch_statement.
 */
struct ad_switch_case {
  ad_expr_t                *expr;
  ad_compound_statement_t   compound_statement;
};

/**
 * `switch` statement.
 */
struct ad_switch_statement {
  ad_expr_t  *expr;
  slist_t     case_list;
};

/**
 */
struct ad_statement {
  union {
    ad_compound_statement_t compound;   ///< Compound statement.
    ad_declaration_t        declaration;///< Declaration.
    ad_switch_statement_t   st_switch;  ///< `switch` statement.
  };
  ad_statement_kind_t kind;
  ad_loc_t            loc;
};

/**
 * TODO
 *
 * @param VAR The `slist_node` loop variable.
 */
#define FOREACH_CASE(VAR,SWITCH) \
  FOREACH_SLIST_NODE( VAR, (SWITCH)->case_list )

/**
 * A data field.
 */
struct ad_field {
  char const *name;                     ///< Field name.
  ad_type_t   type;                     ///< Field type.
  ad_rep_t    rep;                      ///< Repetitions.
  ad_expr_t  *match_expr;               ///< Value it must match, if any.
};

////////// expressions ////////////////////////////////////////////////////////

/**
 * Unary expression; used for:
 *  + Pointer address
 *  + Pointer dereference
 *  + Bitwise complement
 *  + Negation
 */
struct ad_unary_expr {
  ad_expr_t *sub_expr;
};

/**
 * Binary expression; used for:
 *  + Addition
 *  + Bitwise and
 *  + Bitwise or
 *  + Cast
 *  + Division
 *  + Left shift
 *  + Logical and
 *  + Logical or
 *  + Modulus
 *  + Multiplication
 *  + Right shift
 *  + Subtraction
 */
struct ad_binary_expr {
#if 1
  ad_expr_t *lhs_expr;
  ad_expr_t *rhs_expr;
#else
  ad_expr_t *sub_expr[2];
#endif
};

/**
 * Ternary expression; used for:
 *  + `?:`
 */
struct ad_ternary_expr {
  ad_expr_t *cond_expr;
  ad_expr_t *sub_expr[2];
};

/**
 * Constant value expression.
 */
struct ad_value_expr {
  ad_type_t       type;                 ///< The type of the value.
  union {
    // Integer.
    int64_t       i64;                  ///< i8, i16, i32, i64
    uint64_t      u64;                  ///< u8, u16, u32, u64

    // Floating-point.
    double        f64;                  ///< f32, f64

    // UTF characters.
    char8_t       c8;                   ///< UTF-8 character.
    char16_t      c16;                  ///< UTF-16 character.
    char32_t      c32;                  ///< UTF-32 character.

    // UTF strings.
    char         *s;                    ///< Any string.
    char8_t      *s8;                   ///< UTF-8 string.
    char16_t     *s16;                  ///< UTF-16 string.
    char32_t     *s32;                  ///< UTF-32 string.

    // Miscellaneous.
    ad_type_t     cast_type;
    ad_expr_err_t err;
  };
};

/**
 * An expression.
 */
struct ad_expr {
  ad_expr_kind_t  expr_kind;            ///< Expression kind.
  ad_loc_t        loc;                  ///< Source location.

  union {
    ad_unary_expr_t   unary;            ///< Unary expression.
    ad_binary_expr_t  binary;           ///< Binary expression.
    ad_ternary_expr_t ternary;          ///< Ternary expression.
    ad_value_expr_t   value;            ///< Value expression.
  };
};

/**
 * "User data" passed as additional data to certain functions.
 *
 * @note This isn't just a `void*` as is typically used for "user data" since
 * `void*` can't hold a 64-bit integer value on 32-bit platforms.
 * @note Almost all built-in types are included to avoid casting.
 * @note `long double` is not included since that would double the size of the
 * `union`.
 */
union user_data {
  bool                b;                ///< `bool` value.

  char                c;                ///< `char` value.
  signed char         sc;               ///< `signed char` value.
  unsigned char       uc;               ///< `unsigned char` value.

  short               s;                ///< `short` value.
  int                 i;                ///< `int` value.
  long                l;                ///< `long` value.
  long long           ll;               ///< `long long` value.

  unsigned short      us;               ///< `unsigned short` value.
  unsigned int        ui;               ///< `unsigned int` value.
  unsigned long       ul;               ///< `unsigned long` value.
  unsigned long long  ull;              ///< `unsigned long long` value.

  int8_t              i8;               ///< `int8_t` value.
  int16_t             i16;              ///< `int16_t` value.
  int32_t             i32;              ///< `int32_t` value.
  int64_t             i64;              ///< `int64_t` value.

  uint8_t             ui8;              ///< `uint8_t` value.
  uint16_t            ui16;             ///< `uint16_t` value.
  uint32_t            ui32;             ///< `uint32_t` value.
  uint64_t            ui64;             ///< `uint64_t` value.

  float               f;                ///< `float` value.
  double              d;                ///< `double` value.

  void               *p;                ///< Pointer (to non-`const`) value.
  void const         *pc;               ///< Pointer to `const` value.
};

/**
 * Convenience macro for specifying a zero-initialized user_data literal.
 */
#define USER_DATA_ZERO            ((user_data_t){ .i64 = 0 })

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets whether \a tid is a signed integer.
 *
 * @param tid The type ID to use.
 * @return Returns `true` only if \a tid is signed.
 */
NODISCARD AD_TYPES_H_INLINE
bool ad_is_signed( ad_tid_t tid ) {
  return (tid & T_SIGNED) != 0;
}

/**
 * Gets the size (in bits) of the type represented by \a tid.
 *
 * @param tid The type ID to use.
 * @return Returns said size.
 */
NODISCARD AD_TYPES_H_INLINE
unsigned ad_tid_size( ad_tid_t tid ) {
  return 8u << ((tid & T_MASK_SIZE) >> 4);
}

/**
 * Gets the endianness of \a tid.
 *
 * @param tid The type ID to use.
 * @return Returns said endianness.
 */
NODISCARD AD_TYPES_H_INLINE
endian_t ad_tid_endian( ad_tid_t tid ) {
  return STATIC_CAST( endian_t, tid & T_MASK_ENDIAN );
}

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

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_types_H */
/* vim:set et sw=2 ts=2: */
