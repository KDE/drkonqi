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

#ifndef BUGFIELD_H
#define BUGFIELD_H

#include <QObject>
#include <QPointer>
#include <QVariantHash>

namespace Bugzilla {

/// https://bugzilla.readthedocs.io/en/5.0/api/core/v1/field.html
class BugFieldValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    // not mapped because of lazyness and lack of need
    //    Q_PROPERTY(int sort_key READ sort_key WRITE setSort_key)
    //    // visibility_values not mapped
    //    Q_PROPERTY(bool is_active READ is_active WRITE setIs_active)
    //    Q_PROPERTY(QString description READ description WRITE setDescription)
    //    Q_PROPERTY(bool is_open READ is_open WRITE setIs_open)
    //    // can_change_to not mapped
public:
    explicit BugFieldValue(const QVariantHash &obj, QObject *parent = nullptr);

    QString name() const;
    void setName(QString name);

private:
    QString m_name;
};

/// https://bugzilla.readthedocs.io/en/5.0/api/core/v1/field.html
class BugField : public QObject
{
    Q_OBJECT
    // Not mapped for lazyness and lack of need:
    //    Q_PROPERTY(int id READ id WRITE setId)
    //    Q_PROPERTY(Type type READ type WRITE setType)
    //    Q_PROPERTY(bool is_custom READ is_custom WRITE setIs_custom)
    //    Q_PROPERTY(QString name READ name WRITE setName)
    //    Q_PROPERTY(QString display_name READ display_name WRITE setDisplay_name)
    //    Q_PROPERTY(bool is_mandatory READ is_mandatory WRITE setIs_mandatory)
    //    Q_PROPERTY(bool is_on_bug_entry READ is_on_bug_entry WRITE setIs_on_bug_entry)
    //    Q_PROPERTY(QString visibility_field READ visibility_field WRITE setVisibility_field)
    //    Q_PROPERTY(QString value_field READ value_field WRITE setValue_field)
    Q_PROPERTY(QList<BugFieldValue *> values READ values WRITE setValues)
public:
    enum class Type {
        Invalid = -1,
        Unknown = 0,
        SingleLineString = 1,
        SingleValue = 2,
        MultipleValue = 3,
        MultiLineText = 4,
        DateTime = 5,
        BugId = 6,
        SeeAlso = 7,
        Keywords = 8,
        Date = 9,
        Integer = 10
    };

    typedef QPointer<BugField> Ptr;

    explicit BugField(const QVariantHash &obj, QObject *parent = nullptr);

    QList<BugFieldValue *> values() const;

    void setValues(QList<BugFieldValue *> values);

private:
    static void registerVariantConverters();

    QList<BugFieldValue *> m_values;
};

} // namespace Bugzilla

#endif // BUGFIELD_H
