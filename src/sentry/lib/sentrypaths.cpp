// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022-2023 Harald Sitter <sitter@kde.org>

#include "sentrypaths.h"

#include <QDir>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace
{
QString cacheDir(const QString &subdir)
{
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    Q_ASSERT(!dir.isEmpty());
    if (dir.isEmpty()) {
        return {};
    }
    return dir + "/drkonqi/"_L1 + subdir;
}
} // namespace

namespace SentryPaths
{

QString payloadsDir()
{
    static const auto payloadsDir = []() {
        const auto dir = cacheDir(u"/sentry-envelopes"_qs);
        QDir().mkpath(dir);
        return dir;
    }();
    return payloadsDir;
}
QString sentPayloadsDir()
{
    static const auto sentPayloadsDir = []() {
        const auto dir = cacheDir(u"/sentry-sent-envelopes"_qs);
        QDir().mkpath(dir);
        return dir;
    }();
    return sentPayloadsDir;
}

QString payloadPath(const QString &eventId)
{
    const auto dir = payloadsDir();
    if (dir.isEmpty()) {
        return {};
    }
    return dir + '/'_L1 + eventId;
}

QString sentPayloadPath(const QString &eventId)
{
    const auto dir = sentPayloadsDir();
    if (dir.isEmpty()) {
        return {};
    }
    return dir + '/'_L1 + eventId;
}
} // namespace SentryPaths
