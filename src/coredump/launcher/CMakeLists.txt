# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>

add_executable(drkonqi-coredump-launcher main.cpp GlobalNotifierTruck.cpp DevNotifierTruck.cpp)
target_link_libraries(drkonqi-coredump-launcher
    PRIVATE
        Qt::Core
        KF6::ConfigCore
        KF6::Notifications
        KF6::KIOGui
        KF6::I18n
        KF6::Crash
        drkonqi-core
        drkonqi-coredump
)
if(WITH_GLOBAL_NOTIFIER)
    target_compile_definitions(drkonqi-coredump-launcher PRIVATE -DWITH_GLOBAL_NOTIFIER)
endif()
if(WITH_DRKONQI_REPORTING)
    target_compile_definitions(drkonqi-coredump-launcher PRIVATE -DWITH_DRKONQI_REPORTING)
endif()
install(TARGETS drkonqi-coredump-launcher DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(drkonqi-coredump-launcher@.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-launcher@.service)
install(
    FILES drkonqi-coredump-launcher.socket ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-coredump-launcher@.service
    DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)
install(CODE "
    include(${CMAKE_SOURCE_DIR}/cmake/SystemctlEnable.cmake)
    systemctl_enable(drkonqi-coredump-launcher.socket sockets.target ${KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR})
")
install( FILES drkonqi-coredump-launcher.notifyrc DESTINATION  ${KDE_INSTALL_KNOTIFYRCDIR})
