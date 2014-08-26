/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <QUrl>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QResizeEvent>
#include <QQmlExpression>
#include <QQmlProperty>
#include <QMenu>

#include <QDebug>
#include <Plasma/Package>
#include <KLocalizedString>
#include <KActionCollection>

#include "plasmawindowedview.h"

PlasmaWindowedView::PlasmaWindowedView(QWindow *parent)
    : QQuickView(parent)
{
    engine()->rootContext()->setContextProperty("root", contentItem());
    QQmlExpression *expr = new QQmlExpression(engine()->rootContext(), contentItem(), "Qt.createQmlObject('import QtQuick 2.0; import org.kde.plasma.core 2.0; Rectangle {color: theme.backgroundColor; anchors.fill:parent}', root, \"\");");
    expr->evaluate();
}

PlasmaWindowedView::~PlasmaWindowedView()
{
}

void PlasmaWindowedView::setApplet(Plasma::Applet *applet)
{
    m_applet = applet;
    if (!applet) {
        return;
    }

    QQuickItem *i = applet->property("_plasma_graphicObject").value<QQuickItem *>();
    if (!i) {
        return;
    }

    const QRect geom = m_applet->config().readEntry("geometry", QRect());
    if (geom.isValid()) {
        setGeometry(geom);
    }

    i->setParentItem(contentItem());
    i->setVisible(true);
    setTitle(applet->title());
    setIcon(QIcon::fromTheme(applet->icon()));

    QObject::connect(applet->containment(), &Plasma::Containment::configureRequested,
                     this, &PlasmaWindowedView::showConfigurationInterface);
}

void PlasmaWindowedView::resizeEvent(QResizeEvent *ev)
{
    if (!m_applet) {
        return;
    }

    QQuickItem *i = m_applet->property("_plasma_graphicObject").value<QQuickItem *>();
    if (!i) {
        return;
    }

    i->setWidth(ev->size().width());
    i->setHeight(ev->size().height());

    contentItem()->setWidth(ev->size().width());
    contentItem()->setHeight(ev->size().height());

    m_applet->config().writeEntry("geometry", QRect(position(), ev->size()));
}

void PlasmaWindowedView::mouseReleaseEvent(QMouseEvent *ev)
{
    QQuickWindow::mouseReleaseEvent(ev);

    if ((!(ev->buttons() & Qt::RightButton) && ev->button() != Qt::RightButton) || ev->isAccepted()) {
        return;
    }

    QMenu menu;

    foreach (QAction *action, m_applet->contextualActions()) {
        if (action) {
            menu.addAction(action);
        }
    }

    if (!m_applet->failedToLaunch()) {
        QAction *runAssociatedApplication = m_applet->actions()->action("run associated application");
        if (runAssociatedApplication && runAssociatedApplication->isEnabled()) {
            menu.addAction(runAssociatedApplication);
        }

        QAction *configureApplet = m_applet->actions()->action("configure");
        if (configureApplet && configureApplet->isEnabled()) {
            menu.addAction(configureApplet);
        }
    }

    menu.exec(ev->globalPos());
    ev->setAccepted(true);
}

void PlasmaWindowedView::moveEvent(QMoveEvent *ev)
{
    Q_UNUSED(ev)
    m_applet->config().writeEntry("geometry", QRect(position(), size()));
}

void PlasmaWindowedView::hideEvent(QHideEvent *ev)
{
    Q_UNUSED(ev)
    m_applet->config().sync();
    m_applet->deleteLater();
    deleteLater();
}

void PlasmaWindowedView::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_configView) {
        m_configView->hide();
        m_configView->deleteLater();
    }

    if (!applet || !applet->containment()) {
        return;
    }

    m_configView = new PlasmaQuick::ConfigView(applet);

    m_configView->init();
    m_configView->show();
}

#include "plasmawindowedview.moc"
