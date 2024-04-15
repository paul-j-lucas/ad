/*
**      ad -- ASCII dump
**      src/parser.y
**
**      Copyright (C) 2019-2024  Paul J. Lucas, et al.
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

/**
 * @file
 * Defines helper macros, data structures, variables, functions, and the
 * **ad** grammar.
 */

/** @cond DOXYGEN_IGNORE */

%define api.header.include { "parser.h" }
%expect 6

%{
/** @endcond */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#include "debug.h"
#include "did_you_mean.h"
#include "expr.h"
#include "keyword.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "slist.h"
#include "sname.h"
#include "symbol.h"
#include "typedef.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __GNUC__
// Silence these warnings for Bison-generated code.
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Wunreachable-code"
#endif /* __GNUC__ */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

// Developer aid for tracing when Bison %destructors are called.
#if 0
#define DTRACE                    EPRINTF( "%d: destructor\n", __LINE__ )
#else
#define DTRACE                    NO_OP
#endif

#define IF_AD_DEBUG(...) \
  BLOCK( if ( opt_ad_debug != AD_DEBUG_NO ) { __VA_ARGS__ } )

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-group Parser
 * Helper macros, data structures, variables, functions, and the grammar for
 * **ad** declarations.
 * @{
 */

/**
 * Calls #elaborate_error_dym() with a \ref dym_kind_t of #DYM_NONE.
 *
 * @param ... Arguments passed to fl_elaborate_error().
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * @code
 *  | Y_DEFINE error
 *    {
 *      elaborate_error( "name expected" );
 *    }
 * @endcode
 *
 * @sa elaborate_error_dym()
 * @sa keyword_expected()
 * @sa punct_expected()
 */
#define elaborate_error(...) \
  elaborate_error_dym( DYM_NONE, __VA_ARGS__ )

/**
 * Calls fl_elaborate_error() followed by #PARSE_ABORT().
 *
 * @param DYM_KINDS The bitwise-or of the kind(s) of things possibly meant.
 * @param ... Arguments passed to fl_elaborate_error().
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * ```
 *  | error
 *    {
 *      elaborate_error_dym( DYM_COMMANDS, "unexpected token" );
 *    }
 * ```
 *
 * @sa elaborate_error()
 * @sa keyword_expected()
 * @sa punct_expected()
 */
#define elaborate_error_dym(DYM_KINDS,...) BLOCK( \
  fl_elaborate_error( __FILE__, __LINE__, (DYM_KINDS), __VA_ARGS__ ); PARSE_ABORT(); )

/**
 * Aborts the current parse (presumably after an error message has been
 * printed).
 */
#define PARSE_ABORT() \
  BLOCK( parse_cleanup( /*fatal_error=*/true ); YYABORT; )

/**
 * Evaluates \a EXPR: if `false`, calls #PARSE_ABORT().
 *
 * @param EXPR The expression to evalulate.
 */
#define PARSE_ASSERT(EXPR)        BLOCK( if ( !(EXPR) ) PARSE_ABORT(); )

/**
 * Calls fl_punct_expected() followed by #PARSE_ABORT().
 *
 * @param PUNCT The punctuation character that was expected.
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * @code
 *  : ','
 *  | error
 *    {
 *      punct_expected( ',' );
 *    }
 * @endcode
 *
 * @sa elaborate_error()
 * @sa elaborate_error_dym()
 * @sa keyword_expected()
 */
#define punct_expected(PUNCT) BLOCK( \
  fl_punct_expected( __FILE__, __LINE__, (PUNCT) ); PARSE_ABORT(); )

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-dump-group Debugging Macros
 * Macros that are used to dump a trace during parsing when `opt_ad_debug` is
 * `true`.
 * @ingroup parser-group
 * @{
 */

/**
 * Dumps a `bool`.
 *
 * @param KEY The key name to print.
 * @param BOOL The `bool` to dump.
 */
#define DUMP_BOOL(KEY,BOOL)  IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": %s", ((BOOL) ? "true" : "false") ); )


/**
 * Ends a dump block.
 *
 * @sa #DUMP_START()
 */
#define DUMP_END()                IF_AD_DEBUG( PUTS( "\n}\n\n" ); )

/**
 * Dumps an ad_expr.
 *
 * @param KEY The key name to print.
 * @param EXPR The ad_expr to dump.
 *
 * @sa #DUMP_EXPR_LIST()
 */
#define DUMP_EXPR(KEY,EXPR) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ":" ); ad_expr_dump( (EXPR), stdout ); )

/**
 * Dumps an `s_list` of ad_expr_t.
 *
 * @param KEY The key name to print.
 * @param EXPR_LIST The `s_list` of ad_expr_t to dump.
 *
 * @sa #DUMP_EXPR()
 */
#define DUMP_EXPR_LIST(KEY,EXPR_LIST) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": " ); ad_expr_list_dump( &(EXPR_LIST), /*indent=*/1, stdout ); )

/**
 * Possibly dumps a comma and a newline followed by the `printf()` arguments
 * &mdash; used for printing a key followed by a value.
 *
 * @param ... The `printf()` arguments.
 */
#define DUMP_KEY_IMPL(...) BLOCK(           \
  fput_sep( ",\n", &dump_comma, stdout );   \
  PRINTF( "  " __VA_ARGS__ ); )

/**
 * Dumps an integer.
 *
 * @param KEY The key name to print.
 * @param NUM The integer to dump.
 */
#define DUMP_INT(KEY,NUM) \
  DUMP_KEY_IMPL( KEY ": %d", STATIC_CAST( int, (NUM) ) )

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 */
#define DUMP_STR(KEY,STR) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": " ); fputs_quoted( (STR), '"', stdout ); )

