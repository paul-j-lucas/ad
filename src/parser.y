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
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "color.h"
#ifdef ENABLE_AD_DEBUG
#endif /* ENABLE_AD_DEBUG */
#include "expr.h"
#include "keyword.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "slist.h"
#include "types.h"
#include "typedef.h"
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
#define PARSE_ABORT() \
  BLOCK( parse_cleanup( /*fatal_error=*/true ); YYABORT; )

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
NODISCARD
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
 * @param dym_kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param format A `printf()` style format string.
 * @param ... Arguments to print.
 */
static void fl_elaborate_error( char const *file, int line,
                                dym_kind_t dym_kinds, char const *format,
                                ... ) {
  assert( format != NULL );

  EPUTS( ": " );
  print_debug_file_line( file, line );

  char const *const error_token = printable_token();
  if ( error_token != NULL )
    EPRINTF( "\"%s\": ", printable_token() );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  if ( error_token != NULL ) {
    ad_keyword_t const *const k = ad_keyword_find( error_token );
    if ( k != NULL )
      EPRINTF( " (\"%s\" is a keyword)", error_token );
    print_suggestions( dym_kinds, error_token );
  }

  EPUTC( '\n' );
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

  slist_cleanup( &expr_gc_list, (slist_free_fn_t)&ad_expr_free );
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
 *
 * @sa fl_elaborate_error()
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
  unsigned            bitmask;    // multipurpose bitmask (used by show)
  endian_t            endian_val;
  ad_enum_value_t     enum_val;
  ad_expr_t          *expr;       // for the expression being built
  ad_expr_kind_t      expr_kind;  // built-ins, storage classes, & qualifiers
  ad_field_t          field;
  int                 int_val;
  slist_t             list;       // multipurpose list
  char const         *literal;    // token literal
  char               *name;       // name being declared or explained
  ad_rep_t            rep_val;
  ad_statement_t      statement;
  char const         *str_lit;    // string literal
  ad_switch_case_t    switch_case;
  ad_typedef_t        tdef;
  ad_tid_t            tid;
}

                    // ad keywords
%token              Y_ALIGNAS;
%token  <expr_kind> Y_BOOL
%token              Y_BREAK
%token              Y_CASE
%token  <expr_kind> Y_CHAR
%token              Y_DEFAULT
%token  <expr_kind> Y_ENUM
%token  <expr_kind> Y_FALSE
%token  <expr_kind> Y_FLOAT
%token  <expr_kind> Y_INT
%token              Y_OFFSETOF
%token  <expr_kind> Y_STRUCT
%token              Y_SWITCH
%token  <expr_kind> Y_TRUE
%token  <expr_kind> Y_TYPEDEF
%token  <expr_kind> Y_UINT
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
%token  <str_lit>   Y_STR_LIT
%token  <tid>       Y_TYPEDEF_TYPE

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
// Sort using: sort -bdk3

                    // Declarations
%type <rep_val>     array_opt
%type <tid>         builtin_tid
%type <enum_val>    enumerator
%type <list>        enumerator_list
%type <switch_case> switch_case
%type <list>        switch_case_list switch_case_list_opt
%type <tid>         tid tid_exp

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
%type <expr>        multiplicative_expr
%type <expr>        postfix_expr
%type <expr>        primary_expr
%type <expr>        relational_expr
%type <expr>        shift_expr
%type <expr>        unary_expr

                    // Statements
//%type <statement>   compound_statement
//%type <statement>   declaration
//%type <statement>   statement
%type <list>        statement_list statement_list_opt
//%type <statement>   switch_statement

                    // Miscellaneous
%type <list>        argument_expr_list
%type <expr_kind>   assign_op
%type <name>        name_exp name_opt
%type <str_lit>     str_lit str_lit_exp
//%type <endian_val>     type_endian_opt
%type <endian_val>  type_endian_exp
%type <name>        type_name_exp
%type <expr_kind>   unary_op

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
%destructor { DTRACE; FREE( $$ ); } type_name_exp

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
      //$$ = $1;
      //slist_push_back( &$$, $1 );
    }
  | statement
    {
      //slist_init( &$$ );
      //slist_push_back( &$$, $1 );
    }
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
    {
      //$$.kind = AD_STMT_COMPOUND;
      //$$.compound.statements = $2;
    }
  ;

