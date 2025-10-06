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
#include "sname.h"
#include "unicode.h"

// standard
#include <stdbool.h>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

#define T_END_B                 ENDIAN_BIG
#define T_END_L                 ENDIAN_LITTLE
#define T_END_H                 ENDIAN_HOST

#define T_08                    ( 8u << 8)
#define T_16                    (16u << 8)
#define T_32                    (32u << 8)
#define T_64                    (64u << 8)

/** (E)ndian bitmask: `xxxx xxxx xxxx xxEE` */
#define T_MASK_ENDIAN           0x0003u

/** (N)ull bitmask: `xxxx xxxx xxxx xNxx` */
#define T_MASK_NULL             0x0004u

/** (S)igned bitmask: `Sxxx xxxx xxxx xxxx` */
#define T_MASK_SIGN             0x8000u

/** Si(Z)e bitmask: `xZZZ ZZZZ xxxx xxxx` */
#define T_MASK_SIZE             0x7F00u

/** (T)ype bitmask: `xxxx xxxx TTTT xxxx` */
#define T_MASK_TYPE             0x00F0u

/** `bool` */
#define T_BOOL8     (           (unsigned)T_BOOL  | T_08)

/** UTF-8 (multibyte). */
#define T_UTF8      (           (unsigned)T_UTF   | T_08)

/** UTF-16 host. */
#define T_UTF16HE   (T_END_H    (unsigned)T_UTF   | T_16)

/** UTF-16 big. */
#define T_UTF16BE   (T_END_B  | (unsigned)T_UTF   | T_16)

/** UTF-16 little. */
#define T_UTF16LE   (T_END_L  | (unsigned)T_UTF   | T_16)

/** UTF-32 host. */
#define T_UTF32HE   (T_END_H  | (unsigned)T_UTF   | T_32)

/** UTF-32 big. */
#define T_UTF32BE   (T_END_B  | (unsigned)T_UTF   | T_32)

/** UTF-32 little. */
#define T_UTF32LE   (T_END_L  | (unsigned)T_UTF   | T_32)


#define T_NULL      T_MASK_NULL                 /**< Null-terminated.   */

/** UTF string. */
#define T_UTF_0     (T_NULL   | (unsigned)T_UTF)


/** UTF-8, null-terminated strin(unsigned)g. */
#define T_UTF8_0    (           (unsigned)T_UTF_0 | T_08)

/** UTF-16 host-endian, null-ter(unsigned)minated string. */
#define T_UTF16HE_0 (T_END_H  | (unsigned)T_UTF_0 | T_16)

/** UTF-16 big-endian, null-terminated string. */
#define T_UTF16BE_0 (T_END_B  | T_UTF_0 | T_16)

/** UTF-16 little-endian, null-terminated string. */
#define T_UTF16LE_0 (T_END_L  | T_UTF_0 | T_16)

/** UTF-32 host-endian, null-terminated string. */
#define T_UTF32HE_0 (T_END_H  | T_UTF_0 | T_32)

/** UTF-32 big-endian null-terminated string. */
#define T_UTF32BE_0 (T_END_B  | T_UTF_0 | T_32)

/** UTF-32 little-endian, null-terminated string. */
#define T_UTF32LE_0 (T_END_L  | T_UTF_0 | T_32)

#define T_SIGNED    T_MASK_SIGN                 /**< Signed type.       */
#define T_INT8      (T_SIGNED | T_INT   | T_08) /**< `signed int8`      */
#define T_INT16     (T_SIGNED | T_INT   | T_16) /**< `signed int16`     */
#define T_INT32     (T_SIGNED | T_INT   | T_32) /**< `signed int32`     */
#define T_INT64     (T_SIGNED | T_INT   | T_64) /**< `signed int64`     */
#define T_UINT8     (           T_INT   | T_08) /**< `unsigned int8`    */
#define T_UINT16    (           T_INT   | T_16) /**< `unsigned int16`   */
#define T_UINT32    (           T_INT   | T_32) /**< `unsigned int32`   */
#define T_UINT64    (           T_INT   | T_64) /**< `unsigned int64`   */

#define T_FLOAT32   (T_SIGNED | T_FLOAT | T_32) /**< `float32`          */
#define T_FLOAT64   (T_SIGNED | T_FLOAT | T_64) /**< `float64`          */

