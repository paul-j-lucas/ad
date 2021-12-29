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
 * grammar ad C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%expect 22

%{
/** @endcond */

// local
#include "ad.h"                         /* must go first */
#include "c_expr.h"
#include "c_keyword.h"
#include "c_type.h"
#include "color.h"
#ifdef ENABLE_AD_DEBUG
#include "debug.h"
#endif /* ENABLE_AD_DEBUG */
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "slist.h"
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

#define C_AST_CHECK(AST,CHECK) \
  BLOCK( if ( !c_ast_check( (AST), (CHECK) ) ) PARSE_ABORT(); )

#define C_AST_NEW(KIND,LOC) \
  SLIST_PUSH_TAIL( c_ast_t*, &ast_gc_list, c_ast_new( (KIND), ast_depth, (LOC) ) )

#define C_TYPE_ADD(DST,SRC,LOC) \
  BLOCK( if ( !c_type_add( (DST), (SRC), &(LOC) ) ) PARSE_ABORT(); )

// Developer aid for tracing when Bison %destructors are called.
#if 0
#define DTRACE                    EPRINTF( "%d: destructor\n", __LINE__ )
#else
#define DTRACE                    NO_OP
#endif

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-dump-group Debugging Macros
 * Macros that are used to dump a trace during parsing when `opt_cdecl_debug`
 * is `true`.
 * @ingroup parser-group
 * @{
 */

/**
 * Dumps a comma followed by a newline the _second_ and subsequent times it's
 * called.  It's used to separate items being dumped.
 */
#define DUMP_COMMA \
  BLOCK( if ( true_or_set( &debug_comma ) ) PUTS_OUT( ",\n" ); )

/**
 * Dumps an AST.
 *
 * @param KEY The key name to print.
 * @param AST The AST to dump.
 */
#define DUMP_AST(KEY,AST) \
  IF_DEBUG( DUMP_COMMA; c_ast_debug( (AST), 1, (KEY), stdout ); )

/**
 * Dumps an `s_list` of AST.
 *
 * @param KEY The key name to print.
 * @param AST_LIST The `s_list` of AST to dump.
 */
#define DUMP_AST_LIST(KEY,AST_LIST) IF_DEBUG( \
  DUMP_COMMA; PUTS_OUT( "  " KEY " = " );     \
  c_ast_list_debug( &(AST_LIST), 1, stdout ); )

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
  bool debug_comma = false;   \
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
  DUMP_COMMA; PUTS_OUT( "  " KEY " = " ); c_type_debug( TYPE, stdout ); )

/** @} */

///////////////////////////////////////////////////////////////////////////////

#define elaborate_error(...) \
  BLOCK( fl_elaborate_error( __FILE__, __LINE__, __VA_ARGS__ ); PARSE_ABORT(); )

#define PARSE_ABORT()             BLOCK( parse_cleanup( true ); YYABORT; )

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern functions
extern int            yylex( void );

// local variables
static slist_t        expr_gc_list;     ///< `expr` nodes freed after parse.
static bool           error_newlined = true;

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
  if ( !error_newlined ) {
    PUTS_ERR( ": " );
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

    PUTC_ERR( '\n' );
    error_newlined = true;
  }
}

/**
 * Cleans-up parser data after each parse.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag of the lexer.  This should be `true` if an error occurs and
 * `YYABORT` is called.
 */
static void parse_cleanup( bool hard_reset ) {
  lexer_reset( hard_reset );
  slist_cleanup( &expr_gc_list, NULL, (slist_free_fn_t)&ad_expr_free );
}

/**
 * Gets ready to parse a statement.
 */
static void parse_init( void ) {
  if ( !error_newlined ) {
    FPUTC( '\n', fout );
    error_newlined = true;
  }
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
  c_loc_t loc;
  STRUCT_ZERO( &loc );
  lexer_loc( &loc.first_line, &loc.first_column );
  print_loc( &loc );

  SGR_START_COLOR( stderr, error );
  PUTS_ERR( msg );                      // no newline
  SGR_END_COLOR( stderr );
  error_newlined = false;

  parse_cleanup( false );
}

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

%union {
  ad_expr_t          *expr;
  unsigned            bitmask;    /* multipurpose bitmask (used by show) */
  char const         *literal;    /* token literal (for new-style casts) */
  char               *name;       /* name being declared or explained */
  ad_repitition_t     repetition;
  int                 number;     /* for array sizes */
  ad_type_id_t        type_id;    /* built-ins, storage classes, & qualifiers */
}

