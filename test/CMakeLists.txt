llvm_canonicalize_cmake_booleans(
  BUILD_SHARED_LIBS
  )

set(LLVM_MSIL_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  PATHS
  "LLVM_TOOLS_DIR"
  "SHLIBDIR"
  )

# Named check-msil, not check-all, so moving this out of the LLVM tree does not
# silently change what the LLVM test suite builds.
add_lit_testsuite(check-msil "Running the llvm-msil regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS llvm-msil
  )
set_target_properties(check-msil PROPERTIES FOLDER "MSIL/Meta")
