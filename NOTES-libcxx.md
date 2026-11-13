# libc++ on llvm-msil

Working notes for running LLVM's libc++ and its test suite through the backend.
Everything below was set up on 2026-11-12 against the LLVM 24 trunk in
`x:\source\dotnet\llvm-project` (commit 950785cf3aad).

## Shape

Tests are compiled, linked and translated on Windows and executed by .NET 9
inside WSL Ubuntu 24.04, against the real glibc 2.39.

* **Compiler:** the installed clang 23.1.5, emitting bitcode. LLVM only
  promises forward compatibility for bitcode, so LLVM 24's reader takes clang
  23's bitcode; textual IR from 23 failed LLVM 24's verifier on alias-scope
  metadata.
* **Target:** `x86_64-unknown-linux-gnu`, `-fno-pic -mlong-double-64`.
* **Sysroot:** WSL's `libc6-dev` and `linux-libc-dev` headers (11 MB,
  1537 files), copied with symlinks dereferenced.
* **libc++ and libc++abi:** built with the `runtimes` CMake project from the
  same tree, cross-compiled with `-flto` so the archives hold bitcode. The
  libc++abi members `cxa_exception`, `cxa_exception_storage`,
  `cxa_personality` and `fallback_malloc` are left out. MSIL.Runtime provides
  that surface, since the personality and unwinder model can't exist on the
  CLR. Everything else, including `private_typeinfo.cpp`, `cxa_guard`,
  `cxa_handlers` and `stdlib_new_delete`, is linked into one `libcxx.bc`
  together with `libc++experimental` (tzdb, `__log_hardening_failure`), the
  `long double` shims below, and compiler-rt's `divsc3`/`divdc3`/`mulsc3`/
  `muldc3` and `atomic.c` (the lock-based `__atomic_*` libcalls) as bitcode.
  `link-libcxx.sh` in the scratch `cxx-bc` folder rebuilds it.
* **Per test:** the test's bitcode is linked `--only-needed` against
  `libcxx.bc`, then run through llvm-msil with `-msil-lib=libm.so.6` and
  `-msil-lib=libc.so.6` (copies of WSL's, so the writer reads their real
  export tables) and assembled by the Framework ilasm. `MSIL.Runtime.dll` and
  a runtimeconfig go next to the exe.
* **lit:** libc++'s own suite and config machinery, with `compiler` and
  `executor` overridden through `LIBCXX_TEST_PARAMS`. `%{cxx}` is
  `shlex.quote`d by libc++, so the compiler has to be a single executable: a
  small native launcher runs `msil-cxx.py` and forwards the raw command line.
  The executor runs `wsl.exe --cd <execdir> -- timeout 300 dotnet <exe>`. The
  timeout has to be inside WSL, because killing `wsl.exe` leaves the Linux
  process running.

## Why `long double` is 64-bit

`long double` is x87 80-bit on this ABI. The writer rejects `x86_fp80`, and
`-msil-fp80=double` still rejects a `long double` passed to a vararg, which
libc++'s `num_put<long double>` does through `snprintf("%Lf")`. Every
iostream program pulls that in through the facet vtables. So everything is
built with `-mlong-double-64`.

glibc's own `strtold` family still returns in x87 `st0`, hence the shim.
The `l`-suffixed libm functions take x87 values too, so `ldouble_math.c`
defines the C99 ones over their double versions, plus `nexttoward` and
`nexttowardf`, whose second parameter is a `long double`. A `%Lf` format
still goes to glibc's printf with the wrong ABI and prints garbage; expect
those tests to fail.

## Bugs found on the way

Each of these was a miscompile or a crash in code that already existed. All
are fixed, with tests:

* `atomicrmw add`/`sub` returned the new value (`Interlocked.Add`'s
  semantics) instead of the old one. Every `fetch_add` was off by its
  operand. libc++ hit it in `locale::id::__get`: facet ids came out one too
  high, so `locale::classic()` grew its `__sso_allocator` vector. That growth
  path is broken in libc++ itself since upstream 2c1decb7f766, which copies
  the allocator by value into `__split_buffer`, and freed the in-object
  buffer. Native libc++ never takes that path, because its ids start at 0
  and 30 facets fit exactly.
* Vector splats arrive as vector-typed `ConstantInt`/`ConstantFP` on newer
  LLVM and were emitted as one scalar. That's invalid IL, and the JIT crashed
  in `genCodeForCpBlkUnroll`, not rejecting it.
* The vector `fcmp` lane path didn't reload operands for the ordered and
  unordered NaN checks, so the stack underflowed.
* `fcmp ole`/`oge` were true for NaN.
* `Vector<T>` `le`/`ge` mapped to strict compares, unsigned predicates used
  signed `Vector` compares, and the `!=` mask used a nonexistent
  `Vector.op_ExclusiveOr`.
* Vector `sext` from i1 gave 1, not -1, and `zext` into 64-bit lanes
  sign-extended.
* Vector bitcasts to or from a different type reloaded the source unchanged,
  including `<N x i1>` packing to and from integers.
