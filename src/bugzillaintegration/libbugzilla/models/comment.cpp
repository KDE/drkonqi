/*
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "comment.h"

#include <QVariantHash>

namespace Bugzilla
{
Comment::Comment(const QVariantHash &object, QObject *parent)
    : QObject(parent)
{
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        setProperty(qPrintable(it.key()), it.value());
    }
}

int Comment::bug_id() const
{
    return m_bug_id;
}

void Comment::setBug_id(int bug_id)
{
    m_bug_id = bug_id;
}

QString Comment::text() const
{
    return m_text;
}

void Comment::setText(const QString &text)
{
    m_text = text;
}

} // namespace Bugzilla
