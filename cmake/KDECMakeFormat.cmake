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
# ``<files>`` passed to the function with the predefined KDE cmake-format style.
# To format the files you have to invoke the target with ``make cmake-format`` or ``ninja ccmakeang-format``.
# Once the project is formatted it is recommended to enforce the formatting using a pre-commit hook,
# this can be done using :kde-module:`KDEGitCommitHooks`.
#
# The ``.cmake-format.py`` file from ECM will be copied to the source directory. This file should not be
# added to version control. It is recommended to add it to the ``.gitignore`` file: ``/.cmake-format.py``.
#
# Example usage:
#
# .. code-block:: cmake
#
#   include(KDECMakeFormat)
#   file(GLOB_RECURSE ALL_CMAKE_FORMAT_SOURCE_FILES CMakeLists.txt)
#   kde_cmake_format(${ALL_CMAKE_FORMAT_SOURCE_FILES})
#
# Since 5.xx ((TODO))

#=============================================================================
# SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>
# SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause
#
# This file is dervied from KDEClangFormat.cmake

# try to find cmake-format in path
find_program(KDE_CMAKE_FORMAT_EXECUTABLE cmake-format)

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
    set(CMAKE_FORMAT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.cmake-format.py)
    if (EXISTS ${CMAKE_FORMAT_FILE})
        file(READ ${CMAKE_FORMAT_FILE} CMAKE_FORMAT_CONTENTS LIMIT 128)
        string(FIND "${CMAKE_FORMAT_CONTENTS}" "-- CMAKE_FORMAT_MODULE_MARKER --" matchres)
        if(${matchres} EQUAL -1)
            message(WARNING "The .cmake-format.py file already exists. Please remove it in order to use the file provided by ECM")
            return()
        endif()
    endif()

    set(_cmake_files)
    # This is all sorts of meh. a) the parser falls over easily when encountering not-cmake files b) it is dreadfully slow c) even more so when including the "stock" modules
    foreach(_cmake_dir ${CMAKE_MODULE_PATH})#;${CMAKE_ROOT}/Modules)
        file(GLOB_RECURSE _each_files LIST_DIRECTORIES FALSE "${_cmake_dir}/*.cmake")
        # cmake-format stubmles over cmake files that aren't cmake file, fortunately enough we also suffix templates file with .cmake. Hooray! -.-
        set(_sanitized_files)
        foreach(_file ${_each_files})
            get_filename_component(_name ${_file} NAME)
            get_filename_component(_ext ${_file} EXT)
            if(${_ext} STREQUAL ".cmake" AND NOT ${_name} STREQUAL "clang-format.cmake")
                list(APPEND _sanitized_files ${_file})
            endif()
        endforeach()
        list(APPEND _cmake_files ${_sanitized_files})
    endforeach()
    set(_CMAKE_FORMAT_ADDITIONAL_COMMANDS "{}") # set default in case the parser falls over
    execute_process(COMMAND cmake-genparsers -l error --output-format python ${_cmake_files} -o "${CMAKE_CURRENT_BINARY_DIR}/cmake-format.commands")
    file(READ ${CMAKE_CURRENT_BINARY_DIR}/cmake-format.commands _CMAKE_FORMAT_ADDITIONAL_COMMANDS)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake-format.py.cmake ${CMAKE_FORMAT_FILE} @ONLY)
endif()
