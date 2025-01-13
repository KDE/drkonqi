# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

find_program(GDB_EXE gdb)

if(GDB_EXE)
    execute_process(COMMAND ${GDB_EXE} --version OUTPUT_VARIABLE GDB_VERSION)
    # Match the first line only
    string(REGEX MATCH "[^\n]+" GDB_VERSION ${GDB_VERSION})
    # Then match the version number only
    string(REGEX MATCH "[-_]?([0-9][\-+\.:\~0-9a-zA-Z]*)" GDB_VERSION ${GDB_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDB
    FOUND_VAR
        GDB_FOUND
    VERSION_VAR
        GDB_VERSION
    REQUIRED_VARS
        GDB_EXE
)
