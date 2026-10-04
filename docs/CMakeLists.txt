# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_Interpreter_FOUND)
    message(
        FATAL_ERROR
        "BUILD_DOCS requires a Python 3 interpreter.\n"
        "Install Python 3 and re-run CMake, or set -DBUILD_DOCS=OFF to skip."
    )
endif()

set(_python_packages_dir "${CMAKE_CURRENT_BINARY_DIR}/python-packages")

# Install pinned doc dependencies into the build tree (runs once per build dir).
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pip.stamp
    COMMAND
        ${Python3_EXECUTABLE} -m pip install --target "${_python_packages_dir}"
        --upgrade --quiet "markdown==3.10.2" "pygments==2.20.0"
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/pip.stamp
    COMMENT "Installing documentation Python dependencies"
    VERBATIM
)

# Convert Markdown sources to HTML in the binary directory.
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/docs.stamp
    COMMAND
        ${CMAKE_COMMAND} -E env "PYTHONPATH=${_python_packages_dir}"
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/build.py
        ${CMAKE_CURRENT_BINARY_DIR}/html ${PROJECT_VERSION}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/docs.stamp
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/pip.stamp
        ${CMAKE_CURRENT_SOURCE_DIR}/build.py
        ${CMAKE_CURRENT_SOURCE_DIR}/index.md
        ${CMAKE_CURRENT_SOURCE_DIR}/intro.md
        ${CMAKE_CURRENT_SOURCE_DIR}/design.md
        ${CMAKE_CURRENT_SOURCE_DIR}/usage.md
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake.md
        ${CMAKE_CURRENT_SOURCE_DIR}/reference.md
        ${CMAKE_CURRENT_SOURCE_DIR}/html/css/main.css
        ${CMAKE_CURRENT_SOURCE_DIR}/html/css/normalize.css
        ${CMAKE_CURRENT_SOURCE_DIR}/html/css/highlight.css
        ${CMAKE_CURRENT_SOURCE_DIR}/html/css/img/favicon.png
        ${CMAKE_CURRENT_SOURCE_DIR}/html/logo.png
    COMMENT "Building documentation"
    VERBATIM
)
add_custom_target(docs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/docs.stamp)

if(ENABLE_INSTALL)
    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
endif()
