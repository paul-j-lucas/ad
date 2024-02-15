/*
**      ad -- ASCII dump
**      src/lexer.h
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

#ifndef ad_lexer_H
#define ad_lexer_H

/**
 * @file
 * Declares global variables and functions interacting with the lexical
 * analyzer.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern char const  *lexer_token;        ///< Text of current token.

/**
 * Gets the current input line.
 *
 * @param plen If not null, sets the value pointed at to be the length of said
 * line.
 * @return Returns said line.
 */
NODISCARD
char const* lexer_input_line( size_t *plen );

/**
 * Gets the lexer's current location.
 *
 * @return Returns said location.
 */
NODISCARD
ad_loc_t lexer_loc( void );

/**
 * Resets the lexer to its initial state.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag also.
 */
void lexer_reset( bool hard_reset );

/**
 * Flex: gets the next token ID.
 *
 * @return Returns the token ID.
 *
 * @note The definition is provided by Flex.
 */
NODISCARD
int yylex( void );

#ifdef __GNUC__
# pragma GCC diagnostic push
  // Declare yyrestart() so it can be called from anywhere.  However, Flex
  // declares yyrestart() in the generated .c file before it #includes headers,
  // so we'd get a redundant declaration warning -- so suppress that.
# pragma GCC diagnostic ignored "-Wredundant-decls"
#endif /* __GNUC__ */

/**
 * Flex: immediately switch to reading \a file.
 *
 * @param in_file The `FILE` to read from.
 *
 * @note The definition is provided by Flex.
 */
void yyrestart( FILE *in_file );

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif /* __GNUC__ */

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_lexer_H */
/* vim:set et sw=2 ts=2: */
