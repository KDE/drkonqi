# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

set(SYSTEMD_DBUS_XMLS
    org.freedesktop.systemd1.Manager.xml
    org.freedesktop.systemd1.Unit.xml
    org.freedesktop.DBus.Properties.xml
)

set_source_files_properties(${SYSTEMD_DBUS_XMLS} PROPERTIES INCLUDE dbustypes.h)
qt_add_dbus_interfaces(dbus_SRCS ${SYSTEMD_DBUS_XMLS})

add_library(systemd-dbus OBJECT ${dbus_SRCS})
target_link_libraries(systemd-dbus PRIVATE Qt::DBus)
target_include_directories(systemd-dbus PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}>")

target_sources(DrKonqiInternal PRIVATE
    memoryfence.cpp
    memorypressure.cpp
)

target_link_libraries(DrKonqiInternal
    Systemd::systemd
    systemd-dbus
)
