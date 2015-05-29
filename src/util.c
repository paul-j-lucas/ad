/*
**      ad -- ASCII dump
**      util.c
**
**      Copyright (C) 2013-2014  Paul J. Lucas
**
**      This program is free software; you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation; either version 2 of the Licence, or
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

/* local */
#include "util.h"

/* system */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>                     /* for strtoul() */
#include <string.h>

extern char const *me;

/*****************************************************************************/

char const* base_name( char const *path_name ) {
  assert( path_name );
  char const *const slash = strrchr( path_name, '/' );
  if ( slash )
    return slash[1] ? slash + 1 : slash;
  return path_name;
}

unsigned long check_strtoul( char const *s ) {
  assert( s );
  char *end = NULL;
  errno = 0;
  unsigned long const n = strtoul( s, &end, 0 );
  if ( end == s || *end || errno == ERANGE )
    PMESSAGE_EXIT( USAGE, "\"%s\": invalid integer\n", s );
  return n;
}

/*****************************************************************************/

/* vim:set et sw=2 ts=2: */
