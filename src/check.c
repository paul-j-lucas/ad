/*
**      ad -- ASCII dump
**      src/check.c
**
**      Copyright (C) 2024  Paul J. Lucas, et al.
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
 * Defines functions for checking statements for semantic errors.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"
#include "slist.h"

// standard
#include <assert.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

NODISCARD
static bool ad_stmnt_check( ad_stmnt_t const *statement ) {
  assert( statement != NULL );

  // TODO

  return true;
}

////////// extern functions ///////////////////////////////////////////////////

bool ad_stmnt_list_check( slist_t const *statement_list ) {
  assert( statement_list != NULL );

  FOREACH_SLIST_NODE( statement_node, statement_list ) {
    if ( !ad_stmnt_check( statement_node->data ) )
      return false;
  } // for

  return true;
}

bool ad_type_check( ad_type_t const *type ) {
  assert( type != NULL );

  // TODO

  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

