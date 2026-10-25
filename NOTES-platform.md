# Target platform facts

Verified by hand-written IL against ilasm 10.0.13 and the .NET runtime on this
machine (9.0.6, 256-bit SIMD). Every emitter change must be checked against
these rather than against what the .NET docs suggest.

## Invoking the result

Use `dotnet prog.exe`, not `prog.exe`. A bare exe has no `runtimeconfig.json`,
so the host picks a .NET Framework-era resolver, `mscorlib` resolves to the
Facade and newer types are missing. With a `runtimeconfig.json` naming
`Microsoft.NETCore.App` and invoking through `dotnet`, assembly references
resolve normally. (The existing tests get away with a bare exe because they
only use `mscorlib` types, e.g. `Console`, that the facade does provide.)

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

These are **not** primitive types and have **no public constructors**. Build
them through the conversion operator:

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
