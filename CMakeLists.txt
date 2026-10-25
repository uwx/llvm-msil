# Builds either in-tree (as an llvm/projects entry) or standalone against an
# installed LLVM. Standalone is how this is meant to be used during
# development: it links the already-built LLVM libraries, so a rebuild takes
# seconds instead of the half hour LLVM itself needs.
#
#   cmake -S . -B build -G Ninja -DLLVM_DIR=<llvm-build>/lib/cmake/llvm

if (NOT DEFINED LLVM_VERSION_MAJOR)
  cmake_minimum_required(VERSION 3.20)
  set(USE_SYSTEM_LLVM 1)

  if (NOT LLVM_DIR)
    message(FATAL_ERROR
      "Set -DLLVM_DIR=<path>/lib/cmake/llvm (the build tree works too).")
  endif()

  # project() is what fixes the compiler flags, so the build type has to be
  # known first. It is only available from LLVM's config, hence the throwaway
  # project() here and the real one below.
  project(msil-build-type-probe NONE)
  find_package(LLVM 22.1 REQUIRED CONFIG)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "${LLVM_BUILD_TYPE}" CACHE STRING "" FORCE)
  endif()

  project(llvm-msil CXX)
  message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
  message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

  if (NOT CMAKE_BUILD_TYPE STREQUAL LLVM_BUILD_TYPE)
    message(WARNING
      "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE} but LLVM was built as "
      "${LLVM_BUILD_TYPE}; linking will likely fail.")
  endif()

  include_directories(${LLVM_INCLUDE_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
  list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
  include(AddLLVM)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Target names differ between the two modes; the artifact names do not.
if (USE_SYSTEM_LLVM)
  set(MSIL_CODEGEN_TARGET LLVMMSILCodeGenStandalone)
  set(MSIL_INFO_TARGET LLVMMSILInfoStandalone)
  set(MSIL_TOOL_TARGET llvm-msil-standalone)
else()
  set(MSIL_CODEGEN_TARGET LLVMMSILCodeGen)
  set(MSIL_INFO_TARGET LLVMMSILInfo)
  set(MSIL_TOOL_TARGET llvm-msil)
endif()

add_subdirectory(lib)
add_subdirectory(tools)

if (LLVM_INCLUDE_TESTS)
  add_subdirectory(test)
endif()
