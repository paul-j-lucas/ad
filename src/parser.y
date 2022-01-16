/*
**      ad -- ASCII dump
**      src/parser.y
**
**      Copyright (C) 2019  Paul J. Lucas, et al.
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
 * grammar ad C declarations.
 */

/** @cond DOXYGEN_IGNORE */

%{
/** @endcond */

// local
#include "ad.h"                         /* must go first */
#include "color.h"
#ifdef ENABLE_AD_DEBUG
#include "debug.h"
#endif /* ENABLE_AD_DEBUG */
#include "expr.h"
#include "keyword.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "slist.h"
#include "type.h"
#include "typedefs.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

#ifdef ENABLE_AD_DEBUG
#define IF_DEBUG(...)             BLOCK( if ( opt_debug ) { __VA_ARGS__ } )
#else
#define IF_DEBUG(...)             /* nothing */
#endif /* ENABLE_AD_DEBUG */

// Developer aid for tracing when Bison %destructors are called.
#if 0
#define DTRACE                    EPRINTF( "%d: destructor\n", __LINE__ )
#else
#define DTRACE                    NO_OP
#endif

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-dump-group Debugging Macros
 * Macros that are used to dump a trace during parsing when `opt_ad_debug` is
 * `true`.
 * @ingroup parser-group
 * @{
 */

/**
 * Dumps a comma followed by a newline the _second_ and subsequent times it's
 * called.  It's used to separate items being dumped.
 */
#define DUMP_COMMA \
  BLOCK( if ( true_or_set( &dump_comma ) ) PUTS_OUT( ",\n" ); )

/**
 * Dumps a `bool`.
 *
 * @param KEY The key name to print.
 * @param BOOL The `bool` to dump.
 */
#define DUMP_BOOL(KEY,BOOL)  IF_DEBUG(  \
  DUMP_COMMA;                           \
  FPRINTF( stdout, "  " KEY " = %s", ((BOOL) ? "true" : "false") ); )

/**
 * Dumps an ad_expr.
 *
 * @param KEY The key name to print.
 * @param EXPR The ad_expr to dump.
 */
#define DUMP_EXPR(KEY,EXPR) \
  IF_DEBUG( DUMP_COMMA; ad_expr_dump( (EXPR), 1, (KEY), stdout ); )

/**
 * Dumps an `s_list` of ad_expr_t.
 *
 * @param KEY The key name to print.
 * @param EXPR_LIST The `s_list` of ad_expr_t to dump.
 */
#define DUMP_EXPR_LIST(KEY,EXPR_LIST) IF_DEBUG( \
  DUMP_COMMA; PUTS_OUT( "  " KEY " = " );       \
  ad_expr_list_dump( &(EXPR_LIST), 1, stdout ); )

/**
 * Dumps an integer.
 *
 * @param KEY The key name to print.
 * @param NUM The integer to dump.
 */
#define DUMP_NUM(KEY,NUM) \
  IF_DEBUG( DUMP_COMMA; printf( "  " KEY " = %d", (NUM) ); )

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 */
#define DUMP_STR(KEY,NAME) IF_DEBUG(  \
  DUMP_COMMA; PUTS_OUT( "  " );       \
  print_kv( (KEY), (NAME), stdout ); )

#ifdef ENABLE_AD_DEBUG
/**
 * Starts a dump block.
 *
 * @param NAME The grammar production name.
 * @param PROD The grammar production rule.
 *
 * @sa DUMP_END
 */
#define DUMP_START(NAME,PROD) \
  bool dump_comma = false;    \
  IF_DEBUG( PUTS_OUT( "\n" NAME " ::= " PROD " = {\n" ); )
#else
#define DUMP_START(NAME,PROD)     /* nothing */
#endif

/**
 * Ends a dump block.
 *
 * @sa DUMP_START
 */
#define DUMP_END()                IF_DEBUG( PUTS_OUT( "\n}\n" ); )

#define DUMP_TYPE(KEY,TYPE) IF_DEBUG( \
  DUMP_COMMA; PUTS_OUT( "  " KEY " = " ); c_type_dump( TYPE, stdout ); )

