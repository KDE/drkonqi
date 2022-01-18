# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

add_subdirectory(autotests)

add_executable(drkonqi-coredump-cleanup main.cpp)
install(TARGETS drkonqi-coredump-cleanup DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(drkonqi-coredump-cleanup.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-cleanup.service)
install(
    FILES drkonqi-coredump-cleanup.timer ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-cleanup.service
    DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)
