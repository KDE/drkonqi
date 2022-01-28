# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>

add_executable(drkonqi-coredump-processor main.cpp)
target_link_libraries(drkonqi-coredump-processor Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Network Systemd::systemd)
install(TARGETS drkonqi-coredump-processor DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(
    drkonqi-coredump-processor@.service.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-processor@.service
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-processor@.service
    DESTINATION ${KDE_INSTALL_SYSTEMDUNITDIR}/system
)

# https://github.com/systemd/systemd/issues/19437
add_feature_info(
    "drkonqi-coredump-processor@.service"
    FALSE
    "The service does not enable correctly with systemd (up to at least version 248). It needs manual symlinking to enable.")
