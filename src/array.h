/*
**      PJL Library
**      src/array.h
**
**      Copyright (C) 2025  Paul J. Lucas
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

#ifndef pjl_array_H
#define pjl_array_H

// local
#include "pjl_config.h"                 /* must go first */
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>                     /* for ssize_t */

_GL_INLINE_HEADER_BEGIN
#ifndef ARRAY_H_INLINE
# define ARRAY_H_INLINE _GL_INLINE
#endif /* ARRAY_H_INLINE */

/// @endcond

/**
 * Creates a `const` \ref array literal with \a ELEMENT as its only element.
 *
 * @param ELEMENT The element.
 * @return Returns said \ref array.
 *
 * @warning array_cleanup() must _not_ be called on the returned value.
 */
#define ARRAY_LIT(ELEMENT) \
  (array_t const){ &(ELEMENT), sizeof( (ELEMENT) ), 1, 1 }

///////////////////////////////////////////////////////////////////////////////

/**
 * The signature for a function passed to array_cmp() used to compare data
 * associated with element(if necessary).
 *
 * @param i_data A pointer to the first element's data to compare.
 * @param j_data A pointer to the second element's data to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
typedef int (*array_cmp_fn_t)( void const *i_data, void const *j_data );

/**
 * The signature for a function passed to array_dup() used to duplicate data
 * associated with each element (if necessary).
 *
 * @param data A pointer to the element's data to duplicate.
 * @return Returns a duplicate of \a data.
 */
typedef void* (*array_dup_fn_t)( void const *data );

/**
 * The signature for a function passed to array_cleanup() used to free data
 * associated with each element (if necessary).
 *
 * @param data A pointer to the element's data to free.
 */
typedef void (*array_free_fn_t)( void *data );

///////////////////////////////////////////////////////////////////////////////

/**
 * An array.
 */
struct array {
  void   *elements;                     ///< Pointer to array of elements.
  size_t  esize;                        ///< Size of an element.
  size_t  len;                          ///< Length of array.
  size_t  cap;                          ///< Capacity of array.
};
typedef struct array array_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans-up all memory associated with \a array but _not_ \a array itself.
 *
 * @param array A pointer to the array to clean up.  If NULL, does nothing;
 * otherwise, reinitializes \a array upon completion.
 * @param free_fn A pointer to a function to use to free the data at each node
 * of \a array or NULL if none is required.
 *
 * @sa array_init()
 */
void array_cleanup( array_t *array, array_free_fn_t free_fn );

/**
 * Compares two arrays.
 *
 * @param i_array The first array.
 * @param j_array The second array.
 * @param cmp_fn A pointer to a function to use to compare data at each element
 * of \a i_array and \a j_array or NULL if none is required (hence the data
 * will be compared directly as signed integers).
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_array is
 * less than, equal to, or greater than \a j_array, respectively.
 */
NODISCARD
int array_cmp( array_t const *i_array, array_t const *j_array,
               array_cmp_fn_t cmp_fn );

/**
 * Duplicates \a src_array and all of its elements.
 *
 * @param src_array The \ref array to duplicate; may ne NULL.
 * @param n The number of elements to duplicate; -1 is equivalent to
 * array_len().
 * @param dup_fn A pointer to a function to use to duplicate the data at each
 * element of \a src_array or NULL if none is required (hence a shallow copy
 * will be done).
 * @return Returns a duplicate of \a src_array or an empty array only if \a
 * src_array is NULL.  The caller is responsible for calling array_cleanup() on
 * it.
 *
 * @sa array_cleanup()
 */
NODISCARD
array_t array_dup( array_t const *array, ssize_t n, array_dup_fn_t dup_fn );

/**
 * Appends \a data onto the back of \a array.
 *
 * @param array The \ref array to push onto.
 * @param data The data to pushed.
 *
 * @note This is an O(1) operation.
 */
NODISCARD
void* array_push_back( array_t *array );

/**
 * Ensures at least \a res_len additional elements of capacity exist in \a
 * array.
 *
 * @param array A pointer to the \ref array to reserve \a res_len additional
 * element for.
 * @param res_len The number of additional elements to reserve.
 * @return Returns `true` only if a memory reallocation was necessary.
 */
PJL_DISCARD
bool array_reserve( array_t *array, size_t res_len );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Peeks at the element's data at \a offset of \a array.
 *
 * @param array A pointer to the \ref array.
 * @param offset The offset (starting at 0) of the element's data to get.
 * @return Returns the element's data at \a offset.
 *
 * @warning \a offset is _not_ checked to ensure it's &lt; the array's length.
 * A value &ge; the array's length results in undefined behavior.
 *
 * @note This function isn't normally called directly; use either array_at() or
 * array_atr() instead.
 * @note This is an O(1) operation.
 *
 * @sa array_at()
 * @sa array_atr()
 * @sa array_back()
 * @sa array_front()
 */
