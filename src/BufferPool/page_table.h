#pragma once

#include "config.h"
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#define INVALID_PAGE_ID ((uint32_t)-1)

typedef struct HtNode {
  uint32_t PageId;
  uint32_t FrameId;
  struct HtNode* Next;

} HtNode;

typedef struct PageTable {
  HtNode* Buckets[HT_NUM_BUCKETS];
  mtx_t Stripes[HT_LOCK_STRIPES];
  mtx_t FreeListLock;
  HtNode NodePool[POOL_SIZE];
  HtNode* FreeList;

} PageTable;

/// @brief Init page table
void PtInit(PageTable* pt);

/// @brief Destroy page table
void PtDestroy(PageTable* pt);

/// @brief Pop a node off the free list
/// @return Popped node or NULL if the pool is exhausted
HtNode* PtAllocNode(PageTable* pt);

/// @brief Push a node back onto the free list
void PtFreeNode(PageTable* pt, HtNode* node);

/// @brief Look up a pageId
/// @return pageId's frameId or INVALID_FRAME_ID if not present
uint32_t PtLookup(PageTable* pt, uint32_t pageId);

/// @brief Insert or update the mapping pageId -> frameId
/// @return true on success, false if the node pool is exhausted
bool PtInsert(PageTable* pt, uint32_t pageId, uint32_t frameId);

/// @brief Remove the mapping for pageId
/// @return Returns true if an entry was removed, false if no such mapping
/// existed
bool PtDelete(PageTable* pt, uint32_t pageId);
