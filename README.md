# MSIL backend

`llvm-msil` translates an LLVM module into CIL text (`.il`) that Microsoft's
`ilasm` assembles into a .NET assembly.

## Layout

This is an out-of-tree project, discovered by `llvm/projects/CMakeLists.txt`
the same way `llvm-cbe` is. It is not a member of `LLVM_TARGETS_TO_BUILD`, so
nothing is registered in LLVM's `Targets.def` and `llc -march=msil` does not
exist; `llvm-msil` is the only entry point. That is deliberate: the backend
is far more than a machine code emitter, and the `llc` path only ever
duplicated it.

```
llvm/projects/llvm-msil/
  lib/Target/MSIL/     target machine and writer
  tools/llvm-msil/     the driver
  test/                lit suite, llvm-cbe port, and end-to-end programs
```

## Building standalone

The tree builds in-place like any other LLVM project, but rebuilding LLVM to
pick up a change to the backend costs half an hour. `CMakeLists.txt` is
therefore dual-mode: point it at an existing LLVM build and it links against
the exported libraries in about six seconds.

```
cmake -S . -B build -G Ninja \
      -DLLVM_DIR=<llvm-build>/lib/cmake/llvm
cmake --build build -j 8
./test/run_tests.sh ./build/tools/llvm-msil/llvm-msil.exe
```

The build type has to match LLVM's or linking fails with `LNK2038`, so it is
read from LLVM's config rather than defaulted; `project()` fixes the compiler
flags, which is why the CMake file probes the build type before creating the
real project.

### Building on Linux

Same commands; the LLVM build can be the release tarball, which carries a CMake
config, `llvm-config` and the static libraries the backend links:

```
curl -fsSLO https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/LLVM-22.1.8-Linux-X64.tar.xz
tar xf LLVM-22.1.8-Linux-X64.tar.xz            # ~/llvm22/LLVM-22.1.8-Linux-X64
cmake -S . -B build -G Ninja -DLLVM_DIR=~/llvm22/LLVM-22.1.8-Linux-X64/lib/cmake/llvm
cmake --build build -j 8
```

Five things the release tarball does not give you, and where each comes from:

* **FileCheck, `not` and `count`.** The release script builds LLVM with
  `LLVM_INSTALL_UTILS` off, so no archive has them. Either take the
  distribution's (`apt-get install llvm-18-tools` puts them in
  `/usr/lib/llvm-18/bin`) and pass `LLVM_TOOLS_DIR` to the gate, or build them
  from the tagged sources; any LLVM version's FileCheck works, since the
  backend only needs them for the lit suite.
* **lit.** `pip install lit`. `run_tests.sh` finds it on `PATH`.
* **ilasm.** Not in LLVM at all: take `runtime.linux-x64.Microsoft.NETCore.ILAsm`
  from nuget, which carries `runtimes/linux-x64/native/ilasm`.
* **dotnet.** The assembled programs run on the CLR, so a .NET 9 runtime has to
  be on `PATH` (`dotnet-install.sh` is the shortest route).
* **The C runtime.** `zlib1g-dev`, `libzstd-dev` and `libxml2-dev` are what
  `find_package(LLVM)` resolves; without them configure fails on a missing
  `ZLIB::ZLIB`.

Then:

```
LLVM_TOOLS_DIR=/usr/lib/llvm-18/bin CLANG=~/llvm22/.../bin/clang \
ILASM=~/tools/bin/ilasm PYTHON=python3 \
  ./test/run_tests.sh ./build/tools/llvm-msil/llvm-msil
```

`CLANG` compiles the test programs and must be the same LLVM version the backend
links against. The programs compile for `x86_64-unknown-linux-gnu`, so their
`long` is 64-bit; the portable C library is built twice, once per ABI, and the
program's triple picks the copy, so the two agree.

The scripts read `uname` and pick `.exe` output and `dotnet` as the launcher;
the Windows path is unchanged. `.github/workflows/ci.yml` runs this sequence on
both `ubuntu-latest` and `windows-latest`, building the missing FileCheck tools
from the tagged sources rather than assuming a distribution's.

