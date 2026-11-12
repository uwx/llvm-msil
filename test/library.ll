; RUN: llvm-msil %s -o - -msil-library -msil-class-name=L | FileCheck %s --check-prefix=TPL
; RUN: not llvm-msil %s -o - -msil-library 2>&1 | FileCheck %s --check-prefix=NOCLASS
; RUN: llvm-msil %s -o - -msil-class-name=L -msil-assembly-name=LibXmp.Managed | FileCheck %s --check-prefix=NAME

@g = global i32 7

define i32 @add(i32 %a) {
  %v = load i32, ptr @g
  %r = add i32 %v, %a
  ret i32 %r
}

; TPL: .method public static void 'MSILInitialize'()
; TPL: ldsfld bool 'L'::'$MSIL_Initialized'
; TPL: brtrue
; TPL: call {{.*}}'L'::'$MSIL_Init'()
; TPL-NOT: .entrypoint
; TPL-NOT: '$MSIL_Startup'

; NOCLASS: -msil-library needs -msil-class-name

; NAME: .assembly LibXmp.Managed {}