/** @} */

///////////////////////////////////////////////////////////////////////////////

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
 * @note
 * This must be used _only_ after an `error` token, e.g.:
 * @code
 *  | error
 *    {
 *      elaborate_error_dym( DYM_COMMANDS, "unexpected token" );
 *    }
 * @endcode
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
#define PARSE_ABORT()             BLOCK( parse_cleanup( true ); YYABORT; )

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local variables
static slist_t        expr_gc_list;     ///< `expr` nodes freed after parse.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets a printable string of the lexer's current token.
 *
 * @return Returns said string.
 */
PJL_WARN_UNUSED_RESULT
static inline char const* printable_token( void ) {
  switch ( lexer_token[0] ) {
    case '\0': return NULL;
    case '\n': return "\\n";
    default  : return lexer_token;
  } // switch
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints an additional parsing error message to standard error that continues
 * from where `yyerror()` left off.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param format A `printf()` style format string.
 * @param ... Arguments to print.
 */
static void fl_elaborate_error( char const *file, int line, char const *format,
                                ... ) {
  assert( format != NULL );

  EPUTS( ": " );
  char const *const error_token = printable_token();
  if ( error_token != NULL )
    EPRINTF( "\"%s\": ", printable_token() );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

#ifdef ENABLE_AD_DEBUG
  if ( opt_ad_debug )
    PRINTF_ERR( " (%s:%d)", file, line );
#endif /* ENABLE_AD_DEBUG */

  EPUTS( '\n' );
}

/**
 * Cleans-up parser data after each parse.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag of the lexer.  This should be `true` if an error occurs and
 * `YYABORT` is called.
 *
 * @sa parse_init()
 */
static void parse_cleanup( bool hard_reset ) {
  lexer_reset( hard_reset );
  slist_cleanup( &expr_gc_list, NULL, (slist_free_fn_t)&ad_expr_free );
}

/**
 * Gets ready to parse a statement.
 *
 * @sa parse_cleanup()
 */
static void parse_init( void ) {
  // TODO
}

/**
 * Prints a parsing error message to standard error.  This function is called
 * directly by Bison to print just `syntax error` (usually).
 *
 * @note A newline is \e not printed since the error message will be appended
 * to by `elaborate_error()`.  For example, the parts of an error message are
 * printed by the functions shown:
 *
 *      42: syntax error: "int": "into" expected
 *      |--||----------||----------------------|
 *      |   |           |
 *      |   yyerror()   elaborate_error()
 *      |
 *      print_loc()
 *
 * @param msg The error message to print.
 */
static void yyerror( char const *msg ) {
  assert( msg != NULL );

  c_loc_t loc = lexer_loc();
  print_loc( &loc );

  SGR_START_COLOR( stderr, error );
  EPUTS( msg );                         // no newline
  SGR_END_COLOR( stderr );
  error_newlined = false;

  parse_cleanup( false );
}

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

%union {
  unsigned            bitmask;    // multipurpose bitmask (used by show)
  ad_enum_value_t     enum_val;
  ad_expr_t          *expr;       // for the expression being built
  ad_expr_kind_t      expr_kind;  // built-ins, storage classes, & qualifiers
  ad_field_t          field;
  slist_t             list;       // multipurpose list
  char const         *literal;    // token literal (for new-style casts)
  char               *name;       // name being declared or explained
  ad_rep_t            rep_val;
  int                 number;     // for array sizes
  char               *str_val;    // qupted string value
}

                    // ad keywords
%token  <expr_kind> Y_BOOL
%token              Y_BREAK
%token              Y_CASE
%token  <expr_kind> Y_CHAR
%token              Y_DEFAULT
%token  <expr_kind> Y_ENUM
%token  <expr_kind> Y_FALSE
%token  <expr_kind> Y_FLOAT
%token  <expr_kind> Y_INT
%token  <expr_kind> Y_UINT
%token  <expr_kind> Y_STRUCT
%token              Y_SWITCH
%token  <expr_kind> Y_TRUE
%token  <expr_kind> Y_TYPEDEF
%token  <expr_kind> Y_UTF

                    //
                    // C operators that are single-character are represented by
                    // themselves directly.  All multi-character operators are
                    // given Y_ names.
                    //

                    // C operators: precedence 16
%token  <oper_id>   Y_PLUS2       "++"
%token  <oper_id>   Y_MINUS2      "--"
%left   <oper_id>                 '(' ')'
                                  '[' ']'
%token  <oper_id>                 '.'
%token  <oper_id>   Y_ARROW       "->"
                    // C operators: precedence 15
%token  <oper_id>                 '&'
                                  '*'
                                  '!'
                 // Y_UMINUS   // '-' -- covered by '-' below
                 // Y_PLUS     // '+' -- covered by '+' below
%right  <oper_id>   Y_SIZEOF
%right  <oper_id>                 '~'
                    // C operators: precedence 14
                    // C operators: precedence 13
                               // '*' -- covered by '*' above
%left   <oper_id>                 '/'
                                  '%'
                    // C operators: precedence 12
%left   <oper_id>                 '+'
                                  '-'
                    // C operators: precedence 11
%left   <oper_id>   Y_LESS2       "<<"
                    Y_GREATER2    ">>"
                    // C operators: precedence 10
                    // C operators: precedence 9
%left   <oper_id>                 '<' '>'
                    Y_LESS_EQ     "<="
                    Y_GREATER_EQ  ">="

                    // C operators: precedence 8
%left   <oper_id>   Y_EQ2         "=="
                    Y_NOT_EQ      "!="
                    // C operators: precedence 7
                    Y_BIT_AND  // '&' -- covered by Y_AMPER
                    // C operators: precedence 6
%left   <oper_id>                 '^'
                    // C operators: precedence 5
%left   <oper_id>                 '|'
                    // C operators: precedence 4
%token  <oper_id>   Y_AMPER2      "&&"
                    // C operators: precedence 3
%left   <oper_id>   Y_PIPE2       "||"
                    // C operators: precedence 2
%right  <oper_id>                 '?'
                                  '='
                    Y_PERCENT_EQ  "%="
                    Y_AMPER_EQ    "&="
                    Y_STAR_EQ     "*="
                    Y_PLUS_EQ     "+="
                    Y_MINUS_EQ    "-="
                    Y_SLASH_EQ    "/="
                    Y_LESS2_EQ    "<<="
                    Y_GREATER2_EQ ">>="
                    Y_CIRC_EQ     "^="
                    Y_PIPE_EQ     "|="

                    // C operators: precedence 1
%left   <oper_id>                 ','


                    // Miscellaneous
%token                            ';'
%token                            '{' '}'
%token              Y_END
%token              Y_ERROR
%token  <int_val>   Y_INT_LIT
%token  <name>      Y_NAME
%token  <str_val>   Y_STR_LIT

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
//  2. Is specific to C/C++, "_c" is appended; is specific to English,
//     "_english" is appended.
//  3. Is of type:
//      + <ast> or <ast_pair>: "_ast" is appended.
//      + <name>: "_name" is appended.
//      + <number>: "_num" is appended.
//      + <sname>: "_sname" is appended.
//      + <type_id>: "_type" is appended.
//  4. Is expected, "_exp" is appended; is optional, "_opt" is appended.
//
// Sort using: sort -bdk3

                  // Declarations
%type <rep_val>   array_opt
%type <xxxxx>     builtin_type
%type             enum_declaration
%type <enum_val>  enumerator
%type <list>      enumerator_list
%type <field>     field_declaration
%type <xxxxx>     struct_declaration
%type <xxxxx>     switch_case_statement
%type <list>      switch_case_statement_list_opt
%type <xxxxx>     switch_statement
%type <xxxxx>     type type_exp
%type <xxxxx>     typedef_declaration

                  // Expressions
%type <expr>      additive_expr
%type <list>      argument_expr_list
%type <expr>      assign_expr
%type <expr_kind> assign_op
%type <expr>      bitwise_and_expr
%type <expr>      bitwise_exclusive_or_expr
%type <expr>      bitwise_or_expr
%type <expr>      cast_expr
%type <expr>      conditional_expr
%type <expr>      equality_expr
%type <expr>      expr
%type <expr>      logical_and_expr
%type <expr>      logical_or_expr
%type <expr>      multiplicative_expr
%type <expr>      postfix_expr
%type <expr>      primary_expr
%type <expr>      relational_expr
%type <expr>      shift_expr
%type <expr>      unary_expr
%type <expr_kind> unary_op

                  // Miscellaneous
%type <str_lit>   str_lit str_lit_exp
%type <int_val>   type_endian_opt

/*
 * Bison %destructors.  We don't use the <identifier> syntax because older
 * versions of Bison don't support it.
 *
 * Clean-up of expr nodes is done via garbage collection using expr_gc_list.
 */

/* name */
%destructor { DTRACE; FREE( $$ ); } name_exp
%destructor { DTRACE; FREE( $$ ); } name_opt
%destructor { DTRACE; FREE( $$ ); } Y_NAME

/* sname */
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c_exp
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c_opt
%destructor { DTRACE; c_sname_free( &$$ ); } typedef_type_sname
%destructor { DTRACE; c_sname_free( &$$ ); } typedef_type_sname_exp

/*****************************************************************************/
%%

///////////////////////////////////////////////////////////////////////////////
//  STATEMENTS                                                               //
///////////////////////////////////////////////////////////////////////////////

statement_list_opt
  : /* empty */
  | statement_list_opt statement
  ;

statement
  : compound_statement
  | declaration semi_exp
  | switch_statement
  | error
    {
      if ( printable_token() != NULL )
        elaborate_error( "unexpected token" );
      else
        elaborate_error( "unexpected end of statement" );
    }
  ;

compound_statement
  : '{' statement_list_opt '}'
  ;

declaration
  : enum_declaration
  | field_declaration
  | struct_declaration
  | typedef_declaration
  ;

/// enum declaration //////////////////////////////////////////////////////////

enum_declaration
  : Y_ENUM name_exp colon_exp type_exp lbrace_exp enumerator_list '}'
    {
      ad_enum_t *const ad_enum = MALLOC( ad_enum_t, 1 );
      ad_enum->name = $2;
      ad_enum->bits = XX;
      ad_enum->endian = XX;
      ad_enum->base = xx;
    }
  ;

enumerator_list
  : enumerator_list ',' enumerator
    {
      $$ = $1;
      slist_push_tail( &$$, $3 );
    }
  | enumerator
    {
      slist_init( &$$ );
      slist_push_tail( &$$, $1 );
    }
  ;

enumerator
  : Y_NAME equals_exp int_exp
    {
      $$.name = $1;
      $$.value = $3;
    }
  ;

/// field declaration /////////////////////////////////////////////////////////

field_declaration
  : type_exp field_name_exp array_opt
    {
      $$.type = $1;
      $$.name = $2;
      $$.rep = $3;
    }
  ;

array_opt
  : /* empty */                   { $$.times = AD_REP_1; }
  | '[' ']' equals_exp str_lit_exp
    {
    }
  | '[' '?' rbracket_exp          { $$.times = AD_REP_0_1; }
  | '[' '*' rbracket_exp          { $$.times = AD_REP_0_MORE; }
  | '[' '+' rbracket_exp          { $$.times = AD_REP_1_MORE; }
  | '[' expr ']'
    {
      $$.times = AD_REP_EXPR;
      $$.expr = $2;
    }
  | '[' error ']'
    {
      // TODO
    }
  ;

/// struct declaration ////////////////////////////////////////////////////////

struct_declaration
  : Y_STRUCT name_exp lbrace_exp statement_list_opt rbrace_exp
    {
      $$ = MALLOC( ad_struct, 1 );
      $$->name = $2;
      // TODO
    }
  ;

/// switch statement //////////////////////////////////////////////////////////

switch_statement
  : Y_SWITCH lparen_exp expr rparen_exp lbrace_exp
    switch_case_statement_list_opt '}'
  ;

switch_case_statement_list_opt
  : /* empty */
  | switch_case_statement_list_opt switch_case_statement
  ;

switch_case_statement
  : Y_CASE constant_expr_exp colon_exp statement_list_opt
  | Y_DEFAULT colon_exp statement_list_opt
  ;

/// typedef declaration ///////////////////////////////////////////////////////

typedef_declaration
  : Y_TYPEDEF
  ;

///////////////////////////////////////////////////////////////////////////////
//  EXPRESSIONS                                                              //
///////////////////////////////////////////////////////////////////////////////

expr
  : assign_expr
  | expr ',' assign_expr
    {
      $$ = $3;
    }
  ;

additive_expr
  : multiplicative_expr
  | additive_expr '+' multiplicative_expr
    {
      $$ = ad_expr_new( AD_EXPR_ADD );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | additive_expr '-' multiplicative_expr
    {
      $$ = ad_expr_new( AD_EXPR_SUB );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

assign_expr
  : conditional_expr
  | unary_expr assign_op assign_expr
    {
      $$ = ad_expr_new( $2 );
      // TODO
    }
  ;

bitwise_and_expr
  : equality_expr
  | bitwise_and_expr '&' equality_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_AND );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

bitwise_exclusive_or_expr
  : bitwise_and_expr
  | bitwise_exclusive_or_expr '^' bitwise_and_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_XOR );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

bitwise_or_expr
  : bitwise_exclusive_or_expr
  | bitwise_or_expr '|' bitwise_exclusive_or_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_OR );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

