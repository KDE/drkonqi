// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QString>

class SentryScope
{
public:
    [[nodiscard]] static SentryScope *instance();
    QHash<QString, QString> environment();

    QString traceId;
    QString spanId;
    QString release;
};
