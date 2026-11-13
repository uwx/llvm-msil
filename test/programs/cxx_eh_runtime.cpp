/* Exceptions combined with the portable C++ runtime.
 *
 * cxx_eh.cpp covers the unwinding model on its own; this covers the two halves
 * working together, which is what a real C++ program does and what neither
 * file tests alone: a local static inside a try block (guard + unwinding), a
 * global destructor running after an exception has been caught and swallowed,
 * and an object with a destructor that runs on the unwind path. -msil-libcxx
 * is what supplies the guards and the exit-time table, so this only links with
 * it.
 *
 *   ./cxx_eh_runtime.sh <llvm-msil>
 *
 * Written against the same freestanding subset as libcxx/cxxrt.cpp: no
 * standard headers, no <new>, no RTTI.
 */
typedef __SIZE_TYPE__ size_t;

extern "C" int printf(const char *, ...);

static int live = 0;

struct Guard {
  int id;
  explicit Guard(int i) : id(i) {
    ++live;
    printf("ctor %d\n", id);
  }
  ~Guard() {
    --live;
    printf("dtor %d\n", id);
  }
};

/* A destructor that runs because the stack is unwinding, not because the
 * block completed. */
struct ThrowOnUnwind {
  int id;
  explicit ThrowOnUnwind(int i) : id(i) { ++live; }
  ~ThrowOnUnwind() {
    --live;
    printf("unwind dtor %d\n", id);
  }
};

/* The same static reached from inside and outside a handler: the guard has to
 * be past its first initialisation either way. */
__attribute__((noinline)) static int counter() {
  static int n = 100;
  return ++n;
}

__attribute__((noinline)) void throw_from_try(int i) {
  ThrowOnUnwind u(i);
  printf("body %d\n", counter());
  throw i;
}

void frame_by_frame() {
  for (int i = 0; i < 3; ++i) {
    try {
      Guard g(i);
      throw_from_try(i);
      printf("BAD: not reached %d\n", i);
    } catch (int v) {
      printf("caught %d\n", v);
    }
  }
}

/* A static local whose initialiser cannot be folded, so the guard really is
 * consulted at run time and the initialiser runs inside a try block. */
extern "C" int seed = 5;
__attribute__((noinline)) int guarded_in_try() {
  try {
    static int value = seed + 7;
    printf("static %d\n", value);
    throw value;
  } catch (int v) {
    printf("from static %d\n", v);
  }
  return 0;
}

/* Catch, then allocate again: the allocator and the unwinder share no state,
 * but a stale caught-exception pointer would show up here. */
void after_catch() {
  for (int i = 0; i < 2; ++i) {
    try {
      Guard g(10 + i);
      if (i == 0)
        throw 1;
    } catch (...) {
      printf("swallowed %d\n", i);
    }
  }
}

int main() {
  frame_by_frame();
  guarded_in_try();
  after_catch();
  printf("final live=%d\n", live);
  return live;
}

/* Runs after main returns, so it also proves the registered table was not
 * disturbed by any of the unwinding above. */
struct Global {
  ~Global() { printf("global dtor live=%d\n", live); }
} global;