#define T_INT_LIKE  (           T_BOOL  | T_INT)
#define T_NUMBER    (           T_FLOAT | T_INT_LIKE)

///////////////////////////////////////////////////////////////////////////////

#define AD_EXPR_UNARY       0x0100      /**< Unary expression.    */
#define AD_EXPR_BINARY      0x0200      /**< Binary expression.   */
#define AD_EXPR_TERNARY     0x0400      /**< Ternary expression.  */

#define AD_EXPR_MASK        0x0F00      /**< Expression type bitmask. */

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
 * Expression errors.
 */
enum ad_expr_err {
  AD_ERR_NONE,                          ///< No error.
  AD_ERR_BAD_OPERAND,                   ///< Bad operand, e.g., `&` for double.
  AD_ERR_DIV_0,                         ///< Divide by 0.
};

/**
 * The expression kind.
 */
enum ad_expr_kind {
  AD_EXPR_NONE,                         ///< No expression.
  AD_EXPR_ERROR,                        ///< Error expression.

  AD_EXPR_LITERAL,                      ///< Literal value expression.
  AD_EXPR_NAME,                         ///< Name value expression.

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
  AD_EXPR_COMMA,                        ///< Comma expression.

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

  AD_EXPR_STRUCT_MBR_REF,               ///< Structure member reference.
  AD_EXPR_STRUCT_MBR_DEREF,             ///< Structure member via pointer.

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
 * Repetition kind.
 */
enum ad_rep_kind {
  AD_REP_1,                             ///< Repeats once (no repetition).
  AD_REP_EXPR,                          ///< Repeats _expr_ times.
  AD_REP_0_1,                           ///< Repeats 0 or 1 times (optional).
  AD_REP_0_MORE,                        ///< Repeats 0 or more times.
  AD_REP_1_MORE                         ///< Repeats 1 or more times.
};

/**
 * Kinds of statements in the **ad** language.
 */
enum ad_stmnt_kind {
  AD_STMNT_BREAK,                       ///< `break`
  AD_STMNT_DECL,                        ///< Single declaration.
  AD_STMNT_IF,                          ///< `if`
  AD_STMNT_LET,                         ///< `let`
  AD_STMNT_SWITCH                       /// `switch`
};

/**
 * Kinds of types.
 */
enum ad_tid_kind {
  T_NONE    = 0,                        ///< None.
  T_ERROR   = 1 << 4,                   ///< Error.

  T_STRUCT  = 2 << 4,                   ///< Structure.
  T_UNION   = 3 << 4,                   ///< Union.
  T_TYPEDEF = 4 << 4,                   ///< `typedef`.

