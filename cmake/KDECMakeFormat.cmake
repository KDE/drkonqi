#.rst:
# KDECMakeFormat
# --------------------
#
# This module provides a functionality to format the source
# code of your repository according to a predefined KDE
# cmake-format file.
#
# This module provides the following function:
#
# ::
#
#   kde_cmake_format(<files>)
#
# Using this function will create a cmake-format target that will format all
# ``<files>`` passed to the function with the predefined KDE gersemi style.
# To format the files you have to invoke the target with ``make cmake-format`` or ``ninja cmake-format``.
# Once the project is formatted it is recommended to enforce the formatting using a pre-commit hook,
# this can be done using :kde-module:`KDEGitCommitHooks`.
#
# The ``.gersemirc`` file from ECM will be copied to the source directory. This file should not be
# added to version control. It is recommended to add it to the ``.gitignore`` file: ``/.gersemirc``.
#
# Example usage:
#
# .. code-block:: cmake
#
#   include(KDECMakeFormat)
#   file(GLOB_RECURSE ALL_CMAKE_FORMAT_SOURCE_FILES CMakeLists.txt)
#   kde_cmake_format(${ALL_CMAKE_FORMAT_SOURCE_FILES})

#=============================================================================
# SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>
# SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause
#
# This file is dervied from KDEClangFormat.cmake

# try to find cmake-format in path
find_program(KDE_CMAKE_FORMAT_EXECUTABLE gersemi)
message(${KDE_CMAKE_FORMAT_EXECUTABLE})

# formatting target
function(KDE_CMAKE_FORMAT)
    # add target without specific commands first, we add the real calls file-per-file to avoid command line length issues
    add_custom_target(cmake-format COMMENT "Formatting sources in ${CMAKE_CURRENT_SOURCE_DIR} with ${KDE_CMAKE_FORMAT_EXECUTABLE}...")

    # run clang-format only if available, else signal the user what is missing
    if(KDE_CMAKE_FORMAT_EXECUTABLE)
        get_filename_component(_binary_dir ${CMAKE_BINARY_DIR} REALPATH)
        foreach(_file ${ARGV})
            # check if the file is inside the build directory => ignore such files
            get_filename_component(_full_file_path ${_file} REALPATH)
            string(FIND ${_full_file_path} ${_binary_dir} _index)
            if(NOT _index EQUAL 0)
                add_custom_command(TARGET cmake-format
                    COMMAND
                        ${KDE_CMAKE_FORMAT_EXECUTABLE}
                        -i
                        ${_full_file_path}
                    WORKING_DIRECTORY
                        ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT
                        "Formatting ${_full_file_path}..."
                    )
            endif()
        endforeach()
    else()
        add_custom_command(TARGET cmake-format
            COMMAND
                ${CMAKE_COMMAND} -E echo "Could not set up the cmake-format target as the cmake-format executable is missing."
            )
    endif()
endfunction()

# instantiate our cmake-format file, must be in source directory for tooling if we have the tool
if(KDE_CMAKE_FORMAT_EXECUTABLE)
    set(CMAKE_FORMAT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.gersemirc)
    if (EXISTS ${CMAKE_FORMAT_FILE})
        file(READ ${CMAKE_FORMAT_FILE} CMAKE_FORMAT_CONTENTS LIMIT 128)
        string(FIND "${CMAKE_FORMAT_CONTENTS}" "-- CMAKE_FORMAT_MODULE_MARKER --" matchres)
        if(${matchres} EQUAL -1)
            message(WARNING "The .gersemirc file already exists. Please remove it in order to use the file provided by ECM")
            return()
        endif()
    endif()

    set(_cmake_files)
    # NB: "Commands from not deprecated CMake native modules don't have to be provided" so we only glob our module paths, not the root ones.
    foreach(_cmake_dir ${CMAKE_MODULE_PATH})
        file(GLOB_RECURSE _each_files LIST_DIRECTORIES FALSE "${_cmake_dir}/*.cmake")
        list(APPEND _cmake_files ${_each_files})
    endforeach()
    list(JOIN _cmake_files ", " _cmake_format_definitions_array)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/gersemirc.in ${CMAKE_FORMAT_FILE} @ONLY)
endif()
