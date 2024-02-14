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
#include "debug.h"
#include "did_you_mean.h"
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

#ifdef __GNUC__
// Silence these warnings for Bison-generated code.
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunreachable-code"
#endif /* __GNUC__ */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

#define IF_DEBUG(...)             BLOCK( if ( opt_ad_debug ) { __VA_ARGS__ } )

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
 * Dumps a `bool`.
 *
 * @param KEY The key name to print.
 * @param BOOL The `bool` to dump.
 */
#define DUMP_BOOL(KEY,BOOL)  IF_DEBUG( \
  DUMP_KEY( KEY ": %s", ((BOOL) ? "true" : "false") ); )

/**
 * Dumps a comma followed by a newline the _second_ and subsequent times it's
 * called.  It's used to separate items being dumped.
 */
#define DUMP_COMMA                fput_sep( ",\n", &dump_comma, stdout )

/**
 * Dumps an ad_expr.
 *
 * @param KEY The key name to print.
 * @param EXPR The ad_expr to dump.
 */
#define DUMP_EXPR(KEY,EXPR) \
  IF_DEBUG( DUMP_COMMA; ad_expr_dump( (EXPR), (KEY), stdout ); )

/**
 * Dumps an `s_list` of ad_expr_t.
 *
 * @param KEY The key name to print.
 * @param EXPR_LIST The `s_list` of ad_expr_t to dump.
 */
#define DUMP_EXPR_LIST(KEY,EXPR_LIST) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); ad_expr_list_dump( &(EXPR_LIST), /*indent=*/1, stdout ); )

/**
 * Possibly dumps a comma and a newline followed by the `printf()` arguments
 * &mdash; used for printing a key followed by a value.
 *
 * @param ... The `printf()` arguments.
 */
#define DUMP_KEY(...) IF_DEBUG( \
  DUMP_COMMA; PRINTF( "  " __VA_ARGS__ ); )

/**
 * Dumps an integer.
 *
 * @param KEY The key name to print.
 * @param NUM The integer to dump.
 */
#define DUMP_INT(KEY,NUM) \
  DUMP_KEY( KEY ": %d", STATIC_CAST( int, (NUM) ) )

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 */
#define DUMP_STR(KEY,STR) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); str_dump( (STR), stdout ); )

/**
 * Starts a dump block.
 *
 * @param NAME The grammar production name.
 * @param PROD The grammar production rule.
 *
 * @sa #DUMP_END
 */
#define DUMP_START(NAME,PROD) \
  bool dump_comma = false;    \
  IF_DEBUG( FPUTS( NAME " ::= " PROD " = {\n", stdout ); )

/**
 * Ends a dump block.
 *
 * @sa #DUMP_START
 */
#define DUMP_END()                IF_DEBUG( FPUTS( "\n}\n\n", stdout ); )

#define DUMP_TYPE(KEY,TYPE) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); ad_type_dump( TYPE, stdout ); )

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

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local variables
static slist_t        expr_gc_list;     ///< `expr` nodes freed after parse.
static slist_t        statement_list;

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
 * Prints an additional parsing error message including a newline to standard
 * error that continues from where yyerror() left off.  Additionally:
 *
 * + If the printable_token() isn't NULL:
 *     + Checks to see if it's a keyword: if it is, mentions that it's a
 *       keyword in the error message.
 *     + May print "did you mean ...?" \a dym_kinds suggestions.
 *
 * + In debug mode, also prints the file & line where the function was called
 *   from.
 *
 * @note This function isn't normally called directly; use the
 * #elaborate_error() macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param dym_kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param format A `printf()` style format string.
 * @param ... Arguments to print.
 *
 * @sa fl_punct_expected()
 * @sa yyerror()
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

  char const *const error_token = printable_token();
  if ( error_token != NULL )
    EPRINTF( "\"%s\": ", error_token );

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
  ad_field_t         *field;
  int                 int_val;
  slist_t             list;       // multipurpose list
  char const         *literal;    // token literal
  char               *name;       // name being declared or explained
  ad_rep_t            rep_val;
  ad_statement_t     *statement;
  char               *str_val;    // quoted string value
  ad_switch_case_t   *switch_case;
  ad_type_t           type;
  ad_typedef_t const *tdef;
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
%left               Y_COLON2      "::"

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
%right  <oper_id>   Y_sizeof
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
                    Y_EXCLAM_EQ   "!="
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
%right  <oper_id>                 '?' ':'
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
%type <expr>        multiplicative_expr
%type <expr>        postfix_expr
%type <expr>        primary_expr
%type <expr>        relational_expr
%type <expr>        shift_expr
%type <expr>        unary_expr

                    // Statements
