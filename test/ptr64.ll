; RUN: llvm-msil %s -o - | FileCheck %s --check-prefix=DEF
; RUN: llvm-msil %s -o - -msil-pointers=i64 | FileCheck %s --check-prefix=I64

; -msil-pointers=i64 keeps every pointer in an int64 slot in the IL instead of
; the CLR native int (void*), and converts with conv.u immediately before each
; dereference. The data layout is untouched, so struct offsets and strides keep
; matching the LP64 native ABI; only the value representation changes.
;
; The default mode is unchanged: pointers stay void*, and no conv.u is inserted
; at a dereference.
;
; FileCheck reads in order, so the checks follow emission order: the global's
; field first, then each function as it is emitted.

target triple = "x86_64-unknown-linux-gnu"

%pair = type { i32, ptr }

@g = global i32 7

declare i32 @indirect(i32)

; A global's address lives in a field of the pointer's own type.
; DEF: .field static public void* 'g'
; I64: .field static public int64 'g'

; A dereference converts the address it is about to use.
; DEF-LABEL: .method {{.*}} 'simple'(
; DEF-SAME:   void*
; DEF: ldarg
; DEF-NOT: conv.u
; DEF: ldind.i4
;
; I64-LABEL: .method {{.*}} 'simple'(
; I64-SAME:   int64
; I64: ldarg
; I64-NEXT: conv.u
; I64-NEXT: ldind.i4
define i32 @simple(ptr %p) {
  %v = load i32, ptr %p
  ret i32 %v
}

; Loading through a global field: void* by default, int64 under i64, with the
; conversion right before the ldind.
;
; DEF: ldsfld void* 'g'
; DEF-NOT: conv.u
; DEF: ldind.i4
;
; I64: ldsfld int64 'g'
; I64-NEXT: conv.u
; I64-NEXT: ldind.i4
define i32 @readglobal() {
  %v = load i32, ptr @g
  ret i32 %v
}

; A store converts its address, and not the value being stored.
;
; I64-LABEL: .method {{.*}} 'write'(
; I64: ldsfld int64 'g'
; I64-NEXT: conv.u
; I64-NEXT: ldarg
; I64-NEXT: stind.i4
define void @write(i32 %x) {
  store i32 %x, ptr @g
  ret void
}

; A GEP's result is another pointer, so its arithmetic happens in the pointer
; type with no conversion: only the widening of the base is emitted.
;
; I64-LABEL: .method {{.*}} 'gep'(
; I64-NOT: conv.u
; I64: conv.u8
; I64: add
; I64-NOT: conv.u
; I64: ret
define ptr @gep(ptr %p, i64 %i) {
  %q = getelementptr i32, ptr %p, i64 %i
  ret ptr %q
}

; A struct member reached through a GEP is still a dereference: the base is
; converted, and ldfld names the field.
;
; I64-LABEL: .method {{.*}} 'field'(
; I64: conv.u
; I64: ldfld int32
define i32 @field(ptr %s) {
  %f = getelementptr %pair, ptr %s, i64 0, i32 0
  %v = load i32, ptr %f
  ret i32 %v
}

; A pointer loaded out of memory is an int64 in i64 mode, not a native int.
;
; DEF: ldind.i
; I64: ldind.i8
define ptr @loadptr(ptr %p) {
  %v = load ptr, ptr %p
  ret ptr %v
}

; An indirect call converts the function pointer back to a native int before
; calli, since calli takes an address.
;
; I64-LABEL: .method {{.*}} 'callfp'(
; I64: ldarg
; I64: conv.u
; I64-NEXT: calli
define i32 @callfp(ptr %fp, i32 %x) {
  %r = call i32 %fp(i32 %x)
  ret i32 %r
}
