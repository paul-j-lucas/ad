/*
**      ad -- ASCII dump
**      src/in_attr.c
**
**      Copyright (C) 2019-2024  Paul J. Lucas, et al.
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
 * Defines functions and a global variable for inherited attributes for the
 * format parser.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "in_attr.h"

// standard
#include <assert.h>
#include <stdlib.h>

slist_t in_attr_list;

/////////// extern functions //////////////////////////////////////////////////

void ia_free( in_attr_t *ia ) {
  if ( ia != NULL ) {
    sname_cleanup( &ia->scope_sname );
    free( ia );
  }
}

sname_t ia_sname( in_attr_t const *ia, char *name ) {
  assert( ia != NULL );
  sname_t sname = sname_dup( &ia->scope_sname );
  sname_push_back_name( &sname, name );
  return sname;
}

///////////////////////////////////////////////////////////////////////////////

/* vim:set et sw=2 ts=2: */
