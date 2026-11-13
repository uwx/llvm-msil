/* Portable C++ runtime for llvm-msil.
 *
 * This is the part of a C++ implementation that is not in the headers: the
 * allocation operators, the local-static guards, and the exit-time destructor
 * machinery. It is written in freestanding C++ -- no standard headers, no
 * exceptions, no RTTI -- and compiled to IR at build time exactly like
 * libc/*.c, then linked selectively.
 *
 * Nothing here duplicates the C library: the allocator calls malloc/free by
 * name, so the -msil-alloc hook and the IL allocator stay in effect.
 */
#include "cxxrt.h"

/* --- Allocation ---------------------------------------------------------- */

/* C++ requires a failing plain new to throw std::bad_alloc; there are no
 * exceptions here, so it aborts instead, which is what -fno-exceptions already
 * does for any other throwing operation. */
extern "C" void *__msil_cxx_alloc(size_t n, size_t align) {
  /* malloc returns storage aligned for any fundamental type, which covers
   * every alignment up to the compiler's default new alignment; a stricter
   * request has no portable way to be satisfied without an aligned allocator,
   * so it is refused rather than silently under-aligned. */
  if (align > MSIL_CXX_DEFAULT_ALIGNMENT)
    __msil_cxx_abort("over-aligned operator new is not supported");
  void *p = malloc(n ? n : 1);
  if (!p)
    __msil_cxx_abort("operator new failed");
  return p;
}

extern "C" void *__msil_cxx_alloc_nothrow(size_t n, size_t align) {
  /* The nothrow forms return null instead of aborting, which is their whole
   * purpose; an unsatisfiable alignment is a null too. */
  if (align > MSIL_CXX_DEFAULT_ALIGNMENT)
    return 0;
  return malloc(n ? n : 1);
}

extern "C" void __msil_cxx_free(void *p) {
  /* C++ allows delete(nullptr), so the check is part of the contract. */
  if (p)
    free(p);
}

void *operator new(size_t n) { return __msil_cxx_alloc(n, MSIL_CXX_DEFAULT_ALIGNMENT); }
void *operator new[](size_t n) { return __msil_cxx_alloc(n, MSIL_CXX_DEFAULT_ALIGNMENT); }
void operator delete(void *p) noexcept { __msil_cxx_free(p); }
void operator delete[](void *p) noexcept { __msil_cxx_free(p); }
void operator delete(void *p, size_t) noexcept { __msil_cxx_free(p); }
void operator delete[](void *p, size_t) noexcept { __msil_cxx_free(p); }

