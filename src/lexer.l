/*
**      ad -- ASCII dump
**      src/lexer.l
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

/**
 * @file
 * Defines helper macros, data structures, variables, functions, and the
 * tokenizer for C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%option nounput
%option yylineno

%{
/** @endcond */

// local
#include "ad.h"                         /* must go first */
#include "lexer.h"
#include "keyword.h"
#include "literals.h"
#include "options.h"
#include "util.h"
#include "parser.h"                     /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>                     /* for strtol(3) */
#include <string.h>

#ifdef __GNUC__
// Silence these warnings for Flex-generated code.
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif /* __GNUC__ */

#ifdef __clang__
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif /* __clang__ */

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
 * Sets <code>\ref lexer_token</code> to \a S and returns \a T.
 *
 * @param S The string to set the token to.
 * @param T The token to return.
 */
#define SET_TOKEN_TO_AND_RETURN(S,T) \
  BLOCK( SET_TOKEN_TO(S); return (T); )

/**
 * Sets <code>\ref lexer_token</code> to \a S and returns \a S[0].
 *
 * @param S The string to set the token to.
 */
#define SET_TOKEN_TO_AND_RETURN_IT(S) \
  SET_TOKEN_TO_AND_RETURN( (S), lexer_token[0] )

/**
 * Advances \a S over all \a CHARS.
 *
 * @param S The string pointer to advance.
 * @param CHARS A string containing the characters to skip over.
 * @return Returns the updated \a S.
 * @hideinitializer
 */
#define SKIP_CHARS(S,CHARS)       ((S) += strspn( (S), (CHARS) ))

/**
 * Advances \a S over all whitespace.
 *
 * @param S The string pointer to advance.
 * @return Returns the updated \a S.
 * @hideinitializer
 */
#define SKIP_WS(S)                SKIP_CHARS( (S), " \f\r\t\v" )

/**
 * Overrides flex's fatal error message to print the message in our format and
 * also exit with the status code we want.
 *
 * @param MSG The error message to print.
 * @hideinitializer
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( (MSG) )

/**
 * Overrides flex's input.
 *
 * @param BUF The buffer to use.
 * @param BYTES_READ Set to the number of bytes read.
 * @param BYTES_MAX The maximum number of bytes to read.
 * @hideinitializer
 */
#define YY_INPUT(BUF,BYTES_READ,BYTES_MAX) \
  (BYTES_READ) = lexer_get_input( (BUF), (BYTES_MAX) )

/**
 * This code is inserted by flex at the beginning of each rule to set the
 * current token location information.
 *
 * @hideinitializer
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
static int        token_column;         ///< Column position of current token.

// local functions
noreturn
static void       lexer_fatal( char const* );

////////// local functions ////////////////////////////////////////////////////

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
 * Resets the column position upon encountering a newline.
 */
static inline void newline( void ) {
  newline_column = token_column;
  token_column = 0;
}

/**
 * Parses a C/C++ identifier.
 *
 * @param s The string to parse.
 * @param end Upon successful return, set to point to one character past the
 * end of the identifier.
 * @return Returns `true` only if an identifier was parsed.
 */
static bool parse_identifier( char const *s, char const **end ) {
  assert( s != NULL );
  assert( end != NULL );
  if ( !(isalpha( s[0] ) || s[0] == '_') )
    return false;
  while ( *++s != '\0' && (isalnum( *s ) || *s == '_') )
    ;
  *end = s;
  return true;
}

/**
 * Parses an integer from <code>\ref lexer_token</code>.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
static inline int parse_int( int base ) {
  //
  // We don't need to check for errors from strtol(3) here since the lexer
  // guarantees that we'll be passed only a valid number token.
  //
  // Standard strtol(3) doesn't understand a "0b" (binary) prefix, so skip over
  // it if the base is 2.
  //
  return (int)strtol( lexer_token + (base == 2 ? 2 : 0), NULL, base );
}

/**
 * Parses a scoped name, e.g. `a::b::c`.
 *
 * @param s The string to parse.
 * @return Returns the scoped name.
 */
