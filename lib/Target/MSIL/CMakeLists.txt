add_llvm_component_group(MSIL)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(TargetInfo)

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
