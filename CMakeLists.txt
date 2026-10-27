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
  # known first. It is only available from LLVM's config: LLVMConfig.cmake sets
  # LLVM_BUILD_TYPE.
  #
  # Enabling a language is what makes LLVMConfig's find_package(ZLIB)/zstd/
  # LibXml2 see the compiler's library directories -- without one they fail to
  # find the multiarch path and configure dies on a missing ZLIB::ZLIB. Doing so
  # also defaults CMAKE_BUILD_TYPE to Debug, so a user's choice is remembered
  # and LLVM's is put back otherwise.
  project(msil-build-type-probe NONE)
  if (CMAKE_BUILD_TYPE)
    set(MSIL_BUILD_TYPE_GIVEN 1)
  endif()
  enable_language(CXX)
  find_package(LLVM 22.1 REQUIRED CONFIG)
  if (NOT MSIL_BUILD_TYPE_GIVEN)
    set(CMAKE_BUILD_TYPE "${LLVM_BUILD_TYPE}" CACHE STRING "" FORCE)
  endif()

  # The Windows release tarball records the DIA SDK path of the machine that
  # built it, which is not necessarily where DIA is installed, or whether it is
  # installed at all. DIA is reached only through LLVMSymbolize, which nothing
  # here links, so an entry naming a file that is not there is dropped instead
  # of failing the link.
  if (TARGET LLVMDebugInfoPDB)
    get_target_property(MSIL_PDB_DEPS LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)
    if (MSIL_PDB_DEPS)
      set(MSIL_PDB_KEPT)
      foreach(DEP IN LISTS MSIL_PDB_DEPS)
        if (DEP MATCHES "diaguids\\.lib$" AND NOT EXISTS "${DEP}")
          continue()
        endif()
        list(APPEND MSIL_PDB_KEPT "${DEP}")
      endforeach()
      set_target_properties(LLVMDebugInfoPDB PROPERTIES
                            INTERFACE_LINK_LIBRARIES "${MSIL_PDB_KEPT}")
    endif()
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
