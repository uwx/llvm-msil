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
  test/                lit suite, README, and the llvm-cbe port
```

## Usage

```
clang -S -emit-llvm -O1 -o prog.ll prog.c
llvm-msil prog.ll -o prog.il
ilasm /exe /output:prog.exe prog.il
prog.exe
```

The input IR needs a target triple with a data layout, or the writer cannot
size any aggregate type.

## How types map

| IR | IL |
|----|-----|
| `i1` | `bool` |
| `i8`, `i16`, `i32` | `int8`, `int16`, `int32` in globals and signatures; `int32` in locals |
| `i33` ... `i64` | `int64` |
| `i128` | `System.Int128` (a value type; see below) |
| `float`, `double` | `float32`, `float64` |
| pointers | unmanaged `void*` |
| structs, arrays, vectors | explicit-layout value classes (`valuetype`, byte-exact size) |

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

Not supported: i128 lanes inside a vector, and 128-bit atomics. Both abort with
a diagnostic rather than emitting wrong code.

## Globals are writable

A `.data`-backed static field lives in a read-only PE section, so a store
through it faults. Each initialized global is therefore emitted twice:

* a read-only template class holding the initializer bytes, and
* a `void*` static field.

`$MSIL_Init` (called from `$MSIL_Startup`) `Marshal.AllocHGlobal`s the real
storage and `cpblk`s the template into it. A zero-sized global gets no template
but still gets its own allocation, since LLVM gives every global a distinct
address.

## Importing from a DLL that is not the CRT

An external function is bound to `msvcrt.dll` or `ucrtbase.dll` by a name
heuristic, which cannot know about a third-party library. `-msil-lib-map`
names one, matched against the symbol as the module references it (not the
export spelling it is bound to):

```
llvm-msil demo.ll -o demo.il -msil-lib-map 'SDL_*=SDL2.dll'
llvm-msil gpu.ll  -o gpu.il  -msil-lib-map 'vk*=vulkan-1.dll'
```

`*` matches any suffix and the option repeats. Library-specific options are
consulted first, so a mapped symbol never falls back to the CRT. The
[`sdl_demo.c`](test/programs/sdl_demo.c) program is a working example: SDL2,
native-thread callbacks, function pointers from `SDL_GL_GetProcAddress`, and a
frame checksum compared against a native build.

## Tests

`test/` is its own lit suite (`check-msil`, or run `llvm-lit` on it
directly). Those tests check the emitted IL with `FileCheck`.

`test/programs/sdl_demo.c` is the SDL2 demo, built both natively and through
the backend and compared line for line (`test/programs/sdl.sh`). It needs SDL2
development files: set `SDL2_DIR` to a directory with `include/SDL2` and
`lib/SDL2.lib`, and `SDL2_BIN` to `SDL2.dll` if it is not in `lib/`. Without
them the test reports `sdl: skipped` and the gate still passes.

`cbe_port.py` runs the upstream llvm-cbe suite (C, C++ and `.ll` tests, four
optimization levels) against this backend; both sides of that suite only
compare exit codes, so it ports over unchanged:

```
python -I cbe_port.py \
    --cbe-tests ../llvm-cbe/test \
    --clang <clang> --llvm-msil <llvm-msil> --ilasm <ilasm> \
    --lli <lli> --levels 0,1,2,3
```

## Known limitations

* **Varargs.** Calls to external vararg functions (`printf`) work: each call
  signature gets a concrete pinvoke thunk, since `ilasm` rejects `vararg`
  pinvokes.
  Definitions of vararg functions that forward a `va_list` to a CRT internal
  (e.g. the `printf` wrapper MSVC's `stdio.h` produces) are dropped and bound
  to the real export instead.
  A vararg floating-point argument crosses the boundary as its raw `int64`
  bits, because the CRT's `va_arg` reads the general-purpose shadow slot on
  Windows x64.
* **Vectors** are byte blobs: element-wise arithmetic, compares, selects,
  casts, shuffles and folds (including `llvm.vector.reduce.*`) are expanded
  lane by lane; there is no SIMD. This is enough for `-O2`/`-O3` output from
  clang.
* **Atomics** (`atomicrmw`, `cmpxchg`, `fence`, and atomic loads/stores) use
  the real primitives: `System.Threading.Interlocked` for int32/int64/native
  int, a CompareExchange loop for the operations .NET Framework has no
  overload for (`and`, `or`, `xor`, min/max, float add/sub), and the
  `volatile.` prefix for atomic loads and stores. Narrow integers (i8/i16)
  have no Interlocked overload, so they are serialized on a module-wide
  monitor; that is atomic against other narrow accesses in the same assembly,
  not against native code or a second thread bypassing it. See
  `NOTES-atomics.md`.
* **Integers between 65 and 127 bits** are truncated to `int64` (`i128` itself
  maps to `Int128`).
* **C runtime binding** is heuristic (`msvcrt.dll` vs `ucrtbase.dll`); only the
  symbols `clang` emits for the common C library are covered. A symbol from any
  other DLL has to be named with `-msil-lib-map` (see below), since nothing in
  the IR says which library an import came from: neither the SDL headers nor
  `vulkan.h` expand to `__declspec(dllimport)` by default, and `dllimport` is
  not consulted even when it is present.
* **Exceptions** raised by `unreachable` / `abort` go through
  `System.Exception`. Landing pads are not implemented, which is why the
  C++ exception tests in `cbe_port.py` are xfail. A C++ `throw` arrives at the
  CLR as `SEHException` and terminates the program; the reason a faithful
  lowering is out of reach is written up in `NOTES-unwinding.md`.
* The 64-bit CLR layout is the only one. `argc`/`argv` are built from
  `Environment.GetCommandLineArgs()` and leaked for the program's lifetime.
  Thread-locals are ordinary globals, so there is no `[ThreadStatic]`.
