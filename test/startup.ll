; RUN: llvm-msil %s -o - | FileCheck %s

; $MSIL_Startup builds argc/argv for main(int, char**), calls $MSIL_Init for
; writable globals first, and converts main's return to the process exit code.

@x = global i32 1

define i32 @main(i32 %argc, ptr %argv) {
; CHECK-LABEL: 'main'
; CHECK: ret i32
  ret i32 %argc
}

; CHECK: .field static private void* '$MSIL::argc'
; CHECK: .field static private void* '$MSIL::argv'
; CHECK-LABEL: '$MSIL_MakeArgs'
; CHECK: GetCommandLineArgs()
; CHECK: StringToHGlobalAnsi(string)

; CHECK-LABEL: '$MSIL_Startup'
; CHECK: .entrypoint
; CHECK: call void '$MSIL_Init'()
; CHECK: call void '$MSIL_MakeArgs'()
; CHECK: ldsfld void* '$MSIL::argc'
; CHECK: ldsfld void* '$MSIL::argv'
; CHECK: call int32 'main'(int32, void*)
; CHECK: conv.i4
; CHECK: ret
