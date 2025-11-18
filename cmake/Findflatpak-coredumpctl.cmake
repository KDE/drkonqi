# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

find_program(flatpak-coredumpctl_EXE flatpak-coredumpctl)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(flatpak-coredumpctl
    FOUND_VAR
        flatpak-coredumpctl_FOUND
    REQUIRED_VARS
        flatpak-coredumpctl_EXE
)
