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
#include "ad.h"                         /* must go first */
#include "in_attr.h"

// extern variables
in_attr_t in_attr;

/////////// extern functions //////////////////////////////////////////////////

void ia_cleanup( void ) {
  sname_cleanup( &in_attr.scope_sname );
  in_attr = (in_attr_t){ 0 };
}

sname_t ia_sname( char *name ) {
  sname_t sname = sname_dup( &in_attr.scope_sname );
  sname_push_back_name( &sname, name );
  return sname;
}

///////////////////////////////////////////////////////////////////////////////

/* vim:set et sw=2 ts=2: */
