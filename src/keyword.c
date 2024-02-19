/*
**      ad -- ASCII dump
**      src/keyword.c
**
**      Copyright (C) 2021  Paul J. Lucas
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
 * Defines functions for looking up ad keyword information.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "keyword.h"
#include "literals.h"
#include "parser.h"                     /* must go last */

// standard
#include <assert.h>
#include <stdio.h>                      /* for NULL */

///////////////////////////////////////////////////////////////////////////////

/**
 * All ad keywords.
 */
static ad_keyword_t const AD_KEYWORDS[] = {
  { L_alignas,  Y_alignas,  T_NONE    },
  { L_bool,     Y_bool,     T_BOOL    },
  { L_break,    Y_break,    T_NONE    },
  { L_case,     Y_case,     T_NONE    },
  { L_default,  Y_default,  T_NONE    },
  { L_enum,     Y_enum,     T_NONE    },
  { L_false,    Y_false,    T_NONE    },
  { L_float,    Y_float,    T_FLOAT   },
  { L_int,      Y_int,      T_INT     },
  { L_offsetof, Y_offsetof, T_NONE    },
  { L_sizeof,   Y_sizeof,   T_NONE    },
  { L_struct,   Y_struct,   T_STRUCT  },
  { L_switch,   Y_switch,   T_NONE    },
  { L_true,     Y_true,     T_NONE    },
  { L_typedef,  Y_typedef,  T_NONE    },
  { L_uint,     Y_uint,     T_INT     },
  { NULL,       0,          T_NONE    }
};

////////// extern functions ///////////////////////////////////////////////////

ad_keyword_t const* ad_keyword_find( char const *s ) {
  assert( s != NULL );
  for ( ad_keyword_t const *k = AD_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( s, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

ad_keyword_t const* ad_keyword_next( ad_keyword_t const *k ) {
  return k == NULL ? AD_KEYWORDS : (++k)->literal == NULL ? NULL : k;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
