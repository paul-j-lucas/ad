/*
**      ad -- ASCII dump
**      src/sname.h
**
**      Copyright (C) 2019-2024  Paul J. Lucas
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

#ifndef ad_sname_H
#define ad_sname_H

/**
 * @file
 * Declares functions for dealing with "sname" (scoped name) objects, e.g.,
 * `S::T::x`.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "slist.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

_GL_INLINE_HEADER_BEGIN
#ifndef SNAME_H_INLINE
# define SNAME_H_INLINE _GL_INLINE
#endif /* SNAME_H_INLINE */

/// @endcond

/**
 * @defgroup sname-group Scoped Names
 * Functions for dealing with "sname" (scoped name) objects, e.g., `S::T::x`.
 *
 * + The "local" of an sname is the innermost scope, e.g., `x`.  A non-empty
 *   sname always has a local.
 *
 * + The "scope" of an sname is all but the innermost scope, e.g., `S::T`.  A
 *   non-empty sname may or may not have a scope.
 * @{
 */

/**
 * Creates a scoped name literal with a local name of \a NAME.
 *
 * @param NAME The local name.
 * @return Returns said scoped name.
 *
 * @warning c_sname_cleanup() must _not_ be called on the returned value.
 */
#define SNAME_LIT(NAME) \
  SLIST_LIT( (&(sname_scope_t){ CONST_CAST( char*, (NAME) ) }) )

/**
 * Gets the data associated with \a SCOPE.
 *
 * @param SCOPE The scope to get the data of.
 *
 * @note This is a macro instead of an inline function so it'll work with
 * either a `const` or non-`const` \a SCOPE.
 */
#define sname_scope(SCOPE)        POINTER_CAST( sname_scope_t*, (SCOPE)->data )

/**
 * Convenience macro for iterating over all scopes of an sname.
 *
 * @param VAR The \ref slist_node loop variable.
 * @param SNAME The \ref sname_t to iterate over the scopes of.
 *
 * @sa #FOREACH_SNAME_SCOPE_UNTIL()
 */
#define FOREACH_SNAME_SCOPE(VAR,SNAME) \
  FOREACH_SLIST_NODE( VAR, (SNAME) )

/**
 * Convenience macro for iterating over all scopes of an sname up to but not
 * including \a END.
 *
 * @param VAR The \ref slist_node loop variable.
 * @param SNAME The \ref sname_t to iterate over the scopes of.
 * @param END The scope to end before; may be NULL.
 *
 * @sa #FOREACH_SNAME_SCOPE()
 */
#define FOREACH_SNAME_SCOPE_UNTIL(VAR,SNAME,END) \
  FOREACH_SLIST_NODE_UNTIL( VAR, (SNAME), (END) )

///////////////////////////////////////////////////////////////////////////////

typedef struct slist        sname_t;
typedef struct sname_scope  sname_scope_t;

/**
 * Data for each scope of an \ref sname_t.
 */
