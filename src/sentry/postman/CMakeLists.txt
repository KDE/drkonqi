# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

add_executable(drkonqi-sentry-postman main.cpp)
target_link_libraries(drkonqi-sentry-postman
    DrKonqiSentryInternal
)

install(TARGETS drkonqi-sentry-postman DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(drkonqi-sentry-postman.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-sentry-postman.service)
install(
    FILES drkonqi-sentry-postman.timer drkonqi-sentry-postman.path ${CMAKE_CURRENT_BINARY_DIR}/drkonqi-sentry-postman.service
    DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)
install(CODE "execute_process(COMMAND
        ${CMAKE_COMMAND}
        -DKDE_INSTALL_FULL_SYSTEMDUSERUNITDIR=${KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/SystemctlEnable.cmake
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR \"Failed to enable systemd units\")
endif()
")
