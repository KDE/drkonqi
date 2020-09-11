/*
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "bugfieldclient.h"

namespace Bugzilla {

KJob *BugFieldClient::getFields(const QString &idOrName)
{
    return m_connection.get(QStringLiteral("/field/bug/") + idOrName);
}

QList<BugField::Ptr> BugFieldClient::getFields(KJob *kjob)
{
    APIJob *job = qobject_cast<APIJob *>(kjob);

    const auto ary = job->object().value(QStringLiteral("fields")).toArray();

    QList<BugField::Ptr> list;
    list.reserve(ary.size());
    for (const auto &bug : qAsConst(ary)) {
        list.append(BugField::Ptr(new BugField(bug.toObject().toVariantHash())));
    }
    return list;
}

KJob *BugFieldClient::getField(const QString &idOrName)
{
    return getFields(idOrName);
}

BugField::Ptr BugFieldClient::getField(KJob *kjob)
{
    return getFields(kjob).value(0, nullptr);
}

} // namespace Bugzilla
