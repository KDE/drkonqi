/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
*/

#pragma once

#include <QDebug> // Don't use categorized logging here to make the header easy to use by the helper daemon
#include <QFile>
#include <QStandardPaths>
#include <QString>

namespace Metadata
{
// Defined in the header so it can be used by the coredump-helper as well.
static QString resolveMetadataPath(int pid)
{
    const QString name = QStringLiteral("kcrash-metadata/%1.ini").arg(pid);
    const QString path = QStandardPaths::locate(QStandardPaths::GenericCacheLocation, name);

    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << "Unable to find file for pid" << pid << "expected at" << name;
        return {};
    }

    return path;
}
} // namespace Metadata
