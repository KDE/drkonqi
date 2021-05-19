# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>

add_executable(drkonqi-coredump-launcher main.cpp)
target_link_libraries(drkonqi-coredump-launcher Qt::Core Qt::Network Systemd::systemd)
target_compile_options(
    drkonqi-coredump-launcher
    PRIVATE
        -DKDE_INSTALL_FULL_LIBEXECDIR=\"${KDE_INSTALL_FULL_LIBEXECDIR}\"
        -DKDE_PLUGIN_INSTALL_DIR=\"${PLUGIN_INSTALL_DIR}\"
)
install(TARGETS drkonqi-coredump-launcher DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(drkonqi-coredump-launcher@.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-launcher@.service)
install(
    FILES drkonqi-coredump-launcher.socket ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-launcher@.service
    DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR}
)

add_library(KDECoredumpNotifierTruck MODULE NotifierTruck.cpp)
target_link_libraries(KDECoredumpNotifierTruck Qt5::Core KF5::Notifications)
install(TARGETS KDECoredumpNotifierTruck DESTINATION ${PLUGIN_INSTALL_DIR}/drkonqi/)
