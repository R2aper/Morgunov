#include "buffer_pool.h"
#include <string.h>
#include <threads.h>

static void _DiskRead(uint32_t pageId, uint8_t* buffer) {
  // TODO:
  memset(buffer, (char)(pageId % 256), PAGE_SIZE);
}

static void _DiskWrite(uint32_t pageId, uint8_t* buffer) {
  (void)buffer;
  (void)pageId;
  // TODO:
}

// Page replacement algorithm - Clock
static uint32_t _FindVictim(BufferPool* bp) {
  for (;;) {
    uint32_t idx = bp->ClockHand;
    bp->ClockHand = (bp->ClockHand + 1) % POOL_SIZE;

    Frame* f = &bp->Frames[idx];

    // We can't replace page that in use
    if (f->PinCount > 0)
      continue;

    // If the access bit is set, we clear it and give it a "second chance."
    if (f->RefBit) {
      f->RefBit = false;
    } else {
      // if not - this is our victim
      return idx;
    }
  }
}

void BpInit(BufferPool* bp) {
  for (int i = 0; i < POOL_SIZE; i++) {
    bp->Frames[i].PageId = INVALID_PAGE_ID;
    bp->Frames[i].PinCount = 0;
    bp->Frames[i].IsDirty = false;
    bp->Frames[i].RefBit = false;
    mtx_init(&bp->Frames[i].Lock, mtx_plain);
  }
  PtInit(&bp->PageTable);
  mtx_init(&bp->PoolLock, mtx_plain);
  bp->ClockHand = 0;
}

void BpDestroy(BufferPool* bp) {
  // Before destroy write all 'dirty' pages
  for (int i = 0; i < POOL_SIZE; i++) {
    if (bp->Frames[i].PageId != INVALID_PAGE_ID && bp->Frames[i].IsDirty)
      _DiskWrite(bp->Frames[i].PageId, bp->Memory[i]);
    mtx_destroy(&bp->Frames[i].Lock);
  }

  PtDestroy(&bp->PageTable);
  mtx_destroy(&bp->PoolLock);
}

uint8_t* BpFetchPage(BufferPool* bp, uint32_t pageId) {
  mtx_lock(&bp->PoolLock);

  // Check if page already in pool
  uint32_t frameId = PtLookup(&bp->PageTable, pageId);
  if (frameId != INVALID_FRAME_ID) {
    Frame* f = &bp->Frames[frameId];

    mtx_lock(&f->Lock);

    mtx_unlock(&bp->PoolLock);

    f->PinCount++;
    f->RefBit = true;

    mtx_unlock(&f->Lock);
    return bp->Memory[frameId];
  }

  // if not, find victim
  uint32_t victimId = INVALID_FRAME_ID;

  // Search for empty page
  for (uint32_t i = 0; i < POOL_SIZE; i++) {
    if (bp->Frames[i].PageId == INVALID_PAGE_ID) {
      victimId = i;
      break;
    }
  }

  // If there is no empty page, use clock algorithm
  if (victimId == INVALID_FRAME_ID) {
    // NOTE: poolLock is still held to protect from data races(Should be
    // rewritten if necessary)
    victimId = _FindVictim(bp);
    Frame* victim = &bp->Frames[victimId];
    mtx_lock(&victim->Lock);

    if (victim->IsDirty)
      _DiskWrite(victim->PageId, bp->Memory[victimId]);

    PtDelete(&bp->PageTable, victim->PageId);

    _DiskRead(pageId, bp->Memory[victimId]);

    victim->PageId = pageId;
    victim->PinCount = 1;
    victim->IsDirty = false;
    victim->RefBit = true;

    mtx_unlock(&victim->Lock);
  } else {
    Frame* f = &bp->Frames[victimId];
    mtx_lock(&f->Lock);

    _DiskRead(pageId, bp->Memory[victimId]);

    f->PageId = pageId;
    f->PinCount = 1;
    f->IsDirty = false;
    f->RefBit = true;

    mtx_unlock(&f->Lock);
  }

  PtInsert(&bp->PageTable, pageId, victimId);
  mtx_unlock(&bp->PoolLock);

  return bp->Memory[victimId];
}

void BpUnpinPage(BufferPool* bp, uint32_t pageId, bool isDirty) {
  mtx_lock(&bp->PoolLock);
  uint32_t frameId = PtLookup(&bp->PageTable, pageId);

  if (frameId != INVALID_FRAME_ID) {
    Frame* f = &bp->Frames[frameId];

    mtx_lock(&f->Lock);
    mtx_unlock(&bp->PoolLock);

    if (f->PinCount > 0)
      f->PinCount--;

    if (isDirty)
      f->IsDirty = true;

    mtx_unlock(&f->Lock);
  } else
    mtx_unlock(&bp->PoolLock);
}
