/*
    Copyright 2019 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NEWATTACHMENT_H
#define NEWATTACHMENT_H

#include "jsoncommand.h"

namespace Bugzilla {

class NewAttachment : public JsonCommand
{
    Q_OBJECT
    BUGZILLA_MEMBER_PROPERTY(QList<int>, ids);
    BUGZILLA_MEMBER_PROPERTY(QString, data);
    BUGZILLA_MEMBER_PROPERTY(QString, file_name);
    BUGZILLA_MEMBER_PROPERTY(QString, summary);
    BUGZILLA_MEMBER_PROPERTY(QString, content_type);
    BUGZILLA_MEMBER_PROPERTY(QString, comment);
    BUGZILLA_MEMBER_PROPERTY(bool, is_patch);
    BUGZILLA_MEMBER_PROPERTY(bool, is_private);

    // flags property is not supported at this time

public:
    virtual QVariantHash toVariantHash() const override;
};

} // namespace Bugzilla

#endif // NEWATTACHMENT_H
