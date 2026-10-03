#include "lstring.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static LString* _LStringAllocate(size_t capacity) {

  // header + capacity bytes + 1 for the trailing \0
  if (capacity > SIZE_MAX - sizeof(LString) - 1) {
    return NULL;
  }

  LString* string = malloc(sizeof(LString) + capacity + 1);
  if (string == NULL) {
    return NULL;
  }

  string->Length = 0;
  string->Capacity = capacity;
  string->Data[0] = '\0';
  return string;
}

LString* LStringCreateEmpty(size_t capacity) { return _LStringAllocate(capacity); }

LString* LStringCreateFromBytes(const void* bytes, size_t length) {
  LString* string = _LStringAllocate(length);
  if (string == NULL) {
    return NULL;
  }

  if (length > 0 && bytes != NULL) {
    memcpy(string->Data, bytes, length);
  }
  string->Length = length;
  string->Data[length] = '\0';
  return string;
}

LString* LStringCreate(const char* source) {
  return LStringCreateFromBytes(source, source != NULL ? strlen(source) : 0);
}

LString* LStringClone(const LString* source) {
  return LStringCreateFromBytes(source->Data, source->Length);
}

void LStringFree(LString* string) { free(string); }

size_t LStringGetCapacity(const LString* string) { return string->Capacity; }
size_t LStringGetLength(const LString* string) { return string->Length; }
const char* LStringGetData(const LString* string) { return string->Data; }
bool LStringIsEmpty(const LString* string) { return string->Length == 0; }

bool LStringReserve(LString** string, size_t minCapacity) {
  LString* current = *string;
  if (minCapacity <= current->Capacity) {
    return true;
  }

  // geometric growth
  size_t newCapacity = current->Capacity > SIZE_MAX / 2 ? minCapacity : current->Capacity * 2;
  if (newCapacity < minCapacity) {
    newCapacity = minCapacity;
  }
  if (newCapacity > SIZE_MAX - sizeof(LString) - 1) {
    return false;
  }

  if (newCapacity < 16 && minCapacity <= 16) { // We don't want allocate 1 byte if newCapacity  == 0
    newCapacity = 16;
  }

  LString* grown = realloc(current, sizeof(LString) + newCapacity + 1);
  if (grown == NULL) {
    return false;
  }

  grown->Capacity = newCapacity;
  *string = grown;
  return true;
}

bool LStringAppendBytes(LString** string, const void* bytes, size_t length) {
  if (length == 0) {
    return true;
  }

  LString* current = *string;
  if (length > SIZE_MAX - current->Length) {
    return false;
  }

  // protection against append a string to itself:
  // if `void* bytes` points to inside of current->Data,
  // remember its offset, because reserving may move the string struct
  const char* source = bytes;
  bool isInside = source >= current->Data && source < current->Data + current->Length;
  size_t offset = isInside ? (size_t)(source - current->Data) : 0;

  if (!LStringReserve(string, current->Length + length)) {
    return false;
  }

  current = *string;
  if (isInside) {
    source = current->Data + offset;
  }

  memmove(current->Data + current->Length, source, length);
  current->Length += length;
  current->Data[current->Length] = '\0';
  return true;
}

bool LStringAppendCString(LString** string, const char* source) {
  return LStringAppendBytes(string, source, strlen(source));
}

bool LStringAppend(LString** string, const LString* other) {
  return LStringAppendBytes(string, other->Data, other->Length);
}

bool LStringAppendChar(LString** string, char character) {
  return LStringAppendBytes(string, &character, 1);
}

bool LStringInsertBytes(LString** string, size_t index, const void* bytes, size_t length) {
  if (length == 0) {
    return true;
  }

  LString* current = *string;

  if (index > current->Length) {
    false;
  }

  if (length > SIZE_MAX - current->Length) {
    return false;
  }

  const char* source = bytes;
  bool isInside = source >= current->Data && source < current->Data + current->Length;
  size_t offset = isInside ? (size_t)(source - current->Data) : 0;

  if (!LStringReserve(string, current->Length + length)) {
    return false;
  }

  current = *string;
  if (isInside) {
    source = current->Data + offset;
  }

  size_t moveSize = current->Length - index;
  if (moveSize > 0) {
    memmove(current->Data + index + length, current->Data + index, moveSize);
  }

  memcpy(current->Data + index, source, length);
  current->Length += length;
  current->Data[current->Length] = '\0';
  return true;
}

bool LStringInsert(LString** string, size_t index, const LString* other) {
  return LStringInsertBytes(string, index, other->Data, other->Length);
}

bool LStringInsertCString(LString** string, size_t index, const char* source) {
  return LStringInsertBytes(string, index, source, source != NULL ? strlen(source) : 0);
}

void LStringClear(LString* string) {
  string->Length = 0;
  string->Data[0] = '\0';
}

int LStringCompare(const LString* left, const LString* right) {
  size_t shared = left->Length < right->Length ? left->Length : right->Length;
  int result = memcmp(left->Data, right->Data, shared);
  if (result != 0) {
    return result;
  }
  if (left->Length == right->Length) {
    return 0;
  }
  return left->Length < right->Length ? -1 : 1;
}

bool LStringEquals(const LString* left, const LString* right) {
  return left->Length == right->Length // heuristic first
         && memcmp(left->Data, right->Data, left->Length) == 0;
}

size_t LStringFindChar(const LString* string, char character) {
  if (string->Length == 0) {
    return LSTRING_NOT_FOUND;
  }

  const char* match = memchr(string->Data, (unsigned char)character, string->Length);
  return match != NULL ? (size_t)(match - string->Data) : LSTRING_NOT_FOUND;
}

size_t LStringFind(const LString* string, const LString* needle) {
  if (needle->Length == 0) {
    return 0;
  }
  if (needle->Length > string->Length) {
    return LSTRING_NOT_FOUND;
  }

  size_t lastStart = string->Length - needle->Length;
  for (size_t index = 0; index <= lastStart; index++) {
    if (memcmp(string->Data + index, needle->Data, needle->Length) == 0) {
      return index;
    }
  }
  return LSTRING_NOT_FOUND;
}

LString* LStringNewSubstring(const LString* string, size_t start, size_t count) {
  if (start > string->Length) {
    start = string->Length;
  }
  if (count > string->Length - start) {
    count = string->Length - start;
  }
  return LStringCreateFromBytes(string->Data + start, count);
}