cast_expr
  : unary_expr
  | '(' type_name ')' cast_expr
  ;

conditional_expr
  : logical_or_expr
  | logical_or_expr '?' expr ':' conditional_expr
    {
      $$ = ad_expr_new( AD_EXPR_IF_ELSE );
      $$->as.ternary.cond_expr = $1;
      $$->as.ternary.true_expr = $3;
      $$->as.ternary.false_expr = $5;
    }
  ;

equality_expr
  : relational_expr
  | equality_expr "==" relational_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_EQ );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | equality_expr "!=" relational_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_NOT_EQ );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

logical_and_expr
  : inclusive_or_expr
  | logical_and_expr "&&" inclusive_or_expr
    {
      $$ = ad_expr_new( AD_EXPR_LOG_AND );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

logical_or_expr
  : logical_and_expr
  | logical_or_expr "||" logical_and_expr
    {
      $$ = ad_expr_new( AD_EXPR_LOG_OR );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

multiplicative_expr
  : cast_expr
  | multiplicative_expr '*' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_MUL );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | multiplicative_expr '/' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_DIV );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | multiplicative_expr '%' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_MOD );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

postfix_expr
  : primary_expr
  | postfix_expr '[' expr ']'
    {
    }
  | postfix_expr '(' ')'
    {
    }
  | postfix_expr '(' argument_expr_list ')'
    {
    }
  | postfix_expr '.' name_exp
    {
    }
  | postfix_expr "->" name_exp
    {
    }
  | postfix_expr "++"
    {
    }
  | postfix_expr "--"
    {
    }
  ;