%token  <oper_id>   '!'
%token  <oper_id>   '%'
%token  <oper_id>   '&'
%token  <oper_id>   '(' ')'
%token  <oper_id>   '*'
%token  <oper_id>   '+'
%token  <oper_id>   ','
%token  <oper_id>   '-'
%token  <oper_id>   '.'
%token  <oper_id>   '/'
%token              ';'
%token  <oper_id>   '<' '>'
%token  <oper_id>   '[' ']'
%token  <oper_id>   '='
%token  <oper_id>   '?'
%token  <oper_id>   '^'
%token              '{'
%token  <oper_id>   '|'
%token              '}'
%token  <oper_id>   '~'
%token  <oper_id>   Y_NOT_EQ      "!="
%token  <oper_id>   Y_PERCENT_EQ  "%="
%token  <oper_id>   Y_AMPER2      "&&"
%token  <oper_id>   Y_AMPER_EQ    "&="
%token  <oper_id>   Y_STAR_EQ     "*="
%token  <oper_id>   Y_PLUS2       "++"
%token  <oper_id>   Y_PLUS_EQ     "+="
%token  <oper_id>   Y_MINUS2      "--"
%token  <oper_id>   Y_MINUS_EQ    "-="
%token  <oper_id>   Y_ARROW       "->"
%token  <oper_id>   Y_SLASH_EQ    "/="
%token  <oper_id>   Y_LESS2       "<<"
%token  <oper_id>   Y_LESS2_EQ    "<<="
%token  <oper_id>   Y_LESS_EQ     "<="
%token  <oper_id>   Y_EQ2         "=="
%token  <oper_id>   Y_GREATER_EQ  ">="
%token  <oper_id>   Y_GREATER2    ">>"
%token  <oper_id>   Y_GREATER2_EQ ">>="
%token  <oper_id>   Y_CIRC_EQ     "^="
%token  <oper_id>   Y_PIPE_EQ     "|="
%token  <oper_id>   Y_PIPE2       "||"

%token  <type_id>   Y_BOOL
%token              Y_BREAK
%token              Y_CASE
%token  <type_id>   Y_CHAR
%token              Y_DEFAULT
%token  <type_id>   Y_ENUM
%token  <type_id>   Y_FALSE
%token  <type_id>   Y_FLOAT
%token  <type_id>   Y_INT
%token  <type_id>   Y_UINT
%token  <type_id>   Y_SIZEOF
%token  <type_id>   Y_STRUCT
%token              Y_SWITCH
%token  <type_id>   Y_TRUE
%token  <type_id>   Y_TYPEDEF
%token  <type_id>   Y_UTF

                    /* miscellaneous */
%token              Y_END
%token              Y_ERROR
%token  <name>      Y_NAME
%token  <number>    Y_NUMBER

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

%type   <ast_pair>  cast_c_ast cast_c_ast_opt cast2_c_ast
%type   <ast_pair>  array_cast_c_ast
%type   <ast_pair>  func_cast_c_ast
%type   <ast_pair>  nested_cast_c_ast
%type   <ast_pair>  pointer_cast_c_ast
%type   <type_id>   pure_virtual_c_type_opt

%type   <ast_pair>  decl_c_ast decl2_c_ast
%type   <ast_pair>  array_decl_c_ast
%type   <number>    array_size_c_num
%type   <ast_pair>  func_decl_c_ast
%type   <type_id>   func_noexcept_c_type_opt
%type   <ast_pair>  func_trailing_return_type_c_ast_opt
%type   <ast_pair>  nested_decl_c_ast
%type   <ast_pair>  pointer_decl_c_ast
%type   <ast_pair>  pointer_type_c_ast
%type   <ast_pair>  unmodified_type_c_ast

%type   <ast_pair>  builtin_type_c_ast
%type   <ast_pair>  enum_class_struct_union_ast
%type   <ast_pair>  placeholder_c_ast
%type   <ast_pair>  type_c_ast
%type   <ast_pair>  typedef_type_c_ast

%type   <type_id>   attribute_name_c_type
%type   <type_id>   attribute_name_list_c_type attribute_name_list_c_type_opt
%type   <type_id>   attribute_specifier_list_c_type
%type   <type_id>   builtin_type
%type   <type_id>   enum_class_struct_union_type
%type   <type_id>   func_qualifier_list_c_type_opt
%type   <type_id>   noexcept_bool_type

