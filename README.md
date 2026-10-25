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

## Usage

```
clang -S -emit-llvm -O1 -o prog.ll prog.c
llvm-msil prog.ll -o prog.il
ilasm /exe /output:prog.exe prog.il
./prog.exe
```

A bare `.exe` run directly binds the .NET Framework, which is where `mscorlib`
resolves. To run it on modern .NET instead, put a `prog.runtimeconfig.json`
next to it naming `Microsoft.NETCore.App` and use `dotnet prog.exe`; without
that file `dotnet` cannot find the framework. Types this backend emits
(`System.Int128`, `System.Numerics.Vector<T>`, `System.Math`) are modern .NET
types, so the runtimeconfig is what you want in practice.

The input IR needs a target triple with a data layout, or the writer cannot
size any aggregate type.

## Options

| Option | Effect |
|---|---|
| `-msil-class-name=Ns.Type` | Emit module members inside this class instead of `<Module>`, so C# can call them without reflection. |
| `-msil-libc=builtin` | Emit a platform-independent C library into the module: `malloc`, `free`, `calloc`, `realloc`, `memcpy`, `memmove`, `memset`, `memcmp`, the `str*` family. Backed by `NativeMemory`, so there is no pinvoke. |
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
  levels) via `cbe_port.py` — 540 tests, 32 xfail,
* the end-to-end programs in `test/programs/`, each compared against a native
  build of the same source.

The end-to-end programs are the ones that find real bugs; the hand-written IR
tests only cover the cases someone thought to write. They sweep `-O0` to `-O3`,
which matters: the bugs they have caught so far each appeared at a single
level. `int128.sh` and `sdl.sh` are self-contained. `sdl.sh` needs SDL2
development files: set `SDL2_DIR` to a directory with `include/SDL2` and
`lib/SDL2.lib`, and `SDL2_BIN` to `SDL2.dll` if it is not in `lib/`. Without
them the test reports `sdl: skipped` and the gate still passes.

`doom.sh` runs PureDOOM for a fixed number of tics on a virtual clock and
hashes the framebuffer; `stb.sh` exercises stb_image (including a PNG
decode/encode round trip), stb_truetype, stb_rect_pack and stb_ds. Their
sources are third-party, so they are fetched at a pinned commit into
`/tmp/msil-thirdparty` and cached (`MSIL_TEST_CACHE` moves the cache). A
machine with neither the cache nor network reports `skipped` and the gate
still passes. `stb.sh` needs a TrueType font; `STB_TEST_FONT` overrides the
default `C:/Windows/Fonts/arial.ttf`.

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
  bits, because the CRT's `va_arg` reads the general-purpose shadow slot on
  Windows x64.
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
* **Thread-locals** are ordinary globals, so there is no `[ThreadStatic]`.
* The 64-bit CLR layout is the only one. `argc`/`argv` are built from
  `Environment.GetCommandLineArgs()` and leaked for the program's lifetime.

## Notes

`NOTES-platform.md` records the verified .NET facts this backend depends on
(assembly identities, `Int128`'s method-only arithmetic, `Vector<T>` being a
value type). `NOTES-atomics.md` covers the atomics design,
`NOTES-unwinding.md` the exception investigation, and `NOTES-worktrees.md` the
parallel-development setup.
