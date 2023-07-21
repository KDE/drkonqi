# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>

add_executable(drkonqi-coredump-processor main.cpp)
target_link_libraries(drkonqi-coredump-processor Qt::Core drkonqi-coredump)
install(TARGETS drkonqi-coredump-processor DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(
    drkonqi-coredump-processor@.service.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-processor@.service
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-processor@.service
    DESTINATION ${KDE_INSTALL_SYSTEMDUNITDIR}/system
)
install(CODE "
    include(${CMAKE_SOURCE_DIR}/cmake/SystemctlEnable.cmake)
    systemctl_enable(drkonqi-coredump-processor@.service systemd-coredump@.service ${KDE_INSTALL_FULL_SYSTEMDUNITDIR}/system)
")
