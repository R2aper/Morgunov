/**
 * @file lstring.h
 * @brief defines a dynamic string struct
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Represents a dynamic string, which holds a reference to a C-string
 */
typedef struct LString {
  size_t Length;
  size_t Capacity;
  char Data[];
} LString;

#define LSTRING_NOT_FOUND ((size_t)-1)

/**
 * @brief Creates an empty LString with immediately null-terinated underlying cstring
 * 
 * @param capacity String capacity
 * @returns Created string
 */
LString* LStringCreateEmpty(size_t capacity);
/**
 * @brief Creates a string from a cstring by copying it
 * 
 * @param source Cstring
 * @returns Created string 
 */
LString* LStringCreate(const char* source);
/**
 * @brief Same as @ref LStringCreateEmpty(), treats bytes as characters
 * 
 * @param bytes Pointer to data buffer
 * @param length Data buffer length
 * @returns Created string 
 */
LString* LStringCreateFromBytes(const void* bytes, size_t length);
/**
 * @brief Creates a new (deep) copy of the source string
 * 
 * @param source Source string
 * @returns Copied string 
 */
LString* LStringClone(const LString* source);
/**
 * @brief Frees resources associated with provided LString
 * 
 * @param string TargetString
 */
void LStringFree(LString* string);

/**
 * @brief Gets LString's length
 * 
 * @param string Target string
 * @returns Underlying buffer's length
 */
size_t LStringGetLength(const LString* string);
/**
 * @brief Gets LString's capacity
 * 
 * @param string Target string
 * @returns size_t Capacity
 */
size_t LStringGetCapacity(const LString* string);
/**
 * @brief Gets LString's underlying null-terminated string
 * 
 * @param string Target string
 * @return const char* CString
 */
const char* LStringGetData(const LString* string);
/**
 * @brief Checks whether or not the string is empty
 * 
 * @param string Target string
 * @returns Whether string is empty or not
 */
bool LStringIsEmpty(const LString* string);

// mutators take LString** because they may reallocate the whole struct

/**
 * @brief Allocates more space for the string
 * 
 * @param string Target string
 * @param minCapacity Minimal amount of capacity to allocate
 * @returns Whether the allocation succeeded or not
 */
bool LStringReserve(LString** string, size_t minCapacity);
/**
 * @brief Appends provided string to the end of the target one
 * 
 * @param string Target string (appendee)
 * @param other String to append
 * @returns Whether the operation succeeded or not
 */
bool LStringAppend(LString** string, const LString* other);
/**
 * @brief Appends provided byte array to the end of the target string
 * 
 * @param string Target string (appendee)
 * @param other Byte array to append
 * @returns Whether the operation succeeded or not
 */
bool LStringAppendBytes(LString** string, const void* bytes, size_t length);
/**
 * @brief Inserts provided byte array at the specified index
 * 
 * @param string Target string
 * @param index Position at which the array must be inserted
 * @param bytes Byte array pointer
 * @param length Byte array length
 * 
 * @returns Whether the operation succeeded or not
 */
bool LStringInsertBytes(LString** string, size_t index, const void* bytes, size_t length);
/**
 * @brief Appends provided cstring to the end of the target lstring
 * 
 * @param string Target LString
 * @param source CString to append
 * @returns Whether the operation succeeded or not
 */
bool LStringAppendCString(LString** string, const char* source);
/**
 * @brief Appends provided character to the end of the target lstring
 * 
 * @param string Target string
 * @param character Character to append
 * @returns Whether the operation succeeded or not
 */
bool LStringAppendChar(LString** string, char character);
/**
 * @brief Inserts provided string at the specified index of the target string
 * 
 * @param string Target string
 * @param index Position at which the string must be inserted
 * @param other String to be inserted
 * 
 * @returns Whether the operation succeeded or not
 */
bool LStringInsert(LString** string, size_t index, const LString* other);
/**
 * @brief Inserts provided CString at the specified index of the target LString
 * 
 * @param string Target string
 * @param index Position at which the string must be inserted
 * @param source String to be inserted
 * 
 * @returns Whether the operation succeeded or not
 */
bool LStringInsertCString(LString** string, size_t index, const char* source);

/**
 * @brief Clears the underlying character array, making it immediately null-terminated
 * 
 * @param string Target string
 */
void LStringClear(LString* string);

/**
 * @brief Compares strings lengths
 * 
 * @param left Left string
 * @param right Right string
 * @return -1 if left string is less than right string, 1 if left string is greater than right string, 0 if the lengths are equal
 */
int LStringCompare(const LString* left, const LString* right);
/**
 * @brief Checks whether the string lengths are equal
 * @note More efficient than @ref LStringCompare()
 * 
 * @param left Left string
 * @param right Right string
 * 
 * @returns Whether the strings are equal or not
 */
bool LStringEquals(const LString* left, const LString* right);
/**
 * @brief Finds the first entry of the provided character in the target string
 * 
 * @param string Target string
 * @param character Character to look for
 * @returns Index of found character or LSTRING_NOT_FOUND if the character couldn't be found
 */
size_t LStringFindChar(const LString* string, char character);
/**
 * @brief Finds the first entry of the provided substring in the target string
 * 
 * @param string Target string
 * @param needle Substring to look for
 * @returns Index of the substring's first character or LSTRING_NOT_FOUND if the substring couldn't be found
 */
size_t LStringFind(const LString* string, const LString* needle);

/**
 * @brief Creates a deep copy of a substring
 * 
 * @param string Target string
 * @param start Start index of the substring
 * @param count Length of the substring
 * @return New substring object
 */
LString* LStringNewSubstring(const LString* string, size_t start, size_t count);
