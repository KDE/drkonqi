/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
*/

#pragma once

#include <QByteArray>
#include <QHash>
#include <QJsonDocument>
#include <QString>

class Coredump
{
public:
    using EntriesHash = QHash<QByteArray, QByteArray>;

    Coredump(QByteArray cursor, EntriesHash data)
        : m_cursor(std::move(cursor))
        , m_rawData(std::move(data))
        , uid(m_rawData[QByteArrayLiteral("COREDUMP_UID")].toInt())
        , pid(m_rawData[QByteArrayLiteral("COREDUMP_PID")].toInt())
        , exe(QString::fromLocal8Bit(m_rawData[QByteArrayLiteral("COREDUMP_EXE")]))
        , filename(QString::fromLocal8Bit(m_rawData[keyFilename()]))
        , systemd_unit(QString::fromLocal8Bit(m_rawData[QByteArrayLiteral("_SYSTEMD_UNIT")]))
    {
    }

    explicit Coredump(const QJsonDocument &document)
        : Coredump(QByteArray() /* not from journal, has no cursor */, documentToHash(document))
    {
    }

    ~Coredump() = default;

    // In a function cause it is used in more than one location.
    static QByteArray keyFilename()
    {
        return QByteArrayLiteral("COREDUMP_FILENAME");
    }

    // Other bits and bobs
    const QByteArray m_cursor;
    const EntriesHash m_rawData;

    // Journal Entry values
    const uid_t uid = 0;
    const pid_t pid = -1;
    const QString exe;
    const QString filename; // core dump file if available (may be /dev/null if the core is stored in journal directly)
    const QString systemd_unit;

private:
    static EntriesHash documentToHash(const QJsonDocument &document)
    {
        const QVariantMap variantMap = document.toVariant().toMap();
        EntriesHash hash;
        for (auto it = variantMap.cbegin(); it != variantMap.cend(); ++it) {
            hash.insert(it.key().toUtf8(), it->value<QByteArray>());
        }
        return hash;
    }
    Q_DISABLE_COPY_MOVE(Coredump);
};