struct sname_scope {
  char const *name;                     ///< The scope's name.
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Compares two \ref sname_scope.
 *
 * @param i_data The first \ref sname_scope to compare.
 * @param j_data The second \ref sname_scope to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
NODISCARD
int sname_scope_cmp( sname_scope_t const *i_data,
                     sname_scope_t const *j_data );

/**
 * Duplicates \a data.
 *
 * @param data The \ref sname_scope to duplicate; may be NULL.
 * @return Returns a duplicate of \a data or NULL only if \a data is NULL.  The
 * caller is responsible for calling sname_scope_free() on it.
 *
 * @sa sname_scope_free()
 */
NODISCARD
sname_scope_t* sname_scope_dup( sname_scope_t const *data );

/**
 * Frees all memory associated with \a data _including_ \a data itself.
 *
 * @param data The \ref sname_scope to free.  If NULL, does nothing.
 */
void sname_scope_free( sname_scope_t *data );

/**
 * Frees all memory associated with \a sname _including_ \a sname itself.
 *
 * @param sname The scoped name to free.  If NULL, does nothing.
 *
 * @sa sname_cleanup()
 * @sa sname_init()
 * @sa sname_init_name()
 * @sa sname_list_cleanup()
 */
void sname_free( sname_t *sname );

/**
 * Gets the fully scoped name of \a sname.
 *
 * @param sname The scoped name to get the full name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty or NULL.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa sname_english()
 * @sa sname_local_name()
 * @sa sname_name_atr()
 * @sa sname_scope_name()
 */
NODISCARD
char const* sname_full_name( sname_t const *sname );

/**
 * Cleans-up all memory associated with \a list but does _not_ free \a list
 * itself.
 *
 * @param list The list of scoped names to clean up.  If NULL, does nothing;
 * otherwise, reinitializes \a list upon completion.
 *
 * @sa sname_cleanup()
 * @sa sname_free()
 */
void sname_list_cleanup( slist_t *list );

/**
 * Gets the local name of \a sname (which is the name of the last scope), for
 * example the local name of `S::T::x` is `x`.
 *
 * @param sname The scoped name to get the local name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty or NULL.
 *
 * @sa sname_full_name()
 * @sa sname_name_atr()
 * @sa sname_scope_name()
 */
NODISCARD
char const* sname_local_name( sname_t const *sname );

/**
 * Parses a scoped name, for example `a::b::c`.
 *
 * @param s The string to parse.
 * @param rv_sname The scoped name to parse into.
 * @return Returns the number of characters of \a s that were successfully
 * parsed.
 */
NODISCARD
size_t sname_parse( char const *s, sname_t *rv_sname );

/**
 * Pops the last name from \a sname.
 *
 * @param sname The scoped name to pop the last name from.
 */
void sname_pop_back( sname_t *sname );

/**
 * Appends \a name onto the end of \a sname.
 *
 * @param sname The scoped name to append to.
 * @param name The name to append.  Ownership is taken.
 *
 * @sa sname_push_back_sname()
 * @sa sname_push_front_sname()
 * @sa sname_set()
 */
void sname_push_back_name( sname_t *sname, char *name );

/**
 * Gets just the scope name of \a sname.
 * Examples:
 *  + For `a::b::c`, returns `a::b`.
 *  + For `c`, returns the empty string.
 *
 * @param sname The scoped name to get the scope name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty, NULL, or
 * not within a scope.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa sname_full_name()
 * @sa sname_local_name()
 * @sa sname_name_atr()
 */
NODISCARD
char const* sname_scope_name( sname_t const *sname );

/**
 * Sets \a dst_sname to \a src_sname.
 *
 * @param dst_sname The scoped name to set.
 * @param src_sname The scoped name to set \a dst_sname to. Ownership is taken.
 *
 * @note If \a dst_sname `==` \a src_name, does nothing.
 *
 * @sa sname_move()
 * @sa sname_push_back_name()
 * @sa sname_push_back_sname()
 * @sa sname_push_front_sname()
 */
void sname_set( sname_t *dst_sname, sname_t *src_sname );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Cleans-up all memory associated with \a sname but does _not_ free \a sname
 * itself.
 *
 * @param sname The scoped name to clean up.  If NULL, does nothing; otherwise,
 * reinitializes it upon completion.
 *
 * @sa sname_init()
 * @sa sname_init_name()
 * @sa sname_list_cleanup()
 */
void sname_cleanup( sname_t *sname );

/**
 * Compares two scoped names.
 *
 * @param i_sname The first scoped name to compare.
 * @param j_sname The second scoped name to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_sname is
 * less than, equal to, or greater than \a j_sname, respectively.
 */
NODISCARD SNAME_H_INLINE
int sname_cmp( sname_t const *i_sname, sname_t const *j_sname ) {
  return slist_cmp(
    i_sname, j_sname, POINTER_CAST( slist_cmp_fn_t, &sname_scope_cmp )
  );
}

/**
 * Gets the number of names of \a sname, e.g., `S::T::x` is 3.
 *
 * @param sname The scoped name to get the number of names of.
 * @return Returns said number of names.
 *
 * @note This is named "count" rather than "len" to avoid misinterpretation
 * that "len" would be the total length of the strings and `::` separators.
 */
NODISCARD SNAME_H_INLINE
size_t sname_count( sname_t const *sname ) {
  return slist_len( sname );
}

/**
 * Duplicates \a sname.  The caller is responsible for calling
 * sname_cleanup() on the duplicate.
 *
 * @param sname The scoped name to duplicate; may be NULL.
 * @return Returns a duplicate of \a sname or an empty scoped name if \a sname
 * is NULL.
 */
NODISCARD SNAME_H_INLINE
sname_t sname_dup( sname_t const *sname ) {
  return slist_dup(
    sname, -1, POINTER_CAST( slist_dup_fn_t, &sname_scope_dup )
  );
}

/**
 * Gets whether \a sname is empty.
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if \a sname is empty.
 */
NODISCARD SNAME_H_INLINE
bool sname_empty( sname_t const *sname ) {
  return slist_empty( sname );
}

/**
 * Initializes \a sname.
 *
 * @param sname The scoped name to initialize.
 *
 * @note This need not be called for either global or `static` scoped names.
 *
 * @sa sname_cleanup()
 * @sa sname_free()
 * @sa sname_init_name()
 * @sa sname_move()
 */
SNAME_H_INLINE
void sname_init( sname_t *sname ) {
  slist_init( sname );
}

/**
 * Initializes \a sname with \a name.
 *
 * @param sname The scoped name to initialize.
 * @param name The name to set to.  Ownership is taken.
 *
 * @sa sname_cleanup()
 * @sa sname_free()
 * @sa sname_init()
 */
SNAME_H_INLINE
void sname_init_name( sname_t *sname, char *name ) {
  slist_init( sname );
  sname_push_back_name( sname, name );
}

/**
 * Reinitializes \a sname and returns its former value so that it can be
 * "moved" into another scoped name via assignment.  For example:
 *
 * ```c
 * sname_t new_sname = sname_move( old_sname );
 * ```
 *
 * @remarks In many cases, a simple assignment would be fine; however, if
 * there's code that modifies `old_sname` afterwards, it would interfere with
 * `new_sname` since both point to the same underlying data.
 *
 * @param sname The scoped name to move.
 * @return Returns the former value of \a sname.
 *
 * @warning The recipient scoped name _must_ be either uninitialized or empty.
 *
 * @sa sname_init()
 * @sa sname_set()
 */
NODISCARD SNAME_H_INLINE
sname_t sname_move( sname_t *sname ) {
  return slist_move( sname );
}

/**
 * Gets the name at \a roffset of \a sname.
 *
 * @param sname The scoped name to get the name at \a roffset of.
 * @param roffset The reverse offset (starting at 0) of the name to get.
 * @return Returns the name at \a roffset or the empty string if \a roffset
 * &ge; sname_count().
 *
 * @sa sname_full_name()
 * @sa sname_scope_name()
 */
NODISCARD SNAME_H_INLINE
char const* sname_name_atr( sname_t const *sname, size_t roffset ) {
  sname_scope_t const *const data = slist_atr( sname, roffset );
  return data != NULL ? data->name : "";
}

/**
 * Appends \a src onto the end of \a dst.
 *
 * @param dst The scoped name to append to.
 * @param src The scoped name to append.  Ownership is taken.
 *
 * @sa sname_push_back_name()
 * @sa sname_push_front_sname()
 * @sa sname_set()
 */
SNAME_H_INLINE
void sname_push_back_sname( sname_t *dst, sname_t *src ) {
  slist_push_list_back( dst, src );
}

/**
 * Prepends \a src onto the beginning of \a dst.
 *
 * @param dst The scoped name to prepend to.
 * @param src The name to prepend.  Ownership is taken.
 *
 * @sa sname_push_back_name()
 * @sa sname_push_back_sname()
 */
SNAME_H_INLINE
void sname_push_front_sname( sname_t *dst, sname_t *src ) {
  slist_push_list_front( dst, src );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* ad_c_sname_H */
/* vim:set et sw=2 ts=2: */
