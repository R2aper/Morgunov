#pragma once

#include "page_table.h"

typedef struct Frame {
  uint32_t PageId;
  uint32_t PinCount; // How many threads are using page
  bool IsDirty;      // Was it modifed?
  bool RefBit;       // For Clock algorithm
  mtx_t Lock;

} Frame;

typedef struct BufferPool {
  uint8_t Memory[POOL_SIZE][PAGE_SIZE];
  Frame Frames[POOL_SIZE];
  PageTable PageTable;
  mtx_t PoolLock;
  uint32_t ClockHand; // For Clock algorithm

} BufferPool;

/// @brief Init Buffer Pool
/// @note bp should be freed
void BpInit(BufferPool* bp);

/// @brief Destroy Buffer Pool
void BpDestroy(BufferPool* bp);

/// @brief Fetch page from pool
/// @return pointer to page
uint8_t* BpFetchPage(BufferPool* bp, uint32_t pageId);

/// @brief Free page
void BpUnpinPage(BufferPool* bp, uint32_t pageId, bool isDirty);