%type   <ast_pair>  arg_c_ast
%type   <ast>       arg_array_size_c_ast
%type   <ast_list>  arg_list_c_ast arg_list_c_ast_opt
%type   <type_id>   class_struct_union_type
%type   <oper_id>   c_operator
%type   <ast_pair>  name_ast
%type   <name>      name_exp name_opt
%type   <type_id>   namespace_type
%type   <sname>     sname_c sname_c_exp sname_c_opt
%type   <ast_pair>  sname_c_ast
%type   <sname>     typedef_type_sname typedef_type_sname_exp
%type   <type_id>   static_type_opt

/*
 * Bison %destructors.  We don't use the <identifier> syntax because older
 * versions of Bison don't support it.
 *
 * Clean-up of AST nodes is done via garbage collection using ast_gc_list.
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

statement_list
  : /* empty */
  | statement_list statement
  ;

statement
  : compound_statement
  | declaration
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
  : '{' '}'
  | '{' statement_list '}'
  ;

declaration
  : enum_decl
  | field_decl
  | struct_decl
  | typedef_decl
  ;

/// enum declaration //////////////////////////////////////////////////////////

enum_decl
  : Y_ENUM name_exp colon_exp type_exp lbrace_exp enumerator_list '}'
  ;

enumerator_list
  : enumerator
  | enumerator_list ',' enumerator
    {
      slist_push_tail( &$$, $3 );
    }
  ;

enumerator
  : Y_NAME equals_exp number_exp
    {
      $$.name = $1;
      $$.value = $3;
    }
  ;

/// field declaration /////////////////////////////////////////////////////////

field_decl
  : type_exp field_name_exp array_opt
    {
    }
  ;

array_opt
  : /* empty */                   { $$.times = AD_REPETITION_1; }
  | '[' ']' eqeq_exp literal
    {
    }
  | '[' '?' rbracket_exp          { $$.times = AD_REPETITION_0_1; }
  | '[' '*' rbracket_exp          { $$.times = AD_REPETITION_0_MORE; }
  | '[' '+' rbracket_exp          { $$.times = AD_REPETITION_1_MORE; }
  | '[' expr ']'
    {
      $$.times = AD_REPETITION_EXPR;
      $$.expr = $2;
    }
  | '[' error ']'
    {
      // TODO
    }
  ;

/// struct declaration ////////////////////////////////////////////////////////