%type <statement>   compound_statement
%type <statement>   declaration
%type <statement>   enum_declaration
%type <statement>   field_declaration
%type <statement>   statement
%type <list>        statement_list statement_list_opt
%type <statement>   struct_declaration
%type <statement>   switch_statement
%type <statement>   typedef_declaration

                    // Miscellaneous
%type <list>        argument_expr_list argument_expr_list_opt
%type <expr_kind>   assign_op
%type <int_val>     int_exp
%type <name>        name_exp
%type <type>        type
%type <expr>        type_endian_exp
%type <expr_kind>   unary_op

/*
 * Bison %destructors.  We don't use the <identifier> syntax because older
 * versions of Bison don't support it.
 *
 * Clean-up of expr nodes is done via garbage collection using expr_gc_list.
 */

/* name */
%destructor { DTRACE; FREE( $$ ); } name_exp
%destructor { DTRACE; FREE( $$ ); } Y_NAME

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
      slist_push_back( &statement_list, $2 );
    }
  | statement
    {
      slist_init( &$$ );
      slist_push_back( &$$, $1 );
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
      $$->kind = AD_STMT_COMPOUND;
      $$->compound.statements = $2;
    }
  ;

/// switch statement //////////////////////////////////////////////////////////

switch_statement
  : Y_switch lparen_exp expr rparen_exp lbrace_exp switch_case_list_opt '}'
    {
      $$ = MALLOC( ad_statement_t, 1 );
      $$->kind = AD_STMT_SWITCH;
      $$->loc = @$;
      $$->st_switch.expr = $3;
      // $$->st_switch
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
      slist_push_back( &$$, &$1 );
    }
  ;