declaration
  : enum_declaration
  | field_declaration
  | struct_declaration
  | typedef_declaration
  ;

/// enum declaration //////////////////////////////////////////////////////////

enum_declaration
  : Y_ENUM name_exp colon_exp tid_exp lbrace_exp enumerator_list rbracket_exp
    {
   // ad_enum_t *const ad_enum = MALLOC( ad_enum_t, 1 );
   // ad_enum->name = $2;
   // ad_enum->bits = XX;
   // ad_enum->endian = XX;
   // ad_enum->base = xx;
    }
  ;

enumerator_list
  : enumerator_list ',' enumerator
    {
      $$ = $1;
      //slist_push_back( &$$, $3 );
    }
  | enumerator
    {
      slist_init( &$$ );
      //slist_push_back( &$$, $1 );
    }
  ;

enumerator
  : Y_NAME equals_exp int_exp
    {
      $$.name = $1;
   // $$.value = $3;
    }
  ;

/// field declaration /////////////////////////////////////////////////////////

field_declaration
  : tid_exp name_exp array_opt
    {
      ad_field_t *const ad_field = MALLOC( ad_field_t, 1 );
      ad_field->type = $1;
      ad_field->name = $2;
      ad_field->rep = $3;
    }

  | Y_TYPEDEF_TYPE name_exp array_opt
    {
      //ad_field_t *const ad_field = MALLOC( ad_field_t, 1 );
      // TODO
      free( $2 );
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
      ad_struct_t *const ad_struct = MALLOC( ad_struct_t, 1 );
      ad_struct->name = $2;
      // TODO
    }
  ;

/// switch statement //////////////////////////////////////////////////////////

switch_statement
  : Y_SWITCH lparen_exp expr rparen_exp lbrace_exp switch_case_list_opt '}'
    {
    }
  ;

switch_case_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | switch_case_list
  ;

switch_case_list
  : switch_case_list switch_case
    {
      $$ = $1;
      slist_push_back( &$$, &$2 );
    }

  | switch_case
    {
      slist_init( &$$ );
      slist_push_back( &$$, $1 );
    }
  ;

switch_case
  : Y_CASE expr_exp colon_exp statement_list_opt
    {
      $$ = MALLOC( ad_switch_case_t, 1 );
      $$->expr = $2;
      // TODO
    }
  | Y_DEFAULT colon_exp statement_list_opt
    {
      $$ = $3;
    }
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

expr_exp
  : expr
  | error
    {
    }
  ;

additive_expr
  : multiplicative_expr
  | additive_expr '+' multiplicative_expr
    {
      $$ = ad_expr_new( AD_EXPR_ADD );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | additive_expr '-' multiplicative_expr
    {
      $$ = ad_expr_new( AD_EXPR_SUB );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
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
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

bitwise_exclusive_or_expr
  : bitwise_and_expr
  | bitwise_exclusive_or_expr '^' bitwise_and_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_XOR );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

bitwise_or_expr
  : bitwise_exclusive_or_expr
  | bitwise_or_expr '|' bitwise_exclusive_or_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_OR );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

cast_expr
  : unary_expr
  | '(' type_name_exp rparen_exp cast_expr
    {
      // TODO
      (void)$2;
      (void)$4;
    }
  ;

conditional_expr
  : logical_or_expr
  | logical_or_expr '?' expr ':' conditional_expr
    {
      $$ = ad_expr_new( AD_EXPR_IF_ELSE );
      $$->ternary.cond_expr = $1;
      $$->ternary.sub_expr[0] = $3;
      $$->ternary.sub_expr[1] = $5;
    }
  ;

