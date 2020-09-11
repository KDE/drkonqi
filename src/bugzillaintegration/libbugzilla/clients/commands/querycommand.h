/*
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef QUERYCOMMAND_H
#define QUERYCOMMAND_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QUrlQuery>

#include "commandbase.h"

namespace Bugzilla {

class QueryCommand : public QObject
{
public:
    using QObject::QObject;

    virtual QUrlQuery toQuery() const;
    QUrlQuery expandQuery(QUrlQuery &query, const QSet<QString> &seen) const;
};

} // namespace Bugzilla

#endif // QUERYCOMMAND_H