static c_sname_t parse_sname( char const *s ) {
  assert( s != NULL );
  c_sname_t rv;
  c_sname_init( &rv );

  for ( char const *end; parse_identifier( s, &end ); ) {
    c_sname_append_name( &rv, strndup( s, end - s ) );
    SKIP_WS( end );
    if ( strncmp( end, "::", 2 ) != 0 )
      break;
    s = end + 2;
    SKIP_WS( s );
  } // for

  return rv;
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
int_sfx       [lL][lL]?[uU]?|[uU]([lL][lL]?|[zZ])?|[zZ][uU]?

bin_int       0[bB]{B}+{int_sfx}?
oct_int       0{O}*{int_sfx}?
dec_int       [1-9]{D}*{int_sfx}?
hex_int       0[xX]{H}+{int_sfx}?

%%

{identifier}  {
                SET_TOKEN;

                //
                // 1. See if it's an ad keyword.
                //
                keyword_t const *const ak = ad_keyword_find( lexer_token );
                if ( ak != NULL ) {
                  if ( ak->c_keyword_synonym == NULL )
                    return ak->yy_token_id;
                  lexer_token = ak->c_keyword_synonym;
                }

                //
                // 2. See if it's a typedef'd type.
                //
                SLIST_TEMP_INIT_VAR( typedef_name, lexer_token );
                c_typedef_t const *const t = c_typedef_find( &typedef_name );
                if ( t != NULL ) {
                  yylval.c_typedef = t;
                  return Y_TYPEDEF_TYPE;
                }

                //
                // 3. See if it's a C/C++ keyword.
                //
        find_keyword:
                NO_OP;
                keyword_t const *k = keyword_find( lexer_token, opt_lang );
                if ( k != NULL ) {
                  yylval.tid = k->tid;
                  return k->yy_token_id;
                }

                //
                // 4. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

{bin_int}     { SET_TOKEN; yylval.number = parse_int(  2 ); return Y_NUMBER; }
{oct_int}     { SET_TOKEN; yylval.number = parse_int(  8 ); return Y_NUMBER; }
{dec_int}     { SET_TOKEN; yylval.number = parse_int( 10 ); return Y_NUMBER; }
{hex_int}     { SET_TOKEN; yylval.number = parse_int( 16 ); return Y_NUMBER; }

"::"          { SET_TOKEN; return Y_COLON2          ; }
"!="          { SET_TOKEN; return Y_EXCLAM_EQ       ; }
"%="          { SET_TOKEN; return Y_PERCENT_EQ      ; }
"&&"          { SET_TOKEN; return Y_AMPER2          ; }
"&="          { SET_TOKEN; return Y_AMPER_EQ        ; }
"*="          { SET_TOKEN; return Y_STAR_EQ         ; }
"++"          { SET_TOKEN; return Y_PLUS2           ; }
"+="          { SET_TOKEN; return Y_PLUS_EQ         ; }
"--"          { SET_TOKEN; return Y_MINUS2          ; }
"-="          { SET_TOKEN; return Y_MINUS_EQ        ; }
"->"          { SET_TOKEN; return Y_ARROW           ; }
"->*"         { SET_TOKEN; return Y_ARROW_STAR      ; }
".*"          { SET_TOKEN; return Y_DOT_STAR        ; }
"/="          { SET_TOKEN; return Y_SLASH_EQ        ; }
"<<"          { SET_TOKEN; return Y_LESS2           ; }
"<<="         { SET_TOKEN; return Y_LESS2_EQ        ; }
"<="          { SET_TOKEN; return Y_LESS_EQ         ; }
"<=>"         { SET_TOKEN; return Y_LESS_EQ_GREATER ; }
"=="          { SET_TOKEN; return Y_EQ2             ; }
">="          { SET_TOKEN; return Y_GREATER_EQ      ; }
">>"          { SET_TOKEN; return Y_GREATER2        ; }
">>="         { SET_TOKEN; return Y_GREATER2_EQ     ; }
"?"           { SET_TOKEN; return Y_HELP            ; }
"?:"          { SET_TOKEN; return Y_QMARK_COLON     ; }
"^="          { SET_TOKEN; return Y_CIRC_EQ         ; }
"|="          { SET_TOKEN; return Y_PIPE_EQ         ; }
"||"          { SET_TOKEN; return Y_PIPE2           ; }

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
";"           |
"<"           |
"="           |
">"           |
"["           |
"]"           |
"^"           |
"{"           |
"|"           |
"}"           |
"~"           { SET_TOKEN; return lexer_token[0]; }

"/*"          {                         /* ignore C-style comments */
                for ( int c = input(), prev = '\0'; ; prev = c, c = input() ) {
                  if ( c == '\0' || c == EOF )
                    return Y_LEXER_ERROR;
                  ++token_column;
                  if ( c == '/' && prev == '*' )
                    break;
                  if ( c == '\n' )
                    newline();
                } // for
              }

"//".*        ;                         /* ignore C++-style comments */
#.*           ;                         /* ignore preprocessor directives */
{S}+          ;                         /* ignore all other whitespace */
\\\n          ;                         /* eat escaped newlines */

\n            {
                SET_TOKEN;
                newline();
                return Y_END;
              }

<<EOF>>       {
                SET_TOKEN;
                newline();
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

.             {
                SET_TOKEN;
                return Y_ERROR;
              }

%%

/// @endcond

////////// local functions ////////////////////////////////////////////////////

/**
 * Called by flex only when there's a fatal error.
 *
 * @param msg The error message to print.
 */
static void lexer_fatal( char const *msg ) {
  //
  // This is defined down here to avoid having to declare yy_fatal_error
  // ourselves and having to get it right being subject to possible changes in
  // its signature in different flex versions.
  //
  INTERNAL_ERR( "%s\n", msg );

  // Not reached -- "called" just to silence the "unused function" warning.
  yy_fatal_error( msg );
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

////////// extern functions ///////////////////////////////////////////////////

AD_WARN_UNUSED_RESULT
char const* lexer_input_line( size_t *plen ) {
  if ( plen != NULL )
    *plen = input_len;
  return input_buf;
}

c_loc_t lexer_loc( void ) {
  ad_loc_t rv_loc;
  rv_loc.first_line = yylineno;

  if ( token_column == 0 )
    rv_loc.first_column = newline_column;
  else if ( lexer_token[0] == '\n' )
    rv_loc.first_column = token_column;
  else
    rv_loc.first_column = token_column - (int)strlen( lexer_token );

  rv_loc.last_line = rv_loc.first_line;
  rv_loc.last_column = rv_loc.first_column;
  return rv_loc;
}

void lexer_reset( bool hard_reset ) {
  if ( hard_reset ) {
    is_eof = false;
    yylineno = 0;
  }
  input_len = input_sent = 0;
  newline();
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
