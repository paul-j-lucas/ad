/*
**      ad -- ASCII dump
**      src/print.c
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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
 * Defines functions for printing error and warning messages.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "print.h"
#include "color.h"
#include "common.h"
#include "keyword.h"
#include "lexer.h"
#include "options.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

/// @cond DOXYGEN_IGNORE

// local constants
static char const *const  MORE[]     = { "...", "..." };
static size_t const       MORE_LEN[] = { 3,     3     };
static unsigned const     TERM_COLUMNS_DEFAULT = 80;

/// @endcond

// local functions
static void               print_input_line( size_t*, size_t );

NODISCARD
static size_t             token_len( char const*, size_t, size_t );

// extern variables
print_params_t            print_params;

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for print_suggestions() and fprint_list() that gets the
 * string for a \ref did_you_mean token.
 *
 * @param ppelt A pointer to the pointer to the \ref did_you_mean element to
 * get the string of.  On return, it is advanced to the next element.
 * @return Returns a pointer to the next "Did you mean" suggestion string or
 * NULL if none.
 */
NODISCARD
static char const* fprint_list_dym_gets( void const **ppelt ) {
  did_you_mean_t const *const dym = *ppelt;
  if ( dym->token == NULL )
    return NULL;
  *ppelt = dym + 1;

  static strbuf_t sbufs[ 2 ];
  static unsigned buf_index;

  strbuf_t *const sbuf = &sbufs[ buf_index++ % ARRAY_SIZE( sbufs ) ];
  strbuf_reset( sbuf );
  strbuf_printf( sbuf, "\"%s\"", dym->token );
  return sbuf->str;
}

/**
 * Gets the current input line.
 *
 * @param input_line_len A pointer to receive the length of the input line.
 * @return Returns the input line.
 */
NODISCARD
static char const* get_input_line( size_t *input_line_len ) {
  char const *input_line = lexer_input_line( input_line_len );
  assert( input_line != NULL );

  //
  // Chop off whitespace (if any) so we can always print a newline ourselves.
  //
  str_rtrim_len( input_line, input_line_len );

  return input_line;
}

/**
 * Prints the error line (if not interactive) and a `^` (in color, if possible
 * and requested) under the offending token.
 *
 * @param error_column The zero-based column of the offending token.
 * @return Returns \a error_column, adjusted (if necessary).
 */
NODISCARD
static size_t print_caret( size_t error_column ) {
  unsigned term_columns;
#ifdef ENABLE_TERM_SIZE
  get_term_columns_lines( &term_columns, /*plines=*/NULL );
  if ( term_columns == 0 )
#endif /* ENABLE_TERM_SIZE */
    term_columns = TERM_COLUMNS_DEFAULT;

  size_t caret_column;

  if ( opt_interactive ) {
    //
    // If we're interactive, we can put the ^ under the already existing token
    // the user typed for the recent command, but we have to add the length of
    // the prompt.
    //
    caret_column = error_column
      + strlen( OPT_LANG_IS( C_ANY ) ? CDECL : CPPDECL )
      + 2 /* "> " */;
    if ( term_columns > 0 )
      caret_column %= term_columns;
  }
  else {
    //
    // Otherwise we have to print out the line containing the error then put
    // the ^ under that.
    //
    print_input_line( &error_column, term_columns );
    caret_column = error_column;
  }

  EPRINTF( "%*s", STATIC_CAST( int, caret_column ), "" );
  SGR_START_COLOR( stderr, caret );
  EPUTC( '^' );
  SGR_END_COLOR( stderr );
  EPUTC( '\n' );

  return error_column;
}

/**
 * Prints the input line, "scrolled" to the left with `...` printed if
 * necessary, so that \a error_column is always within \a term_columns.
 *
 * @param error_column A pointer to the zero-based column of the offending
 * token.  It is adjusted if necessary to be the terminal column at which the
 * `^` should be printed.
 * @param term_columns The number of columns of the terminal.
 */
static void print_input_line( size_t *error_column, size_t term_columns ) {
  size_t input_line_len;
  char const *input_line = get_input_line( &input_line_len );
  assert( input_line != NULL );
  assert( input_line_len > 0 );

  if ( *error_column > input_line_len )
    *error_column = input_line_len;

  --term_columns;                     // more aesthetically pleasing

  //
  // If the error is due to unexpected end of input, back up the error
  // column so it refers to a non-null character.
  //
  if ( *error_column > 0 && input_line[ *error_column ] == '\0' )
    --*error_column;

  size_t const token_columns =
    token_len( input_line, input_line_len, *error_column );
  size_t const error_end_column = *error_column + token_columns - 1;

  //
  // Start with the number of printable columns equal to the length of the
  // line.
  //
  size_t print_columns = input_line_len;

  //
  // If the number of printable columns exceeds the number of terminal
  // columns, there is "more" on the right, so limit the number of printable
  // columns.
  //
  bool more[2];                       // [0] = left; [1] = right
  more[1] = print_columns > term_columns;
  if ( more[1] )
    print_columns = term_columns;

  //
  // If the error end column is past the number of printable columns, there
  // is "more" on the left since we will "scroll" the line to the left.
  //
  more[0] = error_end_column > print_columns;

  //
  // However, if there is "more" on the right but the end of the error token
  // is at the end of the line, then we can print through the end of the line
  // without any "more."
  //
  if ( more[1] ) {
    if ( error_end_column < input_line_len - 1 )
      print_columns -= MORE_LEN[1];
    else
      more[1] = false;
  }

  if ( more[0] ) {
    //
    // There is "more" on the left so we have to adjust the error column, the
    // number of printable columns, and the offset into the input line that we
    // start printing at to give the appearance that the input line has been
    // "scrolled" to the left.
    //
    assert( print_columns >= token_columns );
    size_t const error_column_term = print_columns - token_columns;
    print_columns -= MORE_LEN[0];
    assert( *error_column > error_column_term );
    input_line += MORE_LEN[0] + (*error_column - error_column_term);
    *error_column = error_column_term;
  }

  EPRINTF( "%s%.*s%s\n",
    (more[0] ? MORE[0] : ""),
    STATIC_CAST( int, print_columns ), input_line,
    (more[1] ? MORE[1] : "")
  );
}

