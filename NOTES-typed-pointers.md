# Why signatures say `void*` instead of `SDL_Renderer*`

Question: `SDL_SetRenderDrawColor` takes an `SDL_Renderer*`, but the emitter
declares it `void*`. Can the parameter types be recovered?

Short answer: **not from the IR.** The type is not there. Everything on the IL
side already works, so this is an information problem, not a codegen problem.

## The IR does not carry the pointee type

LLVM 22 uses opaque pointers: `PointerType` has no element type at all, and
the typed-pointer constructors are deprecated stubs kept only for out-of-tree
users (`DerivedTypes.h`). There is no flag to re-enable typed pointers —
`-no-opaque-pointers` and `-opaque-pointers=0` are both rejected by clang 18.

Compiling the real SDL header through clang 18 with the real signatures:

```
$ clang -I<SDL2>/include/SDL2 -O0 -S -emit-llvm sdl_sig.c
declare dso_local i32 @SDL_SetRenderDrawColor(ptr noundef, i8, i8, i8, i8)
declare dso_local i32 @SDL_RenderCopy(ptr noundef, ptr noundef, ptr noundef, ptr noundef)
declare dso_local void @SDL_GetWindowSize(ptr noundef, ptr noundef, ptr noundef)
```

`SDL_Renderer*`, `SDL_Texture*` and `void*` are all the same `ptr`. The C
source distinction is gone.

## The source that would have it is gone too

The information exists only in the C type system. By the time `llvm-msil` runs
there is no C source, no `DW_TAG_pointer_type` on these declarations, and no
`byval`/`sret` type annotation on the Windows x64 ABI (structs of this size pass
in registers there; the attributes are absent).

Debug info does not rescue it either. `-g` produces typed pointers for
*parameters of defined functions*:

```
!20 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !21, size: 64)
!21 = !DIDerivedType(tag: DW_TAG_typedef, name: "SDL_Renderer", ...)
```

but `SDL_SetRenderDrawColor` is an external **declaration**, and clang emits no
`DISubprogram` for declarations at all — the `!21` node above was a parameter of
a function *defined* in the file. So `-g` helps for definitions and does
nothing for exactly the imports this matters for.

## What the emitter already gets right

The part that is implementable — no pointer arithmetic inside a known struct —
is already done: a `getelementptr %S, ptr %p, 0, N` load or store becomes
`ldfld`/`stfld` whenever the access type equals the declared field type. That
works today and is covered by `test/struct-layout.ll`.

## If this is ever revisited

The type has to arrive from outside the IR. The realistic option is an explicit
side table, the same shape as `-msil-lib-map`, e.g.
`-msil-sig='SDL_SetRenderDrawColor=SDL_Renderer*,i8,i8,i8,i8'`, or reading it
from the C headers. Both put the burden on the user and drift as the headers
change, which is why neither is implemented yet.

The IL side needs no work: a typed pointer in a `pinvokeimpl` assembles,
resolves and marshals correctly (see `NOTES-platform.md`), so this would be a
table lookup and a signature-string change.
