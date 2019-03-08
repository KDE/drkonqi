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

#ifndef PRODUCT_H
#define PRODUCT_H

#include <QObject>
#include <QPointer>

#include "connection.h"

namespace Bugzilla {

class ProductVersion : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName CONSTANT)
    Q_PROPERTY(bool active READ isActive WRITE setActive CONSTANT)
public:
    int id() const { return m_id; }
    QString name() const { return m_name; }
    bool isActive() const { return m_active; }

    explicit ProductVersion(const QVariantHash &object, QObject *parent = nullptr);
private:
    void setId(int id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }
    void setActive(bool active) { m_active = active; }

    int m_id = -1;
    QString m_name = QString();
    bool m_active = false;
};

class ProductComponent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName CONSTANT)
public:
    int id() const { return m_id; }
    QString name() const { return m_name; }

    explicit ProductComponent(const QVariantHash &object, QObject *parent = nullptr);
private:
    void setId(int id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }

    int m_id = -1;
    QString m_name = QString();
};

class Product : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_active READ isActive WRITE setActive CONSTANT)
    Q_PROPERTY(QList<Bugzilla::ProductComponent *> components READ components WRITE setComponents CONSTANT)
    Q_PROPERTY(QList<Bugzilla::ProductVersion *> versions READ versions WRITE setVersions CONSTANT)
public:
    typedef QSharedPointer<Product> Ptr;

    explicit Product(const QVariantHash &object,
                     const Connection &connection = Bugzilla::connection(),
                     QObject *parent = nullptr);
    ~Product();

    bool isActive() const;
    void setActive(bool active);

    QList<ProductComponent *> components() const;
    void setComponents(const QList<ProductComponent *> &components);

    QList<ProductVersion *> versions() const;
    void setVersions(const QList<ProductVersion *> &versions);

    // Convenience methods to get useful content out of the
    QStringList componentNames() const;
    QStringList allVersions() const;
    QStringList activeVersions() const;
    QStringList inactiveVersions() const;

private:
    static void registerVariantConverters();

    const Connection &m_connection;

    bool m_active = false;
    QList<ProductComponent *> m_components;
    QList<ProductVersion *> m_versions;
};

} // namespace Bugzilla

Q_DECLARE_METATYPE(Bugzilla::ProductComponent *)
Q_DECLARE_METATYPE(QList<Bugzilla::ProductComponent *>)
Q_DECLARE_METATYPE(Bugzilla::ProductVersion *)
Q_DECLARE_METATYPE(QList<Bugzilla::ProductVersion *>)

#endif // PRODUCT_H
