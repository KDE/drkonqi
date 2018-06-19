cmake_minimum_required(VERSION 3.0)

project(drkonqi)
set(PROJECT_VERSION "5.13.1")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.9.0")
set(KF5_MIN_VERSION "5.42.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(ECMMarkAsTest)
include(CheckFunctionExists)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Widgets Test DBus Concurrent)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons Service ConfigWidgets JobWidgets KIO Crash Completion XmlRpcClient WidgetsAddons Wallet Notifications IdleTime)

find_package(Qt5X11Extras ${QT_MIN_VERSION} CONFIG)
set_package_properties(Qt5X11Extras PROPERTIES TYPE RECOMMENDED PURPOSE "Recommended for better integration on X11.")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_USE_QSTRINGBUILDER)
add_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

add_subdirectory(src)

install( FILES drkonqi.categories DESTINATION ${KDE_INSTALL_CONFDIR} )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
