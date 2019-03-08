/*******************************************************************
* productmapping.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#ifndef PRODUCTMAPPING__H
#define PRODUCTMAPPING__H

#include <QObject>
#include <QString>
#include <QStringList>

#include "bugzillaintegration/libbugzilla/clients/productclient.h"

class Product;
class BugzillaManager;
class CrashedApplication;

/**
 * Maps our crashed entity to a bugzilla product/component/version.
 */
class ProductMapping: public QObject
{
Q_OBJECT
public:
    explicit ProductMapping(const CrashedApplication *, BugzillaManager *, QObject *parent = nullptr);

    QString bugzillaProduct() const;
    QString bugzillaComponent() const;
    QString bugzillaVersion() const;
    QStringList relatedBugzillaProducts() const;

    bool bugzillaProductDisabled() const;
    bool bugzillaVersionDisabled() const;

private Q_SLOTS:
    void checkProductInfo(const Bugzilla::Product::Ptr);

private:
    void map(const QString&);
    void mapUsingInternalFile(const QString&);
    void getRelatedProductsUsingInternalFile(const QString&);

    QStringList m_relatedBugzillaProducts;
    QString m_bugzillaProduct;
    QString m_bugzillaComponent;

    QString m_bugzillaVersionString;

    const CrashedApplication *m_crashedAppPtr = nullptr;
    BugzillaManager *m_bugzillaManagerPtr = nullptr;

    bool m_bugzillaProductDisabled;
    bool m_bugzillaVersionDisabled;
};

#endif
