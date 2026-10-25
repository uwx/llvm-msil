if (USE_SYSTEM_LLVM)
  add_library(${MSIL_INFO_TARGET} STATIC MSILTargetInfo.cpp)
  set_target_properties(${MSIL_INFO_TARGET}
    PROPERTIES OUTPUT_NAME LLVMMSILInfo)
  target_link_libraries(${MSIL_INFO_TARGET} PRIVATE LLVMMC LLVMSupport)
else()
  add_llvm_component_library(LLVMMSILInfo
    MSILTargetInfo.cpp

    LINK_COMPONENTS
    MC
    Support

    ADD_TO_COMPONENT
    MSIL
    )
endif()
