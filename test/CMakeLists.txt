include(ExternalProject)

# Find git; git clone will be used to retreive the "catch2" external project
find_package(Git)

list(APPEND CATCH_CMAKE_ARGS
     "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external"
     "-DNO_SELFTEST=true"
     )

if (GIT_FOUND)
  # We have a git client; clone the catch2 repository into the `catch2`
  # subdirectory of the build tree, and install it locally to the
  # `external` subdirectory of the build tree.
  ExternalProject_Add(
    catch2
    PREFIX "${CMAKE_BINARY_DIR}/catch2"
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v2.8.0
    CMAKE_ARGS ${CATCH_CMAKE_ARGS}
    LOG_DOWNLOAD 1
    UPDATE_DISCONNECTED 1
    )
else()
  # We don't have git so we cannot clone catch2; we make a dummy target and our
  # dependencies on it will do nothing. If catch2 is installed to a system
  # path, then `#include <catch2/catch.hpp>` will succeed in any case.
  add_custom_target(catch2)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(test_driver STATIC driver.cpp)
set_property(TARGET test_driver PROPERTY CXX_STANDARD 20)

target_include_directories(test_driver PRIVATE
    "${CMAKE_BINARY_DIR}/external/include")

add_dependencies(test_driver catch2)

function(add_csd_test name)
  add_executable(${name} "${name}.cpp")
  set_property(TARGET ${name} PROPERTY FOLDER "csd_tests")
  set_property(TARGET ${name} PROPERTY CXX_STANDARD 20)

  # Ensure catch2 includes can be found, and link to csd
  target_include_directories(${name} PRIVATE
    "${CMAKE_BINARY_DIR}/external/include")
  target_link_libraries(${name} PRIVATE test_driver csd)

  add_test(${name} ${name})
endfunction()

add_csd_test(slist_tests)
add_csd_test(stailq_tests)
add_csd_test(tailq_tests)
