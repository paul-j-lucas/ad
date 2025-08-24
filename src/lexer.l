/*
**      ad -- ASCII dump
**      src/lexer.l
**
**      Copyright (C) 2022-2024  Paul J. Lucas
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
 * tokenizer for the **ad** language.
 */

/** @cond DOXYGEN_IGNORE */

%option warn
%option yylineno

%top {
#include "ad.h"                         /* must go first */
}

%{
/** @endcond */

// local
#include "lexer.h"
#include "keyword.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "strbuf.h"
#include "symbol.h"
#include "util.h"
#include "ad_parser.h"                  /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>                     /* for strtol(3) */
#include <string.h>

// Silence these warnings for Flex-generated code.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * C++ raw string delimiter maximum length.
 */
#define RSTR_DELIM_LEN_MAX        16u

/**
 * Overrides Flex's fatal error message to print the message in our format and
 * also exit with the status code we want.
 *
 * @param MSG The error message to print.
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( (MSG) )

/**
 * Overrides Flex's input.
 *
 * @param BUF The buffer to use.
 * @param BYTES_READ Set to the number of bytes read.
 * @param BYTES_MAX The maximum number of bytes to read.
 */
#define YY_INPUT(BUF,BYTES_READ,BYTES_MAX) \
  (BYTES_READ) = lexer_get_input( (BUF), STATIC_CAST( yy_size_t, (BYTES_MAX) ) )

/**
 * This code is inserted by Flex at the beginning of each rule to set the
 * current token location information.
 */
#define YY_USER_ACTION            lexer_update_loc(); /* must include ';' */

///////////////////////////////////////////////////////////////////////////////

// extern variables
bool              lexer_in_template;

// local variables
static char      *input_buf;            ///< Entire current input line.
static yy_size_t  input_len;            ///< Length of line in `input_buf`.
static size_t     input_sent;           ///< How many bytes returned to Flex.
static bool       is_eof;               ///< Encountered EOF?
static unsigned   less_depth;           ///< Depth of `<>`.
static strbuf_t   str_lit_buf;          ///< String (or character) literal.
static ad_loc_t   str_lit_loc;          ///< String literal starting location.
static int        token_column;         ///< Column position of current token.

/// C++ raw string literal delimiter.
static char       rstr_delim[ RSTR_DELIM_LEN_MAX + 1/*"*/ + 1/*\0*/ ];

// local functions
_Noreturn
static void       lexer_fatal( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans up lexer data.
 *
 * @sa lexer_init()
 */
static void lexer_cleanup( void ) {
  strbuf_cleanup( &str_lit_buf );
}

/**
 * Gets a line of input for Flex and keeps a copy for use later if printing an
 * error message.
 *
 * @param buf A pointer to the buffer to write into.
 * @param buf_cap The capacity of \a buf.
 * @return Returns the number of bytes read.
 */
NODISCARD
static yy_size_t lexer_get_input( char *buf, yy_size_t buf_cap ) {
  assert( buf != NULL );
  yy_size_t input_avail = input_len - input_sent;
  if ( input_avail == 0 ) {
    static size_t input_capacity;
    ssize_t bytes_read = getline( &input_buf, &input_capacity, yyin );
    if ( unlikely( bytes_read == -1 ) ) {
      FERROR( yyin );
      bytes_read = 0;
    }
    input_avail = input_len = STATIC_CAST( yy_size_t, bytes_read );
    input_sent = 0;
    ++yylineno;
  }
  //
  // Given that the default buffer capacity (YY_READ_BUF_SIZE) for Flex is
  // 8192, it's unlikely that this will ever be true and that we'll have to
  // return the input line in chunks; but might as well code for the case.
  //
  if ( unlikely( input_avail > buf_cap ) )
    input_avail = buf_cap;
  memcpy( buf, input_buf + input_sent, input_avail );
  input_sent += input_avail;
  return input_avail;
}

/**
 * Update the parser's location.
 * @note This is called by Flex via #YY_USER_ACTION.
 */
static void lexer_update_loc( void ) {
  yylloc.first_line   = yylloc.last_line = yylineno;
  yylloc.first_column = token_column;
  yylloc.last_column  = token_column + yyleng - 1;
  token_column += yyleng;
}

/**
 * Resets the token column position upon encountering a newline.
 */
static inline void newline( void ) {
  token_column = 0;
}

/**
 * Parses an integer from \ref yytext.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
NODISCARD
static inline int parse_int( int base ) {
  char const *s = yytext;
  bool const is_neg = s[0] == '-';

  if ( is_neg )
    ++s;

  if ( base == 2 ) {
    // Prior to C23, strtol(3) doesn't understand a "0b" (binary) prefix, so
    // skip over it.
    assert( s[0] == '0' );              // String should start with "0b" ...
    assert( tolower( s[1] ) == 'b' );   // ... since it was just lex'd as such.
    s += STRLITLEN( "0b" );
  }

  char no_digit_seps[ MAX_DEC_INT_DIGITS(uintmax_t) + 1/*\0*/ ];

  if ( strchr( s, '\'' ) != NULL ) {
    // Strip digit separator (') characters since strtol(3) doesn't understand
    // them.
    char *t = no_digit_seps;
    do {
      if ( *s != '\'' )
        *t++ = *s;
    } while ( *s++ != '\0' );
    s = no_digit_seps;
  }

  errno = 0;
  long rv = strtol( s, /*endptr=*/NULL, base );
  if ( is_neg )
    rv = -rv;
  if ( unlikely( errno != 0 || rv < INT_MIN || rv > INT_MAX ) ) {
    // LCOV_EXCL_START
    rv = rv < INT_MIN ? INT_MIN : INT_MAX;
    print_warning( &yylloc, "integer out of range; clamped to %ld\n", rv );
    // LCOV_EXCL_STOP
  }

  return STATIC_CAST( int, rv );
}