equality_expr
  : relational_expr
  | equality_expr "==" relational_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_EQ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | equality_expr "!=" relational_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_NOT_EQ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

logical_and_expr
  : logical_or_expr
  | logical_and_expr "&&" logical_or_expr
    {
      $$ = ad_expr_new( AD_EXPR_LOG_AND );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

logical_or_expr
  : logical_and_expr
  | logical_or_expr "||" logical_and_expr
    {
      $$ = ad_expr_new( AD_EXPR_LOG_OR );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

multiplicative_expr
  : cast_expr
  | multiplicative_expr '*' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_MUL );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | multiplicative_expr '/' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_DIV );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | multiplicative_expr '%' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_MOD );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

postfix_expr
  : primary_expr
  | postfix_expr '[' expr ']'
    {
      // TODO
      (void)$1;
      (void)$3;
    }
  | postfix_expr '(' ')'
    {
      // TODO
      (void)$1;
    }
  | postfix_expr '(' argument_expr_list ')'
    {
      // TODO
      (void)$1;
      (void)$3;
    }
  | postfix_expr '.' name_exp
    {
      // TODO
      (void)$1;
      (void)$3;
    }
  | postfix_expr "->" name_exp
    {
      // TODO
      (void)$1;
      (void)$3;
    }
  | postfix_expr "++"
    {
      // TODO
      (void)$1;
    }
  | postfix_expr "--"
    {
      // TODO
      (void)$1;
    }
  ;

argument_expr_list
  : assign_expr
    {
      slist_init( &$$ );
      slist_push_back( &$$, $1 );
    }
  | argument_expr_list ',' assign_expr
    {
      slist_push_back( &$$, $3 );
    }
  ;

primary_expr
  : Y_NAME
    {
      // TODO
      (void)$1;
    }
  | Y_INT_LIT
    {
      $$ = ad_expr_new( AD_EXPR_VALUE );
   // $$.value.type = xx;
      $$.value.i64 = $1;
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
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | relational_expr '>' shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_GREATER );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | relational_expr "<=" shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_LESS_EQ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | relational_expr ">=" shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_GREATER_EQ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

shift_expr
  : additive_expr
  | shift_expr "<<" additive_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_LEFT );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | shift_expr ">>" additive_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_RIGHT );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

unary_expr
  : postfix_expr
  | "++" unary_expr
    {
      // TODO
      (void)$2;
    }
  | "--" unary_expr
    {
      // TODO
      (void)$2;
    }
  | unary_op cast_expr
    {
      // TODO
      (void)$1;
      (void)$2;
    }
  | Y_SIZEOF unary_expr
    {
      // TODO
      (void)$2;
    }
  | Y_SIZEOF '(' type_name_exp rparen_exp
    {
      // TODO
      (void)$3;
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

/// type //////////////////////////////////////////////////////////////////////

type
  : tid '<' expr '>'
  ;

builtin_tid
  : Y_FLOAT                       { $$ = T_FLOAT; }
  | Y_INT                         { $$ = T_INT; }
  | Y_UINT                        { $$ = T_INT; }
  | Y_UTF                         { $$ = T_UTF; }
  ;

tid
  : builtin_tid lt_exp expr gt_exp type_endian_exp
    {
      $$ = $1;
      // TODO
    }
  | Y_TYPEDEF_TYPE
  ;

tid_exp
  : tid
  | error
    {
    }
  ;

type_endian_exp
  : '\\' 'b'                      { $$ = ENDIAN_BIG; }
  | '\\' 'l'                      { $$ = ENDIAN_LITTLE; }
  | '\\' 'h'                      { $$ = ENDIAN_HOST; }
  | '\\' '<' expr '>'             { $$ = $3; }
  ;

type_name_exp
  : name_exp
  ;

///////////////////////////////////////////////////////////////////////////////
//  MISCELLANEOUS                                                            //
///////////////////////////////////////////////////////////////////////////////

colon_exp
  : ':'
  | error
    {
      elaborate_error( "':' expected" );
    }
  ;

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
