; RUN: llvm-msil %s -o - | FileCheck %s

target triple = "x86_64-unknown-linux-gnu"

@_ZTIi = external constant ptr
@_ZTVN10__cxxabiv117__class_type_infoE = external global [0 x ptr]
@_ZTS3Foo = constant [5 x i8] c"3Foo\00"
@_ZTI3Foo = constant { ptr, ptr } { ptr getelementptr inbounds (i8, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 16), ptr @_ZTS3Foo }

declare void @may_throw()
declare ptr @__cxa_begin_catch(ptr)
declare void @__cxa_end_catch()
declare i32 @__gxx_personality_v0(...)
declare i32 @__CxxFrameHandler3(...)
declare i32 @llvm.eh.typeid.for.p0(ptr)

; CHECK: .assembly extern 'MSIL.Runtime'

; CHECK-LABEL: '__cxa_begin_catch'(void*) cil managed
; CHECK: call void* [MSIL.Runtime]MSIL.Runtime.CxxAbi::__cxa_begin_catch(void*)

; CHECK: .field static assembly void* '_ZTIi'
; CHECK: .field static assembly void* '_ZTVN10__cxxabiv117__class_type_infoE'

; CHECK-LABEL: 'catches'()
; CHECK: .locals init ({{.*}}void* '$exn'
; CHECK: .try
; CHECK-NEXT: {
; CHECK-NEXT: call void 'may_throw'()
; CHECK-NEXT: leave [[INVOKE:IL_invoke[0-9]+]]_normal
; CHECK: catch [MSIL.Runtime]MSIL.Runtime.CxxException
; CHECK: call void* [MSIL.Runtime]MSIL.Runtime.CxxAbi::Landing(class [MSIL.Runtime]MSIL.Runtime.CxxException)
; CHECK-NEXT: stloc '$exn'
; CHECK-NEXT: leave [[INVOKE]]_unwind
; CHECK: [[INVOKE]]_unwind:
; CHECK-NEXT: br [[LPAD:IL_[0-9]+]]
; CHECK: [[INVOKE]]_normal:
; CHECK: br IL_

; The typeid the selector compares against and the one the landing pad
; assigns have to agree.
; CHECK: [[LPAD]]:
; CHECK: ldsfld void* '_ZTI3Foo'
; CHECK-NEXT: call bool [MSIL.Runtime]MSIL.Runtime.CxxAbi::Matches(void*, void*)
; CHECK-NEXT: brfalse
; CHECK-NEXT: ldc.i4 1
; CHECK: ldsfld void* '_ZTIi'
; CHECK-NEXT: call bool [MSIL.Runtime]MSIL.Runtime.CxxAbi::Matches(void*, void*)
; CHECK-NEXT: brfalse
; CHECK-NEXT: ldc.i4 2
; CHECK: ldc.i4 1
; CHECK-NEXT: ceq
; CHECK: call class [MSIL.Runtime]MSIL.Runtime.CxxException [MSIL.Runtime]MSIL.Runtime.CxxAbi::Resume(void*)
; CHECK-NEXT: throw
define i32 @catches() personality ptr @__gxx_personality_v0 {
entry:
  invoke void @may_throw()
          to label %ok unwind label %lpad

ok:
  ret i32 0

lpad:
  %lp = landingpad { ptr, i32 }
          cleanup
          catch ptr @_ZTI3Foo
          catch ptr @_ZTIi
  %sel = extractvalue { ptr, i32 } %lp, 1
  %foo = call i32 @llvm.eh.typeid.for.p0(ptr @_ZTI3Foo)
  %is.foo = icmp eq i32 %sel, %foo
  br i1 %is.foo, label %handler, label %unwind

handler:
  %exn = extractvalue { ptr, i32 } %lp, 0
  %obj = call ptr @__cxa_begin_catch(ptr %exn)
  call void @__cxa_end_catch()
  ret i32 1

unwind:
  resume { ptr, i32 } %lp
}

; A funclet personality has no lowering, so its unwind edge is dropped.
; CHECK-LABEL: 'msvc'()
; CHECK-NOT: .try
; CHECK: call void 'may_throw'()
; CHECK-NOT: .try
; CHECK: ret
define void @msvc() personality ptr @__CxxFrameHandler3 {
entry:
  invoke void @may_throw()
          to label %ok unwind label %lpad

ok:
  ret void

lpad:
  %cs = catchswitch within none [label %catch] unwind to caller

catch:
  %cp = catchpad within %cs [ptr null, i32 64, ptr null]
  catchret from %cp to label %ok
}

; CHECK-LABEL: '$MSIL_Init'()
; CHECK: ldstr "_ZTIi"
; CHECK-NEXT: call void* [MSIL.Runtime]MSIL.Runtime.CxxAbi::Symbol(string)
; CHECK-NEXT: stsfld void* '_ZTIi'
