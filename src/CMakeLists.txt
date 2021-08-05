add_definitions(-DTRANSLATION_DOMAIN=\"drkonqi5\")

include(CheckFunctionExists)

check_function_exists("strsignal" HAVE_STRSIGNAL)
check_function_exists("uname" HAVE_UNAME)

if(NOT DEBUG_PACKAGE_INSTALLER_NAME)
    set(DEBUG_PACKAGE_INSTALLER_NAME "installdbgsymbols.sh")
endif()

configure_file(config-drkonqi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-drkonqi.h)

add_subdirectory(bugzillaintegration/libbugzilla)
add_subdirectory(data)
add_subdirectory(parser)
if(WIN32)
    find_package(KDEWin REQUIRED)
    # for finding drkonqi_debug.h
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    add_subdirectory(kdbgwin)
endif()

set(drkonqi_SRCS
    drkonqidialog.cpp
    statuswidget.cpp
    backtraceratingwidget.cpp
    backtracewidget.cpp
    backtracegenerator.cpp
    drkonqi.cpp
    drkonqibackends.cpp
    detachedprocessmonitor.cpp
    debugpackageinstaller.cpp
    systeminformation.cpp
    crashedapplication.cpp
    debugger.cpp
    debuggerlaunchers.cpp
    ptracer.cpp
    debuggermanager.cpp
    statusnotifier.cpp
    statusnotifier_activationclosetimer.cpp
    linuxprocmapsparser.cpp
    drkonqi_globals.cpp
    qmlextensions/duplicatemodel.cpp
    qmlextensions/platformmodel.cpp
    qmlextensions/reproducibilitymodel.cpp
    qmlextensions/credentialstore.cpp
    qmlextensions/commentmodel.cpp
    qmlextensions/qobjectlistmodel.cpp
    qmlextensions/doctore.cpp
)

ki18n_wrap_ui(drkonqi_SRCS
    ui/maindialog.ui
    ui/backtracewidget.ui
)

# if BACKTRACE_PARSER_DEBUG is enabled, it will show both the
# parsed and the unparsed backtrace in the backtrace widget.
# Comment this out for release.
#add_definitions(-DBACKTRACE_PARSER_DEBUG)

set(drkonqi_SRCS
    ${drkonqi_SRCS}
    bugzillaintegration/bugzillalib.cpp
    bugzillaintegration/reportinterface.cpp
    bugzillaintegration/productmapping.cpp
    bugzillaintegration/parsebugbacktraces.cpp
    bugzillaintegration/duplicatefinderjob.cpp
    bugzillaintegration/sentrybeacon.cpp
)

ecm_qt_declare_logging_category(
    drkonqi_SRCS
    HEADER drkonqi_debug.h
    IDENTIFIER DRKONQI_LOG
    CATEGORY_NAME org.kde.drkonqi
    DESCRIPTION "drkonqi"
    EXPORT DRKONQI
)

# transient static lib we can use to link autotests against
add_library(DrKonqiInternal STATIC ${drkonqi_SRCS})

target_link_libraries(
    DrKonqiInternal
    KF5::I18n
    KF5::CoreAddons
    KF5::ConfigGui
    KF5::JobWidgets
    KF5::KIOCore
    KF5::KIOGui
    KF5::Crash
    KF5::Completion
    KF5::WindowSystem
    Qt::DBus
    Qt::Concurrent
    KF5::WidgetsAddons
    KF5::Wallet
    KF5::Notifications # for status notifier
    KF5::IdleTime # hide status notifier only if user saw it
    KF5::SyntaxHighlighting # Backtrace Highlighting
    drkonqi_backtrace_parser
    qbugzilla
    KF5::Declarative
    KUserFeedbackCore
)

if(Systemd_FOUND)
    add_subdirectory(coredump)

    target_sources(DrKonqiInternal PRIVATE coredumpbackend.cpp)
    target_compile_definitions(DrKonqiInternal PRIVATE SYSTEMD_AVAILABLE)
    if(Systemd_VERSION GREATER 247)
        add_compile_definitions(COREDUMPD_SUPPORTS_DEBUGGER_ARGUMENTS)
    endif()
endif()

if(WIN32)
    target_link_libraries(DrKonqiInternal kdewin)
endif()

add_executable(drkonqi main.cpp qml/qml.qrc)
ecm_mark_nongui_executable(drkonqi)
target_link_libraries(drkonqi DrKonqiInternal)

install(TARGETS drkonqi DESTINATION ${KDE_INSTALL_LIBEXECDIR})
configure_file(org.kde.drkonqi.desktop.cmake ${CMAKE_BINARY_DIR}/src/org.kde.drkonqi.desktop)
install(PROGRAMS ${CMAKE_BINARY_DIR}/src/org.kde.drkonqi.desktop DESTINATION ${KDE_INSTALL_APPDIR})

add_subdirectory(tests)

add_subdirectory(bugzillaintegration/libbugzilla/autotests)
