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
#include "typedef.h"
#include "util.h"
#include "parser.h"                     /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>                     /* for strtol(3) */
#include <string.h>

// Silence these warnings for Flex-generated code.
#ifdef __clang__
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif /* __clang__ */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif /* __GNUC__ */

/// @endcond

/**
 * C++ raw string delimiter maximum length.
 */
#define RSTR_DELIM_LEN_MAX        16u

/**
 * Sets <code>\ref lexer_token</code> to the current Flex token.
 */
#define SET_TOKEN                 SET_TOKEN_TO( yytext )

/**
 * Sets <code>\ref lexer_token</code> to \a S.
 *
 * @param S The string to set the token to.
 */
#define SET_TOKEN_TO(S)           (lexer_token = (S))

/**
 * Overrides flex's fatal error message to print the message in our format and
 * also exit with the status code we want.
 *
 * @param MSG The error message to print.
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( (MSG) )

/**
 * Overrides flex's input.
 *
 * @param BUF The buffer to use.
 * @param BYTES_READ Set to the number of bytes read.
 * @param BYTES_MAX The maximum number of bytes to read.
 */
#define YY_INPUT(BUF,BYTES_READ,BYTES_MAX) \
  (BYTES_READ) = lexer_get_input( (BUF), STATIC_CAST( yy_size_t, (BYTES_MAX) ) )

/**
 * This code is inserted by flex at the beginning of each rule to set the
 * current token location information.
 */
#define YY_USER_ACTION            lexer_update_loc(); /* must include ';' */

///////////////////////////////////////////////////////////////////////////////

// extern variables
static char const lexer_token_init_buf; ///< So lexer_token is never null.

// A separate pointer for the current token allows it:
//  + To be `const`.
//  + To be assigned to either other `const` strings or string literals.
char const       *lexer_token = &lexer_token_init_buf;

// local variables
static char      *input_buf;            ///< Entire current input line.
static size_t     input_len;            ///< Length of line in `input_buf`.
static size_t     input_sent;           ///< How many bytes returned to flex.
static bool       is_eof;               ///< Encountered EOF?
static int        newline_column;       ///< Column position of newline.
static strbuf_t   str_lit_buf;          ///< String (or character) literal.
static ad_loc_t   str_lit_loc;          ///< String literal starting location.
static int        token_column;         ///< Column position of current token.

/// C++ raw string literal delimiter.
static char       rstr_delim[ RSTR_DELIM_LEN_MAX + 1/*"*/ + 1/*\0*/ ];

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
 * Called by flex only when there's a fatal error.
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

/**
 * Gets a line of input for flex and keeps a copy for use later if printing an
 * error message.
 *
 * @param buf A pointer to the buffer to write into.
 * @param buf_capacity The capacity of \a buf.
 * @return Returns the number of bytes read.
 */
static size_t lexer_get_input( char *buf, size_t buf_capacity ) {
  assert( buf != NULL );
  size_t input_avail = input_len - input_sent;
  if ( input_avail == 0 ) {
    static size_t input_capacity;
    ssize_t bytes_read = getline( &input_buf, &input_capacity, yyin );
    if ( unlikely( bytes_read == -1 ) ) {
      FERROR( yyin );
      bytes_read = 0;
    }
    input_avail = input_len = (size_t)bytes_read;
    input_sent = 0;
  }
  //
  // Given that the default buffer capacity (YY_READ_BUF_SIZE) for flex is
  // 8192, it's unlikely that this will ever be true and that we'll have to
  // return the input line in chunks; but might as well code for the case.
  //
  if ( unlikely( input_avail > buf_capacity ) )
    input_avail = buf_capacity;
  memcpy( buf, input_buf + input_sent, input_avail );
  input_sent += input_avail;
  return input_avail;
}

/**
 * Update the parser's location.
 * @note This is called by flex via `YY_USER_ACTION`.
 */
static void lexer_update_loc( void ) {
  yylloc.first_line   = yylloc.last_line = yylineno;
  yylloc.first_column = token_column;
  yylloc.last_column  = token_column + yyleng - 1;
  token_column += yyleng;
}

/**
 * Resets the column position upon encountering a newline.
 */
static inline void newline( void ) {
  newline_column = token_column;
  token_column = 0;
}

/**
 * Parses an integer from <code>\ref lexer_token</code>.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
NODISCARD
static inline int parse_int( int base ) {
  //
  // We don't need to check for errors from strtol(3) here since the lexer
  // guarantees that we'll be passed only a valid number token.
  //
  // Standard strtol(3) doesn't understand a "0b" (binary) prefix, so skip over
  // it if the base is 2.
  //
  return STATIC_CAST( int,
    strtol( lexer_token + (base == 2 ? 2 : 0), /*entptr=*/NULL, base )
  );
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
                SET_TOKEN;

                //
                // 1. See if it's an ad keyword.
                //
                ad_keyword_t const *const k = ad_keyword_find( lexer_token );
                if ( k != NULL ) {
                  yylval.tid = k->tid;
                  return k->yy_token_id;
                }

                //
                // 2. See if it's a typedef'd type.
                //
                ad_typedef_t const *const tdef =
                  ad_typedef_find_name( lexer_token );
                if ( tdef != NULL ) {
                  yylval.tdef = tdef;
                  return Y_TYPEDEF_TYPE;
                }

                //
                // 4. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

  /***************************************************************************/
  /*  NUMERIC LITERALS                                                       */
  /***************************************************************************/

