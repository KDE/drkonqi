cmake_minimum_required(VERSION 3.16)

project(drkonqi)
set(PROJECT_VERSION "5.23.5")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.86")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(ECMMarkAsTest)
include(CheckFunctionExists)
include(FeatureSummary)
include(KDEClangFormat)
include(KDEGitCommitHooks)

kde_enable_exceptions()

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Widgets Test DBus Concurrent)

find_package(
    KF5
    ${KF5_MIN_VERSION}
    REQUIRED
    COMPONENTS
        I18n
        CoreAddons
        Service
        ConfigWidgets
        JobWidgets
        KIO
        Crash
        Completion
        WidgetsAddons
        Wallet
        Notifications
        IdleTime
        WindowSystem
        SyntaxHighlighting
)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00)

find_package(Systemd)
set_package_properties(Systemd PROPERTIES TYPE RECOMMENDED PURPOSE "Coredumpd integration.")

if(Systemd_FOUND)
    find_package(coredumpctl)
    set_package_properties(
        coredumpctl
        PROPERTIES
        TYPE RUNTIME
        DESCRIPTION "coredumpd CLI. part of systemd"
        PURPOSE "Required for coredumpd support to work at runtime!"
    )
endif()

if(MINGW)
    find_package(ZLIB REQUIRED)
    find_library(INTL_LIBRARY NAMES intl)
    find_library(IBERTY_LIBRARY NAMES iberty)
    find_library(BFD_LIBRARY NAMES bfd)
    if(IBERTY_LIBRARY)
        set(iberty_FOUND 1)
    else()
        set(msg "iberty")
    endif()
    if(BFD_LIBRARY)
        set(bfd_FOUND 1)
    else()
        set(msg "${msg} bfd")
    endif()
    if(INTL_LIBRARY)
        set(intl_FOUND 1)
    else()
        set(msg "${msg} intl")
    endif()
    if(msg)
        message(FATAL_ERROR "could not find ${msg}")
    endif()

    add_library(intl SHARED IMPORTED)
    set_target_properties(intl PROPERTIES IMPORTED_IMPLIB ${INTL_LIBRARY})
    add_library(iberty STATIC IMPORTED)
    set_target_properties(iberty PROPERTIES IMPORTED_LOCATION ${IBERTY_LIBRARY})
    add_library(bfd STATIC IMPORTED)
    set_target_properties(
        bfd
        PROPERTIES
            IMPORTED_LOCATION ${BFD_LIBRARY}
            # bfd header requires this to be defined
            INTERFACE_COMPILE_DEFINITIONS "PACKAGE;PACKAGE_VERSION"
    )
    find_path(BFD_INCLUDE_DIR bfd.h)
    include_directories(${BFD_INCLUDE_DIR})
endif()

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_subdirectory(src)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

ecm_qt_install_logging_categories(
    EXPORT DRKONQI
    FILE drkonqi.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