argument_expr_list
  : assignment_expr
    {
      slist_init( &$$ );
      slist_push_tail( &$$, $1 );
    }
  | argument_expr_list ',' assignment_expr
    {
      slist_push_tail( &$$, $3 );
    }
  ;

primary_expr
  : Y_NAME
    {
      // TODO
    }
  | Y_INT_LIT
    {
      $$ = ad_expr_new( AD_EXPR_VALUE );
   // $$.as.value.type = xx;
      $$.as.value.as.i64 = $1;
    }
  | Y_STR_LIT
    {
      $$ = ad_expr_new( AD_EXPR_VALUE );
      // TODO
    }
  | '(' expr ')'                  { $$ = $2; }
  ;

relational_expr
  : shift_expr
  | relational_expr '<' shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_LESS );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | relational_expr '>' shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_GREATER );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | relational_expr "<=" shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_LESS_EQ );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | relational_expr ">=" shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_GREATER_EQ );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

shift_expr
  : additive_expr
  | shift_expr "<<" additive_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_LEFT );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  | shift_expr ">>" additive_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_RIGHT );
      $$->as.binary.lhs_expr = $1;
      $$->as.binary.rhs_expr = $3;
    }
  ;

unary_expr
  : postfix_expr
  | "++" unary_expr
  | "--" unary_expr
  | unary_op cast_expr
  | Y_SIZEOF unary_expr
  | Y_SIZEOF '(' type_name ')'
  ;