{bin_int}     { SET_TOKEN; yylval.int_val = parse_int(  2 ); return Y_INT_LIT; }
{oct_int}     { SET_TOKEN; yylval.int_val = parse_int(  8 ); return Y_INT_LIT; }
{dec_int}     { SET_TOKEN; yylval.int_val = parse_int( 10 ); return Y_INT_LIT; }
{hex_int}     { SET_TOKEN; yylval.int_val = parse_int( 16 ); return Y_INT_LIT; }

  /***************************************************************************/
  /*  OPERATORS & PUNCTUATION                                                */
  /***************************************************************************/

              /* Multi-character operators. */
"::"          { SET_TOKEN; return Y_COLON_COLON           ; }
"!="          { SET_TOKEN; return Y_EXCLAM_EQUAL          ; }
"%="          { SET_TOKEN; return Y_PERCENT_EQUAL         ; }
"&&"          { SET_TOKEN; return Y_AMPER_AMPER           ; }
"&="          { SET_TOKEN; return Y_AMPER_EQUAL           ; }
"*="          { SET_TOKEN; return Y_STAR_EQUAL            ; }
"++"          { SET_TOKEN; return Y_PLUS_PLUS             ; }
"+="          { SET_TOKEN; return Y_PLUS_EQUAL            ; }
"--"          { SET_TOKEN; return Y_MINUS_MINUS           ; }
"-="          { SET_TOKEN; return Y_MINUS_EQUAL           ; }
"->"          { SET_TOKEN; return Y_ARROW                 ; }
"/="          { SET_TOKEN; return Y_SLASH_EQUAL           ; }
"<<"          { SET_TOKEN; return Y_LESS_LESS             ; }
"<<="         { SET_TOKEN; return Y_LESS_LESS_EQUAL       ; }
"<="          { SET_TOKEN; return Y_LESS_EQUAL            ; }
"=="          { SET_TOKEN; return Y_EQUAL_EQUAL           ; }
">="          { SET_TOKEN; return Y_GREATER_EQUAL         ; }
">>"          { SET_TOKEN; return Y_GREATER_GREATER       ; }
">>="         { SET_TOKEN; return Y_GREATER_GREATER_EQUAL ; }
"^="          { SET_TOKEN; return Y_CIRC_EQUAL            ; }
"|="          { SET_TOKEN; return Y_PIPE_EQUAL            ; }
"||"          { SET_TOKEN; return Y_PIPE_PIPE             ; }

              /* Single-character operators and miscellaneous punctuation. */
"!"           |
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
"<"           |
"="           |
">"           |
"?"           |
"["           |
"]"           |
"^"           |
"{"           |
"|"           |
"}"           |
"~"           { SET_TOKEN; return lexer_token[0]; }

  /***************************************************************************/
  /*  RAW STRING LITERALS                                                    */
  /***************************************************************************/

{rstr_pfx}    { // e.g.: u8R"abc(...)abc"
                // find:    ^   ^
                char const *q = strchr( yytext, '"' );
                assert( q != NULL );
                char const *const p = strchr( ++q, '(' );
                assert( p != NULL );

                size_t const delim_len = (size_t)(p - q);
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
                  if ( c != *d ) {
                    //
                    // Found a mismatch with the delimiter, e.g.:
                    //
                    //      )abc"       // delimiter
                    //      )abx"       // what was found
                    //
                    // Hence, it's not the actual delimiter, but part of the
                    // string literal: append the part that matched (")ab").
                    //
                    strbuf_printf( &str_lit_buf,
                      ")%.*s", (int)(d - rstr_delim), rstr_delim
                    );
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
                  }
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

\n            {
                SET_TOKEN;
                newline();
                BEGIN( INITIAL );
                return Y_END;
              }

  /***************************************************************************/
  /*  NON-MATCH & EOF                                                        */
  /***************************************************************************/

.             {
                SET_TOKEN;
                return Y_ERROR;
              }

<*><<EOF>>    {
                SET_TOKEN;
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

////////// extern functions ///////////////////////////////////////////////////

void lexer_init( void ) {
  ASSERT_RUN_ONCE();
  ATEXIT( &lexer_cleanup );
}

char const* lexer_input_line( size_t *plen ) {
  if ( plen != NULL )
    *plen = input_len;
  return input_buf;
}

ad_loc_t lexer_loc( void ) {
  ad_loc_t rv_loc;
  rv_loc.first_line = yylineno;

  if ( token_column == 0 )
    rv_loc.first_column = newline_column;
  else if ( lexer_token[0] == '\n' )
    rv_loc.first_column = token_column;
  else {
    //
    // This has to be the original yytext, not lexer_token, since lexer_token
    // could have been set to something else via SET_TOKEN_TO().
    //
    rv_loc.first_column =
      token_column - STATIC_CAST( ad_loc_num_t, strlen( yytext ) );
    assert( rv_loc.first_column >= 0 );
  }

  rv_loc.last_line = rv_loc.first_line;
  rv_loc.last_column = rv_loc.first_column;
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

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
