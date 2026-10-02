#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Required for memmove

typedef struct {
    size_t Capacity;
    size_t Size;
} DynArrayHeader;

#define M_DynArrayGetHeader(v) ((DynArrayHeader *)(v) - 1)
#define M_DynArrayGetSize(v)       ((v) ? M_DynArrayGetHeader(v)->size : 0)
#define M_DynArrayGetCapacity(v)   ((v) ? M_DynArrayGetHeader(v)->capacity : 0)

#define M_DynArrayInit(v) do { \
    DynArrayHeader *h = (DynArrayHeader *)malloc(sizeof(DynArrayHeader) + (4 * sizeof(*(v)))); \
    if (!h) { perror("Allocation failed"); exit(EXIT_FAILURE); } \
    h->capacity = 4; \
    h->size = 0; \
    (v) = (void *)(h + 1); \
} while(0)

#define M_DynArrayGrowIfNeeded(v) do { \
    DynArrayHeader *h = M_DynArrayGetHeader(v); \
    if (h->size >= h->capacity) { \
        h->capacity *= 2; \
        h = (DynArrayHeader *)realloc(h, sizeof(DynArrayHeader) + (h->capacity * sizeof(*(v)))); \
        if (!h) { perror("Reallocation failed"); exit(EXIT_FAILURE); } \
        (v) = (void *)(h + 1); \
    } \
} while(0)


#define M_DynArrayPush(v, val) do { \
    M_DynArrayGrowIfNeeded(v); \
    DynArrayHeader *h = M_DynArrayGetHeader(v); \
    (v)[h->size++] = (val); \
} while(0)

/**
 * Inserts an element at a specific index. 
 * Shifts elements at and after the index to the right.
 */
#define M_DynArrayInsert(v, idx, val) do { \
    DynArrayHeader *h = M_DynArrayGetHeader(v); \
    size_t target_idx = (size_t)(idx); \
    if (target_idx <= h->size) { \
        M_DynArrayGrowIfNeeded(v); \
        h = M_DynArrayGetHeader(v); /* Refresh header pointer in case realloc moved it */ \
        if (target_idx < h->size) { \
            memmove(&(v)[target_idx + 1], &(v)[target_idx], (h->size - target_idx) * sizeof(*(v))); \
        } \
        (v)[target_idx] = (val); \
        h->size++; \
    } \
} while(0)

/**
 * Removes an element at a specific index. 
 * Shifts all subsequent elements to the left.
 */
#define M_DynArrayRemove(v, idx) do { \
    DynArrayHeader *h = M_DynArrayGetHeader(v); \
    size_t target_idx = (size_t)(idx); \
    if (target_idx < h->size) { \
        if (target_idx < h->size - 1) { \
            memmove(&(v)[target_idx], &(v)[target_idx + 1], (h->size - target_idx - 1) * sizeof(*(v))); \
        } \
        h->size--; \
    } \
} while(0)

#define M_DynArrayFree(v) do { \
    if (v) { \
        free(M_DynArrayGetHeader(v)); \
        (v) = NULL; \
    } \
} while(0)