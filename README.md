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
built for the other, which is why the libc is built twice (see below).

## Options

| Option | Effect |
|---|---|
| `-msil-class-name=Ns.Type` | Emit module members inside this class instead of `<Module>`, so C# can call them without reflection. |
| `-msil-libc=builtin` | Link the portable C library into the module: `printf`/`snprintf`/`sprintf`/`puts`/`putchar` and the FILE-less `fputs`/`fputc`/`fwrite`/`strerror`, `atoi`/`strtol`/`strtoul`/`strtoimax` and the rest of the conversion family, `abs`/`div`/`imaxdiv`/`ffs`/`qsort`/`bsearch`/`rand`, the `str*` and `mem*` family (including the POSIX/GNU additions `stpcpy`/`stpncpy`/`memccpy`/`mempcpy`/`strsep`/`strcasestr`/`strndup`/`strchrnul`/`explicit_bzero`), and `<ctype.h>`. Written in C (ported from musl), compiled to IR at build time, and linked selectively, so only what the program reaches is emitted. It is a superset of `il`: `malloc`/`free`/`calloc`/`realloc` come from the IL subset backed by `NativeMemory`, and are emitted for `builtin` too. |
| `-msil-libc=il` | Emit the smaller hand-written IL subset instead: `malloc`/`free`/`calloc`/`realloc` over `NativeMemory`, plus `memcpy`/`memset`/`memcmp` and the `str*` family, but no `printf`. For a module that must link no C in. |
| `-msil-libc=<asm>[:<type>]` | Bind the same calls to a managed assembly you supply (type defaults to `MSIL.Libc`). |
| `-msil-libcxx=builtin` | Link the portable C++ runtime into the module: the global allocation operators (`new`/`delete`, scalar and array, sized, nothrow and aligned), the local-static guard functions (`__cxa_guard_*` and the MSVC `_Init_thread_*` trio), and exit-time destructors (`__cxa_atexit`/`atexit`, run after `main`). Written in freestanding C++ and compiled at build time for both ABIs, like the C library, and linked selectively. Implies nothing about headers: pair it with a freestanding libc++ built for `-fno-exceptions -fno-rtti`. |
| `-msil-alloc=<asm>:<type>` | Route `malloc`/`free`/`calloc`/`realloc` to methods of that name on your type, so unmanaged allocations can be instrumented. |
| `-msil-math=0` | Do not bind libm to `System.Math`/`System.MathF`; pinvoke the CRT instead. |
| `-msil-lib-map='SDL_*=SDL2.dll'` | Bind matching imports to a native library. `*` matches any suffix, and the option repeats. |
| `-msil-library` | Emit a library instead of a program: no `.entrypoint`, and a public `MSILInitialize` static method runs `llvm.global_ctors` behind a one-shot guard. Requires `-msil-class-name`. |
| `-msil-assembly-name=Name` | Name the emitted assembly (default `MSIL`). |
| `-msil-fp80=double` | Map `x86_fp80` (`long double` on the x86 ELF target) to binary64 instead of rejecting it. Lossy: 53 mantissa bits and an 11-bit exponent instead of 64 and 15, so a value round-trips changed or overflows to infinity. The 16-byte slot, the field offsets and `sizeof` are unchanged; only the value stored in the slot is a `float64`. A long-double libm call with no `System.Math` binding, or a `long double` passed through a vararg, is rejected by name rather than pinvoked with a mismatched signature. |
| `-msil-pointers=i64` | Keep every pointer in an `int64` slot in the IL rather than the CLR native int (`void*`), and convert with `conv.u` immediately before each dereference. The module is still laid out LP64, so `sizeof`, struct offsets and pointer strides match the native ABI; only the value representation changes. On a 64-bit JIT `conv.u` is a no-op; on a 32-bit JIT it truncates to the address width, which is correct because such an address fits in the low 32 bits (the upper bits are simply zero). The cost is space: a pointer embedded in a struct still occupies 8 bytes on a 32-bit host, so a struct's layout no longer matches a native 32-bit struct and native interop that expects the real 32-bit layout needs a translation shim. |

