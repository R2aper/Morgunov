/**
 * @file dynarray.h 
 * @brief defines DynArray (dynamic, length-prefixed array)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Value returned by the find macros when no element matches
 */
#define M_DYNARRAY_NOT_FOUND ((int)(SIZE_MAX))

/**
 * @struct
 * @brief Represents a length-prefixed dynamic array's header
 * 
 * This structure defines a simple span of user data, dynamically managed
 * by the controlling macros. The structure itself is prefixed with a header
 * which the struct itself represents. Using pointer arithmetic, you may access
 * dynarray's items, which are located right after this header.
 */
typedef struct {
  size_t Capacity;
  size_t Size;
} DynArrayHeader;

/**
 * @brief Returns a header of a dynarray.
 * @param v DynArray pointer
 * @returns @ref DynArrayHeader of @p v
 */
#define M_DynArrayGetHeader(v) ((DynArrayHeader*)(v) - 1)

/**
 * @brief Returns size of dynarray
 * @param v DynArray pointer
 * @returns Size of @p v
 */
#define M_DynArrayGetSize(v) ((v) ? M_DynArrayGetHeader(v)->Size : 0)

/**
 * @brief Returns capacity of dynarray
 * @param v DynArray pointer
 * @returns Capcity of @p v
 */
#define M_DynArrayGetCapacity(v) ((v) ? M_DynArrayGetHeader(v)->Capacity : 0)

/**
 * @brief Creates a new DynArray
 * 
 * @param v User type pointer
 */
#define M_DynArrayInit(v)                                                                          \
  do {                                                                                             \
    DynArrayHeader* h = (DynArrayHeader*)malloc(sizeof(DynArrayHeader) + (4 * sizeof(*(v))));      \
    if (!h) {                                                                                      \
      perror("Allocation failed");                                                                 \
      exit(EXIT_FAILURE);                                                                          \
    }                                                                                              \
    h->Capacity = 4;                                                                               \
    h->Size = 0;                                                                                   \
    (v) = (void*)(h + 1);                                                                          \
  } while (0)

/**
 * @brief Reallocates a dynarray if size exceeds capacity
 * @attention Not intended for use outside @ref dynarray.h
 */
#define M_DynArrayGrowIfNeeded(v)                                                                  \
  do {                                                                                             \
    DynArrayHeader* h = M_DynArrayGetHeader(v);                                                    \
    if (h->Size >= h->Capacity) {                                                                  \
      h->Capacity *= 2;                                                                            \
      h = (DynArrayHeader*)realloc(h, sizeof(DynArrayHeader) + (h->Capacity * sizeof(*(v))));      \
      if (!h) {                                                                                    \
        perror("Reallocation failed");                                                             \
        exit(EXIT_FAILURE);                                                                        \
      }                                                                                            \
      (v) = (void*)(h + 1);                                                                        \
    }                                                                                              \
  } while (0)

/**
 * @brief Pushes new value into provided dynarray
 * @param v dynarray
 * @param val value to be pushed
 */
#define M_DynArrayPush(v, val)                                                                     \
  do {                                                                                             \
    M_DynArrayGrowIfNeeded(v);                                                                     \
    DynArrayHeader* h = M_DynArrayGetHeader(v);                                                    \
    (v)[h->Size++] = (val);                                                                        \
  } while (0)

/**
 * @brief Inserts an element at a specific index, shifts elements at and after the index to the right.
 * @param v dynarray
 * @param idx index of insertion
 * @param val value to be inserted
 */
#define M_DynArrayInsert(v, idx, val)                                                              \
  do {                                                                                             \
    DynArrayHeader* h = M_DynArrayGetHeader(v);                                                    \
    size_t target_idx = (size_t)(idx);                                                             \
    if (target_idx <= h->Size) {                                                                   \
      M_DynArrayGrowIfNeeded(v);                                                                   \
      h = M_DynArrayGetHeader(v); /* Refresh header pointer in case realloc moved it */            \
      if (target_idx < h->Size) {                                                                  \
        memmove(&(v)[target_idx + 1], &(v)[target_idx], (h->Size - target_idx) * sizeof(*(v)));    \
      }                                                                                            \
      (v)[target_idx] = (val);                                                                     \
      h->Size++;                                                                                   \
    }                                                                                              \
  } while (0)

/**
 * @brief Removes an element at a specific index, shifts all subsequent elements to the left.
 * @param v dynarray
 * @param idx index of removal
 */
#define M_DynArrayRemove(v, idx)                                                                   \
  do {                                                                                             \
    DynArrayHeader* h = M_DynArrayGetHeader(v);                                                    \
    size_t target_idx = (size_t)(idx);                                                             \
    if (target_idx < h->Size) {                                                                    \
      if (target_idx < h->Size - 1) {                                                              \
        memmove(&(v)[target_idx], &(v)[target_idx + 1],                                            \
                (h->Size - target_idx - 1) * sizeof(*(v)));                                        \
      }                                                                                            \
      h->Size--;                                                                                   \
    }                                                                                              \
  } while (0)

/**
 * @brief Frees the memory of dynarray
 * @param v dynarray to be freed
 * @note invalidates @p v
 */
#define M_DynArrayFree(v)                                                                          \
  do {                                                                                             \
    if (v) {                                                                                       \
      free(M_DynArrayGetHeader(v));                                                                \
      (v) = NULL;                                                                                  \
    }                                                                                              \
  } while (0)

/**
 * @brief Finds the first element equal to a value (compared with ==)
 * @param v dynarray
 * @param val value to look for
 * @param out_idx size_t lvalue that receives the index of the first match,
 *                or @ref M_DYNARRAY_NOT_FOUND
 * @note Works for types comparable with ==. For strings or structs use
 *       @ref M_DynArrayFindBy. Safe on a NULL dynarray. @p val is evaluated
 *       once per element, so avoid side effects.
 */
#define M_DynArrayFind(v, val, out_idx)                                                          \
do {                                                                                             \
  size_t n_ = M_DynArrayGetSize(v);                                                              \
  (out_idx) = M_DYNARRAY_NOT_FOUND;                                                              \
  for (size_t i_ = 0; i_ < n_; i_++) {                                                           \
    if ((v)[i_] == (val)) {                                                                      \
      (out_idx) = (int)i_;                                                                       \
      break;                                                                                     \
    }                                                                                            \
  }                                                                                              \
} while (0)

/**
* @brief Finds the first element for which a predicate returns nonzero
* @param v dynarray
* @param out_idx size_t lvalue that receives the index, or @ref M_DYNARRAY_NOT_FOUND
* @param pred function of the form `int pred(const T* elem)`
*/
#define M_DynArrayFindBy(v, out_idx, pred)                                                       \
do {                                                                                             \
  size_t n_ = M_DynArrayGetSize(v);                                                              \
  (out_idx) = M_DYNARRAY_NOT_FOUND;                                                              \
  for (size_t i_ = 0; i_ < n_; i_++) {                                                           \
    if (pred(&(v)[i_])) {                                                                        \
      (out_idx) = i_;                                                                            \
      break;                                                                                     \
    }                                                                                            \
  }                                                                                              \
} while (0)
