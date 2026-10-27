include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# The component group has to exist before TargetInfo adds a library to it.
if (NOT USE_SYSTEM_LLVM)
  add_llvm_component_group(MSIL)
endif()

add_subdirectory(TargetInfo)

#--- Portable C library -------------------------------------------------------
#
# libc/*.c is compiled into one IR module and embedded in the backend, so the
# tool carries its libc with no data file to install. Compiling at build time
# rather than checking in a .ll keeps the C the only source of truth, and keeps
# the IR from drifting away from it.
#
# The compiler must be able to target the Windows ABI: that decides
# sizeof(long) and the vararg accessors the seam emits.
set(MSIL_LIBC_SOURCES
  ctype.c
  string.c
  stdlib.c
  stdio.c
)

# Prefer compiling the C: the checked-in .ll would otherwise be a second copy
# of the truth, and only stays correct until someone edits the C. It is the
# fallback for a build with no clang that can target Windows -- an in-tree
# LLVM build, typically.
set(MSIL_LIBC_PREBUILT "${CMAKE_CURRENT_SOURCE_DIR}/libc.ll")

find_program(MSIL_LIBC_CLANG NAMES clang clang.exe
             HINTS "${LLVM_TOOLS_BINARY_DIR}"
                   "${CMAKE_CURRENT_SOURCE_DIR}/../../../../build/bin"
             PATHS ENV MSIL_CLANG
                   "C:/Program Files/LLVM/bin")