  // Types that have a preset size in bits.
  T_BOOL    = 5 << 4,                   ///< Boolean.
  T_UTF     = 6 << 4,                   ///< Unicode character or string.
  T_INT     = 7 << 4,                   ///< Integer.
  T_FLOAT   = 8 << 4,                   ///< Floating point.
  T_ENUM    = 9 << 4,                   ///< Enumeration.
};

////////// typedefs ///////////////////////////////////////////////////////////

typedef struct  ad_binary_expr  ad_binary_expr_t;
typedef unsigned                ad_bits_t;
typedef struct  ad_bool_type    ad_bool_type_t;
typedef struct  ad_break_stmnt  ad_break_stmnt_t;
typedef enum    ad_debug        ad_debug_t;
typedef struct  ad_decl_stmnt   ad_decl_stmnt_t;
typedef struct  ad_enum_type    ad_enum_type_t;
typedef struct  ad_enum_value   ad_enum_value_t;
typedef struct  ad_expr         ad_expr_t;
typedef enum    ad_expr_err     ad_expr_err_t;
typedef enum    ad_expr_kind    ad_expr_kind_t;
typedef struct  ad_float_type   ad_float_type_t;
typedef struct  ad_fmt_type     ad_fmt_type_t;
typedef struct  ad_if_stmnt     ad_if_stmnt_t;
typedef struct  ad_int_type     ad_int_type_t;
typedef enum    ad_int_base     ad_int_base_t;
typedef struct  ad_keyword      ad_keyword_t;
typedef struct  ad_let_stmnt    ad_let_stmnt_t;
typedef struct  ad_literal_expr ad_literal_expr_t;
typedef struct  ad_loc          ad_loc_t;

/**
 * Underlying source location numeric type for \ref ad_loc.
 *
 * @remarks This should be an unsigned type, but Flex & Bison generate code
 * that assumes it's signed.  Making it unsigned generates warnings; hence this
 * is kept as signed to prevent the warnings.
 */
typedef short                   ad_loc_num_t;

typedef struct  ad_rep          ad_rep_t;
typedef enum    ad_rep_kind     ad_rep_kind_t;
typedef struct  ad_stmnt        ad_stmnt_t;
typedef enum    ad_stmnt_kind   ad_stmnt_kind_t;
typedef slist_t                 ad_stmnt_list_t;
typedef struct  ad_struct_type  ad_struct_type_t;
typedef struct  ad_switch_stmnt ad_switch_stmnt_t;
typedef struct  ad_switch_case  ad_switch_case_t;
typedef struct  ad_ternary_expr ad_ternary_expr_t;
typedef struct  ad_type         ad_type_t;
typedef struct  ad_typedef_type ad_typedef_type_t;
typedef struct  ad_union_type   ad_union_type_t;
typedef struct  ad_utf_type     ad_utf_type_t;

/**
 * Type ID.
 *
 * @remarks
 * @parblock
 * The bits are used as follows:
 *
 *      FEDC BA98 7654 3210
 *      SZZZ ZZZZ TTTT -NEE
 *
 * where:
 *
 *      S[1] = sign:
 *         0 = unsigned
 *         1 = signed
 *
 *      Z[7] = size:
 *         0 = expr
 *         n = size in bits
 *
 *      T[4] = type:
 *         0 = none
 *         1 = error
 *         2 = bool
 *         3 = UTF
 *         4 = int
 *         5 = float
 *         6 = enum
 *         7 = struct
 *         8 = union
 *         9 = typedef
 *
 *      N[1] = null terminated:
 *         0 = single character
 *         1 = null terminated string
 *
 *      E[2] = endianness:
 *         0 = expr
 *         1 = little
 *         2 = big
 *         3 = host
 * @endparblock
 *
 * @sa ad_endian
 * @sa ad_tid_kind
 * @sa #T_MASK_ENDIAN
 * @sa #T_MASK_NULL
 * @sa #T_MASK_SIGN
 * @sa #T_MASK_SIZE
 * @sa #T_MASK_TYPE
 */
typedef uint16_t                      ad_tid_t;

typedef enum    ad_tid_kind           ad_tid_kind_t;
typedef struct  ad_unary_expr         ad_unary_expr_t;
typedef struct  print_params          print_params_t;
typedef slist_t                       sname_t;    ///< Scoped name.

/**
 * The signature for a function passed to **bsearch**(3).
 *
 * @param i_data A pointer to data.
 * @param j_data A pointer to data.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the data pointed to by \a i_data is less than, equal to, or
 * greater than the data pointed to by \a j_data.
 */
typedef int (*bsearch_cmp_fn_t)( void const *i_data, void const *j_data );

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
  ad_rep_kind_t   kind;                 ///< Kind of repetition.
  ad_expr_t      *expr;                 ///< Used only if rep == AD_REP_EXPR
};

/**
 */
struct ad_rep_range {
  unsigned  low;
  unsigned  high;
};

////////// ad types ///////////////////////////////////////////////////////////

/**
 * `bool` type.
 */
struct ad_bool_type {
  char const *printf_fmt;               ///< `printf` format, if any.
};

/**
 * `enum` type.
 */
struct ad_enum_type {
  char const *printf_fmt;               ///< `printf` format, if any.
  slist_t     value_list;               ///< List of ad_enum_value.
};

/**
 * `enum` value.
 *
 * @sa ad_enum_type
 */
struct ad_enum_value {
  char const *printf_fmt;               ///< `printf` format, if any.
  char const *name;                     ///< Name.
  int64_t     value;                    ///< Value.
};

/**
 * `float` type.
 */
struct ad_float_type {
  char const *printf_fmt;               ///< `printf` format, if any.
};

/**
 * Any type that can have a `printf` format.
 */
struct ad_fmt_type {
  char const *printf_fmt;               ///< `printf` format, if any.
};

/**
 * `int` type.
 */
struct ad_int_type {
  char const *printf_fmt;               ///< `printf` format, if any.
};

