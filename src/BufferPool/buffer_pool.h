#include "page_table.h"
#include <string.h>

typedef struct Frame {
  uint32_t page_id;
  uint32_t pin_count; // How many threads are using page
  bool is_dirty;      // Was it modifed?
  bool ref_bit;       // For Clock algorithm
  mtx_t lock;

} Frame;

typedef struct BufferPool {
  uint8_t memory[POOL_SIZE][PAGE_SIZE];
  Frame frames[POOL_SIZE];
  PageTable page_table;
  mtx_t pool_lock;
  uint32_t clock_hand; // For Clock algorithm

} BufferPool;

/// @brief Init Buffer Pool
void bpInit(BufferPool *bp);

/// @brief Destroy Buffer Pool
void bpDestroy(BufferPool *bp);

/// @brief Fetch page from pool
/// @return pointer to page
uint8_t *bpFetchPage(BufferPool *bp, uint32_t page_id);

/// @brief Free page
void bpUnpinPage(BufferPool *bp, uint32_t page_id, bool is_dirty);
