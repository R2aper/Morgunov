#include "buffer_pool.h"
#include <string.h>
#include <threads.h>

static void _DiskRead(uint32_t pageId, uint8_t* buffer) {
  // TODO:
  memset(buffer, (char)(pageId % 256), PAGE_SIZE);
}

static void _DiskWrite(uint32_t pageId, uint8_t* buffer) {
  // TODO:
}

// Page replacement algorithm - Clock
static uint32_t _FindVictim(BufferPool* bp) {
  for (;;) {
    uint32_t idx = bp->clockHand;
    bp->clockHand = (bp->clockHand + 1) % POOL_SIZE;

    Frame* f = &bp->frames[idx];

    // We can't replace page that in use
    if (f->pinCount > 0)
      continue;

    // If the access bit is set, we clear it and give it a "second chance."
    if (f->refBit) {
      f->refBit = false;
    } else {
      // if not - this is our victim
      return idx;
    }
  }
}

void bpInit(BufferPool* bp) {
  for (int i = 0; i < POOL_SIZE; i++) {
    bp->frames[i].pageId = INVALID_PAGE_ID;
    bp->frames[i].pinCount = 0;
    bp->frames[i].isDirty = false;
    bp->frames[i].refBit = false;
    mtx_init(&bp->frames[i].lock, mtx_plain);
  }
  ptInit(&bp->page_table);
  mtx_init(&bp->poolLock, mtx_plain);
  bp->clockHand = 0;
}

void bpDestroy(BufferPool* bp) {
  // Before destroy write all 'dirty' pages
  for (int i = 0; i < POOL_SIZE; i++) {
    if (bp->frames[i].pageId != INVALID_PAGE_ID && bp->frames[i].isDirty)
      _DiskWrite(bp->frames[i].pageId, bp->memory[i]);
    mtx_destroy(&bp->frames[i].lock);
  }

  ptDestroy(&bp->page_table);
  mtx_destroy(&bp->poolLock);
}

uint8_t* bpFetchPage(BufferPool* bp, uint32_t pageId) {
  mtx_lock(&bp->poolLock);

  // Check if page already in pool
  uint32_t frame_id = ptLookup(&bp->page_table, pageId);
  if (frame_id != INVALID_FRAME_ID) {
    Frame* f = &bp->frames[frame_id];

    mtx_lock(&f->lock);

    mtx_unlock(&bp->poolLock);

    f->pinCount++;
    f->refBit = true;

    mtx_unlock(&f->lock);
    return bp->memory[frame_id];
  }

  // if not, find victim
  uint32_t victim_id = INVALID_FRAME_ID;

  // Search for empty page
  for (uint32_t i = 0; i < POOL_SIZE; i++) {
    if (bp->frames[i].pageId == INVALID_PAGE_ID) {
      victim_id = i;
      break;
    }
  }

  // If there is no empty page, use clock algorithm
  if (victim_id == INVALID_FRAME_ID) {
    // NOTE: poolLock is still held to protect from data races(Should be
    // rewritten if necessary)
    victim_id = _FindVictim(bp);
    Frame* victim = &bp->frames[victim_id];
    mtx_lock(&victim->lock);

    if (victim->isDirty)
      _DiskWrite(victim->pageId, bp->memory[victim_id]);

    ptDelete(&bp->page_table, victim->pageId);

    _DiskRead(pageId, bp->memory[victim_id]);

    victim->pageId = pageId;
    victim->pinCount = 1;
    victim->isDirty = false;
    victim->refBit = true;

    mtx_unlock(&victim->lock);
  } else {
    Frame* f = &bp->frames[victim_id];
    mtx_lock(&f->lock);

    _DiskRead(pageId, bp->memory[victim_id]);

    f->pageId = pageId;
    f->pinCount = 1;
    f->isDirty = false;
    f->refBit = true;

    mtx_unlock(&f->lock);
  }

  ptInsert(&bp->page_table, pageId, victim_id);
  mtx_unlock(&bp->poolLock);

  return bp->memory[victim_id];
}

void bpUnpinPage(BufferPool* bp, uint32_t pageId, bool isDirty) {
  mtx_lock(&bp->poolLock);
  uint32_t frame_id = ptLookup(&bp->page_table, pageId);

  if (frame_id != INVALID_FRAME_ID) {
    Frame* f = &bp->frames[frame_id];

    mtx_lock(&f->lock);
    mtx_unlock(&bp->poolLock);

    if (f->pinCount > 0) {
      f->pinCount--;
    }
    if (isDirty) {
      f->isDirty = true;
    }

    mtx_unlock(&f->lock);
  } else {
    mtx_unlock(&bp->poolLock);
  }
}