NODISCARD ARRAY_H_INLINE
void* array_at_nocheck( array_t const *array, size_t index ) {
  return POINTER_CAST( char*, array->elements ) + index * array->esize;
}

/**
 * Peeks at the element's data at \a offset of \a array.
 *
 * @param array A pointer to the \ref array.
 * @param offset The offset (starting at 0) of the element's data to get.
 * @return Returns the element's data at \a offset or NULL if \a offset &ge;
 * array_len().
 *
 * @note This is an O(1) operation.
 *
 * @sa array_at_nocheck()
 * @sa array_atr()
 * @sa array_back()
 * @sa array_front()
 */
NODISCARD ARRAY_H_INLINE
void* array_at( array_t const *array, size_t index ) {
  return index < array->len ? array_at_nocheck( array, index ) : NULL;
}

/**
 * Peeks at the element's data at \a rindex from the back of \a array.
 *
 * @param array A pointer to the \ref array.
 * @param rindex The reverse offset (starting at 0) of the element's data to
 * get.
 * @return Returns the element's data at \a rindex or NULL if \a rindex &ge;
 * array_len().
 *
 * @note This is an O(1) operation.
 *
 * @sa array_at_nocheck()
 * @sa array_at()
 * @sa array_back()
 * @sa array_front()
 */
NODISCARD ARRAY_H_INLINE
void* array_atr( array_t const *array, size_t rindex ) {
  return rindex < array->len ?
    array_at_nocheck( array, array->len - (rindex + 1) ) : NULL;
}

/**
 * Peeks at the element's data at the back of \a array.
 *
 * @param array A pointer to the \ref array.
 * @return Returns the elements's data at the back of \a array or NULL if \a
 * array is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa array_at_nocheck()
 * @sa array_at()
 * @sa array_atr()
 * @sa array_front()
 */
NODISCARD ARRAY_H_INLINE
void* array_back( array_t const *array ) {
  return array->len > 0 ? array_at_nocheck( array, array->len - 1 ) : NULL;
}

/**
 * Gets whether \a array is empty.
 *
 * @param array A pointer to the \ref array to check.
 * @return Returns `true` only if \a array is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa array_len()
 */
NODISCARD ARRAY_H_INLINE
bool array_empty( array_t const *array ) {
  return array->len == 0;
}

/**
 * Peeks at the element's data at the front of \a array.
 *
 * @param array A pointer to the \ref array.
 * @return Returns the elements's data at the front of \a array or NULL if \a
 * array is empty.
 *
 * @note This is an O(1) operation.
 *
 * @sa array_at_nocheck()
 * @sa array_at()
 * @sa array_atr()
 * @sa array_back()
 */
NODISCARD ARRAY_H_INLINE
void* array_front( array_t const *array ) {
  return array->len > 0 ? array->elements : NULL;
}

/**
 * Initializes \a array.
 *
 * @param array A pointer to the \ref array to initialize.
 *
 * @sa array_cleanup()
 * @sa array_move()
 */
ARRAY_H_INLINE
void array_init( array_t *array, size_t esize ) {
  *array = (array_t){ .esize = esize };
}

/**
 * Gets the length of \a array.
 *
 * @param array A pointer to the \ref array to get the length of.
 * @return Returns said length.
 *
 * @note This is an O(1) operation.
 *
 * @sa array_empty()
 */
NODISCARD ARRAY_H_INLINE
size_t array_len( array_t const *array ) {
  return array->len;
}

/**
 * Reinitializes \a array and returns its former value so that it can be
 * "moved" into another array via assignment.  For example:
 * ```
 *  array_t new_array = array_move( &old_array );
 * ```
 * is equivalent to:
 * ```
 *  array_t new_array = old_array;
 *  array_init( &old_array );
 * ```
 *
 * @remarks In many cases, a simple assignment would be fine; however, if
 * there's code that modifies `old_array` afterwards, it would interfere with
 * `new_array` since both point to the same underlying elements.
 *
 * @param array The \ref array to move.  May be NULL.
 * @return Returns the former value of \a array or an empty array if \a array
 * is NULL.
 *
 * @sa array_init()
 */
NODISCARD ARRAY_H_INLINE
array_t array_move( array_t *array ) {
  array_t const rv_array = *array;
  array_init( array, array->esize );
  return rv_array;
}

/**
 * Pops data from the back of \a array.
 *
 * @param array The pointer to the \ref array.
 * @return Returns the element's data from the back of \a array.  The caller is
 * responsible for freeing it if necessary.
 *
 * @note This is an O(1) operation.
 */
NODISCARD ARRAY_H_INLINE
void* array_pop_back( array_t *array ) {
  return array->len > 0 ? array_at_nocheck( array, --array->len ) : NULL;
}

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* pjl_array_H */
/* vim:set et sw=2 ts=2: */
