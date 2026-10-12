#include "unitDynarray.h"

#include "morgutest.h"
#include "src/common/dynarray.h"

void TestDynarrayAlloc(void) {
    int* arr;
    M_DynArrayInit(arr);
    M_Assert(arr != NULL);
    M_DynArrayFree(arr);
    M_Assert(arr == NULL);
}

void TestDynarrayRealloc(void) {
    int* arr;
    M_DynArrayInit(arr);
    int capacity = M_DynArrayGetCapacity(arr);
    M_Assert(capacity != 0);
    for(int i = 0; i < capacity + 2; i++) {
        M_DynArrayPush(arr, 0);
    }
    M_Assert(arr != NULL);
    M_Assert(M_DynArrayGetCapacity(arr) != capacity);
    M_DynArrayFree(arr);
}

void TestDynarrayPush(void) {
    int* arr;
    M_DynArrayInit(arr);
    for(int i = 0; i < 10; i++) {
        M_DynArrayPush(arr, i);
    }
    M_Assert(M_DynArrayGetSize(arr) == 10);
    M_DynArrayFree(arr);
}

void TestDynarrayRemove(void) {
    int* arr;
    M_DynArrayInit(arr);
    for(int i = 0; i < 10; i++) {
        M_DynArrayPush(arr, i);
    }

    M_DynArrayRemove(arr, 0);
    M_Assert(arr[0] == 1);
    M_Assert(M_DynArrayGetSize(arr) == 9);

    M_DynArrayFree(arr);
}
void TestDynarrayFind(void) {
    int* arr;
    int foundIdx = -1;

    M_DynArrayInit(arr);
    for(int i = 0; i < 10; i++) {
        M_DynArrayPush(arr, i);
    }

    M_DynArrayFind(arr, 5, foundIdx);
    M_Assert(foundIdx == 5);

    M_DynArrayFree(arr);
}