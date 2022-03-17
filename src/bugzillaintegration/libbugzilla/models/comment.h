/*
    SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef COMMENT_H
#define COMMENT_H

#include <QObject>
#include <QPointer>

namespace Bugzilla
{
class Comment : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int bug_id READ bug_id MEMBER m_bug_id NOTIFY changed)
    Q_PROPERTY(QString text READ text MEMBER m_text NOTIFY changed)
public:
    typedef QPointer<Comment> Ptr;

    explicit Comment(const QVariantHash &object, QObject *parent = nullptr);

    int bug_id() const;
    QString text() const;

Q_SIGNALS:
    void changed();

private:
    int m_bug_id;
    QString m_text;
};

} // namespace Bugzilla

#endif // COMMENT_H