## Usage

```
clang -S -emit-llvm -O1 -o prog.ll prog.c
llvm-msil prog.ll -o prog.il
ilasm -exe -output:prog.exe prog.il
dotnet prog.exe
```

`-exe`/`-output:` is the .NET Core `ilasm` spelling and works on both hosts;
the PowerShell `.NET Framework` assembler wants `/exe`/`/output:` instead.

`dotnet prog.exe` needs a `prog.runtimeconfig.json` next to the assembly naming
`Microsoft.NETCore.App`; without it `dotnet` cannot find the framework. On
Windows a bare `./prog.exe` also runs, binding the .NET Framework where
`mscorlib` resolves, and there it needs no runtimeconfig (on Linux there is no
launcher for a managed PE, so `dotnet` is the only way). Types this backend
emits (`System.Int128`, `System.Numerics.Vector<T>`, `System.Math`) are modern
.NET types, so the runtimeconfig is what you want in practice.

The input IR needs a target triple with a data layout, or the writer cannot
size any aggregate type.

The IL is the same whichever host emitted it, but three choices do follow the
module's *triple* rather than the host: which library an unbound import pinvokes
(`msvcrt.dll`/`ucrtbase.dll` for a Windows triple, `libc.so.6` for ELF), whether
a vararg `double` crosses as its raw bits (Windows x64's shadow slot) or as a
`float64` (SysV's SSE register), and which of the two portable C library builds
is linked. A Windows triple on Linux and an ELF triple on Windows are both
consistent; what is not is mixing a program compiled for one ABI with a library
built for the other, which is why the libc is built twice (see below). Runtime
support is a separate axis again: a managed vararg method, and therefore the
portable `printf`, runs only on Windows (see "Known limitations").

## Options

| Option | Effect |
|---|---|
| `-msil-class-name=Ns.Type` | Emit module members inside this class instead of `<Module>`, so C# can call them without reflection. |
| `-msil-libc=builtin` | Link the portable C library into the module: `printf`/`snprintf`/`sprintf`/`puts`/`putchar`, `atoi`/`strtol`/`strtoul` and the rest of the conversion family, `abs`/`div`/`qsort`/`bsearch`/`rand`, the `str*` and `mem*` family, and `<ctype.h>`. Written in C (ported from musl), compiled to IR at build time, and linked selectively, so only what the program reaches is emitted. `malloc` and friends still come from the IL subset, which is backed by `NativeMemory`. |
| `-msil-libc=il` | Emit the smaller hand-written IL subset instead: `malloc`/`free`/`calloc`/`realloc` over `NativeMemory`, plus `memcpy`/`memset`/`memcmp` and the `str*` family, but no `printf`. For a module that must link no C in. |
| `-msil-libc=<asm>[:<type>]` | Bind the same calls to a managed assembly you supply (type defaults to `MSIL.Libc`). |
| `-msil-alloc=<asm>:<type>` | Route `malloc`/`free`/`calloc`/`realloc` to methods of that name on your type, so unmanaged allocations can be instrumented. |
| `-msil-math=0` | Do not bind libm to `System.Math`/`System.MathF`; pinvoke the CRT instead. |
| `-msil-lib-map='SDL_*=SDL2.dll'` | Bind matching imports to a native library. `*` matches any suffix, and the option repeats. |

## How types map

| IR | IL |
|----|-----|
| `i1` | `bool` |
| `i2`, `i8`, `i16`, `i32` | `int2`, `int8`, `int16`, `int32` in globals and signatures; `int32` in locals |
| `i33` ... `i64` | `int64` |
| `i128` | `System.Int128` from `System.Runtime` |
| `float`, `double` | `float32`, `float64` |
| pointers | unmanaged `void*` |
| structs, arrays | explicit-layout value classes (`valuetype`, byte-exact `.size`) |
| a vector one SIMD register wide | `System.Numerics.Vector<T>` |
| any other vector | explicit-layout value class (a byte blob) |

Narrow integers are normalized to their declared width at every store and
compare, so unsigned comparisons see the right high bits.

## Signed narrow integers

Because a stored narrow integer is held zero-extended, every *signed* use has
to restore the sign first: `sitofp`, `sdiv`, `srem` and `ashr` emit
`conv.i1`/`conv.i2` on their operand. Without it a negative `i8`/`i16` reads as
a large positive number.

Likewise, `stloc` into a local of a different type *converts*; it does not
reinterpret. A bitcast between integer and float therefore goes through memory
of the source type (`stloc` / `ldloca` / `conv.u8` / `ldind.<dst>`), not
through a local of the destination type.

## 128-bit integers

`i128` maps to `System.Int128` from `System.Runtime`; the assembly reference is
emitted only for modules that use it, since a framework reference needs the
real `.ver`/public key token.

`Int128` is an ordinary value type, so it lives in a local and moves with
`ldobj`/`stobj`. It has no IL arithmetic at all: every operator is a static
method (`op_Addition`, `op_Multiply`, `op_Division`, `op_ShiftLeft`,
`op_Equality`, `op_LessThan`, ...), and comparisons return `bool`. Constants
come from `op_Implicit(int64)`, or the `(uint64 upper, uint64 lower)`
constructor when the value needs both halves.

Unsigned operations use `UInt128`: `udiv`, `urem` and unsigned compares
reinterpret with `Int128::op_Explicit` and back with `UInt128::op_Explicit`.
Signed shifts use `op_RightShift`; `lshr` uses `op_UnsignedRightShift`. Shift
counts become `int32`, which the methods reduce modulo 128 — the same thing
LLVM's shift semantics do with an oversized count.

## Vectors

A vector whose total width is exactly one 32-byte SIMD register and whose
lanes are at least 32 bits wide uses `System.Numerics.Vector<T>`, so
arithmetic, divides, shifts and compare-plus-select run on the hardware. That
means `<8 x i32>`, `<4 x i64>`, `<4 x double>` and `<8 x float>`; a
`<4 x float>` is only half a register and takes the lane path.

Every other shape — a half register, an odd lane count, an 8- or 16-bit lane,
or an operation `Vector<T>` has no helper for (`srem`) — keeps the byte-blob
expansion: the vector is an explicit-layout value class and each lane is
addressed and operated on individually. That path is always correct, just not
vectorized. Narrow lanes are excluded because the CLR has no 8- or 16-bit SIMD
arithmetic; `Vector<T>` would have to widen lane by lane anyway.

`Vector<T>` is a *struct*, and must be declared `valuetype`; declaring it
`class` produces a `TypeLoadException` the first time the method runs.

## Structs

A struct becomes an explicit-layout value class whose fields are declared at
the offsets the LLVM data layout chose (`.pack 1`, `.size N`, `.field [off]`),
so the layout lives in one place.

A load or store through `getelementptr %S, ptr %p, 0, N` compiles to
`ldfld`/`stfld` on that class rather than pointer arithmetic, but only when the
access type is exactly the field's type. A narrower access through the same
address (`load i32` from the start of an `{ i32, i32 }` member) is a different
value with a different stack type and falls back to the pointer path; emitting
`ldfld` of the declared type there pushes a struct where an `int32` is
expected, and the JIT rejects the method.

## Globals

An initialized global is emitted twice: a read-only template class holding the
initializer bytes, and a `void*` static field. `$MSIL_Init` (called from
`$MSIL_Startup`) allocates the real storage and `cpblk`s the template into it.

A `constant` global skips the copy entirely: `$MSIL_Init` points the field
straight at its template, since nothing can store through it. A store would
fault rather than silently corrupt the image, which is the correct behaviour.
A zero-sized global gets no template but still gets its own allocation, since
LLVM gives every global a distinct address.

### Thread-locals

A `thread_local` global keeps the template and the `void*` field, but the
field carries `[ThreadStatic]`, so the pointer itself is per thread. There is
no one-time initialization: `$MSIL_Init` skips the variable, and each thread's
first reference goes through a generated `'<name>$tls'()` accessor that
allocates that thread's copy, copies the template in, patches its pointer
initializers, stores the slot and returns it. Reads and writes then work as
for any other global. Nothing is locked: a slot is only ever read or written
by the thread that owns it, so two threads cannot race on one.

A `constant thread_local` needs no allocation at all: every thread's slot
points at the one read-only image, and a store through it faults. A
declaration-only thread-local is a fatal error rather than silently aliasing
every thread onto one address, since the storage it names is in another
module and this one has no way to reach a given thread's copy.

## The portable C library

`-msil-libc=builtin` gives a program a real C library: `printf` and the rest of
stdio, the conversion family (`atoi`, `strtol`, `strtoul`, ...), `qsort` and
`bsearch`, the string and character functions. It is written in plain C in
`lib/Target/MSIL/libc/`, mostly ported from
[musl](https://github.com/kraj/musl) so that its behaviour is a known quantity
rather than something invented here.

Nothing that needs a syscall, a file, a locale or an OS interface is in it: a
`fopen` or a `time()` still pinvokes the CRT. What remains is the part of libc
that can be expressed in pure C, which is the part a portable program leans on.

### How it gets into the module

The C is compiled to IR at build time, twice: once for the Windows ABI and once
for SysV. `sizeof(long)` and `wchar_t` are what differ, and they are in the
public signatures (`strtol` returns a 32-bit `long` on Windows and a 64-bit one
on Linux), so a program compiled for one ABI cannot link the other's copy. Both
builds are embedded in the backend as string literals, and `linkPortableLibc`
picks by the module's triple. At translate time the chosen copy is linked in
with `LinkOnlyNeeded`, which follows references from the program and skips
everything else: a program that never calls `qsort` does not carry it. The
linked functions then go through the same lowering and emission as user code.

That ordering is why `lib/Target/MSIL/libc/*.c` is the source of truth. The
build compiles it whenever it can find a clang that targets both; a build that
cannot has no SysV copy, and `lib/Target/MSIL/libc.ll` is checked in as the
Windows-ABI fallback for it, with `scripts/update-libc-ir.sh` regenerating that
file. Change the C, run the script, and the checked-in copy stays in step.

Whether the library *runs* is a different question from whether it links: its
`printf` is a managed vararg method, which CoreCLR supports on Windows and
rejects on Linux, so `-msil-libc=builtin` is usable only there. A plain program
that calls the CRT's `printf` through a pinvoke thunk is unaffected on either
host.

### The vararg seam

The one thing C cannot express is reading a vararg argument, because the CLR's
`va_list` is a `System.ArgIterator` and not a C pointer. So `libc.h` declares
three accessors -- `__msil_va_i64`, `__msil_va_f64`, `__msil_va_ptr` -- plus
`__msil_write` for output, and the backend emits them as IL. `printf` is
ordinary C above that seam.

Three details make it work, and each is a place where the obvious thing fails:

* The accessor takes the **address** of the caller's `va_list`, not its value.
  Reading an argument advances the cursor, and the cursor lives in the
  caller's slot: `va_start` stored the address of the `ArgIterator` there.
* The iterator is copied out of that address, read, and copied back, because
  an unmanaged pointer cannot be held in a `byref` local -- the JIT rejects
  the method.
* Each argument crosses as its CLR type, not its C type: `int64` for any
  integer, `float64` for a `double`'s raw bits, `void*` for a pointer. The
  typed `GetNextArg(RuntimeTypeHandle)` overload matches on the exact type, so
  there is one accessor per CLR type; a pointer has no type token at all, so it
  is read through `object` and unboxed as `UIntPtr`, which is how a `void*`
  boxes.

There is no `int32` accessor because the call site narrows to one CLR type per
slot. A vararg `int` is widened to `int64` on the way in (rather than left as
`int32`), because the typed read has to know which of the two shapes to expect
and `printf` cannot know which the caller chose. `%f` reads the `float64` bits
through the `int64` overload for the same reason: the backend already sends a
vararg `double` as an `int64` (the CRT's `va_arg` reads the general-purpose
shadow slot on Windows x64), so the C side reinterprets the bits.

### Where it deliberately differs

* Output goes to `System.Console`, so a `printf` program is portable off
  Windows. Nothing is buffered: `printf` writes as it formats, which differs
  from a line-buffered C library only in how output interleaves with stderr
  and with a crash.
* There is no locale, so character classes are fixed ASCII and `%ls`/`%lc`
  treat their argument as narrow -- which passes a UTF-8 string's bytes
  through unchanged anyway.
* `errno` does not exist. `strtol` saturates silently on overflow instead of
  setting `ERANGE`; `%m`, `%n$` positional arguments and `%n` are not
  implemented.
* `qsort` is an insertion sort. It is stable and obviously correct, which
  matters more here than the asymptotic advantage; musl's smoothsort is
  available if a program ever sorts enough to notice.
* `%a`/`%A` follow musl exactly, and the C libraries do not agree with each
  other here. Verified against glibc 2.39 and musl 1.2.4 for the same inputs:

  | value             | musl (and this port)      | glibc 2.39                | MSVC CRT                  |
  |-------------------|---------------------------|---------------------------|---------------------------|
  | `1.0`             | `0x1p+0`                  | `0x1p+0`                  | `0x1.0000000000000p+0`    |
  | `DBL_TRUE_MIN`    | `0x1p-1074`               | `0x0.0000000000001p-1022` | `0x0.0000000000001p-1022` |

  Two separate disagreements. MSVC is alone in padding to 13 fraction digits on
  every value. On a subnormal, glibc and MSVC agree with each other and musl is
  alone: musl normalises the value (`frexp` does that on the way in) while the
  other two keep the raw encoding's exponent. The port keeps musl's behaviour
  because musl is what it is ported from, and `test/programs/libc.sh` compares
  the `%a` lines against musl's output as a literal rather than against the
  native build, which would be comparing against MSVC. `libc/tests/compare.sh`
  leaves `%a` out for the same reason.

## Math

libm calls bind to `System.Math`/`System.MathF` rather than pinvoking the CRT,
which is what makes a program built this way portable off Windows. The binding
is by C symbol name, so a direct call to `sqrt()` from a C program goes through
the same path as the `llvm.sqrt` intrinsic.

Three places where the managed and C semantics differ, all handled explicitly:

* `Math.Min`/`Max` propagate NaN; C's `fmin`/`fmax` return the other operand.
  So `llvm.minnum`/`maxnum` and the `fmin`/`fmax` symbols keep their CRT call,
  and `minimum`/`maximum` are expanded to a compare and select.
* C's `round()` rounds half away from zero, `Math.Round` rounds half to even.
  `round` is the one forwarder that passes `MidpointRounding`;
  `rint`/`nearbyint`/`roundeven` map to `Round` with no argument.
* `exp2`, `log2`, `trunc`, `round`, `rint`, `lrint`, `lround`, `scalbn`,
  `copysign`, `fmin` and `fmax` are C99 and exist only in `ucrtbase.dll`,
  while `sqrt`, `sin`, `ldexp`, `sinh` and friends are C89 and live in
  `msvcrt.dll`. The name-based split between the two is checked against the
  real export tables.

`-msil-math=0` turns the binding off and restores CRT pinvokes.

## Importing from a DLL that is not the CRT

An external function with no math or libc binding falls back to
`msvcrt.dll`/`ucrtbase.dll` by a name heuristic, which cannot know about a
third-party library. `-msil-lib-map` names one, matched against the symbol as
the module references it (not the export spelling it is bound to):

```
llvm-msil demo.ll -o demo.il -msil-lib-map 'SDL_*=SDL2.dll'
llvm-msil gpu.ll  -o gpu.il  -msil-lib-map 'vk*=vulkan-1.dll'
```

Library-specific options are consulted first, so a mapped symbol never falls
back to the CRT. The [`sdl_demo.c`](test/programs/sdl_demo.c) program is a
working example: SDL2, native-thread callbacks, function pointers from
`SDL_GL_GetProcAddress`, and a frame checksum compared against a native build.

Nothing in the IR says which DLL an import came from — neither the SDL headers
nor `vulkan.h` expand to `__declspec(dllimport)` by default, and `dllimport`
is not consulted even when present — so this has to be told.

## Atomics

`atomicrmw`, `cmpxchg`, `fence` and atomic loads/stores use the real
primitives: `System.Threading.Interlocked` for int32/int64/native int, a
CompareExchange loop for the operations with no overload (`and`, `or`, `xor`,
min/max, float add/sub), and the `volatile.` prefix for atomic loads and
stores.

Narrow integers (i8/i16) have no Interlocked overload, so they are serialized
on a module-wide monitor. That is atomic against other narrow accesses in the
same assembly, not against native code or a thread bypassing it. See
`NOTES-atomics.md`.

## Tests

`test/` is its own lit suite; `test/run_tests.sh` is the gate, and runs three
things:

* the lit tests, which check emitted IL with `FileCheck`,
* the upstream llvm-cbe suite (C, C++, and `.ll` tests at four optimization
  levels) via `cbe_port.py` — 540 tests, 32 xfail (a Linux run skips two more,
  the casts and C++ vtables noted under "Known limitations"),
* the end-to-end programs in `test/programs/`, each compared against a native
  build of the same source.

The end-to-end programs are the ones that find real bugs; the hand-written IR
tests only cover the cases someone thought to write. They sweep `-O0` to `-O3`,
which matters: the bugs they have caught so far each appeared at a single
level. `int128.sh` and `sdl.sh` are self-contained. `sdl.sh` needs SDL2
development files: set `SDL2_DIR` to a directory with `include/SDL2` and
`lib/SDL2.lib`, and `SDL2_BIN` to `SDL2.dll` if it is not in `lib/`. Without
them the test reports `sdl: skipped` and the gate still passes; it is also
skipped off Windows, since `SDL2.dll` is what it pinvokes.

`sdl3_port.sh` is the other half of that: rather than calling SDL, it compiles
SDL3's own C (210 translation units) to CIL and runs it, so it tests the
backend against a large real codebase rather than against a demo. Set `SDL3_DIR`
to a checked-out SDL3 tree that has been configured once — the script applies
`sdl3-msil.patch` itself if the tree is clean — and it reports `sdl3-port:
skipped` when unset. It takes a few minutes, so it is the slowest thing in the
gate. The retained translation units are SDL's win32 and dummy platform
adapters and the pinvoke list is Windows', so it is skipped off Windows too:
porting SDL's own C is a Windows-ABI exercise.

`libc.sh` is the compatibility test for the portable C library: it runs
`test/programs/libc.c` on the CLR at four optimization levels and diffs the
output against a native build of the same file. A port is only as good as its
agreement with the real thing, so the native build is the reference. During
development the same source also runs natively against the ported `.c` files
rather than the CRT, which separates a portability bug from a translation bug
before the MSIL side is involved. The program's `printf` is the portable one, a
managed vararg method, so the script skips off Windows.

`doom.sh` runs PureDOOM for a fixed number of tics on a virtual clock and
hashes the framebuffer; `stb.sh` exercises stb_image (including a PNG
decode/encode round trip), stb_truetype, stb_rect_pack and stb_ds. Their
sources are third-party, so they are fetched at a pinned commit into
`/tmp/msil-thirdparty` and cached (`MSIL_TEST_CACHE` moves the cache). A
machine with neither the cache nor network reports `skipped` and the gate
still passes. `stb.sh` needs a TrueType font; `STB_TEST_FONT` overrides the
default `C:/Windows/Fonts/arial.ttf`, and on Linux the DejaVu font that the
distribution installs.

The scripts are host-neutral through `test/platform.sh`, which they source. It
picks `.exe` output and the ilasm option spelling, writes the runtimeconfig and
starts assemblies through `dotnet` on both hosts, and on Linux compiles the IR
for glibc and non-PIC (an ELF clang's default PIC turns switch tables into
`llvm.load.relative`, which IL has no equivalent of). Tests that need Windows —
an SDL2.dll pinvoke, MSVC's CRT split — print `skipped` and exit 0 there.

`cbe_port.py` can be driven directly:

```
python -I test/cbe_port.py \
    --cbe-tests ../llvm-cbe/test \
    --clang <clang> --llvm-msil <llvm-msil> --ilasm <ilasm> \
    --lli <lli> --levels 0,1,2,3
```

Both sides of that suite only compare exit codes, so it ports over unchanged.

## Known limitations

* **Varargs.** Calls to external vararg functions (`printf`) work: each call
  signature gets a concrete pinvoke thunk, since `ilasm` rejects `vararg`
  pinvokes. Definitions of vararg functions that forward a `va_list` to a CRT
  internal (the wrappers MSVC's `stdio.h` produces) are dropped and bound to
  the real export instead. The forward may go through another printf-family
  wrapper first (`snprintf` reaches `vsnprintf`), so those are matched by name.
  A vararg floating-point argument crosses the boundary as its raw `int64`
  bits on a Windows target, because the CRT's `va_arg` reads the
  general-purpose shadow slot on Windows x64; an ELF target passes it in an
  SSE register and the CLR pinvoke stub follows that ABI.
* **Managed vararg methods do not run on Linux.** A *definition* of a C vararg
  function is emitted as a `vararg` method with `System.ArgIterator` for
  `va_start`. CoreCLR on Windows runs it; CoreCLR on Linux raises
  `InvalidProgramException: Vararg calling convention not supported` for the
  same assembly (verified with the identical, byte-for-byte IL). That is a
  runtime limitation, not something the backend can emit around: it is why
  `-msil-libc=builtin`'s `printf` works on Windows and not on Linux, and why
  `libc.sh` skips there. A vararg *call* to a pinvoke thunk — what a plain
  program that calls `printf` gets — works on both.
* **`calli vararg` is not emitted.** A C function pointer with no prototype
  (`void (*routine)()`) types as vararg in the IR, but every call through it
  passes only the fixed arguments, and some runtimes reject `calli vararg`
  outright. The instruction is emitted as a plain `calli`, which is the same
  call.
* **Two constructs only an ELF clang emits are unsupported.** `long double` is
  `x86_fp80` there (MSVC's is `double`), which the writer rejects as an
  unsupported memory type; and an ELF C++ vtable holds `ptrtoint` function
  addresses as constant expressions, which the constant writer does not fold.
  `cbe_port.py` reports the tests that need either as skipped rather than
  failed, matching on the backend's own error message so it cannot hide a
  different failure. `llvm.global_ctors` (`__attribute__((constructor))`) is
  supported: those run from `$MSIL_Startup` before `main`.
* **`<N x i128>` vectors are broken.** The aggregate class has no valid IL type
  name, so the output does not assemble. There is a `report_fatal_error` on
  element extract/insert, but a vector-typed parameter or return reaches the
  emitter first and produces empty type names instead. Plain scalars `i128`
  and arrays of `i128` are fine. This should be a single fatal check on any
  i128 vector type.
* **Integers between 65 and 127 bits** are truncated to `int64` (`i128` itself
  maps to `Int128`).
* **Exceptions** raised by `unreachable` / `abort` go through
  `System.Exception`. Landing pads are not implemented, which is why the C++
  exception tests in `cbe_port.py` are xfail. A C++ `throw` arrives at the CLR
  as `SEHException` and terminates the program. This is not a gap that can be
  closed incrementally: the funclet/personality model has no IL equivalent,
  and typed catch needs image-relative RTTI this backend does not emit. The
  reasoning is in `NOTES-unwinding.md`.
* **Thread-locals** are per-thread and lazily initialized, as described under
  "Globals". Two limits remain: an external (declaration-only) thread-local is
  rejected, and a thread's copy is never freed when the thread ends, because
  there is no thread-exit hook to run.
* The 64-bit CLR layout is the only one. `argc`/`argv` are built from
  `Environment.GetCommandLineArgs()` and leaked for the program's lifetime.

## Notes

`NOTES-platform.md` records the verified .NET facts this backend depends on
(assembly identities, `Int128`'s method-only arithmetic, `Vector<T>` being a
value type). `NOTES-atomics.md` covers the atomics design,
`NOTES-unwinding.md` the exception investigation, and `NOTES-worktrees.md` the
parallel-development setup.
