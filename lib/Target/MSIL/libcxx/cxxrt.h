/* Freestanding environment for the MSIL C++ runtime.
 *
 * Compiled with -ffreestanding -nostdinc -nostdinc++, so this file depends on
 * nothing but the compiler: no <new>, no <cstddef>, no C library headers. The
 * few C functions the runtime uses are declared here with the same names the
 * portable libc provides, so calls bind to it by name.
 *
 * The allocation operators are ordinary C++ definitions in cxxrt.cpp. Because
 * the build compiles this file once per target triple, their mangled names come
 * out right on their own: `operator new` is `_Znwm` for an ELF triple and
 * `??2@YAPEAX_K@Z` for a Windows one, which is exactly the symbol the front end
 * emits a call to for that triple. Spelling the labels by hand would be a
 * second source of truth for the same fact.
 */
#ifndef MSIL_CXXRT_H
#define MSIL_CXXRT_H

typedef __SIZE_TYPE__ size_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INT32_TYPE__ int32_t;
typedef __UINT8_TYPE__ uint8_t;

#define NULL ((void *)0)

/* malloc/free are the C library's, reached by name so the allocator hook and
 * the IL allocator stay in effect. abort is the front end's trap path. */
extern "C" void *malloc(size_t);
extern "C" void free(void *);
extern "C" void abort(void);
extern "C" void __msil_write(const void *, size_t);

/* Abort with a diagnostic. Defined in the runtime; every other function here
 * may call it. */
extern "C" void __msil_cxx_abort(const char *what);

/* Guard states. Itanium uses byte values; MSVC uses an epoch counter whose
 * busy marker is -1 and done marker is 0. */
#define MSIL_GUARD_EMPTY 0
#define MSIL_GUARD_PENDING 1
#define MSIL_GUARD_DONE 2
#define MSIL_MSVC_GUARD_BUSY (-1)
#define MSIL_MSVC_GUARD_DONE 0

/* The alignment malloc is guaranteed to satisfy, matching the compiler's
 * default new alignment on the supported targets. */
#define MSIL_CXX_DEFAULT_ALIGNMENT 16

/* The two types the C++17 placement forms take. Declaring them here with the
 * same definitions libc++ gives them makes this file's `operator new(size_t,
 * std::align_val_t)` mangle identically to the declaration the front end saw,
 * which is what makes the call resolve.
 *
 * The two ABIs spell align_val_t differently and that changes the mangling: an
 * ELF libc++ makes it a scoped enum while the Windows ABI uses a struct, to be
 * ABI-compatible with vcruntime. libc++ itself branches on the same fact, so
 * this file does too. */
namespace std {
#if defined(_WIN32)
struct align_val_t {
  size_t _Val;
};
#else
enum class align_val_t : size_t {};
#endif
struct nothrow_t {
  explicit nothrow_t() = default;
};
} // namespace std

/* The numeric value of an align_val_t under either definition. */
inline size_t msil_align_value(std::align_val_t al) {
#if defined(_WIN32)
  return al._Val;
#else
  return static_cast<size_t>(al);
#endif
}

/* The allocation primitives both spelling families funnel through, so there is
 * one place the allocator is chosen. */
extern "C" void *__msil_cxx_alloc(size_t n, size_t align);
extern "C" void *__msil_cxx_alloc_nothrow(size_t n, size_t align);
extern "C" void __msil_cxx_free(void *p);

/* Global allocation operators. The ones a freestanding C++ program can reach:
 * plain, array, sized delete (C++14), nothrow (C++11) and aligned (C++17). */
void *operator new(size_t n);
void *operator new[](size_t n);
void operator delete(void *p) noexcept;
void operator delete[](void *p) noexcept;
void operator delete(void *p, size_t n) noexcept;
void operator delete[](void *p, size_t n) noexcept;
void *operator new(size_t n, const std::nothrow_t &) noexcept;
void *operator new[](size_t n, const std::nothrow_t &) noexcept;
void operator delete(void *p, const std::nothrow_t &) noexcept;
void operator delete[](void *p, const std::nothrow_t &) noexcept;
void *operator new(size_t n, std::align_val_t al);
void *operator new[](size_t n, std::align_val_t al);
void operator delete(void *p, std::align_val_t al) noexcept;
void operator delete[](void *p, std::align_val_t al) noexcept;
void operator delete(void *p, size_t n, std::align_val_t al) noexcept;
void operator delete[](void *p, size_t n, std::align_val_t al) noexcept;

#endif
