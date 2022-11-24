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
  { L_ALIGNAS,  Y_ALIGNAS,  T_NONE    },
  { L_BOOL,     Y_BOOL,     T_BOOL    },
  { L_BREAK,    Y_BREAK,    T_NONE    },
  { L_CASE,     Y_CASE,     T_NONE    },
  { L_DEFAULT,  Y_DEFAULT,  T_NONE    },
  { L_ENUM,     Y_ENUM,     T_NONE    },
  { L_FALSE,    Y_FALSE,    T_NONE    },
  { L_FLOAT,    Y_FLOAT,    T_FLOAT   },
  { L_INT,      Y_INT,      T_INT     },
  { L_OFFSETOF, Y_OFFSETOF, T_NONE    },
  { L_SIZEOF,   Y_SIZEOF,   T_NONE    },
  { L_STRUCT,   Y_STRUCT,   T_STRUCT  },
  { L_SWITCH,   Y_SWITCH,   T_SWITCH  },
  { L_TRUE,     Y_TRUE,     T_NONE    },
  { L_TYPEDEF,  Y_TYPEDEF,  T_NONE    },
  { L_UINT,     Y_UINT,     T_INT     },
  { NULL,       0           }
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
