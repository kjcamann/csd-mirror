cmake_minimum_required(VERSION 3.12.1)

project(CSD VERSION 2019.1 LANGUAGES CXX)

set(CSD_RELEASE_TAG_SUFFIX "" CACHE STRING
    "Suffix added to the release, e.g. for alpha, beta versions")

# Add csd as a header-only library target
add_library(csd INTERFACE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

target_include_directories(csd INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# FIXME: this is not recognized yet and CMake does not allow us to set
# CXX_STANDARD on interface libraries. It doesn't matter here because we'll
# set CXX_STANDARD on the test executables anyway but ideally we would only
# set it here.
target_compile_features(csd INTERFACE cxx_std_20)

# FIXME: this should be implied by C++20 but it isn't because of experimental
# concepts support in both clang and gcc
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(csd INTERFACE -Xclang -fconcepts-ts)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(csd INTERFACE -fconcepts)
endif()

option(BUILD_CSD_TESTS "Build unit test executables" ON)
option(BUILD_CSD_DOCS "Build doxygen/sphinx documentation" OFF)

if (BUILD_CSD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if (BUILD_CSD_DOCS)
  add_subdirectory(doc)
endif()

install(DIRECTORY include/csd DESTINATION include)
