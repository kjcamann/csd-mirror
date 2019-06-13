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
    GIT_TAG v2.11.0
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

target_compile_definitions(test_driver INTERFACE CSG_DEBUG_LEVEL=1)
target_link_libraries(test_driver PUBLIC csd)

add_dependencies(test_driver catch2)

function(add_csd_test name)
  add_executable(${name} "${name}.cpp")
  set_property(TARGET ${name} PROPERTY FOLDER "csd_tests")

  # Ensure catch2 includes can be found
  target_include_directories(${name} PRIVATE
    "${CMAKE_BINARY_DIR}/external/include")
  target_link_libraries(${name} PRIVATE test_driver)

  add_test(${name} ${name})
endfunction()

add_csd_test(slist_tests)
add_csd_test(stailq_tests)
add_csd_test(tailq_tests)

function(add_codegen_test)
  # add_codegen_test(NAME <name> SOURCES [source1] [source2 ...]
  #                  DEFINITIONS [compile-def1] [compile-def2 ...]
  #                  [ASM_OUTPUT] [MACHINE_ARCH <arch>)
  #
  # Function to add code generation tests. These are used to compare the
  # quality of generated code for offset vs. invocable entry extractors,
  # and to check that any changes to the code do not result in worse code
  # being generated. The tests always compile with -O3 no matter what the
  # value of CMAKE_BUILD_TYPE is. The machine architecture (the value of
  # the `-march` option for gcc and clang) can be specified using the
  # MACHINE_ARCH keyword, otherwise it will default to `native`.
  #
  # A typical call is:
  #
  #   add_codegen_test(NAME foo SOURCES foo_test.cpp DEFINITIONS FOO_TEST=1
  #                    ASM_OUTPUT MACHINE_ARCH skylake)
  #
  # This creates a static library target called <NAME>_codegen.a having the
  # given source files and setting the given compile definitions. The results
  # of this "test" can be compared by hand, with:
  #
  #   objdump -d <library-file>
  #
  # If the ASM_OUTPUT option is present and CMAKE_OBJDUMP is defined, a
  # custom target action will be added to do this automatically, creating
  # the output file `<NAME>.s`. This output can be compared to the reference
  # codegen output stored in $PROJECT_SOURCE_DIR/test/codegen_results for
  # various architectures. It can also be used to compare the code quality
  # of offset vs. invocable extractors. If invocable extractors can ever
  # match the efficiency of offset-based extractors, then offset-based
  # extractors can be removed, dramatically simplifying the implementation.
  set(options ASM_OUTPUT)
  set(oneValueArgs NAME MACHINE_ARCH)
  set(multiValueArgs DEFINITIONS SOURCES)

  cmake_parse_arguments(CG_TEST "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  if (NOT CG_TEST_NAME)
    message(FATAL_ERROR "codegen test must have a name")
  else()
    set(CG_TEST_TARGET "${CG_TEST_NAME}_codegen")
  endif()

  if (NOT CG_TEST_MACHINE_ARCH)
    set(CG_TEST_MACHINE_ARCH native)
  endif()

  add_library(${CG_TEST_TARGET} STATIC ${CG_TEST_SOURCES})
  target_compile_definitions(${CG_TEST_TARGET} PRIVATE ${CG_TEST_DEFINITIONS})
  target_compile_options(${CG_TEST_TARGET} PRIVATE -O3
                         "-march=${CG_TEST_MACHINE_ARCH}")
  target_link_libraries(${CG_TEST_TARGET} PRIVATE csd)

  if (CG_TEST_ASM_OUTPUT)
    if (CMAKE_OBJDUMP)
      add_custom_command(TARGET ${CG_TEST_TARGET} POST_BUILD
          COMMAND ${CMAKE_OBJDUMP} -d $<TARGET_FILE:${CG_TEST_TARGET}> > "${CG_TEST_NAME}.s"
	  COMMENT "Generating assembly dump of ${CG_TEST_NAME}")
    else()
      message(WARNING "Wanted assembly dump of ${CG_TEST_NAME} codegen test but no objdump found")
    endif()
  endif()
endfunction()

if (CSD_BUILD_CODEGEN_TESTS)
  add_codegen_test(NAME lists_offset ASM_OUTPUT SOURCES lists_codegen.cpp
                   DEFINITIONS CODEGEN_OFFSET)

  add_codegen_test(NAME lists_invoke ASM_OUTPUT SOURCES lists_codegen.cpp
                   DEFINITIONS CODEGEN_INVOKE)
endif()
