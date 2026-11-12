; RUN: llvm-msil %s -o - | FileCheck %s

; The writer cannot emit a `switch` instruction: LowerSwitch lowers it and a
; second LowerSwitch after CFGSimplify keeps it lowered (CFGSimplify can fold
; a small comparison tree back into a switch).
define i32 @sw(i32 %x) {
entry:
  switch i32 %x, label %def [ i32 0, label %a
                              i32 1, label %b
                              i32 2, label %c ]
a: ret i32 10
b: ret i32 20
c: ret i32 30
def: ret i32 0
}

; CHECK: .method {{.*}} 'sw'
; CHECK-NOT: switch
