// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2019-2025 Harald Sitter <sitter@kde.org>

#include "metadata.h"

#include <QFileInfo>
#include <QStandardPaths>

namespace Metadata
{

[[nodiscard]] QString drkonqiMetadataPath(const QString &exe, const QString &bootId, const QString &timestamp, int pid)
{
    const QString command = QFileInfo(exe).fileName();
    // This incorrectly says it is .ini but in reality we store JSON in it. Not worth changing now though.
    const QString name = QStringLiteral("drkonqi/crashes/%1.%2.%3.%4.ini").arg(command, bootId, QString::number(pid), timestamp);
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1Char('/') + name;
}

// Defined in the header so it can be used by the coredump-helper as well.
[[nodiscard]] QString resolveKCrashMetadataPath(const QString &exe, const QString &bootId, int pid)
{
    const QString command = QFileInfo(exe).fileName();

    const QString nameV2 = QStringLiteral("kcrash-metadata/%1.%2.%3.ini").arg(command, bootId, QString::number(pid));
    const QString pathV2 = QStandardPaths::locate(QStandardPaths::GenericCacheLocation, nameV2);

    // Backwards compat for a while. Can be dropped before/after Plasma 6.0
    const QString nameV1 = QStringLiteral("kcrash-metadata/%1.ini").arg(pid);
    const QString pathV1 = QStandardPaths::locate(QStandardPaths::GenericCacheLocation, nameV1);

    for (const auto &path : {pathV2, pathV1}) {
        if (!path.isEmpty() && QFile::exists(path)) {
            return path;
        }
    }

    qWarning() << "Unable to find file for pid" << pid << "expected at" << nameV2;
    return {};
}

[[nodiscard]] QJsonObject readFromDisk(const QString &drkonqiMetadataPath)
{
    if (!QFile::exists(drkonqiMetadataPath)) {
        return {};
    }

    QFile file(drkonqiMetadataPath);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open for reading:" << drkonqiMetadataPath;
    }

    return QJsonDocument::fromJson(file.readAll()).object();
}

[[nodiscard]] bool isPickedUp(const QJsonObject &metadata)
{
    return metadata[DRKONQI_KEY].toObject().value(PICKED_UP_KEY).toBool(false);
}

[[nodiscard]] QStringList metadataArguments(const QVariantHash &kcrash)
{
    QStringList arguments;

    for (auto [key, valueVariant] : kcrash.asKeyValueRange()) {
        const auto value = valueVariant.toString();

        if (key == QLatin1String("exe")) {
            if (value.endsWith(QStringLiteral("/drkonqi"))) {
                qWarning() << "drkonqi crashed, we aren't going to invoke it again, we might be the reason it crashed :O";
                return {};
            }
            // exe purely exists for our benefit, don't forward it to drkonqi.
            continue;
        }

        arguments << QStringLiteral("--%1").arg(key);
        if (value != QLatin1String("true") && value != QLatin1String("false")) { // not a bool value, append as arg
            arguments << value;
        }
    }

    return arguments;
}

} // namespace Metadata
