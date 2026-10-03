#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#define HT_NUM_BUCKETS 2048
#define HT_LOCK_STRIPES 64
#define PAGE_SIZE 4096
#define POOL_SIZE 256
#define INVALID_FRAME_ID ((uint32_t)-1)
#define INVALID_PAGE_ID ((uint32_t)-1)

typedef struct HtNode {
  uint32_t pageId;
  uint32_t frameId;
  struct HtNode* next;

} HtNode;

typedef struct PageTable {
  HtNode* buckets[HT_NUM_BUCKETS];
  mtx_t stripes[HT_LOCK_STRIPES];
  mtx_t freeListLock;
  HtNode nodePool[POOL_SIZE];
  HtNode* freeList;

} PageTable;

/// @brief Init page table
void ptInit(PageTable* pt);

/// @brief Destroy page table
void ptDestroy(PageTable* pt);

/// @brief Pop a node off the free list
/// @return Popped node or NULL if the pool is exhausted
HtNode* ptAllocNode(PageTable* pt);

/// @brief Push a node back onto the free list
void ptFreeNode(PageTable* pt, HtNode* node);

/// @brief Look up a pageId
/// @return pageId's frameId or INVALID_FRAME_ID if not present
uint32_t ptLookup(PageTable* pt, uint32_t pageId);

/// @brief Insert or update the mapping pageId -> frameId
/// @return true on success, false if the node pool is exhausted
bool ptInsert(PageTable* pt, uint32_t pageId, uint32_t frameId);

/// @brief Remove the mapping for pageId
/// @return Returns true if an entry was removed, false if no such mapping
/// existed
bool ptDelete(PageTable* pt, uint32_t pageId);
