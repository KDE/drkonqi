# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

find_program(eu-unstrip_EXE eu-unstrip)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(eu-unstrip
    FOUND_VAR
        eu-unstrip_FOUND
    REQUIRED_VARS
        eu-unstrip_EXE
)
