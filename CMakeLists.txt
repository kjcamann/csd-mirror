cmake_minimum_required(VERSION 3.12.1)

project(CSD VERSION 2021.1 LANGUAGES CXX)

set(CSD_RELEASE_TAG_SUFFIX "" CACHE STRING
    "Suffix added to the release, e.g. for alpha, beta versions")

# Add csd as a header-only library target
add_library(csd INTERFACE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

target_include_directories(csd INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_features(csd INTERFACE cxx_std_23)

option(CSD_BUILD_TESTS "Build unit test executables" ON)
option(CSD_BUILD_CODEGEN_TESTS "Build code generation quality tests" ON)
option(CSD_BUILD_DOCS "Build doxygen/sphinx documentation" OFF)

if (CSD_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if (CSD_BUILD_DOCS)
  add_subdirectory(doc)
endif()

install(DIRECTORY include/csd DESTINATION include)
