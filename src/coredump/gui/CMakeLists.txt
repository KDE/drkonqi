# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

add_executable(drkonqi-coredump-gui main.cpp PatientModel.cpp Patient.cpp DetailsLoader.cpp qml/qml.qrc)
target_compile_definitions(drkonqi-coredump-gui
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
    PRIVATE -DTRANSLATION_DOMAIN=\"drkonqi_coredump_gui\")

target_link_libraries(drkonqi-coredump-gui
    DrKonqiInternal
    KF5::Declarative
    Qt::Quick
    KF5::I18n
    drkonqi-coredump
)

install(TARGETS drkonqi-coredump-gui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.drkonqi.coredump.gui.desktop DESTINATION ${KDE_INSTALL_APPDIR})
