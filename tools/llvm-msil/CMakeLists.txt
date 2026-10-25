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
