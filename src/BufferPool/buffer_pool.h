#pragma once

#include "page_table.h"

typedef struct Frame {
  uint32_t pageId;
  uint32_t pinCount; // How many threads are using page
  bool isDirty;      // Was it modifed?
  bool refBit;       // For Clock algorithm
  mtx_t lock;

} Frame;

typedef struct BufferPool {
  uint8_t memory[POOL_SIZE][PAGE_SIZE];
  Frame frames[POOL_SIZE];
  PageTable page_table;
  mtx_t poolLock;
  uint32_t clockHand; // For Clock algorithm

} BufferPool;

/// @brief Init Buffer Pool
void bpInit(BufferPool* bp);

/// @brief Destroy Buffer Pool
void bpDestroy(BufferPool* bp);

/// @brief Fetch page from pool
/// @return pointer to page
uint8_t* bpFetchPage(BufferPool* bp, uint32_t pageId);

/// @brief Free page
void bpUnpinPage(BufferPool* bp, uint32_t pageId, bool isDirty);
