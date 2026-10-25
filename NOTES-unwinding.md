# Stack unwinding on the CLR: what works, what cannot

Question: can LLVM `landingpad` / `invoke` / `resume` / `__cxa_throw` semantics
be mapped onto CIL exception handling (`try`/`catch`/`filter`/`finally`,
`leave`, `throw`, `rethrow`) well enough to pass the C++ exception tests?

Short answer: **partly, and not through the LLVM EH model.** A useful subset is
implementable — function-local `try`/`catch`, typed catch on the C++ throw
payload, cleanup-on-unwind, and rethrow — but the LLVM construct that carries
all of it (`invoke` plus funclets) has no IL equivalent, and the personality
routine has nowhere to run. The two models are not isomorphic. What follows is
the evidence, measured on ilasm 10.0.13 against the .NET Framework 4.0 runtime
a bare exe binds to and against .NET 9.

## The pipeline removes the information before the writer sees it

`MSILTargetMachine.cpp` runs `createLowerInvokePass()`. That pass is blunt:

```
invoke void @f() to label %ok unwind label %lpad
```
becomes
```
call void @f()
br label %ok          ; and removePredecessor() on the unwind block
```

`LowerInvoke` does not translate anything. It deletes the exceptional edge, so
the landing-pad block becomes unreachable and the later `CFGSimplification`
deletes it. By the time `MSILWriter` runs there is no `catchswitch`,
`catchpad`, `cleanuppad`, `landingpad`, `resume`, `catchret` or `cleanupret`
left. The writer has a `case Instruction::Invoke` but it only routes to
`printCallInstruction`, i.e. it emits the call and drops the unwind edge; it
has no case for any of the other EH opcodes. Anything that wants to emit `try`
blocks has to keep the EH structure alive through the pipeline, which means
removing `LowerInvoke` and handling the constructs itself.

That is the easy part. The hard part is that LLVM's EH is a *two-level*
model and CIL's is a *one-level* one.

## What the two models actually require

LLVM (Windows C++ ABI) gives each function a **personality routine**
(`__CxxFrameHandler3`), and splits a protected region into **funclets**: a
`catchswitch` chooses among `catchpad`s, a `cleanuppad` runs cleanup, and
control leaves each funclet with `catchret` / `cleanupret` / `resume`. The
personality routine, not the function, decides at run time which pad to enter,
by walking the `.pdata`/`.xdata` unwind tables the compiler emits and calling
`__CxxFrameHandler3` for each frame.

CIL has no personality routine and no funclets. A method body carries an
exception-handling clause table (`.try` regions with `catch`/`filter`/
`finally`/`fault` handlers), and the *runtime's* unwinder walks it. There is no
per-function hook to run and no way to express "ask this routine which handler
applies".

So the translation cannot preserve the model. It can only rewrite each
function into clauses the runtime understands, which is a real but limited
win.

## What was measured to work

Each of these was assembled and run; the exit code is the observation.

| Construct | Result |
|---|---|
| CIL `catch [mscorlib]System.Exception` intercepts `_CxxThrowException` | **works** — caught |
| Cross-frame: `try { call Thrower() } catch` catches a C++ throw two frames down | **works** |
| A `finally` runs on a C++ throw, and an outer typed handler still matches afterwards | **works** |
| `rethrow` from a catch, outer frame catches again | **works** |
| `filter` block deciding by the original throw payload | **works** |
| Payload still readable (`GetExceptionPointers`) after a `rethrow` | **works** |

The payload is the key. `_CxxThrowException(object, ThrowInfo)` raises a Win32
SEH exception with code `0xE06D7363`, and the CLR converts that to a
`System.Runtime.InteropServices.SEHException`. Inside a `catch` or `filter`,
`Marshal.GetExceptionPointers()` returns the original `EXCEPTION_POINTERS`,
and `ExceptionRecord->ExceptionInformation[1]` is the C++ object pointer,
`[2]` is the `ThrowInfo*`. Both were read back correctly. That is what makes a
typed catch possible at all: the `ThrowInfo` carries the `CatchableTypeArray`
with the RTTI type descriptors, so a `filter` can decide whether the thrown
object matches the type the IR's `catchpad` names.

## What cannot work

### 1. The personality routine has nowhere to run

`__CxxFrameHandler3` is a function that the OS calls during SEH dispatch with
a `DISPATCHER_CONTEXT`. There is no point in the CLR's unwinding at which user
code is called that way. A `filter` is the closest thing, and it is
per-clause, not per-function. So the personality pointer in the IR
(`personality ptr @__CxxFrameHandler3`) cannot be honoured; it can only be
ignored, with the clause structure reconstructed statically. Any behaviour that
depends on the personality routine making a run-time decision — which is the
whole point of the funclet model — is lost.

### 2. Funclets are not expressible

`catchswitch` with multiple `catchpad`s looks superficially like multiple
`catch` clauses, and for the simple case it maps. But funclets are *separate
functions* in the native model, with their own frame and their own control
flow: a `catchpad` may `catchret` to any block, a `cleanuppad` may
`cleanupret ... unwind to caller`, and `resume` re-raises into the enclosing
personality. CIL's clauses are regions of one method body. The mapping is
one-way and lossy: you can build the simple cases, but you cannot represent a
funclet that is entered from more than one region, or a `resume` that must
re-enter the personality's search rather than the CLR's.