## How types map

| IR | IL |
|----|-----|
| `i1` | `bool` |
| `i2`, `i8`, `i16`, `i32` | `int2`, `int8`, `int16`, `int32` in globals and signatures; `int32` in locals |
| `i33` ... `i64` | `int64` |
| `i128` | `System.Int128` from `System.Runtime` |
| `float`, `double` | `float32`, `float64` |
| `x86_fp80` | rejected, unless `-msil-fp80=double`: then a `float64` value in the same 16-byte slot |
| pointers | unmanaged `void*`, or `int64` with `-msil-pointers=i64` |
| structs, arrays | explicit-layout value classes (`valuetype`, byte-exact `.size`) |
| a vector one SIMD register wide | `System.Numerics.Vector<T>` |
| any other vector | explicit-layout value class (a byte blob) |

`i1`/`i8`/`i16` are normalized to their declared width at every store, and any
integer narrower than its CLR register is normalized before a compare, so
unsigned comparisons see the right high bits. Widths with no CLR type (`i24`
and friends) cannot be masked by a `conv`; their loads mask to the declared
width (see below) while their stores stay `int32`.

## Library mode

`-msil-library` turns a translation unit into something a managed caller links
against rather than a program. It changes three things: the module has no
`.entrypoint` (ilasm's `/dll` is the matching switch), every member goes in the
`-msil-class-name` type as before, and the class gets

```
.method public static void 'MSILInitialize'() cil managed
```

which checks a `'$MSIL_Initialized'` flag, runs `llvm.global_ctors` the first
time, and returns immediately after. A C library's own constructor has no other
way to run: `beforefieldinit` does not promise a class initializer runs before
a static method call, and an IL `.cctor` is not something the module can rely
on when the library is loaded through a managed reference. Call
`MSILInitialize` once from the managed side (a `static` constructor in the
wrapper is the natural place); calling it again is a no-op. If there are no
constructors the guard is still emitted, and the call to `'$MSIL_Init'` is
omitted.

`-msil-library` without `-msil-class-name` is a fatal error: there would be no
type to put the method in.

## Signed narrow integers

Because a stored narrow integer is held zero-extended, every *signed* use has
to restore the sign first: `sitofp`, `sdiv`, `srem` and `ashr` emit
`conv.i1`/`conv.i2` on their operand. Without it a negative `i8`/`i16` reads as
a large positive number.

The same applies to `lshr`, but in the other direction: the IL stack holds the
operand as an `int32` even when the IR type is `i8`, so a logical shift must
*zero*-extend first (`conv.u1`/`conv.u2`) or `lshr i8 0xf0, 2` shifts
`0xfffffff0` and yields `0xfc` instead of `0x3c`. `zext` has the same trap and
masks its source before widening.

A width with no CLR type of its own — `i24` is what an APInt constant can
produce — has no `conv` that isolates it, so those uses mask to the declared
width with `and` instead (`lshr i24 0xffab0a0a, 8` would otherwise be masked
to 8 bits by `conv.u1` and lose the table entirely).

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

Whether the library *runs* is a different question from whether it links, and
it runs on both hosts: its `printf` is a C vararg function, but it is not
emitted as a `vararg` method (see below), so nothing in the module needs the
calling convention CoreCLR on Linux refuses.

### The vararg seam

A C vararg *definition* cannot be a `vararg` method. CoreCLR on Linux raises
`InvalidProgramException: Vararg calling convention not supported` for the
method, and for any method that merely names one -- a `call`, or even an
`ldftn` -- which makes the convention unusable rather than merely limited. So
a definition is emitted as an ordinary method and the trailing arguments are
passed as data:

* The callee's signature is its declared parameters plus one `void*`, the
  buffer of trailing arguments.
* Each call site builds that buffer in a local and fills it, one 8-byte slot
  per trailing argument: an integer widened to `int64`, a `double` as its raw
  bits, a pointer as its address. It then passes the buffer's address.
* `va_start` points the callee's `va_list` at the buffer: on Windows, where a
  `va_list` *is* the cursor, the slot gets the pointer directly; on ELF, where
  it is a `__va_list_tag`, the pointer goes in `overflow_arg_area` and
  `gp_offset`/`fp_offset` are set past their limits so every argument takes
  the stack path. Either way a read advances 8 bytes.

Reading is then just a load, which is why the accessors have no `ArgIterator`
in them. `libc.h` declares three -- `__msil_va_i64`, `__msil_va_f64`,
`__msil_va_ptr` -- plus `__msil_write` for output, and the backend emits them
as IL; `printf` is ordinary C above that seam.

Two details make the accessors work, and each is a place where the obvious
thing fails:

* The accessor takes the **address** of the caller's `va_list`, not its value.
  Reading advances the cursor, and the cursor lives in the caller's slot. On
  ELF that address reaches a variable holding a pointer to the tag -- that is
  what a `va_list` is there -- so the accessor loads once more.
* A `double`'s slot holds its `int64` bits, so the read reinterprets through
  memory. `conv.r8` would turn the integer into a value instead.

A vararg `int` is widened to `int64` on the way in rather than left as
`int32`, because the read has to know which of the two shapes to expect and
`printf` cannot know which the caller chose. `%f` reads the `float64` bits
through the `int64` accessor for the same reason.

Two limits follow from the design. A definition whose address is taken cannot
work: the buffer-form signature is how *this module* calls it, and a `calli`
through the pointer would name its own signature and pass the trailing
arguments as varargs instead of in the buffer. Materializing the address is
therefore a diagnostic rather than a miscompile. (Before this change the same
code compiled and then died at run time with `InvalidProgramException` on
Linux, so the diagnostic replaces a crash, not working behaviour. It is only
the materialized address that is rejected: an address the optimizer proves
unused is gone before this code sees it, so such a program still compiles.)
And a vararg call to a *declaration* -- the CRT's `printf` -- is
unchanged: it still gets a concrete pinvoke thunk per call signature, which
both hosts run.

`va_arg` itself needs no backend work on either target: clang expands it into
plain loads and arithmetic against the tag, which is why forcing the overflow
path is enough.

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
  implemented. `strerror` therefore answers from a fixed English table indexed
  by the C99/POSIX values, since there is no `errno` to consult and no locale
  to translate through.
* There is no `FILE`, so the stream functions that need one (`fopen`,
  `fprintf`, `fread`, `fseek`, `fflush`) are absent and keep their CRT
  pinvoke. The output functions whose only job is to write text -- `fputs`,
  `fputc`, `fwrite` -- are present with a `void *` standing in for the unused
  `FILE *` argument, and write to the same console as `printf`.
* `strtod`, `strtof` and `sscanf` are not implemented: exact decimal-to-binary
  conversion needs either a bignum (musl's `strtod`) or a `long double` wider
  than the CLR has, and a version that merely looked right on common inputs
  would mis-round the hard ones, which is worse than pinvoking the CRT.
  `strtoimax`/`strtoumax` are the integer forms and are present.
* `strtol("0x", &end, 0)` leaves `end` on the `x`, which is what musl and
  glibc do and what C99 7.20.1.4 says (the longest initial subsequence that is
  a prefix of a matching sequence). MSVC steps back over both characters and
  leaves it on the `0x`, so `libc.sh` compares that line against a literal the
  same way it does `%a`.
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

## The portable C++ runtime

`-msil-libcxx=builtin` supplies the part of a C++ implementation that lives
below the headers: the allocation operators, the local-static guard functions,
and the exit-time destructor machinery. The headers are a separate concern; this
is the library a freestanding libc++ (`-nostdinc++`, `-fno-exceptions
-fno-rtti`) needs in order to link and run.

It is written in C++ in `lib/Target/MSIL/libcxx/` and compiled to IR at build
time the same way the C library is: twice, once per ABI, embedded as string
literals, and linked with `LinkOnlyNeeded` when the option asks for it. Because
the source is C++ rather than hand-written IL, the mangled names of the
operators come out of the compiler: `operator new` is `_Znwm` for an ELF triple
and `??2@YAPEAX_K@Z` for a Windows one, which is exactly the symbol the front
end emits a call to for that triple. Spelling the labels by hand would be a
second source of truth for the same fact.

### What is in it

* **Allocation operators.** Plain, array, sized-delete (C++14), nothrow
  (C++11) and aligned (C++17) forms, in both the Itanium and MSVC spellings.
  They funnel through `malloc`/`free` by name, so `-msil-alloc` still redirects
  the allocator and the IL subset's `NativeMemory` allocator is still in effect.
  A plain `new` that cannot allocate aborts (there is nowhere to throw
  `std::bad_alloc`); the nothrow forms return null. A request for more than the
  compiler's default new alignment (16 bytes) is refused rather than silently
  under-aligned, since a portable `malloc` cannot satisfy it.
* **Local static guards.** `__cxa_guard_acquire`/`release`/`abort` for the
  Itanium protocol, and `_Init_thread_header`/`footer` plus the
  `_Init_thread_epoch` global for the MSVC one. The module is single-threaded
  from the CLR's point of view (there is no portable mutex without a threading
  reference), so the guard states are used to catch *recursive* initialisation
  -- a static whose initializer names the same static -- rather than to wait.
* **Exit-time destructors.** `__cxa_atexit` and `atexit` append to one table
  (512 entries); `__msil_cxx_finalize` runs it backwards, which is construction
  order reversed, and `__cxa_finalize` forwards to it. The backend calls
  `__msil_cxx_finalize` from `$MSIL_Startup` after `main` returns.
* **Abort paths.** `__cxa_pure_virtual`, `__cxa_deleted_virtual`,
  `__cxa_bad_cast`, `__cxa_bad_typeid` and `__libcpp_verbose_abort` all print a
  diagnostic and trap, through `__msil_cxx_abort`.

### The MSVC local-static protocol

The MSVC epoch counter needs two details to be right, and both are places where
the obvious implementation fails. The front end emits

```
if (load(guard) > epoch) { _Init_thread_header(&guard);
  if (guard == -1) { <initializer>; atexit(dtor); _Init_thread_footer(&guard); } }
```

so `_Init_thread_epoch` has to start at `-1` and only ever decrease: a fresh
guard is `0`, which is greater than any epoch and therefore uninitialized, while
a guard stamped by the footer holds the epoch value that completed it. The front
end also declares `_Init_thread_epoch` as an external *thread-local*, which this
backend rejects (the storage is in another module and there is no way to reach a
given thread's copy); the writer rewrites that declaration into a plain external
global before the link, and the runtime defines the storage. It is a global here
because the module has one thread.

### What it does not do

* **No exceptions and no RTTI.** The backend does not emit landing pads or
  image-relative type information, so a `throw` reaches the CLR as an
  `SEHException` and terminates the program (see "Known limitations"). Compile
  with `-fno-exceptions -fno-rtti`.
* **No `std::` library.** This is the runtime, not the standard library. The
  container/algorithm half comes from libc++'s headers, which must be built for
  this target; LLVM 22's libc++ headers additionally need a clang that provides
  `__builtin_clzg` and friends (clang 20 or newer), and are not usable with the
  clang 18 the rest of this tree is tested with.
* **No thread safety.** Guards do not synchronise, because there is no thread
  to synchronise with. A program that starts threads and initializes a local
  static from more than one is outside what this supports.

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

`exp2` and `ldexp` never reach that split: `exp2` is a member of
`System.Double`/`System.Single` itself (`System.Math` has no such method), and
`ldexp` maps to `Math.ScaleB`, which is exact by definition. Both would
otherwise pinvoke a CRT export on a module that is meant to have no CRT
imports at all.

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
before the MSIL side is involved. The program's `printf` is the portable one,
and it is that path which exercises the vararg buffer on both hosts.

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
an SDL2.dll pinvoke, SDL3's win32 platform adapters — print `skipped` and exit
0 there, and `fp80.sh` does the reverse, since its subject is a type only the
ELF target has.

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
* **A vararg definition is not a `vararg` method.** CoreCLR on Linux raises
  `InvalidProgramException: Vararg calling convention not supported` for a
  `vararg` method *and* for any method that names one, so a definition is
  emitted with a fixed signature and its trailing arguments are passed in a
  buffer the call sites build (see "The vararg seam"). This runs on both
  hosts, which is why `-msil-libc=builtin`'s `printf` and `libc.sh` are no
  longer Windows-only. Two consequences: the signature `ilasm` shows for such
  a method is not the C one, so it cannot be bound from another assembly that
  expects the C ABI (link a pinvoke instead); and taking its address is an
  error, since a `calli` through the pointer would not pass the buffer.
* **`calli vararg` is not emitted.** A C function pointer with no prototype
  (`void (*routine)()`) types as vararg in the IR, but every call through it
  passes only the fixed arguments, and some runtimes reject `calli vararg`
  outright. The instruction is emitted as a plain `calli`, which is the same
  call.
* **`long double` needs a flag on the ELF target, and is then lossy.** It is
  `x86_fp80` there (MSVC's is `double`), and the CLR has no 80-bit float, so
  the writer rejects it by default. `-msil-fp80=double` maps the value to
  binary64, which preserves the program's structure but not its precision:
  53 mantissa bits instead of 64, so a result can round, and an 11-bit exponent
  instead of 15, so a value that fits a `long double` can overflow to infinity.
  The layout does not change — an `x86_fp80` keeps its 16-byte slot, so struct
  offsets, `sizeof` and array strides still match the native ABI; the upper 8
  bytes of the slot are padding. Library calls the front end spells `sqrtl`,
  `floorl` and so on bind through the double spelling for the same reason.
  Anything that stays an external call is refused instead: a long-double libm
  function with no `System.Math` equivalent (`frexpl`, `modfl`, `nextafterl`,
  ...) and a `long double` passed through a vararg (`printf("%Lf", x)`) would
  become a pinvoke whose narrowed signature no longer matches the real x87
  one, which returns a wrong number rather than failing. Convert to `double`
  at the call site; the writer names the symbol in the error.
* **One ELF-only construct is still unsupported.** An ELF C++ vtable holds
  `ptrtoint` function addresses as constant expressions, which the constant
  writer does not fold. `cbe_port.py` reports the tests that need it as skipped
  rather than failed, matching on the backend's own error message so it cannot
  hide a different failure. `llvm.global_ctors`
  (`__attribute__((constructor))`) is supported: those run from `$MSIL_Startup`
  before `main`.
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
* **`-msil-pointers=i64` trades memory for directness.** The option keeps
  pointers in `int64` slots so an address needs no indirection and `conv.u`
  before each dereference is a no-op on a 64-bit JIT. The cost is on a 32-bit
  host, where a pointer still occupies 8 bytes: a struct embedding one is
  larger than its native 32-bit counterpart and its layout no longer matches,
  so interop with native code expecting real 32-bit struct layouts needs a
  translation shim rather than passing pointers straight through. It also means
  a `void*` crossing a pinvoke boundary is marshalled from an `int64`; the
  forwarders that bind to the managed libc convert at that one boundary.
* **The C++ runtime's destructors run once, after `main` returns.**
  `-msil-libcxx=builtin` calls `__msil_cxx_finalize` from `$MSIL_Startup`, so a
  program that calls `exit()` or `_Exit()` itself skips its static destructors,
  where a hosted implementation would run those registered before the `exit`.
  The registration table holds 512 entries; `__cxa_atexit` returns `-1` past
  that. Static-local guards do not synchronise (single-threaded assumption), and
  a `new` request for more than 16-byte alignment aborts instead of allocating
  over-aligned storage.

## Notes

`NOTES-platform.md` records the verified .NET facts this backend depends on
(assembly identities, `Int128`'s method-only arithmetic, `Vector<T>` being a
value type). `NOTES-atomics.md` covers the atomics design,
`NOTES-unwinding.md` the exception investigation, and `NOTES-worktrees.md` the
parallel-development setup.
