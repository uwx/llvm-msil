# Atomics on the CLR

`atomicrmw`, `cmpxchg`, `fence` and atomic loads/stores lower to real .NET
primitives. Everything below was verified with ilasm 10.0.13 against both
runtimes an assembly here can bind to:

* the .NET Framework 4.0 `mscorlib` in
  `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\`, which is what a bare
  `prog.exe` resolves (the test harness runs exes bare, so this is the one that
  matters for the gate), and
* .NET 9 through `dotnet prog.exe`, which is how the README tells users to run
  the result.

The two do not have the same API surface, and that difference decides the
lowering.

## What the runtime provides

`System.Threading.Interlocked`, as seen by reflection on the Framework 4.0
`mscorlib`:

```
Int32  Add(Int32&, Int32)              Int64  Add(Int64&, Int64)
Int32  Exchange(Int32&, Int32)         Int64  Exchange(Int64&, Int64)
IntPtr Exchange(IntPtr&, IntPtr)
Single Exchange(Single&, Single)       Double Exchange(Double&, Double)
Int32  CompareExchange(Int32&, Int32, Int32)
Int64  CompareExchange(Int64&, Int64, Int64)
IntPtr CompareExchange(IntPtr&, IntPtr, IntPtr)
Single CompareExchange(Single&, Single, Single)
Double CompareExchange(Double&, Double, Double)
Void   MemoryBarrier()
```

There is **no** `And`, `Or`, `Xor`, `Subtract`, `Increment`-on-anything-else,
or narrow-width overload on .NET Framework. `And`/`Or` exist only on .NET Core
and later, so they cannot be used.

`System.Threading.Volatile` has `Read`/`Write` for every width from `bool` to
`IntPtr`, and the `volatile.` IL prefix does the same thing for any `ldind`/
`stind` without a call.

## The lowering

| IR | IL |
|---|---|
| `fence` | `Interlocked.MemoryBarrier()` |
| `atomicrmw xchg` (i32/i64/ptr/f32/f64) | `Interlocked.Exchange` |
| `atomicrmw add`/`sub` (i32/i64/ptr) | `Interlocked.Add`; `sub` negates first, since there is no Subtract |
| `atomicrmw and`/`or`/`xor` | CompareExchange loop |
| `atomicrmw fadd`/`fsub` | CompareExchange loop (no float Add) |
| `atomicrmw` on i8/i16 | `Monitor` lock around load/op/store |
| `cmpxchg` (i32/i64/ptr) | `Interlocked.CompareExchange` |
| `cmpxchg` on i8/i16 | `Monitor` lock around load/compare/store |
| atomic `load`/`store` | `volatile.` + `ldind`/`stind` |

### An unmanaged `void*` is a valid `T&`

The backend represents every pointer as an unmanaged `void*`. `Interlocked`
wants a managed `T&`, and the CLR accepts an unmanaged pointer wherever a
byref is expected, so `ldsfld void* 'g'; ...; call int32
Interlocked::Add(int32&, int32)` works directly. No pinning, no `GCHandle`, no
marshalling. This is the single fact that makes the whole lowering practical.

### Why a CompareExchange loop rather than `And`/`Or`

The loop is the portable spelling of any read-modify-write:

```
IL_rmw:
  <addr>; volatile.; ldind.<w>; stloc old
  ldloc old; <operand>; <op>; stloc new
  <addr>; ldloc new; ldloc old
  call T Interlocked::CompareExchange(T&, T, T); stloc prev
  ldloc prev; ldloc old; bne.un IL_rmw
  ldloc old
```

It needs only the CompareExchange overload for the value's own type, which
exists on every runtime. For float32/float64 the retry test compares the bit
patterns, not the values: IL has no bitwise float compare, and `ceq` is false
for NaN, which would spin forever on a NaN. The CLR's own float
CompareExchange is bitwise, so the bits are also the right test for whether the
exchange happened — verified with `-0.0` vs `+0.0` (no store) and NaN vs NaN
(store).

### Narrow integers

i8/i16 have no Interlocked overload on either runtime. They take a module-wide
monitor:

```
ldsfld object '$MSIL::atomicLock'; stloc L
ldloc L; call void Monitor::Enter(object)
.try {
  load/op/store
  leave IL_done
}
finally {
  ldloc L; call void Monitor::Exit(object); endfinally
}
IL_done:
```

`leave` is the only legal exit from a protected region, and the `finally` runs
on the exceptional path too, so a throwing body cannot leak the lock.

The lock is emitted once, as a field of `<Module>` plus a `<Module>.cctor`,
and only when a narrow atomic actually appears — a module without one is
unchanged. It is `assembly` rather than `private` visibility because a field's
accessibility is checked against its declaring type, and a private `<Module>`
field is not reachable from inside a class when `-msil-class-name` wraps the
module's members.

## What "atomic" does and does not cover here

`Interlocked` and `volatile.` are genuinely atomic and ordered, so i32/i64/ptr
and the float forms are correct under real concurrency.

The narrow path is only atomic against **other narrow atomic accesses in the
same assembly**, because it is a lock the backend owns. A native library, or
hand-written IL, doing an unlocked 16-bit store to the same address is not
excluded. That is the honest limit; it is strictly better than the previous
load/compute/store, which was not atomic against anything, and it matches what
a `std::atomic<short>` is actually used for in the C tests.

## Notes for other agents

* `printIndirectLoad`/`printIndirectSave` are still the plain access path;
  atomic loads and stores branch to `printAtomicLoad`/`printAtomicStore`
  before reaching them. If you add a new memory access, decide explicitly
  which of the two it is.
* The `AtomicExpand` pass in the pipeline is a no-op: the legacy pass returns
  immediately without a `TargetPassConfig`, so raw `atomicrmw`/`cmpxchg`
  reach the writer. Nothing depends on it.
* `printUnderAtomicLock` adds an `ExtraLocals` entry of type `object`; the
  declaration list is built after the body is emitted, so this is safe from
  any instruction case.
