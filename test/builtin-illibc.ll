; RUN: llvm-msil %s -o - --msil-libc=builtin | FileCheck %s

; The portable C library (libc/*.c) deliberately leaves these 18 names to the
; hand-written IL subset. Under 'builtin' they must bind to $MSIL_Libc instead
; of a CRT pinvoke.
declare ptr @malloc(i64)
declare void @free(ptr)
declare ptr @calloc(i64, i64)
declare ptr @realloc(ptr, i64)
declare ptr @memcpy(ptr, ptr, i64)
declare ptr @memmove(ptr, ptr, i64)
declare ptr @memset(ptr, i32, i64)
declare i32 @memcmp(ptr, ptr, i64)
declare i64 @strlen(ptr)
declare i32 @strcmp(ptr, ptr)
declare i32 @strncmp(ptr, ptr, i64)
declare ptr @strcpy(ptr, ptr)
declare ptr @strncpy(ptr, ptr, i64)
declare ptr @strcat(ptr, ptr)
declare ptr @strchr(ptr, i32)
declare ptr @strrchr(ptr, i32)
declare ptr @strstr(ptr, ptr)
declare ptr @strdup(ptr)

define i64 @f(ptr %p, ptr %q) {
  %m = call ptr @malloc(i64 8)
  %c = call ptr @memcpy(ptr %m, ptr %p, i64 8)
  %l = call i64 @strlen(ptr %q)
  call void @free(ptr %m)
  ret i64 %l
}

; CHECK-NOT: pinvokeimpl
; The forwarders come before the class; each calls into $MSIL_Libc.
; CHECK: call {{.*}}$MSIL_Libc::malloc
; CHECK: .class public auto ansi abstract sealed beforefieldinit '$MSIL_Libc'
; CHECK-DAG: .method {{.*}} 'malloc'(int64)
; CHECK-DAG: .method {{.*}} 'free'(void*)
; CHECK-DAG: .method {{.*}} 'calloc'(int64, int64)
; CHECK-DAG: .method {{.*}} 'realloc'(void*, int64)
; CHECK-DAG: .method {{.*}} 'memcpy'(void*, void*, int64)
; CHECK-DAG: .method {{.*}} 'memmove'(void*, void*, int64)
; CHECK-DAG: .method {{.*}} 'memset'(void*, int32, int64)
; CHECK-DAG: .method {{.*}} 'memcmp'(void*, void*, int64)
; CHECK-DAG: .method {{.*}} 'strlen'(void*)
; CHECK-DAG: .method {{.*}} 'strcmp'(void*, void*)
; CHECK-DAG: .method {{.*}} 'strncmp'(void*, void*, int64)
; CHECK-DAG: .method {{.*}} 'strcpy'(void*, void*)
; CHECK-DAG: .method {{.*}} 'strncpy'(void*, void*, int64)
; CHECK-DAG: .method {{.*}} 'strcat'(void*, void*)
; CHECK-DAG: .method {{.*}} 'strchr'(void*, int32)
; CHECK-DAG: .method {{.*}} 'strrchr'(void*, int32)
; CHECK-DAG: .method {{.*}} 'strstr'(void*, void*)
; CHECK-DAG: .method {{.*}} 'strdup'(void*)
