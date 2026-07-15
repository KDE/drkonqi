add_definitions(-DTRANSLATION_DOMAIN=\"drkonqi\")

include(CheckFunctionExists)

check_function_exists("strsignal" HAVE_STRSIGNAL)
check_function_exists("uname" HAVE_UNAME)

if(NOT DEBUG_PACKAGE_INSTALLER_NAME)
    set(DEBUG_PACKAGE_INSTALLER_NAME "installdbgsymbols.sh")
endif()

configure_file(config-drkonqi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-drkonqi.h)

add_subdirectory(core)
add_subdirectory(bugzillaintegration/libbugzilla)
add_subdirectory(data)

set(drkonqi_SRCS
    drkonqidialog.cpp
    backtracegenerator.cpp
    drkonqi.cpp
    drkonqibackends.cpp
    detachedprocessmonitor.cpp
    debugpackageinstaller.cpp
    systeminformation.cpp
    crashedapplication.cpp
    debugger.cpp
    debuggermanager.cpp
    statusnotifier.cpp
    statusnotifier_activationclosetimer.cpp
    linuxprocmapsparser.cpp
    drkonqi_globals.cpp
    qmlextensions/platformmodel.cpp
    qmlextensions/reproducibilitymodel.cpp
    qmlextensions/credentialstore.cpp
    qmlextensions/commentmodel.cpp
    qmlextensions/qobjectlistmodel.cpp
    qmlextensions/doctore.cpp
    drkonqidialog.h
    backtracegenerator.h
    drkonqi.h
    drkonqibackends.h
    detachedprocessmonitor.h
    debugpackageinstaller.h
    systeminformation.h
    crashedapplication.h
    debugger.h
    debuggermanager.h
    statusnotifier.h
    statusnotifier_activationclosetimer.h
    linuxprocmapsparser.h
    drkonqi_globals.h
    parser/backtraceline.h
    parser/backtraceparser.cpp
    parser/backtraceparsergdb.cpp
    parser/backtraceparsergdb.h
    parser/backtraceparser.h
    parser/backtraceparserlldb.cpp
    parser/backtraceparserlldb.h
    parser/backtraceparsernull.cpp
    parser/backtraceparsernull.h
    parser/backtraceparser_p.h
    qmlextensions/platformmodel.h
    qmlextensions/reproducibilitymodel.h
    qmlextensions/credentialstore.h
    qmlextensions/commentmodel.h
    qmlextensions/qobjectlistmodel.h
    qmlextensions/doctore.h
)

set(drkonqi_SRCS
    ${drkonqi_SRCS}
    bugzillaintegration/bugzillalib.cpp
    bugzillaintegration/reportinterface.cpp
    bugzillaintegration/productmapping.cpp
    bugzillaintegration/bugzillalib.h
    bugzillaintegration/reportinterface.h
    bugzillaintegration/productmapping.h
)

ecm_qt_declare_logging_category(
    drkonqi_SRCS
    HEADER drkonqi_parser_debug.h
    IDENTIFIER DRKONQI_PARSER_LOG
    CATEGORY_NAME org.kde.drkonqi.parser
    DESCRIPTION "drkonqi parser"
    EXPORT DRKONQI
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
kconfig_add_kcfg_files(DrKonqiInternal GENERATE_MOC settings.kcfgc)

target_include_directories(DrKonqiInternal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qmlextensions ${CMAKE_CURRENT_SOURCE_DIR}/parser)

set_source_files_properties(
    qml/Globals.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

ecm_add_qml_module(DrKonqiInternal URI org.kde.drkonqi GENERATE_PLUGIN_SOURCE
    QML_FILES
            qml/Main.qml
            qml/MainPage.qml
            qml/DeveloperPage.qml
            qml/BugzillaPage.qml
            qml/RatingItem.qml
            qml/ReportPage.qml
            qml/WelcomePage.qml
            qml/ContextPage.qml
            qml/LoginPage.qml
            qml/PreviewPage.qml
            qml/BacktracePage.qml
            qml/Globals.qml
            qml/FooterActionBar.qml
            qml/SendingPage.qml
            qml/SentPage.qml
            qml/SentryPage.qml
            qml/MainPageButton.qml
            qml/DownloadSymbolsCheckBox.qml
)

target_link_libraries(DrKonqiInternal
    PRIVATE
        KF6::GuiAddons
        KF6::I18n
        KF6::CoreAddons
        KF6::ConfigGui
        KF6::JobWidgets
        KF6::KIOCore
        KF6::KIOGui
        KF6::Crash
        KF6::WindowSystem
        Qt::DBus
        Qt::Concurrent
        KF6::WidgetsAddons
        KF6::Wallet
        KF6::Notifications # for status notifier
        KF6::IdleTime # hide status notifier only if user saw it
        KF6::SyntaxHighlighting # Backtrace Highlighting
        KF6::StatusNotifierItem
        drkonqi-core
        qbugzilla
    PUBLIC
        DrKonqiSentryInternal
        Qt::Qml
)

add_subdirectory(systemd)
add_subdirectory(coredump)

target_sources(DrKonqiInternal PRIVATE coredumpbackend.cpp coredumpbackend.h)
target_link_libraries(DrKonqiInternal PRIVATE drkonqi-coredump) # needed for logs extraction
target_link_libraries(DrKonqiInternal PRIVATE drkonqi-coredumpexcavator)
target_compile_definitions(DrKonqiInternal PRIVATE SYSTEMD_AVAILABLE)

add_executable(drkonqi main.cpp)
ecm_mark_nongui_executable(drkonqi)
target_link_libraries(drkonqi PRIVATE DrKonqiInternal Qt::Widgets Qt::DBus KF6::CoreAddons KF6::ConfigCore KF6::I18n)

target_link_libraries(drkonqi PRIVATE DrKonqiInternalplugin)

install(TARGETS drkonqi DESTINATION ${KDE_INSTALL_LIBEXECDIR})
configure_file(org.kde.drkonqi.desktop.cmake ${CMAKE_BINARY_DIR}/src/org.kde.drkonqi.desktop)
install(PROGRAMS ${CMAKE_BINARY_DIR}/src/org.kde.drkonqi.desktop DESTINATION ${KDE_INSTALL_APPDIR})

add_subdirectory(sentry)

if(BUILD_TESTING)
    add_subdirectory(tests)
    add_subdirectory(bugzillaintegration/libbugzilla/autotests)
endif()
