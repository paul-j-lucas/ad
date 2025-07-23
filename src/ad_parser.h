/*
**      ad -- ASCII dump
**      src/ad_parser.h
**
**      Copyright (C) 2024  Paul J. Lucas
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

#ifndef ad_parser_H
#define ad_parser_H

/**
 * @file
 * Wrapper around the Bison-generated `parser.h` to add necessary `#include`s
 * for the types in Bison's <code>\%union</code> declaration as well as a
 * declaration for the parser_init() function.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "ad.h"
#include "slist.h"
#include "types.h"
#include "parser.h"                     /* must go last */

/**
 * @ingroup parser-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the parser.
 *
 * @note This function must be called exactly once.
 */
void parser_init( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* ad_parser_H */
/* vim:set et sw=2 ts=2: */
