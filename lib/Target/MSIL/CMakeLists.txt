include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# The component group has to exist before TargetInfo adds a library to it.
if (NOT USE_SYSTEM_LLVM)
  add_llvm_component_group(MSIL)
endif()

add_subdirectory(TargetInfo)

# A standalone build links against an LLVM whose export file may already define
# LLVMMSILCodeGen (when that LLVM was itself built with this project in-tree),
# so the target gets a suffix while the artifact keeps its name.
if (USE_SYSTEM_LLVM)
  add_library(${MSIL_CODEGEN_TARGET} STATIC MSILTargetMachine.cpp MSILWriter.cpp)
  set_target_properties(${MSIL_CODEGEN_TARGET}
    PROPERTIES OUTPUT_NAME LLVMMSILCodeGen)
  target_link_libraries(${MSIL_CODEGEN_TARGET} PRIVATE
    LLVMCodeGen LLVMCore LLVMDemangle ${MSIL_INFO_TARGET} LLVMScalarOpts
    LLVMSupport LLVMTarget LLVMTransformUtils)
else()
  # add_llvm_target assumes the in-tree layout with LLVMBuild.txt-style target
  # registration; this target is built as a component library instead.
  add_llvm_component_library(LLVMMSILCodeGen
    MSILTargetMachine.cpp
    MSILWriter.cpp

    LINK_COMPONENTS
    CodeGen
    Core
    Demangle
    MSILInfo
    ScalarOpts
    Support
    Target
    TransformUtils

    ADD_TO_COMPONENT
    MSIL
    )
endif()
