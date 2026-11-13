; RUN: rm -rf %t.dir && mkdir %t.dir
; RUN: yaml2obj %S/Inputs/libfake-elf.yaml -o %t.dir/libfake.so.6
; RUN: llvm-msil %s -o - -msil-lib=%t.dir/libfake.so.6 | FileCheck %s

; -msil-lib reads an ELF shared object's dynamic symbol table the same way it
; reads a DLL's exports. glibc keeps libm out of libc.so.6, so this is how
; lgamma_r and friends find libm.so.6.

target triple = "x86_64-unknown-linux-gnu"

declare double @lgamma_r(double, ptr)
declare void @weak_export()
declare void @imported_here()
declare i32 @puts(ptr)

; CHECK-DAG: pinvokeimpl("libfake.so.6" as "lgamma_r" cdecl)
; CHECK-DAG: pinvokeimpl("libfake.so.6" as "weak_export" cdecl)
; an undefined entry in the library's table is an import, not an export
; CHECK-DAG: pinvokeimpl("libc.so.6" as "imported_here" cdecl)
; CHECK-DAG: pinvokeimpl("libc.so.6" as "puts" cdecl)

define i32 @main() {
  %g = call double @lgamma_r(double 1.0, ptr null)
  call void @weak_export()
  call void @imported_here()
  %r = call i32 @puts(ptr null)
  ret i32 0
}