/**
 * Gets the length of the first token in \a s.  Characters are divided into
 * three classes:
 *
 *  + Whitespace.
 *  + Alpha-numeric.
 *  + Everything else (e.g., punctuation).
 *
 * A token is composed of characters in exclusively one class.  The class is
 * determined by `s[0]`.  The length of the token is the number of consecutive
 * characters of the same class starting at `s[0]`.
 *
 * @param s The string to use.
 * @param s_len The length of \a s.
 * @param token_offset The offset of the start of the token.
 * @return Returns the length of the token.
 */
NODISCARD
static size_t token_len( char const *s, size_t s_len, size_t token_offset ) {
  assert( s != NULL );

  char const *const end = s + s_len;
  s += token_offset;

  if ( s >= end )
    return 0;

  bool const is_s0_alnum = isalnum( s[0] );
  bool const is_s0_space = isspace( s[0] );

  char const *const s0 = s;
  while ( ++s < end && *s != '\0' ) {
    if ( is_s0_alnum ) {
      if ( !isalnum( *s ) )
        break;
    }
    else if ( is_s0_space ) {
      if ( !isspace( *s ) )
        break;
    }
    else {
      if ( isalnum( *s ) || isspace( *s ) )
        break;
    }
  } // while
  return STATIC_CAST( size_t, s - s0 );
}

////////// extern functions ///////////////////////////////////////////////////

void fl_print_error( char const *file, int line, ad_loc_t const *loc,
                     char const *format, ... ) {
  assert( file != NULL );
  assert( format != NULL );

  if ( loc != NULL ) {
    print_loc( loc );
    SGR_START_COLOR( stderr, error );
    EPUTS( "error" );
    SGR_END_COLOR( stderr );
    EPUTS( ": " );
  }

  print_debug_file_line( file, line );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
}

void fl_print_error_unknown_name( char const *file, int line,
                                  ad_loc_t const *loc, char const *name ) {
  assert( name != NULL );

  dym_kind_t dym_kind = DYM_NONE;

  ad_keyword_t const *const k = ad_keyword_find( name );
  if ( k != NULL ) {
    char const *what = NULL;

    switch ( c_tid_tpid( k->tid ) ) {
      case C_TPID_NONE:                 // e.g., "break"
      case C_TPID_STORE:                // e.g., "extern"
        dym_kind = DYM_C_KEYWORDS;
        what = "keyword";
        break;
      case C_TPID_BASE:                 // e.g., "char"
        dym_kind = DYM_C_TYPES;
        what = "type";
        break;
      case C_TPID_ATTR:
        dym_kind = DYM_C_ATTRIBUTES;    // e.g., "noreturn"
        what = "attribute";
        break;
    } // switch

    fl_print_error( file, line, loc,
      "\"%s\": unsupported %s%s", name, what, c_lang_which( k->lang_ids )
    );
  }

  print_suggestions( dym_kind, name );
  EPUTC( '\n' );
  FREE( name );
}

void fl_print_warning( char const *file, int line, ad_loc_t const *loc,
                       char const *format, ... ) {
  assert( file != NULL );
  assert( format != NULL );

  if ( loc != NULL )
    print_loc( loc );
  SGR_START_COLOR( stderr, warning );
  EPUTS( "warning" );
  SGR_END_COLOR( stderr );
  EPUTS( ": " );

  print_debug_file_line( file, line );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
}

void print_debug_file_line( char const *file, int line ) {
#ifdef ENABLE_AD_DEBUG
  assert( file != NULL );
  assert( line > 0 );
  if ( opt_ad_debug )
    EPRINTF( "[%s:%d] ", file, line );
#else
  (void)file;
  (void)line;
#endif /* ENABLE_AD_DEBUG */
}

void print_hint( char const *format, ... ) {
  assert( format != NULL );
  EPUTS( "; did you mean " );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  EPUTS( "?\n" );
}

void print_loc( ad_loc_t const *loc ) {
  assert( loc != NULL );
  size_t const column = print_caret( STATIC_CAST( size_t, loc->first_column ) );
  SGR_START_COLOR( stderr, locus );
  if ( print_params.conf_path != NULL )
    EPRINTF( "%s:%d,", print_params.conf_path, loc->first_line + 1 );
  EPRINTF( "%zu", column + 1 );
  SGR_END_COLOR( stderr );
  EPUTS( ": " );
}

bool print_suggestions( dym_kind_t kinds, char const *unknown_token ) {
  did_you_mean_t const *const dym = dym_new( kinds, unknown_token );
  if ( dym != NULL ) {
    EPUTS( "; did you mean " );
    fprint_list( stderr, dym, &fprint_list_dym_gets );
    EPUTC( '?' );
    dym_free( dym );
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */