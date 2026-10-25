if (USE_SYSTEM_LLVM)
  add_executable(${MSIL_TOOL_TARGET} llvm-msil.cpp)
  set_target_properties(${MSIL_TOOL_TARGET} PROPERTIES OUTPUT_NAME llvm-msil)
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
endif()