void *operator new(size_t n, const std::nothrow_t &) noexcept {
  return __msil_cxx_alloc_nothrow(n, MSIL_CXX_DEFAULT_ALIGNMENT);
}
void *operator new[](size_t n, const std::nothrow_t &) noexcept {
  return __msil_cxx_alloc_nothrow(n, MSIL_CXX_DEFAULT_ALIGNMENT);
}
void operator delete(void *p, const std::nothrow_t &) noexcept { __msil_cxx_free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { __msil_cxx_free(p); }

void *operator new(size_t n, std::align_val_t al) {
  return __msil_cxx_alloc(n, msil_align_value(al));
}
void *operator new[](size_t n, std::align_val_t al) {
  return __msil_cxx_alloc(n, msil_align_value(al));
}
void operator delete(void *p, std::align_val_t) noexcept { __msil_cxx_free(p); }
void operator delete[](void *p, std::align_val_t) noexcept { __msil_cxx_free(p); }
void operator delete(void *p, size_t, std::align_val_t) noexcept { __msil_cxx_free(p); }
void operator delete[](void *p, size_t, std::align_val_t) noexcept { __msil_cxx_free(p); }

/* --- Local static initialisation ---------------------------------------- */

/* Itanium: guard variables are byte-sized and hold one of three states. */
extern "C" int __cxa_guard_acquire(uint64_t *guard) {
  /* A single-threaded program never has to wait, and the CLR gives this module
   * no portable mutex without a threading reference. The states are still
   * checked so a re-entrant initialisation (the initializer referring to its
   * own static) is caught rather than looping. */
  if (*(volatile uint8_t *)guard == MSIL_GUARD_DONE)
    return 0;
  if (*(volatile uint8_t *)guard == MSIL_GUARD_PENDING)
    __msil_cxx_abort("recursive local static initialisation");
  *(volatile uint8_t *)guard = MSIL_GUARD_PENDING;
  return 1;
}

extern "C" void __cxa_guard_release(uint64_t *guard) {
  *(volatile uint8_t *)guard = MSIL_GUARD_DONE;
}

extern "C" void __cxa_guard_abort(uint64_t *guard) {
  *(volatile uint8_t *)guard = MSIL_GUARD_EMPTY;
}

/* MSVC: the guard is a thread-safe-init epoch counter and the runtime is three
 * calls around the initializer. The protocol the front end emits is:
 *
 *     if (load(guard) > epoch) { _Init_thread_header(&guard);
 *       if (guard == -1) { <initializer>; atexit(dtor);
 *                          _Init_thread_footer(&guard); } }
 *
 * -1 means "in progress", 0 means "done". The epoch rises with each completed
 * initialisation so the comparison tells an initialized guard from an
 * untouched one. */
/* A definition, not just a declaration: the front end declares this as an
 * external thread-local and the writer rewrites that to a plain external
 * global, so this module is the one that has to provide the storage.
 *
 * It starts at -1 and only ever decreases. A fresh guard is 0, so the emitted
 * `guard > epoch` test is true for every uninitialized static; a guard that has
 * been initialized holds the epoch value it completed at, so the same test is
 * false for it forever after. */
extern "C" int _Init_thread_epoch = -1;

extern "C" void _Init_thread_header(int32_t *guard) {
  /* Claim it: the caller's next load sees -1 and runs the initializer. A
   * guard already at -1 is another thread inside it; with no threads to
   * synchronise there is nothing to wait for, so it is treated as recursive. */
  if (*guard == MSIL_MSVC_GUARD_BUSY)
    __msil_cxx_abort("recursive local static initialisation");
  *guard = MSIL_MSVC_GUARD_BUSY;
}

extern "C" void _Init_thread_footer(int32_t *guard) {
  /* Stamp the guard with the new epoch so it compares less than or equal to it
   * from now on, and make the epoch lower still so any *other* static's fresh
   * guard (0) compares greater and gets initialized. */
  _Init_thread_epoch--;
  *guard = _Init_thread_epoch;
}

/* --- Exit-time destructors ---------------------------------------------- */

/* One table for the whole image. Registration order is the order the front end
 * emits atexit calls in, which is construction order; running the table
 * backwards is therefore destruction order, as the standard requires. */
namespace {
struct DtorEntry {
  void (*fn)(void *);
  void *arg;
};
constexpr unsigned kMaxDtors = 512;
DtorEntry Dtors[kMaxDtors];
unsigned NumDtors;
} // namespace

extern "C" int __cxa_atexit(void (*fn)(void *), void *arg, void *dso) {
  (void)dso; // one image, so the handle cannot distinguish anything
  if (NumDtors == kMaxDtors)
    return -1;
  Dtors[NumDtors].fn = fn;
  Dtors[NumDtors].arg = arg;
  NumDtors++;
  return 0;
}

/* atexit's callback takes no argument while __cxa_atexit's takes one; the
 * MSVC protocol above calls atexit directly for a local static's destructor,
 * so both spellings register the same way. */
extern "C" int atexit(void (*fn)(void)) {
  typedef void (*dtor_t)(void *);
  return __cxa_atexit(reinterpret_cast<dtor_t>(fn), 0, 0);
}

/* Run every registered destructor, newest first. Called once from the module's
 * teardown hook (see MSILWriter::printModuleStartup). */
extern "C" void __msil_cxx_finalize() {
  while (NumDtors) {
    DtorEntry &E = Dtors[--NumDtors];
    E.fn(E.arg);
  }
}

/* __cxa_finalize is the standard spelling, called with a null handle to mean
 * "everything". A non-null handle would name one image, and there is only this
 * one, so it is treated the same. */
extern "C" void __cxa_finalize(void *dso) {
  (void)dso;
  __msil_cxx_finalize();
}

/* --- Misc runtime ------------------------------------------------------- */

/* A pure virtual call reaches here from an abstract class's slot. */
extern "C" void __cxa_pure_virtual() {
  __msil_cxx_abort("pure virtual method called");
}

extern "C" void __cxa_deleted_virtual() {
  __msil_cxx_abort("deleted virtual method called");
}

extern "C" void __cxa_bad_cast() { __msil_cxx_abort("bad cast"); }
extern "C" void __cxa_bad_typeid() { __msil_cxx_abort("bad typeid"); }

/* libc++'s hardened assertions and its abort path both land here. The message
 * is a printf format; without a va_list this cannot interpret it, so the
 * format is written verbatim, which for the one shape libc++ passes ("%s",
 * message) is the message itself. */
extern "C" void __libcpp_verbose_abort(const char *format, ...) {
  __msil_cxx_abort(format);
}

/* The C++ runtime cannot call abort() itself: that lives in the C library,
 * which may not be linked. This is the one place the two runtimes meet. The
 * message is fixed, not formatted. */
extern "C" void __msil_cxx_abort(const char *what) {
  if (what && what[0]) {
    const char *s = what;
    size_t n = 0;
    while (s[n])
      n++;
    __msil_write("C++ runtime error: ", 19);
    __msil_write(s, n);
    __msil_write("\n", 1);
  }
  abort();
}
