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
static keyword_t const AD_KEYWORDS[] = {
  { L_BOOL,     Y_BOOL      },
  { L_BREAK,    Y_BREAK     },
  { L_CASE,     Y_CASE      },
  { L_DEFAULT,  Y_DEFAULT   },
  { L_ENUM,     Y_ENUM      },
  { L_FALSE,    Y_FALSE     },
  { L_FLOAT,    Y_FLOAT     },
  { L_INT,      Y_INT       },
  { L_OFFSETOF, Y_OFFSETOF  },
  { L_SIZEOF,   Y_SIZEOF    },
  { L_STRUCT,   Y_STRUCT    },
  { L_SWITCH,   Y_SWITCH    },
  { L_TRUE,     Y_TRUE      },
  { L_TYPEDEF,  Y_TYPEDEF   },
  { L_UINT,     Y_UINT      },
  { NULL,       0           }
};

////////// extern functions ///////////////////////////////////////////////////

keyword_t const* ad_keyword_find( char const *s ) {
  assert( s != NULL );
  for ( keyword_t const *k = AD_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( s, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_keyword_H */
/* vim:set et sw=2 ts=2: */
