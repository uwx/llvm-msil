if (USE_SYSTEM_LLVM)
  add_executable(${MSIL_TOOL_TARGET} llvm-msil.cpp)
  # The driver asks the target machine whether the emitted module needs
  # MSIL.Runtime, so it includes that header.
  target_include_directories(${MSIL_TOOL_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/Target/MSIL)
  set_target_properties(${MSIL_TOOL_TARGET} PROPERTIES OUTPUT_NAME llvm-msil)
  llvm_update_compile_flags(${MSIL_TOOL_TARGET})
  target_link_libraries(${MSIL_TOOL_TARGET} PRIVATE
    ${MSIL_CODEGEN_TARGET} ${MSIL_INFO_TARGET}
    LLVMAnalysis LLVMAsmParser LLVMBitReader LLVMCodeGen LLVMCore
    LLVMIRReader LLVMMC LLVMScalarOpts LLVMSupport LLVMTarget
    LLVMTransformUtils)
else()
  set(LLVM_LINK_COMPONENTS
    Analysis
    AsmParser
    BitReader
    CodeGen
    Core
    IRReader
    Linker
    MC
    MSILCodeGen
    MSILInfo
    ScalarOpts
    Support
    Target
    TransformUtils
    )

  add_llvm_tool(llvm-msil
    llvm-msil.cpp

    DEPENDS
    intrinsics_gen
    )

  # The driver asks the target machine whether the emitted module needs
  # MSIL.Runtime, so it includes that header.
  target_include_directories(llvm-msil PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/Target/MSIL)
endif()

# The driver copies MSIL.Runtime.dll beside whatever it emits, for a module
# that needs the C++ exception runtime. It looks next to its own executable
# first, so build the runtime into that directory. The build is skipped when
# there is no .NET SDK, which leaves the warning in the driver as the only
# sign -- an ordinary C module never asks for the DLL.
find_program(MSIL_DOTNET NAMES dotnet dotnet.exe)
set(MSIL_RUNTIME_PROJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/MSIL.Runtime/MSIL.Runtime.csproj)
if (MSIL_DOTNET AND EXISTS "${MSIL_RUNTIME_PROJECT}")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/MSIL.Runtime.dll"
    COMMAND "${MSIL_DOTNET}" build "${MSIL_RUNTIME_PROJECT}"
            -c Release -nologo -v q
            -o "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${MSIL_RUNTIME_PROJECT}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/MSIL.Runtime/CxxAbi.cs
            ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/MSIL.Runtime/CxxException.cs
            ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/MSIL.Runtime/ExceptionHeader.cs
            ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/MSIL.Runtime/RuntimeSymbols.cs
            ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/MSIL.Runtime/TypeInfo.cs
            ${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/MSIL.Runtime/TypeMatcher.cs
    COMMENT "Building MSIL.Runtime.dll beside llvm-msil"
    VERBATIM)
  add_custom_target(MSILRuntimeDll ALL
                    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/MSIL.Runtime.dll")
  add_dependencies(${MSIL_TOOL_TARGET} MSILRuntimeDll)
else()
  message(STATUS "MSIL.Runtime: no .NET SDK found; the driver will warn when "
                 "a module needs it")
endif()
