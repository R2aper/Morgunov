#pragma once

#include <stdio.h>

static int _MorgutestTotal = 0;
static int _MorgutestFailed = 0;

#define MORGUTEST_COLOR_GREEN   "\033[0;32m"
#define MORGUTEST_COLOR_RED     "\033[0;31m"
#define MORGUTEST_COLOR_RESET   "\033[0m"

#define M_RunTest(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        int initial_failures = _MorgutestFailed; \
        test_func(); \
        if (_MorgutestFailed == initial_failures) { \
            printf(MORGUTEST_COLOR_GREEN "PASSED" MORGUTEST_COLOR_RESET "\n"); \
        } \
    } while(0)

#define M_Assert(expr) \
    do { \
        _MorgutestTotal++; \
        if (!(expr)) { \
            _MorgutestFailed++; \
            printf("\n  " MORGUTEST_COLOR_RED "FAIL: %s" MORGUTEST_COLOR_RESET " at %s:%d (Expected true but got false)\n  ", #expr, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define M_Report() \
do { \
    printf("\n=====================================\n"); \
    if (_MorgutestFailed == 0) { \
        printf(MORGUTEST_COLOR_GREEN "ALL TESTS PASSED" MORGUTEST_COLOR_RESET " (%d/%d assertions)\n", _MorgutestTotal, _MorgutestTotal); \
    } else { \
        printf(MORGUTEST_COLOR_RED "TESTS FAILED" MORGUTEST_COLOR_RESET ": %d out of %d assertions failed.\n", minitest_failed, minitest_total); \
    } \
    printf("=====================================\n"); \
} while(0)