assign_op
  : '='                         { $$ = AD_EXPR_ASSIGN; }
  | "%="                        { $$ = AD_EXPR_MATH_MOD; }
  | "&="                        { $$ = AD_EXPR_BIT_AND; }
  | "*="                        { $$ = AD_EXPR_MATH_MUL; }
  | "+="                        { $$ = AD_EXPR_MATH_ADD; }
  | "-="                        { $$ = AD_EXPR_MATH_SUB; }
  | "/="                        { $$ = AD_EXPR_MATH_DIV; }
  | "<<="                       { $$ = AD_EXPR_BIT_SHIFT_LEFT; }
  | ">>="                       { $$ = AD_EXPR_BIT_SHIFT_RIGHT; }
  | "^="                        { $$ = AD_EXPR_BIT_XOR; }
  | "|="                        { $$ = AD_EXPR_BIT_OR; }
  ;

unary_op
  : '&'                         { $$ = AD_EXPR_ADDR; }
  | '*'                         { $$ = AD_EXPR_DEREF; }
  | '+'                         { $$ = AD_EXPR_MATH_ADD; }
  | '-'                         { $$ = AD_EXPR_MATH_NEG; }
  | '~'                         { $$ = AD_EXPR_BIT_COMPL; }
  | '!'                         { $$ = AD_EXPR_LOG_NOT; }
  ;

