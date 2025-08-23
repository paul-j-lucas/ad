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
%expect 4

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
#include "red_black.h"
#include "slist.h"
#include "sname.h"
#include "symbol.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Silence these warnings for Bison-generated code.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunreachable-code"

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
 * @param ... Arguments passed to l_elaborate_error().
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
 * @sa punct_expected()
 */
#define elaborate_error(...) \
  elaborate_error_dym( DYM_NONE, __VA_ARGS__ )

/**
 * Calls l_elaborate_error() followed by #PARSE_ABORT().
 *
 * @param DYM_KINDS The bitwise-or of the kind(s) of things possibly meant.
 * @param ... Arguments passed to l_elaborate_error().
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
 * @sa punct_expected()
 */
#define elaborate_error_dym(DYM_KINDS,...) BLOCK( \
  l_elaborate_error( __LINE__, (DYM_KINDS), __VA_ARGS__ ); PARSE_ABORT(); )

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
 * Calls l_punct_expected() followed by #PARSE_ABORT().
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
 */
#define punct_expected(PUNCT) BLOCK( \
  l_punct_expected( __LINE__, (PUNCT) ); PARSE_ABORT(); )

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-dump-group Debugging Macros
 * Macros that are used to dump a trace during parsing when \ref opt_ad_debug
 * is `true`.
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
  DUMP_KEY_IMPL( KEY ": " ); ad_expr_dump( (EXPR), stdout ); )

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
#define DUMP_INT(KEY,NUM) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": %d", STATIC_CAST( int, (NUM) ) ); )

/**
 * Dumps \a REP.
 *
 * @param KEY The key name to print.
 * @param REP The \ref ad_rep to dump.
 */
#define DUMP_REP(KEY,REP) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": " ); ad_rep_dump( (REP), stdout ); )

/**
 * Dumps a scoped name.
 *
 * @param KEY The key name to print.
 * @param SNAME The scoped name to dump.
 *
 * @sa #DUMP_STR()
 */
#define DUMP_SNAME(KEY,SNAME) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": " ); sname_dump( &(SNAME), stdout ); )

/**
 * Dumps an \ref ad_statement.
 *
 * @param KEY The key name to print.
 * @param statement The \ref ad_statement to dump.
 */
#define DUMP_STATEMENT(KEY,STATEMENT) IF_AD_DEBUG( \
  DUMP_KEY_IMPL( KEY ": " ); ad_statement_dump( (STATEMENT), stdout ); )

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
  sname_t     scope_sname;              ///< Current scope name, if any.
  ad_type_t  *cur_type;                 ///< Current type.
};
typedef struct in_attr in_attr_t;

// extern functions
NODISCARD
bool  ad_statement_list_check( slist_t const* ),
      ad_type_check( ad_type_t const* );

// local functions
PJL_PRINTF_LIKE_FUNC(3)
static void l_elaborate_error( int, dym_kind_t, char const*, ... );
PJL_DISCARD
static bool print_error_token( char const* );

// local variables
static in_attr_t      in_attr;          ///< Inherited attributes.
slist_t               statement_list;   ///< List of non-declaration statements.

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
static bool define_type( ad_type_t *type ) {
  assert( type != NULL );

  if ( !ad_type_check( type ) )
    goto error;                         // error message was already printed

  synfo_t *const synfo = sym_add( type, &type->sname, SYM_TYPE, /*scope=*/0 );

  if ( synfo->kind != SYM_TYPE ) {
    print_error( &type->loc, "type " );
    print_type_aka( type, stderr );
    EPUTS( " previously declared as \"" );
    print_decl( synfo->decl, stderr );
    EPUTS( "\"\n" );
    goto error;
  }

  if ( !ad_type_equal( synfo->type, type ) ) {
    //
    // Type was NOT added because a previously declared type having the same
    // name was returned and the types are NOT equal.
    //
    print_error( &type->loc, "type " );
    print_type_aka( type, stderr );
    EPUTS( " redefinition incompatible with original type \"" );
    print_type( synfo->type, stderr );
    EPUTS( "\"\n" );
    goto error;
  }

  return true;

error:
  ad_type_free( type );
  return false;
}

