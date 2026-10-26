//===-- tls.c - thread-local storage on the CLR ----------------------------===//
//
// Two real threads update their own `__thread` variables while the main thread
// holds a different value, which is the property that fails if the value is a
// plain global. The workers run in native threads created by CreateThread, so
// the CLR has to enter managed code from a thread it did not create -- the
// same boundary sdl_demo.c exercises through SDL's timer.
//
// Each worker prints one line and never prints concurrently, because the
// portable printf is not thread-safe: it formats through a shared buffer and
// writes with several Console calls. The counters are read after the threads
// join, so the printed order is fixed.
//===----------------------------------------------------------------------===//

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

// A scalar initializer, a pointer initializer, and one with no stated value.
__thread int counter = 5;
__thread const char *tag = "start";
_Thread_local int seen;

struct args {
  int id;
  // What the worker saw, so main can print it after the join: the portable
  // printf is not thread-safe, and two threads printing at once interleave.
  int counter;
  const char *tag;
  int seen;
};

#ifdef _WIN32
static DWORD WINAPI worker(LPVOID p) {
  struct args *a = (struct args *)p;
  for (int i = 0; i < 1000; i++) {
    counter += a->id;
    seen++;
  }
  a->counter = counter;
  a->tag = tag;
  a->seen = seen;
  return 0;
}
#endif

int main(void) {
  // Deliberately unlike the initializer, so a shared global would show up as
  // the workers starting from 40 rather than from 5.
  counter = 40;
  tag = "main";
  seen = -1;
  printf("main before: counter=%d tag=%s seen=%d\n", counter, tag, seen);

#ifdef _WIN32
  struct args a = {2, 0, NULL, 0}, b = {3, 0, NULL, 0};
  HANDLE ta = CreateThread(NULL, 0, worker, &a, 0, NULL);
  HANDLE tb = CreateThread(NULL, 0, worker, &b, 0, NULL);
  if (!ta || !tb) {
    printf("no threads\n");
    return 1;
  }
  WaitForSingleObject(ta, INFINITE);
  WaitForSingleObject(tb, INFINITE);
  CloseHandle(ta);
  CloseHandle(tb);
  // The join is the synchronization point; print in thread order so the
  // output does not depend on which worker finished first.
  struct args *both[2] = {&a, &b};
  for (int i = 0; i < 2; i++)
    printf("thread %d: counter=%d tag=%s seen=%d\n", both[i]->id,
           both[i]->counter, both[i]->tag, both[i]->seen);
#else
  // Not a failure: this test only claims to prove thread isolation where it
  // can create threads, and the lit test still covers the emitted shape.
  printf("threads: skipped (thread creation is Windows-only here)\n");
#endif

  printf("main after: counter=%d tag=%s seen=%d\n", counter, tag, seen);
  int ok = counter == 40 && tag[0] == 'm' && seen == -1;
  printf("%s\n", ok ? "ok" : "FAIL main thread was disturbed");
  return !ok;
}