/// type //////////////////////////////////////////////////////////////////////

type
  : builtin_type lt_exp expr type_endian_opt '>'
  | Y_TYPEDEF_TYPE
  ;

type_exp
  : type
  | error
    {
    }
  ;

builtin_type
  : Y_FLOAT
  | Y_INT
  | Y_UINT
  | Y_UTF
  ;

type_endian_opt
  : /* empty */                   { $$ = 0; }
  | ',' expr                      { $$ = $2; }
  ;

///////////////////////////////////////////////////////////////////////////////
//  MISCELLANEOUS                                                            //
///////////////////////////////////////////////////////////////////////////////

comma_exp
  : ','
  | error
    {
      elaborate_error( "',' expected" );
    }
  ;

equals_exp
  : '='
  | error
    {
      elaborate_error( "'=' expected" );
    }
  ;

gt_exp
  : '>'
  | error
    {
      elaborate_error( "'>' expected" );
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
      elaborate_error( "'{' expected" );
    }
  ;

lparen_exp
  : '('
  | error
    {
      elaborate_error( "'(' expected" );
    }
  ;

lt_exp
  : '<'
  | error
    {
      elaborate_error( "'<' expected" );
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

name_opt
  : /* empty */                   { $$ = NULL; }
  | Y_NAME
  ;

rbrace_exp
  : '}'
  | error
    {
      elaborate_error( "'}' expected" );
    }
  ;

rbracket_exp
  : ']'
  | error
    {
      elaborate_error( "']' expected" );
    }
  ;

rparen_exp
  : ')'
  | error
    {
      elaborate_error( "')' expected" );
    }
  ;

semi_exp
  : ';'
  | error
    {
      elaborate_error( "';' expected" );
    }
  ;

semi_opt
  : /* empty */
  | ';'
  ;

semi_or_end
  : ';'
  | Y_END
  ;

str_lit
  : Y_STR_LIT
  | Y_LEXER_ERROR
    {
      $$ = NULL;
      PARSE_ABORT();
    }
  ;

str_lit_exp
  : str_lit
  | error
    {
      $$ = NULL;
      elaborate_error( "string literal expected" );
    }
  ;

%%

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans up global parser data at program termination.
 */
void parser_cleanup( void ) {
  // do nothing
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
