include(ExternalProject)

# Find git; git clone will be used to retreive the "catch2" external project
find_package(Git)

list(APPEND CATCH_CMAKE_ARGS
     "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external"
     "-DNO_SELFTEST=true"
     )

if (GIT_FOUND)
  # We have a git client; clone the catch2 repository into the `catch`
  # subdirectory of the build tree, and install it locally to the
  # `external` subdirectory of the build tree.
  ExternalProject_Add(
    catch
    PREFIX "${CMAKE_BINARY_DIR}/catch"
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v2.5.0
    CMAKE_ARGS ${CATCH_CMAKE_ARGS}
    LOG_DOWNLOAD 1
    UPDATE_DISCONNECTED 1
    )
else()
  # We don't have git so we cannot clone catch; we make a dummy target and our
  # dependencies on it will do nothing. If catch2 is installed to a system
  # path, then `#include <catch/catch.hpp>` will succeed in any case.
  add_custom_target(catch)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(test_driver STATIC driver.cpp)
set_property(TARGET test_driver PROPERTY CXX_STANDARD 17)

target_include_directories(test_driver PRIVATE
    "${CMAKE_BINARY_DIR}/external/include")

function(add_bds_test name)
  add_executable(${name} "${name}.cpp")
  set_property(TARGET ${name} PROPERTY FOLDER "bds_tests")
  set_property(TARGET ${name} PROPERTY CXX_STANDARD 17)

  # Ensure catch includes can be found, and link to bds
  target_include_directories(${name} PRIVATE
    "${CMAKE_BINARY_DIR}/external/include")
  target_link_libraries(${name} PRIVATE test_driver bds)

  add_dependencies("${name}" catch)
  add_test(${name} ${name})
endfunction()

add_bds_test(slist_tests)
add_bds_test(stailq_tests)
add_bds_test(tailq_tests)