/**
 * `struct` type.
 */
struct ad_struct_type {
  /// @cond DOXYGEN_IGNORE
  /// So members below does not occupy the bytes used for printf_fmt in other
  /// types.
  DECL_UNUSED(char const*);
  /// @endcond

  ad_stmnt_list_t member_list;          ///< Structure members.
};

/**
 * `union` type.
 */
struct ad_union_type {
  /// @cond DOXYGEN_IGNORE
  /// So members below does not occupy the bytes used for printf_fmt in other
  /// types.
  DECL_UNUSED(char const*);
  /// @endcond

  ad_stmnt_list_t member_list;          ///< Union members.
};

/**
 * `typedef` type.
 */
struct ad_typedef_type {
  /// @cond DOXYGEN_IGNORE
  /// So members below does not occupy the bytes used for printf_fmt in other
  /// types.
  DECL_UNUSED(char const*);
  /// @endcond

  ad_type_t const *type;                ///< What it's a `typedef` for.
};

/**
 * `utf` (character only) type.
 */
struct ad_utf_type {
  char const *printf_fmt;               ///< `printf` format, if any.
};

/**
 * A type.
 */
struct ad_type {
  sname_t             sname;            ///< Name of type.
  ad_tid_t            tid;              ///< Type of type.

  /// Size (in bits) of type --- used only if \ref tid `&` #T_MASK_SIZE is 0.
  ad_expr_t          *size_expr;

  /// Endianness of type --- used only if \ref tid `&` #T_MASK_ENDIAN is 0.
  ad_expr_t          *endian_expr;

  ad_loc_t            loc;              ///< Source location.
  ad_rep_t            rep;              ///< Repetition.

  union {
    ad_bool_type_t    bool_t;           ///< #T_BOOL members.
    ad_enum_type_t    enum_t;           ///< #T_ENUM members.
                        // nothing needed for T_ERROR
    ad_float_type_t   float_t;          ///< #T_FLOAT members.
    ad_fmt_type_t     fmt_t;            ///< Any format type.
    ad_int_type_t     int_t;            ///< #T_INT members.
    ad_struct_type_t  struct_t;         ///< #T_STRUCT members.
    ad_typedef_type_t typedef_t;        ///< #T_TYPEDEF members.
    ad_union_type_t   union_t;          ///< #T_UNION members.
    ad_utf_type_t     utf_t;            ///< #T_UTF members.
  };
};

extern ad_type_t const TB_BOOL8;        ///< Built-in `bool` type.
extern ad_type_t const TB_FLOAT64;      ///< Built-in `float` type.
extern ad_type_t const TB_INT64;        ///< Built-in `int64` type.
extern ad_type_t const TB_UINT64;       ///< Built-in `uint64` type.
extern ad_type_t const TB_UTF8_0;       ///< Built-in UTF-8 string type.

////////// ad statements //////////////////////////////////////////////////////

/**
 * A declaration in the **ad** language.
 */
struct ad_decl_stmnt {
  char const       *name;               ///< Name.
  ad_type_t        *type;               ///< Type.
  unsigned          align;              ///< Alignment.
  ad_rep_t          rep;                ///< Repetition.
  ad_expr_t const  *if_expr;            ///< If expression, if any.
  ad_expr_t const  *requires_expr;      ///< Requires expression, if any.
  char const       *printf_fmt;         ///< `printf` format, if any.
};

/**
 * A `break` statement in the **ad** language.
 */
struct ad_break_stmnt {
  unsigned  goto_stmnt_idx;             ///< Statement index to go to.
};

/**
 * A `if` statement in the **ad** language.
 */
struct ad_if_stmnt {
  ad_expr_t        *expr;               ///< `if` expression.
  ad_stmnt_list_t   list[2];            ///< 1 = `if`; 0 = `else`.
};

/**
 * A `let` statement in the **ad** language.
 */
struct ad_let_stmnt {
  char const *name;                     ///< Name.
  ad_expr_t  *expr;                     ///< Expression.
};

/**
 * An individual `case` for an \ref ad_switch_stmnt.
 */
struct ad_switch_case {
  ad_expr_t        *expr;
  ad_stmnt_list_t   stmnts;
};

/**
 * A `switch` statement in the **ad** language.
 */
