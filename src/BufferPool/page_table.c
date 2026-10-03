#include "page_table.h"

static inline uint32_t _HashPageId(uint32_t id) {
  id = ((id >> 16) ^ id) * 0x45d9f3b;
  id = ((id >> 16) ^ id) * 0x45d9f3b;
  id = (id >> 16) ^ id;
  return id;
}

static inline uint32_t _GetBucketIdx(uint32_t hash) { return hash & (HT_NUM_BUCKETS - 1); }

static inline uint32_t _GetStripeIdx(uint32_t hash) { return (hash >> 11) & (HT_LOCK_STRIPES - 1); }

void ptInit(PageTable* pt) {
  for (int i = 0; i < HT_NUM_BUCKETS; i++) {
    pt->buckets[i] = NULL;
  }
  for (int i = 0; i < HT_LOCK_STRIPES; i++) {
    mtx_init(&pt->stripes[i], mtx_plain);
  }

  mtx_init(&pt->freeListLock, mtx_plain);

  for (int i = 0; i < POOL_SIZE - 1; i++) {
    pt->nodePool[i].next = &pt->nodePool[i + 1];
  }
  pt->nodePool[POOL_SIZE - 1].next = NULL;
  pt->freeList = &pt->nodePool[0];
}

void ptDestroy(PageTable* pt) {
  for (int i = 0; i < HT_LOCK_STRIPES; i++)
    mtx_destroy(&pt->stripes[i]);

  mtx_destroy(&pt->freeListLock);
}

HtNode* ptAllocNode(PageTable* pt) {
  mtx_lock(&pt->freeListLock);
  if (!pt->freeList) { // out of pre-allocated nodes
    mtx_unlock(&pt->freeListLock);
    return NULL;
  }
  HtNode* node = pt->freeList;
  pt->freeList = node->next;
  mtx_unlock(&pt->freeListLock);

  return node;
}

void ptFreeNode(PageTable* pt, HtNode* node) {
  mtx_lock(&pt->freeListLock);
  node->next = pt->freeList;
  pt->freeList = node;
  mtx_unlock(&pt->freeListLock);
}

uint32_t ptLookup(PageTable* pt, uint32_t pageId) {
  uint32_t hash = _HashPageId(pageId);
  uint32_t b_idx = _GetBucketIdx(hash);
  uint32_t s_idx = _GetStripeIdx(hash);

  mtx_lock(&pt->stripes[s_idx]);
  HtNode* curr = pt->buckets[b_idx];
  while (curr) {
    if (curr->pageId == pageId) {
      uint32_t fid = curr->frameId;
      mtx_unlock(&pt->stripes[s_idx]);
      return fid;
    }
    curr = curr->next;
  }
  mtx_unlock(&pt->stripes[s_idx]);

  return INVALID_FRAME_ID;
}

bool ptInsert(PageTable* pt, uint32_t pageId, uint32_t frameId) {
  uint32_t hash = _HashPageId(pageId);
  uint32_t b_idx = _GetBucketIdx(hash);
  uint32_t s_idx = _GetStripeIdx(hash);

  mtx_lock(&pt->stripes[s_idx]);

  // If an entry for this page already exists, update its FrameId in place
  HtNode* curr = pt->buckets[b_idx];
  while (curr) {
    if (curr->pageId == pageId) {
      curr->frameId = frameId;
      mtx_unlock(&pt->stripes[s_idx]);
      return true;
    }
    curr = curr->next;
  }

  HtNode* new_node = ptAllocNode(pt);
  if (!new_node) { // Pool full
    mtx_unlock(&pt->stripes[s_idx]);

    return false;
  }

  new_node->pageId = pageId;
  new_node->frameId = frameId;
  new_node->next = pt->buckets[b_idx];
  pt->buckets[b_idx] = new_node;

  mtx_unlock(&pt->stripes[s_idx]);

  return true;
}

bool ptDelete(PageTable* pt, uint32_t pageId) {
  uint32_t hash = _HashPageId(pageId);
  uint32_t b_idx = _GetBucketIdx(hash);
  uint32_t s_idx = _GetStripeIdx(hash);

  mtx_lock(&pt->stripes[s_idx]);
  HtNode* curr = pt->buckets[b_idx];
  HtNode* prev = NULL;

  while (curr) {
    if (curr->pageId == pageId) {
      if (prev == NULL) {
        pt->buckets[b_idx] = curr->next;
      } else {
        prev->next = curr->next;
      }
      ptFreeNode(pt, curr);
      mtx_unlock(&pt->stripes[s_idx]);
      return true;
    }
    prev = curr;
    curr = curr->next;
  }
  mtx_unlock(&pt->stripes[s_idx]);

  return false;
}
