#include "buffer_pool.h"
#include <threads.h>

static void _DiskRead(uint32_t page_id, uint8_t *buffer) {
  // TODO:
  memset(buffer, (char)(page_id % 256), PAGE_SIZE);
}

static void _DiskWrite(uint32_t page_id, uint8_t *buffer) {
  // TODO:
}

// Page replacement algorithm - Clock
static uint32_t _FindVictim(BufferPool *bp) {
  for (;;) {
    uint32_t idx = bp->clock_hand;
    bp->clock_hand = (bp->clock_hand + 1) % POOL_SIZE;

    Frame *f = &bp->frames[idx];

    // We can't replace page that in use
    if (f->pin_count > 0)
      continue;

    // If the access bit is set, we clear it and give it a "second chance."
    if (f->ref_bit) {
      f->ref_bit = false;
    } else {
      // if not - this is our victim
      return idx;
    }
  }
}

void bpInit(BufferPool *bp) {
  for (int i = 0; i < POOL_SIZE; i++) {
    bp->frames[i].page_id = INVALID_PAGE_ID;
    bp->frames[i].pin_count = 0;
    bp->frames[i].is_dirty = false;
    bp->frames[i].ref_bit = false;
    mtx_init(&bp->frames[i].lock, mtx_plain);
  }
  ptInit(&bp->page_table);
  mtx_init(&bp->pool_lock, mtx_plain);
  bp->clock_hand = 0;
}

void bpDestroy(BufferPool *bp) {
  // Before destroy write all 'dirty' pages
  for (int i = 0; i < POOL_SIZE; i++) {
    if (bp->frames[i].page_id != INVALID_PAGE_ID && bp->frames[i].is_dirty)
      _DiskWrite(bp->frames[i].page_id, bp->memory[i]);
    mtx_destroy(&bp->frames[i].lock);
  }

  ptDestroy(&bp->page_table);
  mtx_destroy(&bp->pool_lock);
}

uint8_t *bpFetchPage(BufferPool *bp, uint32_t page_id) {
  mtx_lock(&bp->pool_lock);

  // Check if page already in pool
  uint32_t frame_id = ptLookup(&bp->page_table, page_id);
  if (frame_id != INVALID_FRAME_ID) {
    Frame *f = &bp->frames[frame_id];

    mtx_lock(&f->lock);

    mtx_unlock(&bp->pool_lock);

    f->pin_count++;
    f->ref_bit = true;

    mtx_unlock(&f->lock);
    return bp->memory[frame_id];
  }

  // if not, find victim
  uint32_t victim_id = INVALID_FRAME_ID;

  // Search for empty page
  for (uint32_t i = 0; i < POOL_SIZE; i++) {
    if (bp->frames[i].page_id == INVALID_PAGE_ID) {
      victim_id = i;
      break;
    }
  }

  // If there is no empty page, use clock algorithm
  if (victim_id == INVALID_FRAME_ID) {
    // NOTE: pool_lock is still held to protect from data races(Should be
    // rewritten if necessary)
    victim_id = _FindVictim(bp);
    Frame *victim = &bp->frames[victim_id];
    mtx_lock(&victim->lock);

    if (victim->is_dirty)
      _DiskWrite(victim->page_id, bp->memory[victim_id]);

    ptDelete(&bp->page_table, victim->page_id);

    _DiskRead(page_id, bp->memory[victim_id]);

    victim->page_id = page_id;
    victim->pin_count = 1;
    victim->is_dirty = false;
    victim->ref_bit = true;

    mtx_unlock(&victim->lock);
  } else {
    Frame *f = &bp->frames[victim_id];
    mtx_lock(&f->lock);

    _DiskRead(page_id, bp->memory[victim_id]);

    f->page_id = page_id;
    f->pin_count = 1;
    f->is_dirty = false;
    f->ref_bit = true;

    mtx_unlock(&f->lock);
  }

  ptInsert(&bp->page_table, page_id, victim_id);
  mtx_unlock(&bp->pool_lock);

  return bp->memory[victim_id];
}

void bpUnpinPage(BufferPool *bp, uint32_t page_id, bool is_dirty) {
  mtx_lock(&bp->pool_lock);
  uint32_t frame_id = ptLookup(&bp->page_table, page_id);

  if (frame_id != INVALID_FRAME_ID) {
    Frame *f = &bp->frames[frame_id];

    mtx_lock(&f->lock);
    mtx_unlock(&bp->pool_lock);

    if (f->pin_count > 0) {
      f->pin_count--;
    }
    if (is_dirty) {
      f->is_dirty = true;
    }

    mtx_unlock(&f->lock);
  } else {
    mtx_unlock(&bp->pool_lock);
  }
}
