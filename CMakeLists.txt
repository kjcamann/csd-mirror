cmake_minimum_required(VERSION 3.12.1)

project(BDS VERSION 2019.1 LANGUAGES CXX)

set(BDS_RELEASE_TAG_SUFFIX "" CACHE STRING
    "Suffix added to the release, e.g. for alpha, beta versions")

# Add bds as a header-only library target
add_library(bds INTERFACE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

target_include_directories(bds INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_features(bds INTERFACE cxx_std_20)

# FIXME: this flag only works for gcc for now, but concepts will probably be
# listed in cmake-compile-featues(7) in a future list.
target_compile_options(bds INTERFACE -fconcepts)

option(BUILD_BDS_TESTS "Build unit test executables" ON)
option(BUILD_BDS_DOCS "Build doxygen/sphinx documentation" OFF)

if (BUILD_BDS_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if (BUILD_BDS_DOCS)
  add_subdirectory(doc)
endif()

install(DIRECTORY include/bds DESTINATION include)
