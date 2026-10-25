add_llvm_component_library(LLVMMSILInfo
  MSILTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  MSIL
  )
