#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct LString {
  size_t Length;
  size_t Capacity;
  char Data[];
} LString;

#define LSTRING_NOT_FOUND ((size_t)-1)

LString* LStringCreateEmpty(size_t capacity);
LString* LStringCreate(const char* source);
LString* LStringCreateFromBytes(const void* bytes, size_t length);
LString* LStringClone(const LString* source);
void LStringFree(LString* string);

size_t LStringGetLength(const LString* string);
size_t LStringGetCapacity(const LString* string);
const char* LStringGetData(const LString* string);
bool LStringIsEmpty(const LString* string);

// mutators take LString** because they may reallocate the whole struct
bool LStringReserve(LString** string, size_t minCapacity);
bool LStringAppend(LString** string, const LString* other);
bool LStringAppendBytes(LString** string, const void* bytes, size_t length);
bool LStringInsertBytes(LString** string, size_t index, const void* bytes, size_t length);
bool LStringAppendCString(LString** string, const char* source);
bool LStringAppendChar(LString** string, char character);
bool LStringInsert(LString** string, size_t index, const LString* other);
bool LStringInsertCString(LString** string, size_t index, const char* source);

void LStringClear(LString* string);

int LStringCompare(const LString* left, const LString* right);
bool LStringEquals(const LString* left, const LString* right);
size_t LStringFindChar(const LString* string, char character);
size_t LStringFind(const LString* string, const LString* needle);

LString* LStringNewSubstring(const LString* string, size_t start, size_t count);