struct ad_switch_stmnt {
  ad_expr_t  *expr;                     ///< `switch` expression.
  slist_t     case_list;                ///< `switch` cases.
};

/**
 * A statement in the **ad** language.
 */
struct ad_stmnt {
  ad_stmnt_kind_t     kind;             ///< Statement kind.
  ad_loc_t            loc;              ///< Source location.

  /**
   * Additional data for each \ref kind.
   */
  union {
    ad_break_stmnt_t  break_stmnt;      ///< \ref ad_break_stmnt members.
    // nothing needed for compound statement
    ad_decl_stmnt_t   decl_stmnt;       ///< \ref ad_decl_stmnt members.
    ad_if_stmnt_t     if_stmnt;         ///< \ref ad_if_stmnt members.
    ad_let_stmnt_t    let_stmnt;        ///< \ref ad_let_stmnt members.
    ad_switch_stmnt_t switch_stmnt;     ///< \ref ad_switch_stmnt members.
  };
};

/**
 * Convenience macro to iterate over the cases of a `switch` statement.
 *
 * @param VAR The \ref slist_node loop variable.
 * @param STATEMENT The \ref ad_switch_stmnt to iterate the cases of.
 */
#define FOREACH_SWITCH_CASE(VAR,STATEMENT) \
  FOREACH_SLIST_NODE( VAR, &(STATEMENT)->switch_stmnt.case_list )

////////// expressions ////////////////////////////////////////////////////////

/**
 * Literal value expression.
 */
struct ad_literal_expr {
  ad_type_t const  *type;               ///< The type of the value.

  /**
   * The value.
   */
  union {
    int64_t         ival;               ///< Signed integer value.
    uint64_t        uval;               ///< Unsigned integer value.
    double          fval;               ///< Floating-point value.

    // UTF characters.
    char8_t         c8;                 ///< UTF-8 character.
    char16_t        c16;                ///< UTF-16 character.
    char32_t        c32;                ///< UTF-32 character.

    // UTF strings.
    char           *s;                  ///< Any string.
    char8_t        *s8;                 ///< UTF-8 string.
    char16_t       *s16;                ///< UTF-16 string.
    char32_t       *s32;                ///< UTF-32 string.

    // Miscellaneous.
    ad_type_t       cast_type;
  };
};

/**
 * Unary expression; used for:
 *  + Pointer address
 *  + Pointer dereference
 *  + Bitwise complement
 *  + Negation
 */
struct ad_unary_expr {
  ad_expr_t *sub_expr;                  ///< Sub-expression.
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
  ad_expr_t *lhs_expr;                  ///< Left-hand-side expression.
  ad_expr_t *rhs_expr;                  ///< Right-hand-side expression.
};

/**
 * Ternary expression; used for:
 *  + `?:`
 */
struct ad_ternary_expr {
  ad_expr_t *cond_expr;                 ///< Condition expression.
  ad_expr_t *true_expr;                 ///< True-expression.
  ad_expr_t *false_expr;                ///< False-expression.
};

/**
 * An expression.
 */
struct ad_expr {
  ad_expr_kind_t      expr_kind;        ///< Expression kind.
  ad_loc_t            loc;              ///< Source location.

