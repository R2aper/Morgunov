#include "page_table.h"

static inline uint32_t _HashPageId(uint32_t id) {
  id = ((id >> 16) ^ id) * 0x45d9f3b;
  id = ((id >> 16) ^ id) * 0x45d9f3b;
  id = (id >> 16) ^ id;
  return id;
}

static inline uint32_t _GetBucketIdx(uint32_t hash) {
  return hash & (HT_NUM_BUCKETS - 1);
}

static inline uint32_t _GetStripeIdx(uint32_t hash) {
  return (hash >> 11) & (HT_LOCK_STRIPES - 1);
}

void ptInit(PageTable *pt) {
  for (int i = 0; i < HT_NUM_BUCKETS; i++) {
    pt->buckets[i] = NULL;
  }
  for (int i = 0; i < HT_LOCK_STRIPES; i++) {
    mtx_init(&pt->stripes[i], mtx_plain);
  }

  mtx_init(&pt->free_list_lock, mtx_plain);

  for (int i = 0; i < POOL_SIZE - 1; i++) {
    pt->node_pool[i].next = &pt->node_pool[i + 1];
  }
  pt->node_pool[POOL_SIZE - 1].next = NULL;
  pt->free_list = &pt->node_pool[0];
}

void ptDestroy(PageTable *pt) {
  for (int i = 0; i < HT_LOCK_STRIPES; i++)
    mtx_destroy(&pt->stripes[i]);

  mtx_destroy(&pt->free_list_lock);
}

HtNode *ptAllocNode(PageTable *pt) {
  mtx_lock(&pt->free_list_lock);
  if (!pt->free_list) { // out of pre-allocated nodes
    mtx_unlock(&pt->free_list_lock);
    return NULL;
  }
  HtNode *node = pt->free_list;
  pt->free_list = node->next;
  mtx_unlock(&pt->free_list_lock);

  return node;
}

void ptFreeNode(PageTable *pt, HtNode *node) {
  mtx_lock(&pt->free_list_lock);
  node->next = pt->free_list;
  pt->free_list = node;
  mtx_unlock(&pt->free_list_lock);
}

uint32_t ptLookup(PageTable *pt, uint32_t page_id) {
  uint32_t hash = _HashPageId(page_id);
  uint32_t b_idx = _GetBucketIdx(hash);
  uint32_t s_idx = _GetStripeIdx(hash);

  mtx_lock(&pt->stripes[s_idx]);
  HtNode *curr = pt->buckets[b_idx];
  while (curr) {
    if (curr->page_id == page_id) {
      uint32_t fid = curr->frame_id;
      mtx_unlock(&pt->stripes[s_idx]);
      return fid;
    }
    curr = curr->next;
  }
  mtx_unlock(&pt->stripes[s_idx]);

  return INVALID_FRAME_ID;
}

bool ptInsert(PageTable *pt, uint32_t page_id, uint32_t frame_id) {
  uint32_t hash = _HashPageId(page_id);
  uint32_t b_idx = _GetBucketIdx(hash);
  uint32_t s_idx = _GetStripeIdx(hash);

  mtx_lock(&pt->stripes[s_idx]);

  // If an entry for this page already exists, update its frame_id in place
  HtNode *curr = pt->buckets[b_idx];
  while (curr) {
    if (curr->page_id == page_id) {
      curr->frame_id = frame_id;
      mtx_unlock(&pt->stripes[s_idx]);
      return true;
    }
    curr = curr->next;
  }

  HtNode *new_node = ptAllocNode(pt);
  if (!new_node) { // Pool full
    mtx_unlock(&pt->stripes[s_idx]);

    return false;
  }

  new_node->page_id = page_id;
  new_node->frame_id = frame_id;
  new_node->next = pt->buckets[b_idx];
  pt->buckets[b_idx] = new_node;

  mtx_unlock(&pt->stripes[s_idx]);

  return true;
}

bool ptDelete(PageTable *pt, uint32_t page_id) {
  uint32_t hash = _HashPageId(page_id);
  uint32_t b_idx = _GetBucketIdx(hash);
  uint32_t s_idx = _GetStripeIdx(hash);

  mtx_lock(&pt->stripes[s_idx]);
  HtNode *curr = pt->buckets[b_idx];
  HtNode *prev = NULL;

  while (curr) {
    if (curr->page_id == page_id) {
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
