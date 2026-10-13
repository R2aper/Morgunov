#include "pageTable.h"

static inline uint32_t _HashPageId(uint32_t id) {
  id = ((id >> 16) ^ id) * 0x45d9f3b;
  id = ((id >> 16) ^ id) * 0x45d9f3b;
  id = (id >> 16) ^ id;
  return id;
}

static inline uint32_t _GetBucketIdx(uint32_t hash) { return hash & (HT_NUM_BUCKETS - 1); }

static inline uint32_t _GetStripeIdx(uint32_t hash) { return (hash >> 11) & (HT_LOCK_STRIPES - 1); }

void PtInit(PageTable* pt) {
  for (int i = 0; i < HT_NUM_BUCKETS; i++)
    pt->Buckets[i] = NULL;

  for (int i = 0; i < HT_LOCK_STRIPES; i++)
    (void)mtx_init(&pt->Stripes[i], mtx_plain);

  (void)mtx_init(&pt->FreeListLock, mtx_plain);

  for (int i = 0; i < POOL_SIZE - 1; i++)
    pt->NodePool[i].Next = &pt->NodePool[i + 1];

  pt->NodePool[POOL_SIZE - 1].Next = NULL;
  pt->FreeList = &pt->NodePool[0];
}

void PtDestroy(PageTable* pt) {
  for (int i = 0; i < HT_LOCK_STRIPES; i++)
    mtx_destroy(&pt->Stripes[i]);

  mtx_destroy(&pt->FreeListLock);
}

HtNode* PtAllocNode(PageTable* pt) {
  (void)mtx_lock(&pt->FreeListLock);
  if (!pt->FreeList) { // out of pre-allocated nodes
    (void)mtx_unlock(&pt->FreeListLock);
    return NULL;
  }
  HtNode* node = pt->FreeList;
  pt->FreeList = node->Next;
  (void)mtx_unlock(&pt->FreeListLock);

  return node;
}

void PtFreeNode(PageTable* pt, HtNode* node) {
  (void)mtx_lock(&pt->FreeListLock);
  node->Next = pt->FreeList;
  pt->FreeList = node;
  (void)mtx_unlock(&pt->FreeListLock);
}

uint32_t PtLookup(PageTable* pt, uint32_t pageId) {
  uint32_t hash = _HashPageId(pageId);
  uint32_t bIdx = _GetBucketIdx(hash);
  uint32_t sIdx = _GetStripeIdx(hash);

  (void)mtx_lock(&pt->Stripes[sIdx]);
  HtNode* curr = pt->Buckets[bIdx];
  while (curr) {
    if (curr->PageId == pageId) {
      uint32_t fid = curr->FrameId;
      (void)mtx_unlock(&pt->Stripes[sIdx]);
      return fid;
    }
    curr = curr->Next;
  }
  (void)mtx_unlock(&pt->Stripes[sIdx]);

  return INVALID_FRAME_ID;
}

bool PtInsert(PageTable* pt, uint32_t pageId, uint32_t frameId) {
  uint32_t hash = _HashPageId(pageId);
  uint32_t bIdx = _GetBucketIdx(hash);
  uint32_t sIdx = _GetStripeIdx(hash);

  (void)mtx_lock(&pt->Stripes[sIdx]);

  // If an entry for this page already exists, update its FrameId in place
  HtNode* curr = pt->Buckets[bIdx];
  while (curr) {
    if (curr->PageId == pageId) {
      curr->FrameId = frameId;
      (void)mtx_unlock(&pt->Stripes[sIdx]);
      return true;
    }
    curr = curr->Next;
  }

  HtNode* newNode = PtAllocNode(pt);
  if (!newNode) { // Pool full
    (void)mtx_unlock(&pt->Stripes[sIdx]);

    return false;
  }

  newNode->PageId = pageId;
  newNode->FrameId = frameId;
  newNode->Next = pt->Buckets[bIdx];
  pt->Buckets[bIdx] = newNode;

  (void)mtx_unlock(&pt->Stripes[sIdx]);

  return true;
}

bool PtDelete(PageTable* pt, uint32_t pageId) {
  uint32_t hash = _HashPageId(pageId);
  uint32_t bIdx = _GetBucketIdx(hash);
  uint32_t sIdx = _GetStripeIdx(hash);

  (void)mtx_lock(&pt->Stripes[sIdx]);
  HtNode* curr = pt->Buckets[bIdx];
  HtNode* prev = NULL;

  while (curr) {
    if (curr->PageId == pageId) {
      if (prev == NULL)
        pt->Buckets[bIdx] = curr->Next;
      else
        prev->Next = curr->Next;

      PtFreeNode(pt, curr);
      (void)mtx_unlock(&pt->Stripes[sIdx]);
      return true;
    }
    prev = curr;
    curr = curr->Next;
  }
  (void)mtx_unlock(&pt->Stripes[sIdx]);

  return false;
}