  /**
   * Additional data for each \ref expr_kind.
   */
  union {
    ad_expr_err_t     err;              ///< Error expression.
    ad_literal_expr_t literal;          ///< Literal expression.
    char const       *name;             ///< Name expression.
    ad_unary_expr_t   unary;            ///< Unary expression.
    ad_binary_expr_t  binary;           ///< Binary expression.
    ad_ternary_expr_t ternary;          ///< Ternary expression.
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
 * Frees all memory used by \a value _including_ \a value itself.
 *
 * @param value The \ref ad_enum_value to free.
 */
void ad_enum_value_free( ad_enum_value_t *value );

/**
 * Gets the name of \a kind.
 *
 * @param kind The \ref ad_rep_kind to get the name of.
 * @return Returns said name.
 */
NODISCARD
char const* ad_rep_kind_name( ad_rep_kind_t kind );

/**
 * Frees all the memory used by \a statement.
 *
 * @param statement The \ref ad_stmnt to free.  If NULL, does nothing.
 *
 * @sa ad_stmnt_list_cleanup()
 */
void ad_stmnt_free( ad_stmnt_t *statement );

/**
 * Cleans-up all memory associated with \a statement_list.
 *
 * @param statement_list The \ref ad_stmnt_list_t to clean-up.  If NULL, does
 * nothing.
 *
 * @sa ad_stmnt_free()
 */
void ad_stmnt_list_cleanup( ad_stmnt_list_t *statement_list );

/**
 * Gets whether \a is a null-terminated string (as opposed to a single
 * character).
 *
 * @param tid The type ID to use.
 * @return Returns `true` only if \a tid is null-terminated.
 */
NODISCARD
inline bool ad_tid_is_null_term( ad_tid_t tid ) {
  return (tid & T_NULL) != 0;
}

/**
 * Gets whether \a tid is a signed integer.
 *
 * @param tid The type ID to use.
 * @return Returns `true` only if \a tid is signed.
 */
NODISCARD
inline bool ad_tid_is_signed( ad_tid_t tid ) {
  return (tid & T_SIGNED) != 0;
}

/**
 * Gets the size (in bits) of the type represented by \a tid.
 *
 * @param tid The type ID to use.
 * @return Returns said size.
 */
NODISCARD
inline unsigned ad_tid_size( ad_tid_t tid ) {
  return (tid & T_MASK_SIZE) >> 8;
}

/**
 * Gets the endianness of \a tid.
 *
 * @param tid The type ID to use.
 * @return Returns said endianness.
 */
NODISCARD
inline endian_t ad_tid_endian( ad_tid_t tid ) {
  return tid & T_MASK_ENDIAN;
}

/**
 * Gets the \ref ad_tid_kind of \a tid.
 *
 * @param tid The \ref ad_tid_t to get the \ref ad_tid_kind of.
 * @return Returns said \a ref ad_tid_kind.
 *
 * @sa ad_tid_kind_name()
 */
NODISCARD
inline ad_tid_kind_t ad_tid_kind( ad_tid_t tid ) {
  return tid & T_MASK_TYPE;
}

/**
 * Gets the name of \a kind.
 *
 * @param kind The \ref ad_tid_kind to get the name of.
 * @return Returns said name.
 *
 * @sa ad_tid_kind()
 */
NODISCARD
char const* ad_tid_kind_name( ad_tid_kind_t kind );

/**
 * Comparison function for \ref ad_type objects.
 *
 * @param i_type A pointer to the first \ref ad_type.
 * @param j_type A pointer to the second \ref ad_type.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the type name pointed to by \a i_type is less than, equal to, or
 * greater than the `typedef` name pointed to by \a j_type.
 */
NODISCARD
int ad_type_cmp( ad_type_t const *i_tdef, ad_type_t const *j_tdef );

/**
 * Checks whether two types are equal _except_ for their names.
 *
 * @param i_ast The first type; may be NULL.
 * @param j_ast The second type; may be NULL.
 * @return Returns `true` only if the two types are equal _except_ for their
 * names.
 */
NODISCARD
bool ad_type_equal( ad_type_t const *i_type, ad_type_t const *j_type );

/**
 * Frees all the memory used by \a type.
 *
 * @param type The `ad_type` to free.  If NULL, does nothing.
 *
 * @sa ad_type_new()
 */
void ad_type_free( ad_type_t *type );

/**
 * Creates a new \ref ad_type.
 *
 * @param tid The \ref ad_tid_t  of the type to create.
 * @return Returns a new \ref ad_Type.
 *
 * @sa ad_type_free()
 */
NODISCARD
ad_type_t* ad_type_new( ad_tid_t tid );

/**
 * Gets the size in bits of \a t.
 *
 * @param t A pointer to the \ref ad_type to get the size of.
 * @return Returns said size.
 */
NODISCARD
unsigned ad_type_size( ad_type_t const *t );

/**
 * Un-`typedef`s \a type, i.e., if \a type is of type #T_TYPEDEFl returns the
 * \ref ad_type the `typedef` is for.
 *
 * @param type The \ref ad_type to un-`typedef`.
 * @return Returns the \ref ad_type the `typedef` is for or \a type if \a type
 * is not of kind #T_TYPEDEF>
 */
NODISCARD
ad_type_t const* ad_type_untypedef( ad_type_t const *type );

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_types_H */
/* vim:set et sw=2 ts=2: */
