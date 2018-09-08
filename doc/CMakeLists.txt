find_package(Sphinx REQUIRED)
find_package(Doxygen REQUIRED OPTIONAL_COMPONENTS dot)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in"
               "${CMAKE_CURRENT_BINARY_DIR}/doxygen/Doxyfile" @ONLY)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sphinx/conf.py.in"
               "${CMAKE_CURRENT_BINARY_DIR}/sphinx/conf.py" @ONLY)

set(SPHINX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sphinx")
set(SPHINX_CONF_DIR "${CMAKE_CURRENT_BINARY_DIR}/sphinx")
set(SPHINX_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/sphinx/html"
    CACHE PATH "Path where the Sphinx HTML documentation will be generated")
set(SPHINX_DOCTREE_DIR "${CMAKE_CURRENT_BINARY_DIR}/sphinx/_doctrees-html")

add_custom_target(doxygen
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/doxygen/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen"
    COMMENT "Generating bds doxygen documentation"
    VERBATIM)

add_custom_target(doc
    COMMAND "${SPHINX_EXECUTABLE}" -b html -c "${SPHINX_CONF_DIR}"
     -d "${SPHINX_DOCTREE_DIR}" -q "${SPHINX_SOURCE_DIR}" "${SPHINX_OUTPUT_DIR}"
    COMMENT "Generating html Sphinx documentation into ${SPHINX_OUTPUT_DIR}"
    VERBATIM)

add_dependencies(doc doxygen)

# Sphinx resolves directories like `_static` relative to the location of
# conf.py, which was moved into the build tree via the above `configure_file`
# call. Add a pre-build command to copy those directories into the build tree.
# NOTE: we cannot use a add_custom_command PRE_BUILD event here, because they
# do not actually run before the build, see CMake bug #15059
add_custom_target(sphinx_pre_build
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/sphinx/_static"
        "${CMAKE_CURRENT_BINARY_DIR}/sphinx/_static"
    COMMENT "Copying `_static` directory into sphinx build tree")

add_dependencies(doc sphinx_pre_build)