/**
 * Starts a dump block.
 *
 * @remarks If a production has a result, it should be dumped as the final
 * thing before the #DUMP_END() with the KEY of `$$_` followed by a suffix
 * denoting the type, e.g., `expr`.  For example:
 * ```
 *  DUMP_START( "rule",
 *              "subrule_1 subrule_2 ..." );
 *  DUMP_AST( "subrule_1", $1 );
 *  DUMP_STR( "name", $2 );
 *  DUMP_AST( "subrule_2", $3 );
 *  // ...
 *  DUMP_AST( "$$_expr", $$ );
 *  DUMP_END();
 * ```
 *
 * @param NAME The grammar production name.
 * @param RULE The grammar production rule.
 *
 * @note The dump block _must_ end with #DUMP_END().
 *
 * @sa #DUMP_END()
 */
#define DUMP_START(NAME,RULE) \
  bool dump_comma = false;    \
  IF_AD_DEBUG( PUTS( "{\n  rule: {\n    lhs: \"" NAME "\",\n    rhs: \"" RULE "\"\n  },\n" ); )

/**
 * Dumps a \ref ad_tid_t.
 *
 * @param KEY The key name to print.
 * @param TID The \ref ad_tid_t to dump.
 */
#define DUMP_TID(KEY,TID) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": " ); ad_tid_dump( (TID), stdout ); )

/**
 * Dumps a \ref ad_type.
 *
 * @param KEY The key name to print.
 * @param TYPE The \ref ad_type to dump.
 */
#define DUMP_TYPE(KEY,TYPE) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": " ); ad_type_dump( TYPE, stdout ); )