struct_decl
  : Y_STRUCT name_exp lbrace_exp statement_list rbrace_exp
    {
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
  : Y_CASE constant_expr_exp colon_exp statement_list
  | Y_DEFAULT colon_exp statement_list
  ;

/// typedef declaration ///////////////////////////////////////////////////////

typedef_declaration_c
  : Y_TYPEDEF type_c_ast
    {
      //
      // Explicitly add T_TYPEDEF to prohibit cases like:
      //
      //      typedef extern int eint
      //      typedef register int rint
      //      typedef static int sint
      //      ...
      //
      //  i.e., a defined type with a storage class.
      //
      C_TYPE_ADD( &$2.ast->type_id, T_TYPEDEF, @2 );
      type_push( $2.ast );
    }
    decl_c_ast
    {
      type_pop();

      DUMP_START( "typedef_declaration_c", "TYPEDEF type_c_ast decl_c_ast" );
      DUMP_AST( "type_c_ast", $2.ast );
      DUMP_AST( "decl_c_ast", $4.ast );

      c_ast_t *ast;
      c_sname_t temp_sname;

      if ( $2.ast->kind == K_TYPEDEF && $4.ast->kind == K_TYPEDEF ) {
        //
        // This is for a case like:
        //
        //      typedef size_t foo;
        //
        // that is: an existing typedef name followed by a new name.
        //
        ast = $2.ast;
      }
      else if ( $4.ast->kind == K_TYPEDEF ) {
        //
        // This is for a case like:
        //
        //      typedef int int_least32_t;
        //
        // that is: a type followed by an existing typedef name, i.e.,
        // redefining an existing typedef name to be the same type.
        //
        ast = $2.ast;
        temp_sname = c_ast_sname_dup( $4.ast->as.c_typedef->ast );
        c_ast_sname_set_sname( ast, &temp_sname );
      }
      else {
        //
        // This is for a case like:
        //
        //      typedef int foo;
        //
        // that is: a type followed by a new name.
        //
        ast = c_ast_patch_placeholder( $2.ast, $4.ast );
        temp_sname = c_ast_take_name( $4.ast );
        c_ast_sname_set_sname( ast, &temp_sname );
      }

      C_AST_CHECK( ast, CHECK_DECL );
      // see the comment in define_english about T_TYPEDEF
      (void)c_ast_take_typedef( ast );

      if ( c_ast_sname_count( ast ) > 1 ) {
        print_error( &@4,
          "%s names can not be scoped; use: %s %s { %s ... }",
          L_TYPEDEF, L_NAMESPACE, c_ast_sname_scope_c( ast ), L_TYPEDEF
        );
        PARSE_ABORT();
      }

      DUMP_AST( "typedef_declaration_c", ast );
      DUMP_END();

      temp_sname = c_sname_dup( &in_attr.current_scope );
      c_ast_sname_set_type( ast, c_sname_type( &in_attr.current_scope ) );
      c_ast_sname_prepend_sname( ast, &temp_sname );

      switch ( c_typedef_add( ast ) ) {
        case TD_ADD_ADDED:
          // See the comment in define_english about ast_typedef_list.
          slist_push_list_tail( &ast_typedef_list, &ast_gc_list );
          break;
        case TD_ADD_DIFF:
          print_error( &@4,
            "\"%s\": \"%s\" redefinition with different type",
            c_ast_sname_full_c( ast ), L_TYPEDEF
          );
          PARSE_ABORT();
        case TD_ADD_EQUIV:
          // Do nothing.
          break;
      } // switch
    }
  ;

/// expressions ///////////////////////////////////////////////////////////////

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
  | postfix_expr '(' ')'
  | postfix_expr '(' argument_expr_list ')'
  | postfix_expr '.' IDENTIFIER
  | postfix_expr PTR_OP IDENTIFIER
  | postfix_expr "++"
  | postfix_expr "--"
  ;

argument_expr_list
  : assignment_expr
  | argument_expr_list ',' assignment_expr
  ;

primary_expr
  : identifier
  | constant
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

/*****************************************************************************/
/*  declaration gibberish productions                                        */
/*****************************************************************************/

decl_c_ast
  : decl2_c_ast
  | pointer_decl_c_ast
  ;

decl2_c_ast
  : array_decl_c_ast
  | func_decl_c_ast
  | nested_decl_c_ast
  | sname_c_ast
  | typedef_type_c_ast
  ;

array_decl_c_ast
  : decl2_c_ast array_size_c_num
    {
      DUMP_START( "array_decl_c_ast", "decl2_c_ast array_size_c_num" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "decl2_c_ast", $1.ast );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );
      DUMP_NUM( "array_size_c_num", $2 );

      c_ast_t *const array = C_AST_NEW( K_ARRAY, &@$ );
      array->as.array.size = $2;
      c_ast_set_parent( C_AST_NEW( K_PLACEHOLDER, &@1 ), array );

      if ( $1.target_ast != NULL ) {    // array-of or function-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array );
      } else {
        $$.ast = c_ast_add_array( $1.ast, array );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

array_size_c_num
  : '[' ']'                       { $$ = C_ARRAY_SIZE_NONE; }
  | '[' Y_NUMBER ']'              { $$ = $2; }
  | '[' error ']'
    {
      elaborate_error( "integer expected for array size" );
    }
  ;

func_decl_c_ast
  : /* type_c_ast */ decl2_c_ast '(' arg_list_c_ast_opt ')'
    func_qualifier_list_c_type_opt
    func_noexcept_c_type_opt func_trailing_return_type_c_ast_opt
    pure_virtual_c_type_opt
    {
      DUMP_START( "func_decl_c_ast",
                  "decl2_c_ast '(' arg_list_c_ast_opt ')' "
                  "func_qualifier_list_c_type_opt "
                  "func_noexcept_c_type_opt "
                  "func_trailing_return_type_c_ast_opt "
                  "pure_virtual_c_type_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "decl2_c_ast", $1.ast );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $3 );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", $5 );
      DUMP_TYPE( "func_noexcept_c_type_opt", $7 );
      DUMP_AST( "func_trailing_return_type_c_ast_opt", $8.ast );
      DUMP_TYPE( "pure_virtual_c_type_opt", $9 );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );

      c_ast_t *const func = C_AST_NEW( K_FUNCTION, &@$ );
      func->type_id = $5 | $6 | $7 | $9;
      func->as.func.args = $3;

      if ( $8.ast != NULL ) {
        $$.ast = c_ast_add_func( $1.ast, $8.ast, func );
      }
      else if ( $1.target_ast != NULL ) {
        $$.ast = $1.ast;
        (void)c_ast_add_func( $1.target_ast, type_peek(), func );
      }
      else {
        $$.ast = c_ast_add_func( $1.ast, type_peek(), func );
      }
      $$.target_ast = func->as.func.ret_ast;

      DUMP_AST( "func_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

func_qualifier_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | func_qualifier_list_c_type_opt
    {
      DUMP_START( "func_qualifier_list_c_type_opt",
                  "func_qualifier_list_c_type_opt func_qualifier_c_type" );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", $1 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "func_qualifier_list_c_type_opt", $$ );
      DUMP_END();
    }
  ;

sname_c_ast
  : /* type_c_ast */ sname_c
    {
      DUMP_START( "sname_c_ast", "sname_c" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_STR( "sname", c_sname_full_c( &$1 ) );

      $$.ast = type_peek();
      $$.target_ast = NULL;
      c_ast_sname_set_sname( $$.ast, &$1 );

      DUMP_AST( "sname_c_ast", $$.ast );
      DUMP_END();
    }
  ;

sname_c
  : sname_c "::" Y_NAME
    {
      // see the comment in "of_scope_list_english_opt"
      if ( c_init >= INIT_READ_CONF && !C_LANG_IS_CPP() ) {
        print_error( &@2, "scoped names not supported in %s", C_LANG_NAME() );
        PARSE_ABORT();
      }
      $$ = $1;
      c_type_id_t sn_type = c_sname_type( &$1 );
      if ( sn_type == T_NONE )
        sn_type = T_SCOPE;
      c_sname_set_type( &$$, sn_type );
      c_sname_append_name( &$$, $3 );
    }
  | sname_c "::" Y_TYPEDEF_TYPE
    {
      //
      // This is for a case like:
      //
      //      define S::int8_t as char
      //
      // that is: the type int8_t is an existing type in no scope being defined
      // as a distinct type in a new scope.
      //
      $$ = $1;
      c_sname_t temp = c_ast_sname_dup( $3->ast );
      c_sname_set_type( &$$, c_sname_type( &temp ) );
      c_sname_append_sname( &$$, &temp );
    }
  | Y_NAME
    {
      c_sname_init( &$$ );
      c_sname_append_name( &$$, $1 );
    }
  ;

nested_decl_c_ast
  : '(' placeholder_c_ast { type_push( $2.ast ); ++ast_depth; } decl_c_ast ')'
    {
      type_pop();
      --ast_depth;

      DUMP_START( "nested_decl_c_ast",
                  "'(' placeholder_c_ast decl_c_ast ')'" );
      DUMP_AST( "placeholder_c_ast", $2.ast );
      DUMP_AST( "decl_c_ast", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

placeholder_c_ast
  : /* empty */
    {
      $$.ast = C_AST_NEW( K_PLACEHOLDER, &@$ );
      $$.target_ast = NULL;
    }
  ;

pointer_decl_c_ast
  : pointer_type_c_ast { type_push( $1.ast ); } decl_c_ast
    {
      type_pop();

      DUMP_START( "pointer_decl_c_ast", "pointer_type_c_ast decl_c_ast" );
      DUMP_AST( "pointer_type_c_ast", $1.ast );
      DUMP_AST( "decl_c_ast", $3.ast );

      (void)c_ast_patch_placeholder( $1.ast, $3.ast );
      $$ = $3;

      DUMP_AST( "pointer_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_type_c_ast
  : /* type_c_ast */ '*'
    {
      DUMP_START( "pointer_type_c_ast", "*" );
      DUMP_AST( "(type_c_ast)", type_peek() );

      $$.ast = C_AST_NEW( K_POINTER, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = $2;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "pointer_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

/// type //////////////////////////////////////////////////////////////////////

type
  : builtin_type lt_exp expr type_endian_opt '>'
  | Y_TYPEDEF_TYPE
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

/*****************************************************************************/
/*  cast productions                                                         */
/*****************************************************************************/

cast_c_ast_opt
  : /* empty */                   { $$.ast = $$.target_ast = NULL; }
  | cast_c_ast
  ;

cast_c_ast
  : cast2_c_ast
  | pointer_cast_c_ast
  ;

cast2_c_ast
  : array_cast_c_ast
  | func_cast_c_ast
  | nested_cast_c_ast
  | sname_c_ast
  ;

array_cast_c_ast
  : /* type */ cast_c_ast_opt arg_array_size_c_ast
    {
      DUMP_START( "array_cast_c_ast", "cast_c_ast_opt array_size_c_num" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "cast_c_ast_opt", $1.ast );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );
      DUMP_AST( "arg_array_size_c_ast", $2 );

      c_ast_set_parent( C_AST_NEW( K_PLACEHOLDER, &@1 ), $2 );

      if ( $1.target_ast != NULL ) {    // array-of or function-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, $2 );
      } else {
        c_ast_t *const ast = $1.ast != NULL ? $1.ast : type_peek();
        $$.ast = c_ast_add_array( ast, $2 );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

arg_array_size_c_ast
  : array_size_c_num
    {
      $$ = C_AST_NEW( K_ARRAY, &@$ );
      $$->as.array.size = $1;
    }
  ;

func_cast_c_ast
  : /* type_c_ast */ cast2_c_ast '(' arg_list_c_ast_opt ')'
    func_qualifier_list_c_type_opt func_trailing_return_type_c_ast_opt
    {
      DUMP_START( "func_cast_c_ast",
                  "cast2_c_ast '(' arg_list_c_ast_opt ')' "
                  "func_qualifier_list_c_type_opt "
                  "func_trailing_return_type_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "cast2_c_ast", $1.ast );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $3 );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", $5 );
      DUMP_AST( "func_trailing_return_type_c_ast_opt", $6.ast );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );

      c_ast_t *const func = C_AST_NEW( K_FUNCTION, &@$ );
      func->type_id = $5;
      func->as.func.args = $3;

      if ( $6.ast != NULL ) {
        $$.ast = c_ast_add_func( $1.ast, $6.ast, func );
      }
      else if ( $1.target_ast != NULL ) {
        $$.ast = $1.ast;
        (void)c_ast_add_func( $1.target_ast, type_peek(), func );
      }
      else {
        $$.ast = c_ast_add_func( $1.ast, type_peek(), func );
      }
      $$.target_ast = func->as.func.ret_ast;

      DUMP_AST( "func_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

nested_cast_c_ast
  : '(' placeholder_c_ast
    {
      type_push( $2.ast );
      ++ast_depth;
    }
    cast_c_ast_opt ')'
    {
      type_pop();
      --ast_depth;

      DUMP_START( "nested_cast_c_ast",
                  "'(' placeholder_c_ast cast_c_ast_opt ')'" );
      DUMP_AST( "placeholder_c_ast", $2.ast );
      DUMP_AST( "cast_c_ast_opt", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_cast_c_ast
  : pointer_type_c_ast { type_push( $1.ast ); } cast_c_ast_opt
    {
      type_pop();

      DUMP_START( "pointer_cast_c_ast", "pointer_type_c_ast cast_c_ast_opt" );
      DUMP_AST( "pointer_type_c_ast", $1.ast );
      DUMP_AST( "cast_c_ast_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "pointer_cast_c_ast", $$.ast );
      DUMP_END();
    }
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

eqeq_exp
  : "=="
  | error
    {
      elaborate_error( "\"==\" expected" );
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

name_ast
  : Y_NAME
    {
      DUMP_START( "name_ast", "NAME" );
      DUMP_STR( "NAME", $1 );

      $$.ast = C_AST_NEW( K_NAME, &@$ );
      $$.target_ast = NULL;
      c_ast_sname_set_name( $$.ast, $1 );

      DUMP_AST( "name_ast", $$.ast );
      DUMP_END();
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

number_exp
  : Y_NUMBER
  | error
    {
      elaborate_error( "number expected" );
    }
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

typedef_type_sname
  : Y_TYPEDEF_TYPE                { $$ = c_ast_sname_dup( $1->ast ); }
  | Y_TYPEDEF_TYPE "::" sname_c
    {
      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define S::T as struct T
      //
      $$ = c_ast_sname_dup( $1->ast );
      c_sname_set_type( &$$, c_sname_type( &$3 ) );
      c_sname_append_sname( &$$, &$3 );
    }
  | sname_c
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
