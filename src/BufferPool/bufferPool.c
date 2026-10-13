#include "bufferPool.h"
#include <malloc.h>
#include <string.h>
#include "src/common/crossthreads.h"

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
  for (uint32_t i = 0; i < POOL_SIZE; i++) {
    uint32_t idx = bp->ClockHand;
    bp->ClockHand = (bp->ClockHand + 1) % POOL_SIZE;

    Frame* f = &bp->Frames[idx];

    // We can't replace page that in use or loading
    if (f->PinCount > 0 || f->IsLoading)
      continue;

    // If the access bit is set, we clear it and give it a "second chance."
    if (f->RefBit) {
      f->RefBit = false;
    } else {
      // if not - this is our victim
      return idx;
    }
  }

  return INVALID_FRAME_ID; // No victim today
}

void BpInit(BufferPool* bp) {
  bp = (BufferPool*)calloc(1, sizeof(BufferPool));

  for (int i = 0; i < POOL_SIZE; i++) {
    bp->Frames[i].PageId = INVALID_PAGE_ID;
    bp->Frames[i].PinCount = 0;
    bp->Frames[i].IsDirty = false;
    bp->Frames[i].IsLoading = false;
    bp->Frames[i].RefBit = false;
    (void)mtx_init(&bp->Frames[i].Lock, mtx_plain);
  }
  PtInit(&bp->PageTable);
  (void)mtx_init(&bp->PoolLock, mtx_plain);
  bp->ClockHand = 0;
}

void BpDestroy(BufferPool* bp) {
  // Before destroy write all 'dirty' pages
  for (int i = 0; i < POOL_SIZE; i++) {
    if (bp->Frames[i].PageId != INVALID_PAGE_ID && bp->Frames[i].IsDirty)
      _DiskWrite(bp->Frames[i].PageId, bp->Memory[i]);
    (void)mtx_destroy(&bp->Frames[i].Lock);
  }

  PtDestroy(&bp->PageTable);
  (void)mtx_destroy(&bp->PoolLock);

  free(bp);
}

uint8_t* BpFetchPage(BufferPool* bp, uint32_t pageId) {
  (void)mtx_lock(&bp->PoolLock);

  // Check if page already in pool
  uint32_t frameId = PtLookup(&bp->PageTable, pageId);
  if (frameId != INVALID_FRAME_ID) {
    Frame* f = &bp->Frames[frameId];

    (void)mtx_lock(&f->Lock);

    (void)mtx_unlock(&bp->PoolLock);

    f->PinCount++;
    f->RefBit = true;

    (void)mtx_unlock(&f->Lock);
    return bp->Memory[frameId];
  }

  // if not, find victim
  uint32_t victimId = INVALID_FRAME_ID;

  // Search for empty page
  for (uint32_t i = 0; i < POOL_SIZE; i++) {
    if (bp->Frames[i].PageId == INVALID_PAGE_ID && !bp->Frames[i].IsLoading) {
      victimId = i;
      break;
    }
  }

  if (victimId == INVALID_FRAME_ID) {
    victimId = _FindVictim(bp);
    if (victimId == INVALID_FRAME_ID) {
      (void)mtx_unlock(&bp->PoolLock);
      return NULL;
    }
  }

  Frame* victim = &bp->Frames[victimId];
  (void)mtx_lock(&victim->Lock);

  // Secure loading frame
  victim->IsLoading = true;
  uint32_t oldPageId = victim->PageId;
  bool wasDirty = victim->IsDirty;

  // Unlock global pool lock
  (void)mtx_unlock(&bp->PoolLock);

  if (wasDirty && oldPageId != INVALID_PAGE_ID)
    _DiskWrite(oldPageId, bp->Memory[victimId]);

  _DiskRead(pageId, bp->Memory[victimId]);

  // Lock global pool lock for PageTable
  (void)mtx_lock(&bp->PoolLock);

  // Protection from dupicates
  uint32_t existingFrameId = PtLookup(&bp->PageTable, pageId);
  if (existingFrameId != INVALID_FRAME_ID && existingFrameId != victimId) {
    Frame* existingFrame = &bp->Frames[existingFrameId];
    (void)mtx_lock(&existingFrame->Lock);
    (void)mtx_unlock(&bp->PoolLock);

    existingFrame->PinCount++;
    existingFrame->RefBit = true;
    (void)mtx_unlock(&existingFrame->Lock);

    // Clear victim
    (void)mtx_lock(&victim->Lock);
    victim->IsLoading = false;
    victim->PinCount = 0;
    victim->PageId = INVALID_PAGE_ID;
    victim->IsDirty = false;
    (void)mtx_unlock(&victim->Lock);

    return bp->Memory[existingFrameId];
  }

  if (oldPageId != INVALID_PAGE_ID)
    PtDelete(&bp->PageTable, oldPageId);

  if (!PtInsert(&bp->PageTable, pageId, victimId)) {
    victim->IsLoading = false;
    victim->PageId = INVALID_PAGE_ID;
    (void)mtx_unlock(&victim->Lock);
    (void)mtx_unlock(&bp->PoolLock);
    return NULL;
  }

  victim->PageId = pageId;
  victim->IsDirty = false;
  victim->PinCount = 1;
  victim->RefBit = true;
  victim->IsLoading = false;

  (void)mtx_unlock(&victim->Lock);
  (void)mtx_unlock(&bp->PoolLock);

  return bp->Memory[victimId];
}

void BpUnpinPage(BufferPool* bp, uint32_t pageId, bool isDirty) {
  (void)mtx_lock(&bp->PoolLock);
  uint32_t frameId = PtLookup(&bp->PageTable, pageId);

  if (frameId != INVALID_FRAME_ID) {
    Frame* f = &bp->Frames[frameId];

    (void)mtx_lock(&f->Lock);
    (void)mtx_unlock(&bp->PoolLock);

    if (f->PageId == pageId) {
      if (f->PinCount > 0)
        f->PinCount--;
      if (isDirty)
        f->IsDirty = true;
    }

    (void)mtx_unlock(&f->Lock);
  } else
    (void)mtx_unlock(&bp->PoolLock);
}