switch_case
  : Y_case expr_exp colon_exp statement_list_opt
    {
      DUMP_START( "switch_case", "CASE expr ':' statement_list_opt" );
      DUMP_EXPR( "expr", $2 );

      $$ = MALLOC( ad_switch_case_t, 1 );
      $$->expr = $2;
      $$->statement_list = $4;

      DUMP_END();
    }
  | Y_default colon_exp statement_list_opt
    {
      $$ = MALLOC( ad_switch_case_t, 1 );
      $$->expr = NULL;
      $$->statement_list = $3;
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  DECLARATIONS                                                             //
///////////////////////////////////////////////////////////////////////////////

declaration
  : enum_declaration
  | field_declaration
  | struct_declaration
  | typedef_declaration
  ;

/// enum declaration //////////////////////////////////////////////////////////

enum_declaration
  : Y_enum name_exp colon_exp type lbrace_exp enumerator_list rbrace_exp
    {
      ad_enum_t *const ad_enum = MALLOC( ad_enum_t, 1 );
   // ad_enum->name = $2;
   // ad_enum->bits = XX;
   // ad_enum->endian = XX;
   // ad_enum->base = xx;
      ad_enum->values = $6;
      (void)$2;
    }
  ;

enumerator_list
  : enumerator_list ',' enumerator
    {
      $$ = $1;
      slist_push_back( &$$, &$3 );
    }
  | enumerator
    {
      slist_init( &$$ );
      slist_push_back( &$$, &$1 );
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
  : type Y_NAME array_opt
    {
      ad_field_t *const ad_field = MALLOC( ad_field_t, 1 );
      ad_field->name = $2;
      ad_field->rep = $3;
      ad_field->type = $1;
    }
  ;

array_opt
  : /* empty */                   { $$.times = AD_REP_1; }
  | '[' ']' equals_exp expr_exp
    {
      $$.times = AD_REP_1;
      $$.expr = $4;
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
      elaborate_error( "array size expected" );
    }
  ;

/// struct declaration ////////////////////////////////////////////////////////

struct_declaration
  : Y_struct name_exp lbrace_exp statement_list_opt rbrace_exp
    {
      ad_struct_t *const ad_struct = MALLOC( ad_struct_t, 1 );
      ad_struct->name = $2;
      ad_struct->members = $4;
    }
  ;

/// typedef declaration ///////////////////////////////////////////////////////

typedef_declaration
  : Y_typedef field_declaration
    {
      // TODO
    }
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
      elaborate_error( "expression expected" );
    }
  ;

additive_expr
  : multiplicative_expr
  | additive_expr '+' multiplicative_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_ADD, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | additive_expr '-' multiplicative_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_SUB, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

assign_expr
  : conditional_expr
  | unary_expr assign_op assign_expr
    {
      $$ = ad_expr_new( $2, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

bitwise_and_expr
  : equality_expr
  | bitwise_and_expr '&' equality_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_AND, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

bitwise_exclusive_or_expr
  : bitwise_and_expr
  | bitwise_exclusive_or_expr '^' bitwise_and_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_XOR, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

bitwise_or_expr
  : bitwise_exclusive_or_expr
  | bitwise_or_expr '|' bitwise_exclusive_or_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_OR, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

cast_expr
  : unary_expr
  | '(' Y_NAME rparen_exp cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_CAST, &@$ );
      //$$->binary.lhs_expr = $2;
      (void)$2;
      $$->binary.rhs_expr = $4;
    }
  ;

conditional_expr
  : logical_or_expr
  | logical_or_expr '?' expr ':' conditional_expr
    {
      $$ = ad_expr_new( AD_EXPR_IF_ELSE, &@$ );
      $$->ternary.cond_expr = $1;
      $$->ternary.sub_expr[0] = $3;
      $$->ternary.sub_expr[1] = $5;
    }
  ;

equality_expr
  : relational_expr
  | equality_expr "==" relational_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_EQ, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | equality_expr "!=" relational_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_NOT_EQ, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

logical_and_expr
  : bitwise_or_expr
  | logical_and_expr "&&" bitwise_or_expr
    {
      $$ = ad_expr_new( AD_EXPR_LOG_AND, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

logical_or_expr
  : logical_and_expr
  | logical_or_expr "||" logical_and_expr
    {
      $$ = ad_expr_new( AD_EXPR_LOG_OR, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

multiplicative_expr
  : cast_expr
  | multiplicative_expr '*' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_MUL, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | multiplicative_expr '/' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_DIV, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | multiplicative_expr '%' cast_expr
    {
      $$ = ad_expr_new( AD_EXPR_MATH_MOD, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

postfix_expr
  : primary_expr
  | postfix_expr '[' expr ']'
    {
      $$ = ad_expr_new( AD_EXPR_ARRAY, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | postfix_expr '(' argument_expr_list_opt ')'
    {
      // TODO
      (void)$1;
      (void)$3;
    }
  | postfix_expr '.' Y_NAME
    {
      // TODO
      (void)$1;
      (void)$3;
    }
  | postfix_expr "->" Y_NAME
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

argument_expr_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | argument_expr_list
  ;

argument_expr_list
  : argument_expr_list ',' assign_expr
    {
      slist_push_back( &$$, $3 );
    }
  | assign_expr
    {
      slist_init( &$$ );
      slist_push_back( &$$, $1 );
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
      $$ = ad_expr_new( AD_EXPR_VALUE, &@$ );
      $$->value.type.tid = T_INT;
      $$->value.i64 = $1;
    }
  | Y_STR_LIT
    {
      $$ = ad_expr_new( AD_EXPR_VALUE, &@$ );
      $$->value.s = $1;
    }
  | '(' expr ')'                  { $$ = $2; }
  ;

relational_expr
  : shift_expr
  | relational_expr '<' shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_LESS, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | relational_expr '>' shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_GREATER, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | relational_expr "<=" shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_LESS_EQ, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | relational_expr ">=" shift_expr
    {
      $$ = ad_expr_new( AD_EXPR_REL_GREATER_EQ, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  ;

shift_expr
  : additive_expr
  | shift_expr "<<" additive_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_LEFT, &@$ );
      $$->binary.lhs_expr = $1;
      $$->binary.rhs_expr = $3;
    }
  | shift_expr ">>" additive_expr
    {
      $$ = ad_expr_new( AD_EXPR_BIT_SHIFT_RIGHT, &@$ );
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
      $$ = ad_expr_new( $1, &@$ );
      $$->unary.sub_expr = $2;
    }
  | Y_sizeof unary_expr
    {
      // TODO
      (void)$2;
    }
  | Y_sizeof '(' Y_NAME rparen_exp
    {
      // TODO
      free( $3 );
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
  : builtin_tid lt_exp expr gt_exp type_endian_exp
    {
      $$.tid = $1;
      $$.size_expr = $3;
      $$.endian_expr = $5;
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

type_endian_exp
  : '\\' 'b'
    {
      $$ = ad_expr_new( AD_EXPR_VALUE, &@$ );
      $$->value.type.tid = T_INT8;
      $$->value.i64 = ENDIAN_BIG;
    }
  | '\\' 'l'
    {
      $$ = ad_expr_new( AD_EXPR_VALUE, &@$ );
      $$->value.type.tid = T_INT8;
      $$->value.i64 = ENDIAN_LITTLE;
    }
  | '\\' 'h'
    {
      $$ = ad_expr_new( AD_EXPR_VALUE, &@$ );
      $$->value.type.tid = T_INT8;
      $$->value.i64 = ENDIAN_HOST;
    }
  | '\\' '<' expr gt_exp
    {
      $$ = $3;
    }
  | '\\' error
    {
      elaborate_error( "one of 'b', 'l', 'h', or '<expr>' expected" );
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

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans up global parser data at program termination.
 */
void parser_cleanup( void ) {
  // do nothing
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
