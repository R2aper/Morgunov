#pragma once

#if !defined(_MSC_VER) || defined(__STDC_NO_THREADS__) || !__has_include(<threads.h>)
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>

// Map C11 Types to POSIX Types
typedef pthread_t thrd_t;
typedef pthread_mutex_t mtx_t;
typedef pthread_cond_t cnd_t;
typedef int (*thrd_start_t)(void*);

// Map Mutex Types (C11 flags)
enum { mtx_plain = 0, mtx_recursive = 1, mtx_timed = 2 };

enum { thrd_success = 0, thrd_error = 1, thrd_nomem = 2, thrd_timedout = 3, thrd_busy = 4 };

// Internal helper struct to safely pass data through pthread_create
struct __thrd_wrapper_args {
  thrd_start_t func;
  void* arg;
};

// Intermediary function that matches the signature pthread expects (void*)
static void* __thrd_pthread_proxy(void* p) {
  struct __thrd_wrapper_args* args = (struct __thrd_wrapper_args*)p;
  thrd_start_t func = args->func;
  void* arg = args->arg;

  free(args); // Clean up the heap-allocated bridge arguments

  // Call your actual C11 thread function
  int exit_code = func(arg);

  // Cast int return to void* to satisfy pthread
  return (void*)(uintptr_t)exit_code;
}

static inline int thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
  struct __thrd_wrapper_args* args =
      (struct __thrd_wrapper_args*)malloc(sizeof(struct __thrd_wrapper_args));
  if (!args) {
    return thrd_nomem;
  }

  args->func = func;
  args->arg = arg;

  if (pthread_create(thr, NULL, __thrd_pthread_proxy, args) != 0) {
    free(args); // Clean up if thread creation failed immediately
    return thrd_error;
  }

  return thrd_success;
}

static inline int thrd_join(thrd_t thr, int* res) {
  void* pthread_res;
  if (pthread_join(thr, &pthread_res) != 0) {
    return thrd_error;
  }
  if (res) {
    *res = (int)(uintptr_t)pthread_res;
  }
  return thrd_success;
}

static inline void thrd_yield(void) { sched_yield(); }

static inline int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {
  return nanosleep(duration, remaining);
}

#define mtx_init(mutex, type) (pthread_mutex_init(mutex, NULL) == 0 ? thrd_success : thrd_error)
#define mtx_lock(mutex) (pthread_mutex_lock(mutex) == 0 ? thrd_success : thrd_error)
#define mtx_unlock(mutex) (pthread_mutex_unlock(mutex) == 0 ? thrd_success : thrd_error)
#define mtx_destroy(mutex) pthread_mutex_destroy(mutex)

#define cnd_init(cond) (pthread_cond_init(cond, NULL) == 0 ? thrd_success : thrd_error)
#define cnd_signal(cond) (pthread_cond_signal(cond) == 0 ? thrd_success : thrd_error)
#define cnd_broadcast(cond) (pthread_cond_broadcast(cond) == 0 ? thrd_success : thrd_error)
#define cnd_wait(cond, mutex) (pthread_cond_wait(cond, mutex) == 0 ? thrd_success : thrd_error)
#define cnd_destroy(cond) pthread_cond_destroy(cond)

static inline int cnd_timedwait(cnd_t* cond, mtx_t* mutex, const struct timespec* time_point) {
  int ret = pthread_cond_timedwait(cond, mutex, time_point);
  if (ret == 0)
    return thrd_success;
  if (ret == ETIMEDOUT)
    return thrd_timedout;
  return thrd_error;
}

#else
#include <threads.h>
#endif
