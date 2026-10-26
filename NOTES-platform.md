# Target platform facts

Verified by hand-written IL against ilasm 10.0.13 and the .NET runtime on this
machine (9.0.6, 256-bit SIMD). Every emitter change must be checked against
these rather than against what the .NET docs suggest.

## Invoking the result

Use `dotnet prog.exe`, not `prog.exe`. A bare exe has no `runtimeconfig.json`,
so the host picks a .NET Framework-era resolver, `mscorlib` resolves to the
Facade and newer types are missing. With a `runtimeconfig.json` naming
`Microsoft.NETCore.App` and invoking through `dotnet`, assembly references
resolve normally. (The lit and llvm-cbe tests get away with a bare exe
because they only use `mscorlib` types, e.g. `Console`, that the facade does
provide; `test/programs/*.sh` all write a runtimeconfig and invoke `dotnet`.)

## Assembly references

`.assembly extern <name> {}` alone emits `Version=0.0.0.0` and a null public key,
which fails to resolve for framework assemblies. Use the real identity:

```
.assembly extern System.Runtime { .ver 9:0:0:0
  .publickeytoken = (B0 3F 5F 7F 11 D5 0A 3A ) }
```

The token is the same for every assembly in the shared framework.

Which assembly a type lives in (all under `Microsoft.NETCore.App/9.0.6`):

| Type | Assembly |
|------|----------|
| `System.Int128`, `System.UInt128` | `System.Runtime` |
| `System.Numerics.Vector<T>` | `System.Numerics.Vectors` (token same) |
| `System.Console` | `System.Console` (token same) |
| `System.Runtime.InteropServices.NativeMemory` | `System.Private.CoreLib` |
| `System.Runtime.InteropServices.Marshal` | `System.Runtime` / `mscorlib` |
| `System.Numerics.BitOperations` | `System.Private.CoreLib` |
| `System.Threading.Interlocked` | `System.Private.CoreLib` |

`mscorlib` does **not** contain `Int128`, `Vector<T>`, `NativeMemory`,
`BitOperations` or `Interlocked`; it is a facade for the .NET Framework
surface. `System.Private.CoreLib` is the implementation assembly and resolves
fine through `dotnet`.

## Int128 / UInt128

These are **not** primitive types. The one public constructor is
`.ctor(uint64 upper, uint64 lower)` — **upper first**; the runtime's own
`Int128` implements the operators through it, so the obvious
`(uint64 lower, uint64 upper)` reading silently builds the wrong value. For a
value that fits in `int64`, build it through the conversion operator instead:

```
ldc.i8 5
call valuetype [System.Runtime]System.Int128
     [System.Runtime]System.Int128::op_Implicit(int64)
```

They are ordinary value types (not byref-like), so `ldloc`/`stloc`/`ldobj`/
`stobj` work, but there is **no IL arithmetic** for them: `add`, `mul`, `shl`,
`div` etc. are all instance methods (`op_Addition`, `op_Multiply`, `op_ShiftLeft`,
`op_Division`, ...) and comparisons are `op_Equality`, `op_LessThan`, ... So a
128-bit IR add becomes a `call`, and a 128-bit compare becomes a `call` plus a
branch, not `add`/`clt`.

Verified details that are not obvious from the API listing:

* Shift counts are `int32` and are reduced **modulo 128** by the method, which
  matches LLVM's shift semantics. `Int128` also has `op_UnsignedRightShift`
  (the `>>>` operator) — use it for `lshr`; `op_RightShift` is arithmetic.
* There is **no unsigned division or comparison on `Int128`**. Convert to
  `UInt128` with `Int128::op_Explicit`, compute there, convert back with
  `UInt128::op_Explicit` (the reverse conversion is declared on `UInt128`).
* `op_Explicit(int64)` truncates to the low half — that is `trunc i128 to i64`.
  `op_Implicit(uint64)` zero-extends, `op_Implicit(int64)` sign-extends; those
  are `zext`/`sext` from 64 bits.
* Value types cross the `call` boundary and sit in locals with `ldloc`/`stloc`
  directly; `ldobj`/`stobj` work through a `conv.u8` unmanaged pointer too.
* A local declared `valuetype [System.Runtime]System.Int128` is initialized to
  zero by `.locals init` — an `initobj` is not needed for the zero constant.

## Vector<T>

A **struct**, so it must be written `valuetype ...Vector`1<int32>` — writing
`class` assembles but throws `TypeLoadException: value type mismatch` at run
time.

* `Vector<T>.Count` is the *vector register width in elements* (8 for
  `int32` on this 256-bit-SIMD machine), not the IR vector length.
* Constructors take a scalar to broadcast or an array; there is no
  "load N elements from a memory blob" constructor, so a shorter IR vector
  (<8 x i32>) must be zero-padded into a full `Vector<T>` and the extra lanes
  ignored, or handled with the scalar fallback.
* Operators: `op_Addition`, `op_Subtraction`, `op_Multiply`, `op_Division`,
  `op_BitwiseAnd`, `op_BitwiseOr`, `op_ExclusiveOr`, `op_Equals` (returns
  `Vector<T>` mask), etc.
* `ldobj`/`stobj` on a `Vector<T>` local work; it is 32 bytes here.

## Other useful primitives (verified)

```
NativeMemory.Alloc(native uint) -> void*     System.Private.CoreLib
NativeMemory.Free(void*)                     System.Private.CoreLib
NativeMemory.Copy(void*, void*, native uint) System.Private.CoreLib
BitOperations.PopCount(uint32) -> int32      System.Private.CoreLib
Interlocked.Add(int64&, int64) -> int64      System.Private.CoreLib
```

`Interlocked.Add` needs `int64&`; a `native int&` overload does not exist.
An `InvalidProgramException` at JIT time still means a stack imbalance or a
type mismatch, exactly as with the rest of this backend — it is not specific
to these calls.

## Typed pointers in signatures

IL accepts a typed pointer (`valuetype 'X'*`) wherever `void*` goes, including
in a `pinvokeimpl`, and it resolves and marshals identically: verified by
declaring `SDL_SetRenderDrawColor(valuetype 'SDL_Renderer'*, uint8×4)` as a
pinvoke on the real SDL2.dll and observing both calls execute. `ldfld`/`stfld`
work through the typed pointer, and it can be passed along as an argument.

So the emitter is not what prevents typed signatures — see
`NOTES-typed-pointers.md` for why the IR cannot supply the type.

## [ThreadStatic] on a <Module> field

It works, so thread-local globals need no helper class. The attribute is
attached to an ordinary module-level field:

```
.field static assembly void* 'x'
.custom instance void [mscorlib]System.ThreadStaticAttribute::.ctor() = ( 01 00 00 00 )
```

Verified by hand-written IL with two `System.Threading.Thread`s incrementing
the field 100 times each: every thread saw its own slot start at zero and the
main thread's value was untouched, including for a `void*` field. `ldsfld`,
`stsfld` and `brfalse` on the slot behave as for any other static field, and
accesses from a native thread the CLR did not create (a `CreateThread`
callback) get a slot of their own too.

The one thing that does *not* work is declaring an accessor method with the
class-qualified spelling inside a class: `.method ... 'My.App'::'x$tls'()`
is a syntax error, so the definition uses the bare name and only call sites
name the class.

