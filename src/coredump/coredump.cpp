/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>
*/

#include "coredump.h"

Coredump::Coredump(QByteArray cursor, EntriesHash data)
    : m_cursor(std::move(cursor))
    , m_rawData(std::move(data))
    , uid(m_rawData[QByteArrayLiteral("COREDUMP_UID")].toInt())
    , pid(m_rawData[QByteArrayLiteral("COREDUMP_PID")].toInt())
    , exe(QString::fromLocal8Bit(m_rawData[QByteArrayLiteral("COREDUMP_EXE")]))
    , filename(QString::fromLocal8Bit(m_rawData[keyFilename()]))
    , systemd_unit(QString::fromLocal8Bit(m_rawData[QByteArrayLiteral("_SYSTEMD_UNIT")]))
{
}

Coredump::Coredump(const QJsonDocument &document)
    : Coredump(QByteArray() /* not from journal, has no cursor */, documentToHash(document))
{
}

QByteArray Coredump::keyFilename()
{
    return QByteArrayLiteral("COREDUMP_FILENAME");
}

Coredump::EntriesHash Coredump::documentToHash(const QJsonDocument &document)
{
    const QVariantMap variantMap = document.toVariant().toMap();
    EntriesHash hash;
    for (auto it = variantMap.cbegin(); it != variantMap.cend(); ++it) {
        hash.insert(it.key().toUtf8(), it->value<QByteArray>());
    }
    return hash;
}
