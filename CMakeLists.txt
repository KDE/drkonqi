cmake_minimum_required(VERSION 3.16)

project(drkonqi)
set(PROJECT_VERSION "6.4.91")

set(QT_MIN_VERSION "6.9.0")
set(KF6_MIN_VERSION "6.18.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

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
include(ECMDeprecationSettings)
include(ECMFindQmlModule)
include(ECMSourceVersionControl)
include(CMakeDependentOption)
include(FindPythonModule)

option(WITH_PYTHON_VENDORING "Python dependency vendoring (cmake will install python dependencies into drkonqi's python tree)" ON)

kde_enable_exceptions()

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Widgets DBus Concurrent Quick)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
endif()

find_package(
    KF6
    ${KF6_MIN_VERSION}
    REQUIRED
    COMPONENTS
        I18n
        CoreAddons
        Config
        Service
        JobWidgets
        KIO
        Crash
        WidgetsAddons
        Wallet
        Notifications
        IdleTime
        WindowSystem
        SyntaxHighlighting
        StatusNotifierItem
)

ecm_find_qmlmodule(org.kde.kirigami 2.20)
ecm_find_qmlmodule(org.kde.kitemmodels 1.0)
ecm_find_qmlmodule(org.kde.syntaxhighlighting 1.0)

find_package(Python3 3.11 COMPONENTS Interpreter)

find_pythonmodule(psutil)
find_pythonmodule(sentry_sdk)

find_package(GDB 12)
set_package_properties(GDB PROPERTIES TYPE RUNTIME PURPOSE "GDB is required for debugging")

find_package(eu-unstrip)
set_package_properties(eu-unstrip PROPERTIES TYPE RUNTIME PURPOSE "eu-unstrip is required for debugging")

ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 5.240
)

find_package(Systemd 254)
set_package_properties(Systemd PROPERTIES TYPE REQUIRED PURPOSE "Coredumpd integration.")

find_package(PolkitQt6-1) # This is only used when extracting crashes from coredumpd
set_package_properties(PolkitQt6-1 PROPERTIES TYPE REQUIRED PURPOSE "Reading kwin_wayland crashes")

find_package(coredumpctl)
set_package_properties(
    coredumpctl
    PROPERTIES
    TYPE RUNTIME
    DESCRIPTION "coredumpd CLI. part of systemd"
    PURPOSE "Required for coredumpd support to work at runtime!"
)

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

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
