// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2019-2025 Harald Sitter <sitter@kde.org>

#include "drkonqipaths.h"

#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QStandardPaths>

QString Paths::drkonqiExe()
{
    // Borrowed from kcrash.cpp
    static QStringList paths = QFile::decodeName(qgetenv("LIBEXEC_PATH")).split(QLatin1Char(':'), Qt::SkipEmptyParts)
        + QStringList{
            QCoreApplication::applicationDirPath(), // then look where our application binary is located
            QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath), // look where libexec path is (can be set in qt.conf)
            QFile::decodeName(KDE_INSTALL_FULL_LIBEXECDIR), // look at our installation location
        };
    static QString exec = QStandardPaths::findExecutable(QStringLiteral("drkonqi"), paths);
    return exec;
}