(These five tests happen to use only `catchret` and
`cleanupret ... unwind to caller`, with no `resume`, so this particular
blocker is not the one that stops *them*; the block layout and the RTTI data
below are.)

### 3. `invoke` has no IL equivalent, and try regions are lexical

IL has no "call that may unwind to *this specific block*". A `try` region is a
*contiguous range of instructions*, so emitting one means laying out the basic
blocks in a different order from the IR.

This is not a small rearrangement. clang emits the handler blocks **between**
the invoke's two successors:

```
0:  invoke ... to %5 unwind %6
5:  <normal path>            <- block 5 is laid out before 6
6:  catchswitch ...
8:  catchpad ...
10: <catchret target>
```

A CIL `try` covering blocks 0..5 would have to exclude 6, 8 and 10, which sit
in the middle. So the writer would need a block-ordering pass that puts the
protected region first, then the handlers, then the continuation — and then
rewrite every branch that crossed the new boundaries. That is a real piece of
work, not a lowering, and it is where an implementation would spend most of
its effort.

### 3b. `unreachable` after a noreturn call

After `invoke @_CxxThrowException`, clang emits a block containing only
`unreachable`. The writer turns that into `throw new Exception("unreachable")`,
so even a correctly built catch would see a second, bogus exception unless
that block is special-cased away.

### 4. The C++ ABI cannot be bridged in the general case

This is the deepest blocker, and it is why the honest answer is "a subset, not
the model". For a typed catch to match the way C++ requires, the code must:

* read the `ThrowInfo` from `ExceptionInformation[2]`;
* walk its `CatchableTypeArray`, whose entries are **image-relative RVAs**, not
  pointers;
* compare each entry's RTTI type descriptor against the type the catch names;
* for a by-reference catch, hand the handler the object pointer; for a
  by-value catch, run the copy constructor through `CatchableType`'s copy-func
  RVA.

The backend emits globals as heap blocks with a `void*` static field, not as
image-relative data at a fixed base (`printGlobals`, and the `$MSIL_Init`
template-copy). So every RVA in the `ThrowInfo` data is *zero* in the emitted
IL — visible in `exc3.il`, where `_TI1...$tpl$data` is
`{ int8 (0) [4], int64 (0), ... }`. A `ThrowInfo` built that way has no
catchable types at all. Making typed catch work means re-emitting the whole
RTTI/ThrowInfo graph with a consistent synthetic image base, and implementing
the `CatchableType` matching rules (including the by-value copy constructor)
in IL — i.e. writing a small runtime, not a lowering.

`__cxa_throw` (the Itanium spelling) has the same problem in a different
shape: it is a different ABI entirely and is not what clang emits for the
Windows triple these tests use.

### 5. The tests that are xfail exercise all of the above

The five C++ tests under `llvm-cbe/test/cpp_tests` are not simple. Between
them they need: typed catch on two different classes with an inheritance
relationship (`test_multiple_exception_handlers`), catch-by-reference
(`const std::exception &`), a destructor that runs *during* unwinding
(`test_dtor_called_when_unwinding`), and a throw from a constructor
(`test_exception_from_ctor`). The first three are exactly the cases blocked by
(2)–(4). `std::exception` additionally needs the MSVC STL's
`std::exception` type to exist, which it does not in an assembly that only
references `mscorlib`.

## The subset that is feasible

If the pipeline keeps the EH structure (drop `LowerInvoke`, teach the writer
the EH opcodes), the following is implementable and would be a real
improvement over the current state, where a C++ throw is an unhandled
`SEHException` that terminates the program:

* **Function-local `try`/`catch`** where the `catchswitch` has one
  `catchpad`, and the catch body is reachable only from it.
* **Typed catch by `filter`**, matching on the `ThrowInfo`'s
  `CatchableTypeArray` — *if* the RTTI/ThrowInfo data is re-emitted with a
  consistent image base (blocker 4). Without that, only catch-all
  (`catchpad ... [ptr null, i32 64, ptr null]`) can match.
* **Cleanup-on-unwind** via `finally`, which the measurements show composes
  correctly with an outer typed handler.
* **Rethrow** via `rethrow`, which preserves the payload.
* **`unreachable` / `abort`** keep their current `throw System.Exception`
  behaviour.

That subset is enough to make `catch (...) { }` work and to run destructors on
the way out. It is *not* enough to pass the five xfail tests as written,
because they all depend on typed matching against MSVC RTTI and (for two of
them) on the MSVC STL.

A half-subset is also actively dangerous. If a `catch (Foo&)` were emitted as
an unconditional CIL `catch`, a `throw Bar()` would silently enter the `Foo`
handler: a miscompile that produces wrong answers instead of a clean abort.
The current behaviour (unhandled `SEHException`, process exits non-zero) is
wrong too, but it fails loudly. So a partial implementation is only defensible
if it refuses to emit a typed handler it cannot match, and falls back to the
current abort for those functions.

## Recommendation

Implement the subset only if the goal is "a C++ program that throws does not
abort the process", and only with the typed-handler fallback above. Do not
claim landing-pad support: the personality-routine and funclet model cannot be
represented, and the C++ ABI bridge needs a runtime this backend does not
have. The five tests should stay xfail, with the `xfail` reason updated from
"no landingpad support" to name the specific blockers, so the next person does
not rediscover this.

The atomics work in this branch is independent and stands on its own.
