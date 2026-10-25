; RUN: llvm-msil %s -o - | FileCheck %s --check-prefix=DEFAULT
; RUN: llvm-msil %s -o - -msil-class-name=My.App | FileCheck %s --check-prefix=CLASS

; Without -msil-class-name every member is free-standing in <Module>, which is
; valid IL but can only be reached from C# through reflection.
; DEFAULT-NOT: .class public
; DEFAULT: .method static public hidebysig int32 'main'()

; With it, the callable members move into the named class. C# can then write
; `My.App.main()` directly.
; CLASS: .class public auto ansi beforefieldinit 'My.App'
; CLASS: .method static public hidebysig int32 'add'
; CLASS: .method static public hidebysig int32 'main'
; The entry point has to stay in <Module> — IL does not allow .entrypoint
; inside a class — so it names the class explicitly.
; CLASS: .method static public int32 '$MSIL_Startup'()
; CLASS: .entrypoint
; CLASS: call {{.*}}int32 'My.App'::'main'()

@g = global i32 100

define i32 @add(i32 %a) {
  %v = load i32, ptr @g
  %r = add i32 %v, %a
  ret i32 %r
}

define i32 @main() {
  %r = call i32 @add(i32 7)
  ret i32 %r
}
