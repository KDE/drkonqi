cmake_minimum_required(VERSION 3.0)

project(drkonqi)
set(PROJECT_VERSION "5.20.2")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.74")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} )

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(ECMMarkAsTest)
include(CheckFunctionExists)
include(FeatureSummary)
include(KDEClangFormat)

kde_enable_exceptions()

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Widgets Test DBus Concurrent)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons Service
    ConfigWidgets JobWidgets KIO Crash Completion WidgetsAddons Wallet
    Notifications IdleTime WindowSystem SyntaxHighlighting)
find_package(Qt5X11Extras ${QT_MIN_VERSION} CONFIG)
set_package_properties(Qt5X11Extras PROPERTIES TYPE RECOMMENDED PURPOSE "Recommended for better integration on X11.")

if (MINGW)
    find_package(ZLIB REQUIRED)
    find_library(INTL_LIBRARY NAMES intl)
    find_library(IBERTY_LIBRARY NAMES iberty)
    find_library(BFD_LIBRARY NAMES bfd)
    if (IBERTY_LIBRARY)
        set(iberty_FOUND 1)
    else()
        set(msg "iberty")
    endif()
    if (BFD_LIBRARY)
        set(bfd_FOUND 1)
    else()
        set(msg "${msg} bfd")
    endif()
    if (INTL_LIBRARY)
        set(intl_FOUND 1)
    else()
        set(msg "${msg} intl")
    endif()
    if (msg)
        message(FATAL_ERROR "could not find ${msg}")
    endif()

    add_library(intl SHARED IMPORTED)
    set_target_properties(intl PROPERTIES
        IMPORTED_IMPLIB ${INTL_LIBRARY}
    )
    add_library(iberty STATIC IMPORTED)
    set_target_properties(iberty PROPERTIES
        IMPORTED_LOCATION ${IBERTY_LIBRARY}
    )
    add_library(bfd STATIC IMPORTED)
    set_target_properties(bfd PROPERTIES
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

install(FILES drkonqi.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

if (${ECM_VERSION} STRGREATER "5.58.0")
    install(FILES drkonqi.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES drkonqi.categories DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
