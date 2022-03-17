/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>
*/

#pragma once

#include <QByteArray>
#include <QHash>
#include <QJsonDocument>
#include <QString>

#include "memory.h"

class Coredump
{
public:
    using EntriesHash = QHash<QByteArray, QByteArray>;

    Coredump(QByteArray cursor, EntriesHash data);
    explicit Coredump(const QJsonDocument &document);

    ~Coredump() = default;

    // In a function cause it is used in more than one location.
    static QByteArray keyFilename();

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
    static EntriesHash documentToHash(const QJsonDocument &document);
    Q_DISABLE_COPY_MOVE(Coredump)
};