/**
 * Standard lex function to know whether to continue parsing upon reaching EOF.
 *
 * @return Returns 1 if done or 0 to continue parsing (a new file set via
 * \c yyin).
 */
static int yywrap( void ) {
  return 1 /* done */;
}

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

L             [A-Za-z_]
B             [01]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]
S             [ \f\r\t\v]

identifier    {L}({L}|{D})*

cstr_pfx      L|u8?|U
rstr_pfx      {cstr_pfx}?R\"[^ \f\n\r\t\v()\\]*"("
int_sfx       [lL][lL]?[uU]?|[uU]([lL][lL]?|[zZ])?|[zZ][uU]?

bin_int       0[bB]{B}+{int_sfx}?
oct_int       0{O}*{int_sfx}?
dec_int       [1-9]{D}*{int_sfx}?
hex_int       0[xX]{H}+{int_sfx}?

/*
 * For character and string literals.
 */
%x X_CHAR X_STR X_RSTR

%%

  /***************************************************************************/
  /*  IDENTIFIERS                                                            */
  /***************************************************************************/

{identifier}  {
                //
                // 1. See if it's an ad keyword.
                //
                ad_keyword_t const *const k = ad_keyword_find( yytext );
                if ( k != NULL ) {
                  yylval.tid = k->tid;
                  return k->yy_token_id;
                }

                //
                // 2. See if it's a type or declaration.
                //
                synfo_t const *const synfo = sym_find_name( yytext );
                if ( synfo != NULL ) {
                  switch ( synfo->kind ) {
                    case SYM_DECL:
                      yylval.decl = synfo->decl;
                      return Y_DECL;
                    case SYM_TYPE:
                      yylval.type = synfo->type;
                      return Y_TYPE;
                  } // switch
                  UNEXPECTED_INT_VALUE( synfo->kind );
                }

                //
                // 3. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( yytext );
                return Y_NAME;
              }

  /***************************************************************************/
  /*  NUMERIC LITERALS                                                       */
  /***************************************************************************/

{bin_int}     { yylval.int_val = parse_int(  2 ); return Y_INT_LIT; }
{oct_int}     { yylval.int_val = parse_int(  8 ); return Y_INT_LIT; }
{dec_int}     { yylval.int_val = parse_int( 10 ); return Y_INT_LIT; }
{hex_int}     { yylval.int_val = parse_int( 16 ); return Y_INT_LIT; }

  /***************************************************************************/
  /*  OPERATORS & PUNCTUATION                                                */
  /***************************************************************************/

              /* Multi-character operators. */
"::"          { return Y_COLON_COLON           ; }
"!="          { return Y_EXCLAM_EQUAL          ; }
"%="          { return Y_PERCENT_EQUAL         ; }
"&&"          { return Y_AMPER_AMPER           ; }
"&="          { return Y_AMPER_EQUAL           ; }
"*="          { return Y_STAR_EQUAL            ; }
"++"          { return Y_PLUS_PLUS             ; }
"+="          { return Y_PLUS_EQUAL            ; }
"--"          { return Y_MINUS_MINUS           ; }
"-="          { return Y_MINUS_EQUAL           ; }
"->"          { return Y_MINUS_GREATER         ; }
"/="          { return Y_SLASH_EQUAL           ; }
"<<"          { return Y_LESS_LESS             ; }
"<<="         { return Y_LESS_LESS_EQUAL       ; }
"<="          { return Y_LESS_EQUAL            ; }
"=="          { return Y_EQUAL_EQUAL           ; }
">="          { return Y_GREATER_EQUAL         ; }
">>"          { return Y_GREATER_GREATER       ; }
">>="         { return Y_GREATER_GREATER_EQUAL ; }
"^="          { return Y_CIRC_EQUAL            ; }
"|="          { return Y_PIPE_EQUAL            ; }
"||"          { return Y_PIPE_PIPE             ; }

"<"           {
                if ( lexer_in_template )
                  ++less_depth;
                return '<';
              }

">"           {
                if ( lexer_in_template && less_depth > 0 && --less_depth == 0 )
                  return Y_TEMPLATE_END;
                return '>';
              }

              /* Single-character operators and miscellaneous punctuation. */
"!"           |
"$"           |
"%"           |
"&"           |
"("           |
")"           |
"*"           |
"+"           |
","           |
"-"           |
"."           |
"/"           |
":"           |
";"           |
"="           |
"?"           |
"["           |
"]"           |
"^"           |
"{"           |
"|"           |
"}"           |
"~"           { return yytext[0]               ; }

  /***************************************************************************/
  /*  RAW STRING LITERALS                                                    */
  /***************************************************************************/

{rstr_pfx}    { // e.g.: u8R"abc(...)abc"
                // find:    ^   ^
                char const *q = strchr( yytext, '"' );
                assert( q != NULL );
                char const *const p = strchr( ++q, '(' );
                assert( p != NULL );

                size_t const delim_len = STATIC_CAST( size_t, p - q );
                if ( delim_len > RSTR_DELIM_LEN_MAX ) {
                  print_error( &yylloc,
                    "raw string literal delimiter "
                    "exceeds maximum length of %u\n",
                    RSTR_DELIM_LEN_MAX
                  );
                  return Y_LEXER_ERROR;
                }

                strncpy( rstr_delim, q, delim_len );
                //
                // To make the delimiter-match code simpler, include the
                // closing '"' as part of the delimiter.  (Using strcpy() also
                // has the benefit of ensuring rstr_delim is null-terminated.)
                //
                strcpy( rstr_delim + delim_len, "\"" );

                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;

                BEGIN( X_RSTR );
              }

<X_RSTR>{

  [^)]+       { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  ")"         {
                char const *d;
                for ( d = rstr_delim; *d != '\0'; ++d, ++token_column ) {
                  int const c = input();
                  if ( c == EOF ) {
                    print_error( &yylloc, "unterminated string literal\n" );
                    strbuf_reset( &str_lit_buf );
                    return Y_LEXER_ERROR;
                  }
                  if ( c == *d )
                    continue;
                  //
                  // Found a mismatch with the delimiter, e.g.:
                  //
                  //      )abc"       // delimiter
                  //      )abx"       // what was found
                  //
                  // Hence, it's not the actual delimiter, but part of the
                  // string literal: append the part that matched (")ab").
                  //
                  int const part_len = STATIC_CAST( int, d - rstr_delim );
                  strbuf_printf( &str_lit_buf, ")%.*s", part_len, rstr_delim );
                  //
                  // Unput the character that caused the mismatch since it
                  // could be ')' that could potentially start a real match,
                  // e.g.:
                  //
                  //      R"abc(X)ab)abc"
                  //                ^
                  //
                  // Hence the raw string is "X)ab".
                  //
                  unput( c );
                  break;
                } // for

                if ( *d == '\0' ) {     // found delimiter
                  yylloc.first_line   = str_lit_loc.first_line;
                  yylloc.first_column = str_lit_loc.first_column;
                  yylval.str_val = strbuf_take( &str_lit_buf );
                  BEGIN( INITIAL );
                  return Y_STR_LIT;
                }
              }

} /* <X_RSTR> */

  /***************************************************************************/
  /*  STRING LITERALS & CHARACTER LITERALS                                   */
  /***************************************************************************/

{cstr_pfx}?['"] {
                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;

                char const quote = yytext[ yyleng - 1 ];
                BEGIN( quote == '"' ? X_STR : X_CHAR );
              }

              /*
               * Common code between X_CHAR and X_STR.  Multicharacter literals
               * are legal, but implementation-defined.
               */
<X_CHAR,X_STR>{

              /* Escaped characters are copied verbatim, not interpreted. */
  \\(.|\n)    { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  \n          {
                print_error( &str_lit_loc,
                  "unterminated %s literal\n",
                  YY_START == X_CHAR ? "character" : "string"
                );
                strbuf_reset( &str_lit_buf );
                return Y_LEXER_ERROR;
              }

} /* <X_CHAR,X_STR> */

<X_CHAR>{

  [^'\\\n]+   { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  \'          {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val = strbuf_take( &str_lit_buf );
                BEGIN( INITIAL );
                return Y_CHAR_LIT;
              }

} /* <X_CHAR> */

<X_STR>{      /* X_STR is the same as X_CHAR except " replaces '. */

  [^"\\\n]+   { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  \"          {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val = strbuf_take( &str_lit_buf );
                BEGIN( INITIAL );
                return Y_STR_LIT;
              }

} /* <X_STR> */

  /***************************************************************************/
  /*  COMMENTS                                                               */
  /***************************************************************************/

"/*"          {                         /* ignore C-style comments */
                for ( int c = input(), prev = '\0'; ; prev = c, c = input() ) {
                  if ( c == EOF ) {
                    print_error( &yylloc, "unterminated comment\n" );
                    return Y_LEXER_ERROR;
                  }
                  ++token_column;
                  if ( c == '/' && prev == '*' )
                    break;
                  if ( c == '\n' )
                    newline();
                } // for
              }

"//".*        ;                         /* ignore C++-style comments */
#.*           ;                         /* ignore preprocessor directives */

  /***************************************************************************/
  /*  WHITESPACE                                                             */
  /***************************************************************************/

{S}+          ;                         /* ignore all other whitespace */
\\\n          ;                         /* eat escaped newlines */
\n            { newline(); }            /* ignore newlines */

  /***************************************************************************/
  /*  NON-MATCH & EOF                                                        */
  /***************************************************************************/

.             {
                return Y_ERROR;
              }

<*><<EOF>>    {
                newline();
                BEGIN( INITIAL );
                //
                // The first time we encounter EOF, we want to treat it as if
                // it were a newline by returning Y_END so commands in the
                // parser always end in Y_END.
                //
                // Requesting more characters after encountering EOF will
                // simply continue to return EOF, so the second time we
                // encounter EOF, treat it as EOF by returning no token.
                //
                return (is_eof = !is_eof) ? Y_END : YY_NULL;
              }

%%

/// @endcond

// Re-enable warnings.
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

////////// local functions ////////////////////////////////////////////////////

// LCOV_EXCL_START
/**
 * Called by Flex only when there's a fatal error.
 *
 * @param msg The error message to print.
 */
_Noreturn
static void lexer_fatal( char const *msg ) {
  if ( msg == NULL ) {
    // Never true -- here just to silence the "unused function" warning.
    yy_fatal_error( msg );
  }

  //
  // This is defined down here to avoid having to declare yy_fatal_error
  // ourselves and having to get it right being subject to possible changes in
  // its signature in different Flex versions.
  //
  INTERNAL_ERROR( "%s\n", msg );
}

////////// extern functions ///////////////////////////////////////////////////

void lexer_init( void ) {
  ASSERT_RUN_ONCE();
  ATEXIT( &lexer_cleanup );
}

char const* lexer_input_line( size_t *rv_len ) {
  assert( rv_len != NULL );
  if ( rv_len != NULL )
    *rv_len = STATIC_CAST( size_t, input_len );
  return input_buf;
}

ad_loc_t lexer_loc( void ) {
  ad_loc_t rv_loc = yylloc;
  if ( yytext[0] == '\n' ) {
    //
    // If the current token is '\n', it means first_line & last_line are
    // already on the next line, but we want the line that the '\n' is on which
    // is the previous line.
    //
    --rv_loc.first_line;
    --rv_loc.last_line;
  }
  return rv_loc;
}

void lexer_reset( bool hard_reset ) {
  if ( hard_reset ) {
    is_eof = false;
    newline();
    yylineno = 0;
  }
  BEGIN( INITIAL );
  input_len = input_sent = 0;
  strbuf_reset( &str_lit_buf );
}

char const* printable_yytext( void ) {
  switch ( yytext[0] ) {
    case '\0':
    case '\n':
      return NULL;
    default:
      return yytext;
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