/**
 * Cleans-up all resources used by \ref in_attr "inherited attributes".
 */
static void ia_cleanup( void ) {
  sname_cleanup( &in_attr.scope_sname );
  in_attr = (in_attr_t){ 0 };
}

/**
 * A special case of l_elaborate_error() that prevents oddly worded error
 * messages when a punctuation character is expected by not doing keyword look-
 * ups of the error token.
 *
 * For example, if l_elaborate_error() were used for the following \b ad
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
 * @param line The line number within \a file where this function was called
 * from.
 * @param punct The punctuation character that was expected.
 *
 * @sa l_elaborate_error()
 * @sa yyerror()
 */
static void l_punct_expected( int line, char punct ) {
  EPUTS( ": " );
  print_debug_file_line( __FILE__, line );
  if ( print_error_token( printable_yytext() ) )
    EPUTS( ": " );
  EPRINTF( "'%c' expected\n", punct );
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
 * Gets the current full scoped name.
 *
 * @remarks
 * Given the declarations:
 * @code
 *  struct APP0 {
 *    enum unit_t : uint<8> {
 *      // ...
 * @endcode
 * and a \a name of `"unit_t"`, this function would return the scoped name of
 * `APP0::unit_t`.
 *
 * @param name The local name.  Ownership is taken.
 * @return Returns the current full scoped name.
 */
static sname_t sname_current( char *name ) {
  sname_t sname = sname_dup( &in_attr.scope_sname );
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
 * It's also more flexible to be able to call either #elaborate_error() or
 * #punct_expected() at the point of the error rather than having a single
 * function try to figure out the best type of error message to print.
 *
 * @note A newline is _not_ printed since the error message will be appended to
 * by l_elaborate_error().  For example, the parts of an error message are
 * printed by the functions shown:
 *
 *      42: syntax error: "int": "into" expected
 *      |--||----------||----------------------|
 *      |   |           |
 *      |   yyerror()   l_elaborate_error()
 *      |
 *      print_loc()
 *
 * @param msg The error message to print.  Bison invariably passes `syntax
 * error`.
 *
 * @sa l_elaborate_error()
 * @sa l_punct_expected()
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
  ad_decl_t          *decl;
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
  ad_tid_t            tid;
}

                    // ad keywords
%token              Y_alignas
%token  <expr_kind> Y_bool
%token              Y_break
%token              Y_case
%token              Y_default
%token              Y_else
%token  <expr_kind> Y_enum
%token  <expr_kind> Y_false
%token  <expr_kind> Y_float
%token              Y_if
%token  <expr_kind> Y_int
%token              Y_offsetof
%token  <expr_kind> Y_struct
%token              Y_switch
%token  <expr_kind> Y_true
%token  <expr_kind> Y_typedef
%token  <expr_kind> Y_uint
%token              Y_union
%token  <expr_kind> Y_utf

                    //
                    // C operators that are single-character are represented by
                    // themselves directly.  All multi-character operators are
                    // given Y_ names.
                    //

                    // C operators: precedence 17
%left               Y_COLON_COLON           "::"

                    // C operators: precedence 16
%token              Y_PLUS_PLUS             "++"
%token              Y_MINUS_MINUS           "--"
%left                                       '(' ')'
                                            '[' ']'
%token                                      '.'
%token              Y_MINUS_GREATER         "->"
                    // C operators: precedence 15
%token                                      '&'
                                            '*'
                                            '!'
                 // Y_UMINUS            // '-' -- covered by '-' below
                 // Y_PLUS              // '+' -- covered by '+' below
%right              Y_sizeof
%right                                      '~'
                    // C operators: precedence 14
                    // C operators: precedence 13
                                        // '*' -- covered by '*' above
%left                                       '/'
                                            '%'
                    // C operators: precedence 12
%left                                       '+'
                                            '-'
                    // C operators: precedence 11
%left               Y_LESS_LESS             "<<"
                    Y_GREATER_GREATER       ">>"
                    // C operators: precedence 10
                    // C operators: precedence 9
%left                                       '<' '>'
                    Y_LESS_EQUAL            "<="
                    Y_GREATER_EQUAL         ">="

                    // C operators: precedence 8
%left               Y_EQUAL_EQUAL           "=="
                    Y_EXCLAM_EQUAL          "!="
                    // C operators: precedence 7
                    Y_BIT_AND           // '&' -- covered by Y_AMPER
                    // C operators: precedence 6
%left                                       '^'
                    // C operators: precedence 5
%left                                       '|'
                    // C operators: precedence 4
%left               Y_AMPER_AMPER           "&&"
                    // C operators: precedence 3
%left               Y_PIPE_PIPE             "||"
                    // C operators: precedence 2
%right                                      '?' ':'
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
%left                                       ','


                    // Miscellaneous
%token                                      ';'
%token                                      '{' '}'
%token  <str_val>   Y_CHAR_LIT
%token  <decl>      Y_DECL
%token              Y_END
%token              Y_ERROR
%token  <int_val>   Y_INT_LIT
%token  <name>      Y_NAME
%token  <str_val>   Y_STR_LIT
%token              Y_TEMPLATE_END      // '>' matching '<'
%token  <type>      Y_TYPE              // user-defined type

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
%type <expr>        expr
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
%type <list>        else_statement_opt
%type <statement>   enum_declaration
%type <statement>   field_declaration
%type <statement>   if_statement
%type <statement>   statement
%type <list>        statement_list statement_list_opt
%type <statement>   struct_declaration
%type <statement>   switch_statement
%type <statement>   typedef_declaration
%type <statement>   union_declaration

                    // Miscellaneous
%type <int_val>     alignas_opt
%type <list>        argument_expr_list argument_expr_list_opt
%type <expr_kind>   assign_op
%type <type>        decl_or_type
%type <str_val>     format_opt
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

ad_file
  : statement_list_opt Y_END
    {
      PARSE_ASSERT( ad_statement_list_check( &statement_list ) );
    }
  | error
    {
      elaborate_error( "statement list expected" );
    }
  ;

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
  | if_statement
  | switch_statement
  | error
    {
      if ( printable_yytext() != NULL )
        elaborate_error( "unexpected token" );
      else
        elaborate_error( "unexpected end of statement" );
      $$ = NULL;
    }
  ;

////////// break statement ////////////////////////////////////////////////////

break_statement
  : Y_break
    {
      DUMP_START( "break_statement", "break" );

      $$ = MALLOC( ad_statement_t, 1 );
      *$$ = (ad_statement_t){
        .kind = AD_STMNT_BREAK,
        .loc = @$
      };

      DUMP_STATEMENT( "$$_statement", $$ );
      DUMP_END();
    }
  ;

////////// if statement ///////////////////////////////////////////////////////

if_statement
  : Y_if lparen_exp expr rparen_exp
    lbrace_exp statement_list_opt[if_list] '}'
    else_statement_opt[else_list]
    {
      DUMP_START( "if_statement", "IF ( expr ) statement_list_opt" );
      DUMP_EXPR( "expr", $expr );

      $$ = MALLOC( ad_statement_t, 1 );
      *$$ = (ad_statement_t){
        .kind = AD_STMNT_IF,
        .loc = @$,
        .if_s = {
          .expr = $expr,
          .if_list = slist_move( &$if_list ),
          .else_list = slist_move( &$else_list )
        }
      };

      DUMP_STATEMENT( "$$_statement", $$ );
      DUMP_END();
    }
  ;

else_statement_opt
  : /* empty */                   { slist_init( &$$ ); }
  | Y_else lbrace_exp statement_list_opt[else_list] '}'
    {
      $$ = $else_list;
    }
  ;

////////// switch statement ///////////////////////////////////////////////////

switch_statement
  : Y_switch lparen_exp expr rparen_exp
    lbrace_exp switch_case_list_opt[case_list] '}'
    {
      DUMP_START( "switch_statement",
                  "switch '(' expr ')' '{' switch_case_list_opt '}'" );
      DUMP_EXPR( "expr", $expr );

      $$ = MALLOC( ad_statement_t, 1 );
      *$$ = (ad_statement_t){
        .kind = AD_STMNT_SWITCH,
        .loc = @$,
        .switch_s = {
          .expr = $expr,
          .case_list = slist_move( &$case_list )
        }
      };

      DUMP_STATEMENT( "$$_statement", $$ );
      DUMP_END();
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
  : Y_case assign_expr[expr] colon_exp statement_list_opt[statement_list]
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
      DUMP_START( "declaration", "field_declaration match_expr_opt" );
      DUMP_STATEMENT( "field_declaration", $field );
      DUMP_EXPR( "match_expr_opt", $match_expr );

      assert( $field->kind = AD_STMNT_DECLARATION );
      $field->decl_s.match_expr = $match_expr;
      $$ = $field;

      DUMP_STATEMENT( "$$_statement", $$ );
      DUMP_END();
    }
  | struct_declaration
  | typedef_declaration
  | union_declaration
  ;

match_expr_opt
  : /* empty */                   { $$ = NULL; }
  | Y_EQUAL_EQUAL expr            { $$ = $expr; }
  ;

////////// enum declaration ///////////////////////////////////////////////////

enum_declaration
  : Y_enum name_exp[name] colon_exp type
    lbrace_exp enumerator_list[values] rbrace_exp
    {
      DUMP_START( "enum_declaration",
                  "enum NAME ':' type '{' enumerator_list '}'" );
      DUMP_STR( "name", $name );

      ad_type_t *const new_type = MALLOC( ad_type_t, 1 );
      *new_type = (ad_type_t){
        .sname = sname_current( $name ),
        .tid = T_ENUM | ($type->tid & (T_MASK_ENDIAN | T_MASK_SIZE)),
        .loc = @$,
        .enum_t = {
          .value_list = slist_move( &$values )
        }
      };
      $name = NULL;
      PARSE_ASSERT( define_type( new_type ) );
      $$ = NULL;                        // do not add to statement_list

      DUMP_TYPE( "$$_type", new_type );
      DUMP_END();
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

////////// field declaration //////////////////////////////////////////////////

field_declaration
  : alignas_opt[align] type name_exp[name] array_opt[rep] format_opt[format]
    {
      DUMP_START( "field_declaration",
                  "alignas_opt type name array_opt format_opt" );
      DUMP_INT( "alignas", $align );
      DUMP_TYPE( "type", $type );
      DUMP_STR( "name", $name );
      DUMP_REP( "rep", &$rep );
      DUMP_STR( "format", $format );

      $$ = MALLOC( ad_statement_t, 1 );
      *$$ = (ad_statement_t){
        .kind = AD_STMNT_DECLARATION,
        .loc = @$,
        .decl_s = {
          .name = $name,
          .type = $type,
          .align = $align,
          .rep = $rep,
          .printf_fmt = $format
        }
      };

      DUMP_STATEMENT( "$$_statement", $$ );
      DUMP_END();
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

format_opt
  : /* empty */                   { $$ = NULL; }
  | Y_STR_LIT
  ;

////////// struct declaration /////////////////////////////////////////////////

struct_declaration
  : Y_struct name_exp[name] lbrace_exp
    {
      sname_append_name( &in_attr.scope_sname, $name );

      in_attr.cur_type = MALLOC( ad_type_t, 1 );
      *in_attr.cur_type = (ad_type_t){
        .sname = sname_dup( &in_attr.scope_sname ),
        .tid = T_STRUCT
      };
      PARSE_ASSERT( define_type( in_attr.cur_type ) );
      sym_open_scope();
    }
    statement_list_opt[members] rbrace_exp
    {
      sym_close_scope();

      DUMP_START( "struct_declaration",
                  "struct NAME '{' statement_list_opt '}'" );
      DUMP_SNAME( "in_attr__scope_sname", in_attr.scope_sname );
      DUMP_STR( "name", $name );

      in_attr.cur_type->loc = @$;
      in_attr.cur_type->struct_t.member_list = slist_move( &$members );

      $$ = NULL;                        // do not add to statement_list

      DUMP_TYPE( "$$_type", in_attr.cur_type );
      DUMP_END();

      in_attr.cur_type = NULL;
    }
  ;

////////// typedef declaration ////////////////////////////////////////////////

typedef_declaration
  : Y_typedef field_declaration[field]
    {
      DUMP_START( "typedef_declaration", "TYPEDEF field_declaration" );
      DUMP_STATEMENT( "field_declaration", $field );

      assert( $field->kind == AD_STMNT_DECLARATION );
      ad_decl_t *const decl = &$field->decl_s;
      ad_type_t *const new_type = MALLOC( ad_type_t, 1 );
      *new_type = (ad_type_t){
        .sname = sname_current( check_strdup( decl->name ) ),
        .tid = T_TYPEDEF,
        .loc = @$,
        .rep = decl->rep,
        .typedef_t = { .type = decl->type }
      };
      ad_statement_free( $field );
      PARSE_ASSERT( define_type( new_type ) );
      $$ = NULL;                        // do not add to statement_list

      DUMP_TYPE( "$$_type", new_type );
      DUMP_END();
    }
  ;

////////// union declaration //////////////////////////////////////////////////

union_declaration
  : Y_union name_exp[name] lbrace_exp
    {
      sname_append_name( &in_attr.scope_sname, $name );

      in_attr.cur_type = MALLOC( ad_type_t, 1 );
      *in_attr.cur_type = (ad_type_t){
        .sname = sname_dup( &in_attr.scope_sname ),
        .tid = T_UNION
      };
      PARSE_ASSERT( define_type( in_attr.cur_type ) );
      sym_open_scope();
    }
    statement_list_opt[members] rbrace_exp
    {
      sym_close_scope();

      DUMP_START( "union_declaration",
                  "union NAME '{' statement_list_opt '}'" );
      DUMP_SNAME( "in_attr__scope_sname", in_attr.scope_sname );
      DUMP_STR( "name", $name );

      in_attr.cur_type->loc = @$;
      in_attr.cur_type->union_t.member_list = slist_move( &$members );

      $$ = NULL;                        // do not add to statement_list

      DUMP_TYPE( "$$_type", in_attr.cur_type );
      DUMP_END();

      in_attr.cur_type = NULL;
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
      // TODO: $$->binary.lhs_expr = $2;
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
      $$->ternary.true_expr = $t_expr;
      $$->ternary.false_expr = $f_expr;

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

equality_expr
  : relational_expr
  | equality_expr[lhs_expr] Y_EQUAL_EQUAL relational_expr[rhs_expr]
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
  | equality_expr[lhs_expr] Y_EXCLAM_EQUAL relational_expr[rhs_expr]
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
  | logical_and_expr[lhs_expr] Y_AMPER_AMPER bitwise_or_expr[rhs_expr]
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
  | logical_or_expr[lhs_expr] Y_PIPE_PIPE logical_and_expr[rhs_expr]
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
      // TODO: call a function
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
  | Y_sizeof '(' decl_or_type[type] rparen_exp
    {
      DUMP_START( "unary_expr", "SIZEOF '(' NAME ')'" );
      DUMP_TYPE( "decl_or_type", $type );

      $$ = ad_expr_new( AD_EXPR_LITERAL, &@$ );
      $$->literal = (ad_literal_expr_t){
        .type = &TB_UINT64,
        .uval = ad_type_size( $type )
      };

      DUMP_EXPR( "$$_expr", $$ );
      DUMP_END();
    }
  ;

decl_or_type
  : Y_DECL                        { $$ = $1->type; }
  | Y_TYPE
  | error
    {
      elaborate_error( "type or object name expected" );
      $$ = NULL;
    }
  ;

assign_op
  : '='                           { $$ = AD_EXPR_ASSIGN; }
  | Y_PERCENT_EQUAL               { $$ = AD_EXPR_MATH_MOD; }
  | Y_AMPER_EQUAL                 { $$ = AD_EXPR_BIT_AND; }
  | Y_STAR_EQUAL                  { $$ = AD_EXPR_MATH_MUL; }
  | Y_PLUS_EQUAL                  { $$ = AD_EXPR_MATH_ADD; }
  | Y_MINUS_EQUAL                 { $$ = AD_EXPR_MATH_SUB; }
  | Y_SLASH_EQUAL                 { $$ = AD_EXPR_MATH_DIV; }
  | Y_LESS_LESS_EQUAL             { $$ = AD_EXPR_BIT_SHIFT_LEFT; }
  | Y_GREATER_GREATER_EQUAL       { $$ = AD_EXPR_BIT_SHIFT_RIGHT; }
  | Y_CIRC_EQUAL                  { $$ = AD_EXPR_BIT_XOR; }
  | Y_PIPE_EQUAL                  { $$ = AD_EXPR_BIT_OR; }
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
  : builtin_tid[tid]
    {
      lexer_in_template = true;
    }
    lt_exp assign_expr[size] type_endian_expr_opt[endian] template_end_exp
    {
      lexer_in_template = false;

      DUMP_START( "type", "builtin_tid '<' expr type_endian_expr_opt '>'" );
      DUMP_TID( "builtin_tid", $tid );
      DUMP_EXPR( "expr", $size );
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
  | Y_TYPE
  ;

template_end_exp
  : Y_TEMPLATE_END
  | error
    {
      punct_expected( '>' );
    }
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
#pragma GCC diagnostic pop

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints an additional parsing error message including a newline to standard
 * error that continues from where yyerror() left off.  Additionally:
 *
 * + If the printable_yytext() isn't NULL:
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
 * @param line The line number within \a file where this function was called
 * from.
 * @param dym_kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param format A `printf()` style format string.  It _must not_ end in a
 * newline since this function prints its own newline.
 * @param ... Arguments to print.
 *
 * @sa #elaborate_error()
 * @sa #elaborate_error_dym()
 * @sa l_punct_expected()
 * @sa yyerror()
 */
static void l_elaborate_error( int line, dym_kind_t dym_kinds,
                               char const *format, ... ) {
  assert( format != NULL );

  EPUTS( ": " );
  print_debug_file_line( __FILE__, line );

  char const *const error_token = printable_yytext();
  if ( print_error_token( error_token ) )
    EPUTS( ": " );

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

/**
 * Cleans up global parser data at program termination.
 */
static void parser_cleanup( void ) {
  ad_statement_list_cleanup( &statement_list );
}

/**
 * Prints \a token, quoted; if \ref opt_ad_debug `!=` #AD_DEBUG_NO, also prints
 * the look-ahead character within `[]`.
 *
 * @param token The error token to print, if any.
 * @return Returns `true` only if anything was printed.
 */
static bool print_error_token( char const *token ) {
  if ( token == NULL )
    return false;
  EPRINTF( "\"%s\"", token );
  if ( opt_ad_debug != AD_DEBUG_NO ) {
    switch ( yychar ) {
      case YYEMPTY:
        EPUTS( " [<empty>]" );
        break;
      case YYEOF:
        EPUTS( " [<EOF>]" );
        break;
      case YYerror:
        EPUTS( " [<error>]" );
        break;
      case YYUNDEF:
        EPUTS( " [<UNDEF>]" );
        break;
      default:
        EPRINTF( isprint( yychar ) ? " ['%c']" : " [%d]", yychar );
    } // switch
  }
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

void parser_init( void ) {
  ASSERT_RUN_ONCE();
  ATEXIT( &parser_cleanup );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
