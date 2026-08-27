# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

add_executable(drkonqi-coredump-gui main.cpp PatientModel.cpp Patient.cpp DetailsLoader.cpp PatientModel.h Patient.h DetailsLoader.h statusnotifier.cpp statusnotifier.h)
target_compile_definitions(drkonqi-coredump-gui
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
    PRIVATE -DTRANSLATION_DOMAIN=\"drkonqi\")

target_link_libraries(drkonqi-coredump-gui PRIVATE
    DrKonqiInternal
    Qt::Quick
    Qt::Widgets
    KF6::I18n
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::Service
    KF6::KIOGui
    KF6::Notifications
    KF6::StatusNotifierItem
    KF6::WidgetsAddons
    KF6::DBusAddons
    KF6::WindowSystem
    drkonqi-core
    drkonqi-coredump
    drkonqi-coredumpexcavator
)

install(TARGETS drkonqi-coredump-gui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.drkonqi.coredump.gui.desktop DESTINATION ${KDE_INSTALL_APPDIR})

ecm_add_qml_module(drkonqi-coredump-gui URI org.kde.drkonqi.coredump.gui GENERATE_PLUGIN_SOURCE
    QML_FILES
        qml/Main.qml
        qml/ListPage.qml
        qml/DetailsPage.qml
)
