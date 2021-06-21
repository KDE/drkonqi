/*
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef PRODUCT_H
#define PRODUCT_H

#include <QObject>

#include "connection.h"

namespace Bugzilla
{
class ProductVersion : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(bool is_active READ isActive WRITE setActive)
public:
    int id() const
    {
        return m_id;
    }
    QString name() const
    {
        return m_name;
    }
    bool isActive() const
    {
        return m_active;
    }

    explicit ProductVersion(const QVariantHash &object, QObject *parent = nullptr);

private:
    void setId(int id)
    {
        m_id = id;
    }
    void setName(const QString &name)
    {
        m_name = name;
    }
    void setActive(bool active)
    {
        m_active = active;
    }

    int m_id = -1;
    QString m_name = QString();
    bool m_active = false;
};

class ProductComponent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId)
    Q_PROPERTY(QString name READ name WRITE setName)
public:
    int id() const
    {
        return m_id;
    }
    QString name() const
    {
        return m_name;
    }

    explicit ProductComponent(const QVariantHash &object, QObject *parent = nullptr);

private:
    void setId(int id)
    {
        m_id = id;
    }
    void setName(const QString &name)
    {
        m_name = name;
    }

    int m_id = -1;
    QString m_name = QString();
};

class Product : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_active READ isActive WRITE setActive)
    Q_PROPERTY(QList<Bugzilla::ProductComponent *> components READ components WRITE setComponents)
    Q_PROPERTY(QList<Bugzilla::ProductVersion *> versions READ versions WRITE setVersions)
public:
    typedef QSharedPointer<Product> Ptr;

    explicit Product(const QVariantHash &object, const Connection &connection = Bugzilla::connection(), QObject *parent = nullptr);
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