/** @} */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup parser-group
 * @{
 */

/**
 * Inherited attributes.
 *
 * @remarks These are grouped into a `struct` (rather than having them as
 * separate global variables) so that they can all be reset (mostly) via a
 * single assignment from `{0}`.
 *
 * @sa ia_cleanup()
 */
struct in_attr {
  sname_t scope_sname;                  ///< Current scope name, if any.
};
typedef struct in_attr in_attr_t;

// local functions
PJL_PRINTF_LIKE_FUNC(4)
static void fl_elaborate_error( char const*, int, dym_kind_t, char const*,
                                ... );

// local variables
static in_attr_t      in_attr;          ///< Inherited attributes.
static slist_t        statement_list;   ///< List of non-declaration statements.

////////// local functions ////////////////////////////////////////////////////

/**
 * Defines a type by adding it to the global set.
 *
 * @param type The type to define.
 * @return Returns `true` either if the type was defined or it's equivalent to
 * an existing type; `false` if a different type already exists having the same
 * name.
 */
NODISCARD
static bool define_type( ad_type_t const *type ) {
  assert( type != NULL );

  ad_typedef_t const *const tdef = ad_typedef_add( type );
  if ( tdef->type != type ) {
    //
    // Type was NOT added because a previously declared type having the same
    // name was returned : check if the types are equal.
    //
    if ( !ad_type_equal( tdef->type, type ) ) {
      print_error( &type->loc, "type " );
      print_type_aka( type, stderr );
      EPUTS( " redefinition incompatible with original type \"" );
      print_type( tdef->type, stderr );
      EPUTS( "\"\n" );
      return false;
    }
  }

  return true;
}

/**
 * A special case of fl_elaborate_error() that prevents oddly worded error
 * messages when a punctuation character is expected by not doing keyword look-
 * ups of the error token.
 *
 * For example, if fl_elaborate_error() were used for the following \b ad
 * command, you'd get the following:
 * @code
 * explain void f(int g const)
 *                      ^
 * 29: syntax error: "const": ',' expected ("const" is a keyword)
 * @endcode
 * and that looks odd since, if a punctuation character was expected, it seems
 * silly to point out that the encountered token is a keyword.
 *
 * @note This function isn't normally called directly; use the
 * #punct_expected() macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param punct The punctuation character that was expected.
 *
 * @sa fl_elaborate_error()
 * @sa yyerror()
 */
static void fl_punct_expected( char const *file, int line, char punct ) {
  EPUTS( ": " );
  print_debug_file_line( file, line );

  char const *const error_token = lexer_printable_token();
  if ( error_token != NULL )
    EPRINTF( "\"%s\": ", error_token );

  EPRINTF( "'%c' expected\n", punct );
}

/**
 * Cleans-up all resources used by \ref in_attr "inherited attributes".
 */
static void ia_cleanup( void ) {
  sname_cleanup( &in_attr.scope_sname );
  in_attr = (in_attr_t){ 0 };
}

/**
 * Cleans-up parser data after each parse.
 *
 * @param fatal_error Must be `true` only if a fatal semantic error has
 * occurred and `YYABORT` is about to be called to bail out of parsing by
 * returning from yyparse().
 */
static void parse_cleanup( bool fatal_error ) {
  //
  // We need to reset the lexer differently depending on whether we completed a
  // parse with a fatal error.  If so, do a "hard" reset that also resets the
  // EOF flag of the lexer.
  //
  lexer_reset( /*hard_reset=*/fatal_error );

  ia_cleanup();
}

/**
 * TODO
 *
 * @param name TODO
 * @return TODO
 */
static sname_t sname_current( char const *name ) {
  sname_t sname;
  sname_init( &sname );
  sname_append_sname( &sname, &in_attr.scope_sname );
  sname_append_name( &sname, name );
  return sname;
}

/**
 * Called by Bison to print a parsing error message to standard error.
 *
 * @remarks A custom error printing function via `%%define parse.error custom`
 * and
 * [`yyreport_syntax_error()`](https://www.gnu.org/software/bison/manual/html_node/Syntax-Error-Reporting-Function.html)
 * is not done because printing a (perhaps long) list of all the possible
 * expected tokens isn't helpful.
 * @par
 * It's also more flexible to be able to call one of #elaborate_error(),
 * #keyword_expected(), or #punct_expected() at the point of the error rather
 * than having a single function try to figure out the best type of error
 * message to print.
 *
 * @note A newline is _not_ printed since the error message will be appended to
 * by fl_elaborate_error().  For example, the parts of an error message are
 * printed by the functions shown:
 *
 *      42: syntax error: "int": "into" expected
 *      |--||----------||----------------------|
 *      |   |           |
 *      |   yyerror()   fl_elaborate_error()
 *      |
 *      print_loc()
 *
 * @param msg The error message to print.  Bison invariably passes `syntax
 * error`.
 *
 * @sa fl_elaborate_error()
 * @sa fl_punct_expected()
 * @sa print_loc()
 */
static void yyerror( char const *msg ) {
  assert( msg != NULL );

  ad_loc_t loc = lexer_loc();
  print_loc( &loc );

  color_start( stderr, sgr_error );
  EPUTS( msg );                         // no newline
  color_end( stderr, sgr_error );

  //
  // A syntax error has occurred, but syntax errors aren't fatal since Bison
  // tries to recover.  We'll clean-up the current parse, but YYABORT won't be
  // called so we won't bail out of parsing by returning from yyparse(); hence,
  // parsing will continue.
  //
  parse_cleanup( /*fatal_error=*/false );
}

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

%union {
  endian_t            endian_val;
  ad_enum_value_t     enum_val;
  ad_expr_t          *expr;       // for the expression being built
  ad_expr_kind_t      expr_kind;  // expression kind
  int                 int_val;
  slist_t             list;       // multipurpose list
  char const         *literal;    // token literal
  char               *name;       // name being declared
  ad_rep_t            rep_val;
  ad_statement_t     *statement;
  char               *str_val;    // quoted string value
  ad_switch_case_t   *switch_case;
  ad_type_t          *type;
  ad_typedef_t const *tdef;       // typedef
  ad_tid_t            tid;
}

                    // ad keywords
%token              Y_alignas;
%token  <expr_kind> Y_bool
%token              Y_break
%token              Y_case
%token              Y_default
%token  <expr_kind> Y_enum
%token  <expr_kind> Y_false
%token  <expr_kind> Y_float
%token  <expr_kind> Y_int
%token              Y_offsetof
%token  <expr_kind> Y_struct
%token              Y_switch
%token  <expr_kind> Y_true
%token  <expr_kind> Y_typedef
%token  <expr_kind> Y_uint
%token  <expr_kind> Y_utf

                    //
                    // C operators that are single-character are represented by
                    // themselves directly.  All multi-character operators are
                    // given Y_ names.
                    //

                    // C operators: precedence 17
%left               Y_COLON_COLON           "::"

                    // C operators: precedence 16
%token  <oper_id>   Y_PLUS_PLUS             "++"
%token  <oper_id>   Y_MINUS_MINUS           "--"
%left   <oper_id>                           '(' ')'
                                            '[' ']'
%token  <oper_id>                           '.'
%token  <oper_id>   Y_ARROW                 "->"
                    // C operators: precedence 15
%token  <oper_id>                           '&'
                                            '*'
                                            '!'
                 // Y_UMINUS             // '-' -- covered by '-' below
                 // Y_PLUS               // '+' -- covered by '+' below
%right  <oper_id>   Y_sizeof
%right  <oper_id>                           '~'
                    // C operators: precedence 14
                    // C operators: precedence 13
                                         // '*' -- covered by '*' above
%left   <oper_id>                           '/'
                                            '%'
                    // C operators: precedence 12
%left   <oper_id>                           '+'
                                            '-'
                    // C operators: precedence 11
%left   <oper_id>   Y_LESS_LESS             "<<"
                    Y_GREATER_GREATER       ">>"
                    // C operators: precedence 10
                    // C operators: precedence 9
%left   <oper_id>                           '<' '>'
                    Y_LESS_EQUAL            "<="
                    Y_GREATER_EQUAL         ">="

                    // C operators: precedence 8
%left   <oper_id>   Y_EQUAL_EQUAL           "=="
                    Y_EXCLAM_EQUAL          "!="
                    // C operators: precedence 7
                    Y_BIT_AND            // '&' -- covered by Y_AMPER
                    // C operators: precedence 6
%left   <oper_id>                           '^'
                    // C operators: precedence 5
%left   <oper_id>                           '|'
                    // C operators: precedence 4
%token  <oper_id>   Y_AMPER_AMPER           "&&"
                    // C operators: precedence 3
%left   <oper_id>   Y_PIPE_PIPE             "||"
                    // C operators: precedence 2
%right  <oper_id>                           '?' ':'
                                            '='
                    Y_PERCENT_EQUAL         "%="
                    Y_AMPER_EQUAL           "&="
                    Y_STAR_EQUAL            "*="
                    Y_PLUS_EQUAL            "+="
                    Y_MINUS_EQUAL           "-="
                    Y_SLASH_EQUAL           "/="
                    Y_LESS_LESS_EQUAL       "<<="
                    Y_GREATER_GREATER_EQUAL ">>="
                    Y_CIRC_EQUAL            "^="
                    Y_PIPE_EQUAL            "|="

                    // C operators: precedence 1
%left   <oper_id>                           ','


                    // Miscellaneous
%token                                      ';'
%token                                      '{' '}'
%token  <str_val>   Y_CHAR_LIT
%token              Y_END
%token              Y_ERROR
%token  <int_val>   Y_INT_LIT
%token  <name>      Y_NAME
%token  <str_val>   Y_STR_LIT
%token  <type>      Y_TYPEDEF_TYPE

                    //
                    // When the lexer returns Y_LEXER_ERROR, it means that
                    // there was a lexical error and that it's already printed
                    // an error message so the parser should NOT print an error
                    // message and just call PARSE_ABORT().
                    ///
%token              Y_LEXER_ERROR

//
// Grammar rules are named according to the following conventions.  In order,
// if a rule:
//
//  1. Is a list, "_list" is appended.
//  2. Is of type:
//      + <name>: "_name" is appended.
//      + <int_val>: "_int" is appended.
//      + <sname>: "_sname" is appended.
//      + <tid>: "_tid" is appended.
//  3. Is expected, "_exp" is appended; is optional, "_opt" is appended.
//
// Sort using: sort -bdfk3

                    // Declarations
%type <rep_val>     array_opt
%type <tid>         builtin_tid
%type <enum_val>    enumerator
%type <list>        enumerator_list
%type <switch_case> switch_case
%type <list>        switch_case_list switch_case_list_opt

                    // Expressions
%type <expr>        additive_expr
%type <expr>        assign_expr
%type <expr>        bitwise_and_expr
%type <expr>        bitwise_exclusive_or_expr
%type <expr>        bitwise_or_expr
%type <expr>        cast_expr
%type <expr>        conditional_expr
%type <expr>        equality_expr
%type <expr>        expr expr_exp
%type <expr>        logical_and_expr
%type <expr>        logical_or_expr
%type <expr>        match_expr_opt
%type <expr>        multiplicative_expr
%type <expr>        postfix_expr
%type <expr>        primary_expr
%type <expr>        relational_expr
%type <expr>        shift_expr
%type <expr>        unary_expr

                    // Statements
%type <statement>   break_statement
%type <statement>   declaration
%type <statement>   enum_declaration
%type <statement>   field_declaration
%type <statement>   statement
%type <list>        statement_list statement_list_opt
%type <statement>   struct_declaration
%type <statement>   switch_statement
%type <statement>   typedef_declaration

                    // Miscellaneous
%type <int_val>     alignas_opt
%type <list>        argument_expr_list argument_expr_list_opt
%type <expr_kind>   assign_op
%type <int_val>     int_exp
%type <name>        name_exp
%type <type>        type
%type <expr>        type_endian_expr type_endian_expr_opt
%type <expr_kind>   unary_op

// Bison %destructors.
%destructor { DTRACE; FREE( $$ );               } <name>
%destructor { DTRACE; free( $$ );               } <str_val>
%destructor { DTRACE; ad_expr_free( $$ );       } <expr>
%destructor { DTRACE; ad_type_free( $$ );       } <type>
%destructor { DTRACE; ad_statement_free( $$ );  } <statement>

/*****************************************************************************/
%%

///////////////////////////////////////////////////////////////////////////////
//  STATEMENTS                                                               //
///////////////////////////////////////////////////////////////////////////////

statement_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | statement_list
  ;

statement_list
  : statement_list statement
    {
      if ( $statement != NULL )
        slist_push_back( &statement_list, $statement );
    }
  | statement
    {
      slist_init( &$$ );
      if ( $statement != NULL )
        slist_push_back( &$$, $statement );
    }
  ;

statement
  : break_statement semi_exp
  | declaration semi_exp
  | switch_statement
  | error
    {
      if ( lexer_printable_token() != NULL )
        elaborate_error( "unexpected token" );
      else
        elaborate_error( "unexpected end of statement" );
      $$ = NULL;
    }
  ;

/// break statement ///////////////////////////////////////////////////////////

break_statement
  : Y_break
    {
      $$ = MALLOC( ad_statement_t, 1 );
      *$$ = (ad_statement_t){
        .kind = S_BREAK,
        .loc = @$
      };
    }
  ;

/// switch statement //////////////////////////////////////////////////////////

switch_statement
  : Y_switch lparen_exp expr rparen_exp
    lbrace_exp switch_case_list_opt[case_list] '}'
    {
      $$ = MALLOC( ad_statement_t, 1 );
      *$$ = (ad_statement_t){
        .kind = S_SWITCH,
        .loc = @$,
        .switch_s = {
          .expr = $expr,
          .case_list = slist_move( &$case_list )
        }
      };
    }
  ;

switch_case_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | switch_case_list
  ;

switch_case_list
  : switch_case_list[case_list] switch_case[case]
    {
      $$ = $case_list;
      slist_push_back( &$$, &$case );
    }

  | switch_case[case]
    {
      slist_init( &$$ );
      slist_push_back( &$$, &$case );
    }
  ;

switch_case
  : Y_case expr_exp[expr] colon_exp statement_list_opt[statement_list]
    {
      DUMP_START( "switch_case", "CASE expr ':' statement_list_opt" );
      DUMP_EXPR( "expr", $expr );

      $$ = MALLOC( ad_switch_case_t, 1 );
      *$$ = (ad_switch_case_t){
        .expr = $expr,
        .statement_list = slist_move( &$statement_list )
      };

      DUMP_END();
    }

  | Y_default colon_exp statement_list_opt[statement_list]
    {
      $$ = MALLOC( ad_switch_case_t, 1 );
      *$$ = (ad_switch_case_t){
        .statement_list = slist_move( &$statement_list )
      };
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  DECLARATIONS                                                             //
///////////////////////////////////////////////////////////////////////////////

declaration
  : enum_declaration
  | field_declaration[field] match_expr_opt[match_expr]
    {
      assert( $field->kind = S_DECLARATION );
      $field->decl_s.match_expr = $match_expr;
      $$ = $field;
    }
  | struct_declaration
  | typedef_declaration
  ;

/// enum declaration //////////////////////////////////////////////////////////

enum_declaration
  : Y_enum name_exp[name] colon_exp type
    lbrace_exp enumerator_list[values] rbrace_exp
    {
      ad_type_t *const new_type = MALLOC( ad_type_t, 1 );
      *new_type = (ad_type_t){
        .sname = sname_current( $name ),
        .tid = T_ENUM | ($type->tid & (T_MASK_ENDIAN | T_MASK_SIZE)),
        .loc = @$,
        .enum_t = {
          .value_list = slist_move( &$values )
        }
      };
      PARSE_ASSERT( define_type( new_type ) );
      $$ = NULL;
    }
  ;

enumerator_list
  : enumerator_list[enum_list] ',' enumerator[enum]
    {
      $$ = $enum_list;
      slist_push_back( &$$, &$enum );
    }
  | enumerator[enum]
    {
      slist_init( &$$ );
      slist_push_back( &$$, &$enum );
    }
  ;

enumerator
  : Y_NAME[name] equals_exp int_exp[value]
    {
      $$.name = $name;
      $$.value = $value;
    }
  ;

/// field declaration /////////////////////////////////////////////////////////

field_declaration
  : alignas_opt[align] type name_exp[name] array_opt[array]
    {
      $$ = MALLOC( ad_statement_t, 1 );
      *$$ = (ad_statement_t){
        .kind = S_DECLARATION,
        .loc = @$,
        .decl_s = {
          .name = $name,
          .type = $type,
          .align = $align,
          .rep = $array
        }
      };
    }
  ;

alignas_opt
  : /* empty */                   { $$ = 0; }
  | Y_alignas lparen_exp int_exp[value] rparen_exp
    {
      $$ = $value;
    }
  ;

array_opt
  : /* empty */                   { $$ = (ad_rep_t){ .kind = AD_REP_1      }; }
  | '[' ']'                       { $$ = (ad_rep_t){ .kind = AD_REP_1      }; }
  | '[' '?' rbracket_exp          { $$ = (ad_rep_t){ .kind = AD_REP_0_1    }; }
  | '[' '*' rbracket_exp          { $$ = (ad_rep_t){ .kind = AD_REP_0_MORE }; }
  | '[' '+' rbracket_exp          { $$ = (ad_rep_t){ .kind = AD_REP_1_MORE }; }
  | '[' expr ']'
    {
      $$ = (ad_rep_t){ .kind = AD_REP_EXPR, .expr = $expr };
    }
  | '[' error ']'
    {
      elaborate_error( "array size expected" );
    }
  ;

match_expr_opt
  : /* empty */                   { $$ = NULL; }
  | "==" expr                     { $$ = $expr; }
  ;

/// struct declaration ////////////////////////////////////////////////////////

struct_declaration
  : Y_struct name_exp[name] lbrace_exp statement_list_opt[members] rbrace_exp
    {
      ad_type_t *const new_type = MALLOC( ad_type_t, 1 );
      *new_type = (ad_type_t){
        .sname = sname_current( $name ),
        .tid = T_STRUCT,
        .loc = @$,
        .struct_t = {
          .member_list = slist_move( &$members )
        }
      };
      PARSE_ASSERT( define_type( new_type ) );
      $$ = NULL;
    }
  ;

/// typedef declaration ///////////////////////////////////////////////////////

typedef_declaration
  : Y_typedef field_declaration[statement]
    {
      assert( $statement->kind == S_DECLARATION );
      ad_decl_t *const decl = &$statement->decl_s;
      ad_type_t *const new_type = MALLOC( ad_type_t, 1 );
      *new_type = (ad_type_t){
        .sname = sname_current( decl->name ),
        .tid = T_TYPEDEF,
        .loc = @$,
        .rep = decl->rep,
        .typedef_t = { .type = decl->type }
      };
      PARSE_ASSERT( define_type( new_type ) );
      $$ = NULL;
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  EXPRESSIONS                                                              //
///////////////////////////////////////////////////////////////////////////////

expr
  : assign_expr
  | expr[lhs_expr] ',' assign_expr[rhs_expr]
    {
      DUMP_START( "expr", "expr ',' assign_expr" );
      DUMP_EXPR( "expr", $lhs_expr );
      DUMP_EXPR( "assign_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_COMMA, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

expr_exp
  : expr
  | error
    {
      elaborate_error( "expression expected" );
      $$ = NULL;
    }
  ;

additive_expr
  : multiplicative_expr
  | additive_expr[lhs_expr] '+' multiplicative_expr[rhs_expr]
    {
      DUMP_START( "additive_expr", "unary_expr '+' assign_expr" );
      DUMP_EXPR( "additive_expr", $lhs_expr );
      DUMP_EXPR( "multiplicative_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_MATH_ADD, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | additive_expr[lhs_expr] '-' multiplicative_expr[rhs_expr]
    {
      DUMP_START( "additive_expr", "unary_expr '-' assign_expr" );
      DUMP_EXPR( "additive_expr", $lhs_expr );
      DUMP_EXPR( "multiplicative_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_MATH_SUB, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

assign_expr
  : conditional_expr
  | unary_expr[lhs_expr] assign_op[op] assign_expr[rhs_expr]
    {
      DUMP_START( "assign_expr", "unary_expr '&' assign_expr" );
      DUMP_EXPR( "unary_expr", $lhs_expr );
      DUMP_EXPR( "assign_expr", $rhs_expr );

      $$ = ad_expr_new( $op, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

bitwise_and_expr
  : equality_expr
  | bitwise_and_expr[lhs_expr] '&' equality_expr[rhs_expr]
    {
      DUMP_START( "bitwise_and_expr", "bitwise_and_expr '&' equality_expr" );
      DUMP_EXPR( "bitwise_and_expr", $lhs_expr );
      DUMP_EXPR( "equality_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_BIT_AND, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

bitwise_exclusive_or_expr
  : bitwise_and_expr
  | bitwise_exclusive_or_expr[lhs_expr] '^' bitwise_and_expr[rhs_expr]
    {
      DUMP_START( "bitwise_exclusive_or_expr",
                  "bitwise_exclusive_or_expr '^' bitwise_and_expr" );
      DUMP_EXPR( "bitwise_exclusive_or_expr", $lhs_expr );
      DUMP_EXPR( "bitwise_and_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_BIT_XOR, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

bitwise_or_expr
  : bitwise_exclusive_or_expr
  | bitwise_or_expr[lhs_expr] '|' bitwise_exclusive_or_expr[rhs_expr]
    {
      DUMP_START( "bitwise_or_expr",
                  "bitwise_or_expr '|' bitwise_exclusive_or_expr" );
      DUMP_EXPR( "bitwise_or_expr", $lhs_expr );
      DUMP_EXPR( "bitwise_exclusive_or_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_BIT_OR, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

cast_expr
  : unary_expr
  | '(' Y_NAME[name] rparen_exp cast_expr[expr]
    {
      $$ = ad_expr_new( AD_EXPR_CAST, &@$ );
      //$$->binary.lhs_expr = $2;
      (void)$name;
      $$->binary.rhs_expr = $expr;
    }
  ;

conditional_expr
  : logical_or_expr
  | logical_or_expr[c_expr] '?' expr[t_expr] ':' conditional_expr[f_expr]
    {
      DUMP_START( "conditional_expr",
                  "logical_or_expr '?' expr ':' conditional_expr" );
      DUMP_EXPR( "logical_or_expr", $c_expr );
      DUMP_EXPR( "expr", $t_expr );
      DUMP_EXPR( "conditional_expr", $f_expr );

      $$ = ad_expr_new( AD_EXPR_IF_ELSE, &@$ );
      $$->ternary.cond_expr = $c_expr;
      $$->ternary.sub_expr[0] = $t_expr;
      $$->ternary.sub_expr[1] = $f_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

equality_expr
  : relational_expr
  | equality_expr[lhs_expr] "==" relational_expr[rhs_expr]
    {
      DUMP_START( "equality_expr",
                  "equality_expr '==' relational_expr" );
      DUMP_EXPR( "equality_expr", $lhs_expr );
      DUMP_EXPR( "relational_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_REL_EQ, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | equality_expr[lhs_expr] "!=" relational_expr[rhs_expr]
    {
      DUMP_START( "equality_expr",
                  "equality_expr '!=' relational_expr" );
      DUMP_EXPR( "equality_expr", $lhs_expr );
      DUMP_EXPR( "relational_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_REL_NOT_EQ, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

logical_and_expr
  : bitwise_or_expr
  | logical_and_expr[lhs_expr] "&&" bitwise_or_expr[rhs_expr]
    {
      DUMP_START( "logical_and_expr",
                  "logical_and_expr '||' bitwise_or_expr" );
      DUMP_EXPR( "logical_and_expr", $lhs_expr );
      DUMP_EXPR( "bitwise_or_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_LOG_AND, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

logical_or_expr
  : logical_and_expr
  | logical_or_expr[lhs_expr] "||" logical_and_expr[rhs_expr]
    {
      DUMP_START( "logical_or_expr", "logical_or_expr '||' logical_and_expr" );
      DUMP_EXPR( "logical_or_expr", $lhs_expr );
      DUMP_EXPR( "logical_and_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_LOG_OR, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

multiplicative_expr
  : cast_expr
  | multiplicative_expr[lhs_expr] '*' cast_expr[rhs_expr]
    {
      DUMP_START( "multiplicative_expr", "multiplicative_expr '*' cast_expr" );
      DUMP_EXPR( "multiplicative_expr", $lhs_expr );
      DUMP_EXPR( "cast_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_MATH_MUL, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | multiplicative_expr[lhs_expr] '/' cast_expr[rhs_expr]
    {
      DUMP_START( "multiplicative_expr", "multiplicative_expr '/' cast_expr" );
      DUMP_EXPR( "multiplicative_expr", $lhs_expr );
      DUMP_EXPR( "cast_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_MATH_DIV, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | multiplicative_expr[lhs_expr] '%' cast_expr[rhs_expr]
    {
      DUMP_START( "multiplicative_expr", "multiplicative_expr '%' cast_expr" );
      DUMP_EXPR( "multiplicative_expr", $lhs_expr );
      DUMP_EXPR( "cast_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_MATH_MOD, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

postfix_expr
  : primary_expr
  | postfix_expr[lhs_expr] '[' expr[rhs_expr] ']'
    {
      DUMP_START( "postfix_expr", "postfix_expr '[' expr ']'" );
      DUMP_EXPR( "postfix_expr", $lhs_expr );
      DUMP_EXPR( "expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_ARRAY, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | postfix_expr[expr] '(' argument_expr_list_opt[arg_list] ')'
    {
      // TODO
      (void)$expr;
      (void)$arg_list;
      $$ = NULL;
    }
  | postfix_expr[expr] '.' Y_NAME[name]
    {
      DUMP_START( "postfix_expr", "postfix_expr '.' NAME" );
      DUMP_EXPR( "postfix_expr", $expr );
      DUMP_STR( "NAME", $name );

      ad_expr_t *const name_expr = ad_expr_new( AD_EXPR_NAME, &@name );
      name_expr->name = $name;

      $$ = ad_expr_new( AD_EXPR_STRUCT_MBR_REF, &@$ );
      $$->binary.lhs_expr = $expr;
      $$->binary.rhs_expr = name_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | postfix_expr[expr] "->" Y_NAME[name]
    {
      DUMP_START( "postfix_expr", "postfix_expr '->' NAME" );
      DUMP_EXPR( "postfix_expr", $expr );
      DUMP_STR( "NAME", $name );

      ad_expr_t *const name_expr = ad_expr_new( AD_EXPR_NAME, &@name );
      name_expr->name = $name;

      $$ = ad_expr_new( AD_EXPR_STRUCT_MBR_REF, &@$ );
      $$->binary.lhs_expr = $expr;
      $$->binary.rhs_expr = name_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | postfix_expr[expr] "++"
    {
      DUMP_START( "postfix_expr", "postfix_expr '++'" );
      DUMP_EXPR( "postfix_expr", $expr );

      $$ = ad_expr_new( AD_EXPR_MATH_INC_POST, &@$ );
      $$->unary.sub_expr = $expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | postfix_expr[expr] "--"
    {
      DUMP_START( "postfix_expr", "postfix_expr '--'" );
      DUMP_EXPR( "postfix_expr", $expr );

      $$ = ad_expr_new( AD_EXPR_MATH_DEC_POST, &@$ );
      $$->unary.sub_expr = $expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

argument_expr_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | argument_expr_list
  ;

argument_expr_list
  : argument_expr_list[list] ',' assign_expr[expr]
    {
      $$ = $list;
      slist_push_back( &$$, $expr );
    }
  | assign_expr[expr]
    {
      slist_init( &$$ );
      slist_push_back( &$$, $expr );
    }
  ;

primary_expr
  : Y_NAME[name]
    {
      $$ = ad_expr_new( AD_EXPR_NAME, &@$ );
      $$->name = $name;
    }
  | Y_INT_LIT
    {
      $$ = ad_expr_new( AD_EXPR_LITERAL, &@$ );
      $$->literal.type = &TB_INT64;
      $$->literal.ival = $1;
    }
  | Y_STR_LIT
    {
      $$ = ad_expr_new( AD_EXPR_LITERAL, &@$ );
      $$->literal.type = &TB_UTF8_0;
      $$->literal.s = $1;
    }
  | '(' expr ')'                  { $$ = $expr; }
  ;

relational_expr
  : shift_expr
  | relational_expr[lhs_expr] '<' shift_expr[rhs_expr]
    {
      DUMP_START( "relational_expr", "relational_expr '<' shift_expr" );
      DUMP_EXPR( "relational_expr", $lhs_expr );
      DUMP_EXPR( "shift_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_REL_LESS, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | relational_expr[lhs_expr] '>' shift_expr[rhs_expr]
    {
      DUMP_START( "relational_expr", "relational_expr '>' shift_expr" );
      DUMP_EXPR( "relational_expr", $lhs_expr );
      DUMP_EXPR( "shift_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_REL_GREATER, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | relational_expr[lhs_expr] "<=" shift_expr[rhs_expr]
    {
      DUMP_START( "relational_expr", "relational_expr '<=' shift_expr" );
      DUMP_EXPR( "relational_expr", $lhs_expr );
      DUMP_EXPR( "shift_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_REL_LESS_EQ, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | relational_expr[lhs_expr] ">=" shift_expr[rhs_expr]
    {
      DUMP_START( "relational_expr", "relational_expr '>=' shift_expr" );
      DUMP_EXPR( "relational_expr", $lhs_expr );
      DUMP_EXPR( "shift_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_REL_GREATER_EQ, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

shift_expr
  : additive_expr
  | shift_expr[lhs_expr] "<<" additive_expr[rhs_expr]
    {
      DUMP_START( "shift_expr", "shift_expr '<<' additive_expr" );
      DUMP_EXPR( "shift_expr", $lhs_expr );
      DUMP_EXPR( "additive_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_LEFT, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | shift_expr[lhs_expr] ">>" additive_expr[rhs_expr]
    {
      DUMP_START( "shift_expr", "shift_expr '>>' additive_expr" );
      DUMP_EXPR( "shift_expr", $lhs_expr );
      DUMP_EXPR( "additive_expr", $rhs_expr );

      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_RIGHT, &@$ );
      $$->binary.lhs_expr = $lhs_expr;
      $$->binary.rhs_expr = $rhs_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

unary_expr
  : postfix_expr
  | "++" unary_expr[expr]
    {
      DUMP_START( "unary_expr", "'++' unary_expr" );
      DUMP_EXPR( "unary_expr", $expr );

      $$ = ad_expr_new( AD_EXPR_MATH_INC_PRE, &@$ );
      $$->unary.sub_expr = $expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | "--" unary_expr[expr]
    {
      DUMP_START( "unary_expr", "'--' unary_expr" );
      DUMP_EXPR( "unary_expr", $expr );

      $$ = ad_expr_new( AD_EXPR_MATH_DEC_PRE, &@$ );
      $$->unary.sub_expr = $expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | unary_op[op] cast_expr[expr]
    {
      DUMP_START( "unary_expr", "unary_op cast_expr" );
      DUMP_EXPR( "cast_expr", $expr );

      $$ = ad_expr_new( $op, &@$ );
      $$->unary.sub_expr = $expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | Y_sizeof unary_expr[expr]
    {
      DUMP_START( "unary_expr", "SIZEOF unary_expr" );
      DUMP_EXPR( "unary_expr", $expr );

      $$ = ad_expr_new( AD_EXPR_SIZEOF, &@$ );
      $$->unary.sub_expr = $expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | Y_sizeof '(' Y_NAME[name] rparen_exp
    {
      DUMP_START( "unary_expr", "SIZEOF '(' NAME ')'" );
      DUMP_STR( "NAME", $name );

      ad_typedef_t const *const tdef = ad_typedef_find_name( $name );
      if ( tdef == NULL ) {
        print_error( &@name, "\"%s\": no such type\n", $name );
        free( $name );
        PARSE_ABORT();
      }

      $$ = ad_expr_new( AD_EXPR_LITERAL, &@$ );
      $$->literal = (ad_literal_expr_t){
        .type = &TB_UINT64,
        .uval = ad_type_size( tdef->type )
      };
      free( $name );

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

assign_op
  : '='                           { $$ = AD_EXPR_ASSIGN; }
  | "%="                          { $$ = AD_EXPR_MATH_MOD; }
  | "&="                          { $$ = AD_EXPR_BIT_AND; }
  | "*="                          { $$ = AD_EXPR_MATH_MUL; }
  | "+="                          { $$ = AD_EXPR_MATH_ADD; }
  | "-="                          { $$ = AD_EXPR_MATH_SUB; }
  | "/="                          { $$ = AD_EXPR_MATH_DIV; }
  | "<<="                         { $$ = AD_EXPR_BIT_SHIFT_LEFT; }
  | ">>="                         { $$ = AD_EXPR_BIT_SHIFT_RIGHT; }
  | "^="                          { $$ = AD_EXPR_BIT_XOR; }
  | "|="                          { $$ = AD_EXPR_BIT_OR; }
  ;

unary_op
  : '&'                           { $$ = AD_EXPR_PTR_ADDR; }
  | '*'                           { $$ = AD_EXPR_PTR_DEREF; }
  | '+'                           { $$ = AD_EXPR_MATH_ADD; }
  | '-'                           { $$ = AD_EXPR_MATH_NEG; }
  | '~'                           { $$ = AD_EXPR_BIT_COMPL; }
  | '!'                           { $$ = AD_EXPR_LOG_NOT; }
  ;

///////////////////////////////////////////////////////////////////////////////
//  TYPES                                                                    //
///////////////////////////////////////////////////////////////////////////////

type
  : builtin_tid[tid] lt_exp expr_exp[size] type_endian_expr_opt[endian] gt_exp
    {
      DUMP_START( "type", "builtin_tid '<' expr type_endian_expr_opt '>'" );
      DUMP_TID( "builtin_tid", $tid );
      DUMP_EXPR( "type_endian_expr_opt", $endian );

      $$ = MALLOC( ad_type_t, 1 );
      *$$ = (ad_type_t){
        .tid = $tid,
        .size_expr = $size,
        .endian_expr = $endian,
        .loc = @$
      };

      DUMP_TYPE( "$$_type", $$ );
      DUMP_END();
    }
  | Y_TYPEDEF_TYPE
  ;

builtin_tid
  : Y_bool                        { $$ = T_BOOL; }
  | Y_float                       { $$ = T_FLOAT; }
  | Y_int                         { $$ = T_SIGNED | T_INT; }
  | Y_uint                        { $$ = T_INT; }
  | Y_utf                         { $$ = T_UTF; }
  ;

type_endian_expr_opt
  : /* empty */                   { $$ = NULL; }
  | type_endian_expr
  ;

type_endian_expr
  : ',' 'b'
    {
      DUMP_START( "type_endian_expr", "',' 'b'" );

      $$ = ad_expr_new( AD_EXPR_LITERAL, &@$ );
      $$->literal = (ad_literal_expr_t){
        .type = &TB_UINT64,
        .uval = ENDIAN_BIG
      };

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | ',' 'l'
    {
      DUMP_START( "type_endian_expr", "',' 'l'" );

      $$ = ad_expr_new( AD_EXPR_LITERAL, &@$ );
      $$->literal = (ad_literal_expr_t){
        .type = &TB_UINT64,
        .uval = ENDIAN_LITTLE
      };

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | ',' 'h'
    {
      DUMP_START( "type_endian_expr", "',' 'h'" );

      $$ = ad_expr_new( AD_EXPR_LITERAL, &@$ );
      $$->literal = (ad_literal_expr_t){
        .type = &TB_UINT64,
        .uval = ENDIAN_HOST
      };

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  | ',' expr
    {
      $$ = $expr;
    }
  | ',' error
    {
      elaborate_error( "one of 'b', 'l', 'h', or expression expected" );
      $$ = NULL;
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  MISCELLANEOUS                                                            //
///////////////////////////////////////////////////////////////////////////////

colon_exp
  : ':'
  | error
    {
      punct_expected( ':' );
    }
  ;

equals_exp
  : '='
  | error
    {
      punct_expected( '=' );
    }
  ;

gt_exp
  : '>'
  | error
    {
      punct_expected( '>' );
    }
  ;

int_exp
  : Y_INT_LIT
  | error
    {
      elaborate_error( "integer expected" );
    }
  ;

lbrace_exp
  : '{'
  | error
    {
      punct_expected( '{' );
    }
  ;

lparen_exp
  : '('
  | error
    {
      punct_expected( '(' );
    }
  ;

lt_exp
  : '<'
  | error
    {
      punct_expected( '<' );
    }
  ;

name_exp
  : Y_NAME
  | error
    {
      $$ = NULL;
      elaborate_error( "name expected" );
    }
  ;

rbrace_exp
  : '}'
  | error
    {
      punct_expected( '}' );
    }
  ;

rbracket_exp
  : ']'
  | error
    {
      punct_expected( ']' );
    }
  ;

rparen_exp
  : ')'
  | error
    {
      punct_expected( ')' );
    }
  ;

semi_exp
  : ';'
  | error
    {
      punct_expected( ';' );
    }
  ;

%%

/// @endcond

// Re-enable warnings.
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif /* __GNUC__ */

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints an additional parsing error message including a newline to standard
 * error that continues from where yyerror() left off.  Additionally:
 *
 * + If the lexer_printable_token() isn't NULL:
 *     + Checks to see if it's a keyword: if it is, mentions that it's a
 *       keyword in the error message.
 *     + May print "did you mean ...?" \a dym_kinds suggestions.
 *
 * + In debug mode, also prints the file & line where the function was called
 *   from as well as the ID of the lookahead token, if any.
 *
 * @note A newline _is_ printed.
 * @note This function isn't normally called directly; use the
 * #elaborate_error() or #elaborate_error_dym() macros instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param dym_kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param format A `printf()` style format string.  It _must not_ end in a
 * newline since this function prints its own newline.
 * @param ... Arguments to print.
 *
 * @sa #elaborate_error()
 * @sa #elaborate_error_dym()
 * @sa fl_punct_expected()
 * @sa yyerror()
 */
PJL_PRINTF_LIKE_FUNC(4)
static void fl_elaborate_error( char const *file, int line,
                                dym_kind_t dym_kinds, char const *format,
                                ... ) {
  assert( format != NULL );

  EPUTS( ": " );
  print_debug_file_line( file, line );

  char const *const error_token = lexer_printable_token();
  if ( error_token != NULL ) {
    EPRINTF( "\"%s\"", error_token );
    if ( opt_ad_debug != AD_DEBUG_NO ) {
      switch ( yychar ) {
        case YYEMPTY:
          EPUTS( " [<empty>]" );
          break;
        case YYEOF:
          EPUTS( " [<EOF>]" );
          break;
        default:
          EPRINTF( " [%d]", yychar );
      } // switch
    }
    EPUTS( ": " );
  }

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  if ( error_token != NULL ) {
    print_error_token_is_a( error_token );
    print_suggestions( dym_kinds, error_token );
  }

  EPUTC( '\n' );
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans up global parser data at program termination.
 */
void parser_cleanup( void ) {
  // do nothing
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
