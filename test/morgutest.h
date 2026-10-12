#pragma once

#include <stdio.h>

static int _MorgutestTotal = 0;
static int _MorgutestFailed = 0;

#define MORGUTEST_COLOR_GREEN   "\033[0;32m"
#define MORGUTEST_COLOR_RED     "\033[0;31m"
#define MORGUTEST_COLOR_RESET   "\033[0m"

#define M_Expect(condition) do { \
    _MorgutestTotal++; \
    if (!(condition)) { \
        printf("  \033[1;31m[FAIL]\033[0m %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
        _MorgutestFailed++; \
    } else { \
        printf("  \033[1;32m[PASS]\033[0m %s:%d\n", __FILE__, __LINE__); \
    } \
} while(0)

#define M_RunTest(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        int initial_failures = _MorgutestFailed; \
        test_func(); \
        if (_MorgutestFailed == initial_failures) { \
            printf(MORGUTEST_COLOR_GREEN "PASSED" MORGUTEST_COLOR_RESET "\n"); \
        } \
    } while(0)

#define M_Assert(condition) do { \
    _MorgutestTotal++; \
    if (!(condition)) { \
        printf("  \033[1;31m[FATAL ASSERT FAIL]\033[0m %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        printf("Halting execution.\n"); \
        exit(1); \
    } else { \
        printf("  \033[1;32m[PASS]\033[0m %s:%d\n", __FILE__, __LINE__); \
    } \
} while(0)

#define M_Report() __extension__ ({ \
    printf("----------------------------------------\n"); \
    if (_MorgutestFailed == 0) { \
        printf("\033[1;32mSUCCESS:\033[0m All %d assertions passed!\n", _MorgutestTotal); \
    } else { \
        printf("\033[1;31mFAILURE:\033[0m %d out of %d assertions failed.\n", _MorgutestFailed, _MorgutestTotal); \
    } \
    printf("----------------------------------------\n"); \
    (_MorgutestFailed > 0) ? 1 : 0; \
})