* Integers of 65 to 127 bits were truncated to `int64`. InstCombine makes
  `sadd.with.overflow.i65` from a mixed-sign `__builtin_add_overflow`, which
  libc++'s `num_get` uses.
* `llvm.global_ctors` ignored priority, so test constructors would have run
  before libc++'s iostreams init at priority 100.
* Float signaling-NaN constants came out quiet. The JIT holds float
  constants as doubles, so `ldc.r4` of an sNaN loses the signaling bit; they
  now go through an `int32` reread from memory.

Found by the first full run:

* Constant globals ignored their alignment. A constant's storage is its
  `.data` image, and ilasm packs those back to back, so a `const double`
  came out at an address 2 mod 8 (checked with a test program printing
  `(uintptr_t)p % 8`). glibc's SIMD `wcslen` and `wmemcmp` assume a 4-byte
  aligned `wchar_t*`, which explains about 40 `wchar_t` string and stream
  tests failing their asserts. Over-aligned writable globals (`alignas(64)`)
  were only malloc-aligned as well.
* Integers whose store size is 3, 5, 6 or 7 bytes (`i24`, `i40`, `i48`,
  `i56`) were fatal in memory. clang coerces a 3- or 6-byte struct to one at
  -O0, so 97 tests hit it. Signatures also printed `int24`, `int13` and
  `int28`, which ilasm rejects.
* A `ptrtoint` of a function as a leaf of an aggregate constant, which is
  every member function pointer constant, was fatal (about 90 tests).
* `atomicrmw` `min`/`max`/`umin`/`umax` were fatal.
* The `i128` overflow and saturating intrinsics were expanded through `i256`,
  which produced invalid IL (`saturating_add`/`sub`/`mul`, `charconv`
  integral).
* `llvm.frexp` and `llvm.modf` had no lowering.
* ilasm rejects names over 1023 characters; deeply nested template
  manglings (`tuple` recursion depth, `zip_transform`) exceed that.
* The writer's comment claimed glibc 2.34 merged libm into libc. It merged
  libpthread, libdl and librt; libm is still separate, and `atan2`,
  `lgamma_r` and `hypotf` were `EntryPointNotFoundException`s from
  `libc.so.6`. `-msil-lib` now reads an ELF shared object's dynamic symbols.
* Every multi-threaded test segfaulted (119). gdb on a `vector<thread>`
  repro showed the fault in `libcoreclr.so` under glibc's
  `__nptl_deallocate_tsd`: libc++'s `thread.cpp` registers a
  `pthread_key_create` destructor, which is a managed method, and glibc runs
  it after the runtime has detached the exiting thread. The start routine
  had the same problem in principle: glibc called an `ldftn` pointer on a
  thread the runtime never saw. The runtime's `pthread_create` now starts the
  thread through an `[UnmanagedCallersOnly]` trampoline that calls the
  routine. When it returns, the trampoline runs that thread's `thread_local`
  destructors (`__cxa_thread_atexit_impl` used to queue them for process
  exit) and then the key destructors, which are kept in the runtime rather
  than given to glibc.

## CLR and JIT facts learned

* An invalid stack type (a `float64` stored into a struct local) crashes the
  JIT on .NET 9, 10.0.13 and 11 rc2 rather than raising
  `InvalidProgramException`. A stack underflow does raise it.
* `ilverify` stops at the first `localloc` in a method, so it can't vet our
  IL past the prologue.
* `RuntimeHelpers.PrepareMethod` over every global method finds which method
  kills the JIT. `llvm-extract` plus `llvm-reduce` with an "it still fails to
  JIT" test then reduces it.
* On Linux, `DOTNET_DbgEnableMiniDump=1 DOTNET_EnableCrashReport=1` writes a
  JSON crash report with managed frames on SIGABRT. That's how the
  `__sso_allocator` free was found.
* The .NET Framework ilasm can't write output past MAX_PATH; the wrapper
  assembles into a short temp path and moves the result.

## Known gaps

* 16-byte atomics: atomic load/store and the locked `cmpxchg`/RMW path all
  assume `ldind`, so `has-128-bit-atomics` probes false.
* `_BitInt` wider than 128 bits is a fatal error.
* `<N x i128>` vectors are fatal. libc++'s vectorized `find` makes
  `<1 x i128>` for `__int128` elements, and `experimental::simd` tests use
  them throughout (21 tests).
* Native code calling a managed function pointer on a thread the CLR doesn't
  know. `pthread_create`, `pthread_key_create` and `pthread_key_delete` are
  bound to MSIL.Runtime in modules that use it, which covers `std::thread`
  (see below). A plain C program with pthreads, or any other native API that
  calls back on its own thread, still runs managed code unregistered.
* Static destructors run from `ProcessExit`. A test that leaves through
  glibc's `exit()` presumably skips them.
* `libc++.x.dll`, a shared library assembly, needs cross-assembly imports,
  data symbol accessors and name-compared RTTI. Not started.

TODO(wam): results of the full run