if (MSIL_LIBC_CLANG)
  get_filename_component(MSIL_CLANG_DIR "${MSIL_LIBC_CLANG}" DIRECTORY)
  find_program(MSIL_LIBC_LLVM_LINK NAMES llvm-link llvm-link.exe
               HINTS "${MSIL_CLANG_DIR}" "${LLVM_TOOLS_BINARY_DIR}"
               PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../build/bin")
endif()

if (MSIL_LIBC_CLANG AND MSIL_LIBC_LLVM_LINK)
  # Two ABI builds. long and wchar_t follow the target triple, and the
  # library's public signatures have to match the program's: a Linux-triple
  # program declares strtol as returning i64, which the Windows-ABI copy
  # (i32) cannot satisfy. linkPortableLibc picks by the module's triple.
  set(MSIL_LIBC_TRIPLES x86_64-pc-windows-msvc x86_64-unknown-linux-gnu)
  set(MSIL_LIBC_SUFFIXES win sysv)
  set(MSIL_LIBC_EMBEDS)
  list(LENGTH MSIL_LIBC_TRIPLES NTRIPLES)
  math(EXPR LAST_TRIPLE "${NTRIPLES} - 1")
  foreach(I RANGE ${LAST_TRIPLE})
    list(GET MSIL_LIBC_TRIPLES ${I} TRIPLE)
    list(GET MSIL_LIBC_SUFFIXES ${I} SUFFIX)
    set(MSIL_LIBC_OBJS)
    foreach(SRC IN LISTS MSIL_LIBC_SOURCES)
      get_filename_component(NAME "${SRC}" NAME_WE)
      set(OBJ "${CMAKE_CURRENT_BINARY_DIR}/libc-${SUFFIX}-${NAME}.ll")
      add_custom_command(
        OUTPUT "${OBJ}"
        COMMAND "${MSIL_LIBC_CLANG}" -target ${TRIPLE}
                -ffreestanding -nostdinc -fno-builtin
                -O2 -fno-vectorize -fno-slp-vectorize
                -S -emit-llvm -w -fno-discard-value-names
                -o "${OBJ}" "${CMAKE_CURRENT_SOURCE_DIR}/libc/${SRC}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libc/${SRC}"
                "${CMAKE_CURRENT_SOURCE_DIR}/libc/libc.h"
        COMMENT "Compiling portable libc ${SRC} to ${TRIPLE} IR"
        VERBATIM)
      list(APPEND MSIL_LIBC_OBJS "${OBJ}")
    endforeach()

    # llvm-link merges the four modules into one the backend can parse; plain
    # concatenation would repeat the module header and be rejected.
    set(IR "${CMAKE_CURRENT_BINARY_DIR}/libc-${SUFFIX}.ll")
    add_custom_command(
      OUTPUT "${IR}"
      COMMAND "${MSIL_LIBC_LLVM_LINK}" -S ${MSIL_LIBC_OBJS} -o "${IR}"
      DEPENDS ${MSIL_LIBC_OBJS}
      COMMENT "Linking portable libc IR (${TRIPLE})"
      VERBATIM)
    if(SUFFIX STREQUAL "win")
      list(APPEND MSIL_LIBC_EMBEDS "${IR}=libc-win=PortableLibcIR")
    else()
      list(APPEND MSIL_LIBC_EMBEDS "${IR}=libc-sysv=PortableLibcIRSysV")
    endif()
  endforeach()
elseif(EXISTS "${MSIL_LIBC_PREBUILT}")
  # The checked-in file is the Windows-ABI build, so it carries the original
  # symbol name and there is no SysV copy to select between.
  set(MSIL_LIBC_EMBEDS "${MSIL_LIBC_PREBUILT}=libc-win=PortableLibcIR")
  message(STATUS "Portable libc: using the checked-in IR (no clang found); "
                 "run scripts/update-libc-ir.sh after changing libc/*.c")
else()
  message(FATAL_ERROR
    "The portable libc needs clang and llvm-link to build its IR, and there "
    "is no checked-in libc.ll to fall back on. Set MSIL_CLANG in the "
    "environment or pass -DMSIL_LIBC_CLANG=...")
endif()

# The IR ships as a C string literal so there is no data file to find at run
# time. The Windows-ABI copy keeps the original symbol name.
set(MSIL_LIBC_INCS)
foreach(ENTRY IN LISTS MSIL_LIBC_EMBEDS)
  string(REPLACE "=" ";" PAIR "${ENTRY}")
  list(GET PAIR 0 IR)
  list(GET PAIR 1 STEM)
  list(GET PAIR 2 SYMBOL)
  set(INC "${CMAKE_CURRENT_BINARY_DIR}/${STEM}.inc")
  add_custom_command(
    OUTPUT "${INC}"
    COMMAND ${CMAKE_COMMAND}
            -DINPUT=${IR}
            -DOUTPUT=${INC}
            -DNAME=${SYMBOL}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/EmbedIR.cmake"
    DEPENDS "${IR}" "${CMAKE_CURRENT_SOURCE_DIR}/EmbedIR.cmake"
    COMMENT "Embedding portable libc IR"
    VERBATIM)
  list(APPEND MSIL_LIBC_INCS "${INC}")
endforeach()
add_custom_target(MSILLibcIRInc DEPENDS ${MSIL_LIBC_INCS})
set_source_files_properties(MSILWriter.cpp PROPERTIES OBJECT_DEPENDS
                            "${MSIL_LIBC_INCS}")

# A standalone build links against an LLVM whose export file may already define
# LLVMMSILCodeGen (when that LLVM was itself built with this project in-tree),
# so the target gets a suffix while the artifact keeps its name.
if (USE_SYSTEM_LLVM)
  add_library(${MSIL_CODEGEN_TARGET} STATIC MSILTargetMachine.cpp MSILWriter.cpp)
  set_target_properties(${MSIL_CODEGEN_TARGET}
    PROPERTIES OUTPUT_NAME LLVMMSILCodeGen)
  # add_library does not apply LLVM's -fno-rtti/-fno-exceptions the way
  # add_llvm_* does; without them the vtables reference typeinfo that an
  # RTTI-off LLVM does not export (and /EHs-c- and _HAS_EXCEPTIONS=0 are the
  # MSVC half of the same setting).
  llvm_update_compile_flags(${MSIL_CODEGEN_TARGET})
  target_link_libraries(${MSIL_CODEGEN_TARGET} PRIVATE
    LLVMAsmParser LLVMCodeGen LLVMCore LLVMDemangle LLVMIRReader
    LLVMLinker ${MSIL_INFO_TARGET} LLVMScalarOpts LLVMSupport LLVMTarget
    LLVMTransformUtils)
else()
  # add_llvm_target assumes the in-tree layout with LLVMBuild.txt-style target
  # registration; this target is built as a component library instead.
  add_llvm_component_library(LLVMMSILCodeGen
    MSILTargetMachine.cpp
    MSILWriter.cpp

    LINK_COMPONENTS
    AsmParser
    CodeGen
    Core
    Demangle
    IRReader
    Linker
    MSILInfo
    ScalarOpts
    Support
    Target
    TransformUtils

    ADD_TO_COMPONENT
    MSIL
    )
endif()

add_dependencies(${MSIL_CODEGEN_TARGET} MSILLibcIRInc)
