// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
// SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>

#include "duplicatemodel.h"

#include <QMetaEnum>

#include "bugzillaintegration/bugzillalib.h"
#include "bugzillaintegration/reportinterface.h"

void DuplicateModel::setManager(BugzillaManager *manager)
{
    if (m_manager) {
        m_manager->disconnect(this);
    }
    m_manager = manager;

    connect(m_manager, &BugzillaManager::searchFinished, this, &DuplicateModel::searchFinished);
    connect(m_manager, &BugzillaManager::searchError, this, [] {
        qDebug() << "search error";
    });

    Q_EMIT managerChanged();
}

void DuplicateModel::searchBugs(const QStringList &products, const QString &severity, const QString &comment)
{
    m_searching = true;
    Q_EMIT searchingChanged();
    m_manager->searchBugs(products, severity, comment, m_offset);
}

int DuplicateModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_list.size();
}

QVariant DuplicateModel::data(const QModelIndex &index, int intRole) const
{
    if (!index.isValid()) {
        return {};
    }

    auto bug = m_list.at(index.row());
    switch (static_cast<Role>(intRole)) {
    case Role::Title:
        return bug->summary();
    case Role::Number:
        return bug->id();
    case Role::Object:
        return QVariant::fromValue(bug.data());
    }

    return {};
}

QHash<int, QByteArray> DuplicateModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    if (!roles.isEmpty()) {
        return roles;
    }

    const QMetaEnum roleEnum = QMetaEnum::fromType<Role>();
    for (int i = 0; i < roleEnum.keyCount(); ++i) {
        const int value = roleEnum.value(i);
        Q_ASSERT(value != -1);
        roles[static_cast<int>(value)] = QByteArray("ROLE_") + roleEnum.valueToKey(value);
    }
    return roles;
}

void DuplicateModel::searchFinished(const QList<Bugzilla::Bug::Ptr> &list)
{
    int results = list.count();

    qDebug() << "search finished" << results;
    m_offset += results;
    if (results > 0) {
        m_atEnd = false;
        Q_EMIT atEndChanged();
        beginResetModel();
        for (int i = 0; i < results; ++i) {
            const Bugzilla::Bug::Ptr &bug = list.at(i);
            m_list.append(bug);
        }
        endResetModel();

        if (!m_foundDuplicate) {
            m_searching = true;
            Q_EMIT searchingChanged();
            auto *job = new DuplicateFinderJob(list, m_manager, this);
            connect(job, &KJob::result, this, &DuplicateModel::analyzedDuplicates);
            job->start();
        }
    } else {
        m_atEnd = true;
        Q_EMIT atEndChanged();
        m_searching = false;
        Q_EMIT searchingChanged();
    }
}

void DuplicateModel::analyzedDuplicates(KJob *j)
{
    qDebug() << "ANALYZE";

    auto *job = static_cast<DuplicateFinderJob *>(j);
    m_result = job->result();
    m_foundDuplicate = m_result.parentDuplicate;
    qDebug() << m_result.parentDuplicate << m_foundDuplicate;
    m_reportInterface->setDuplicateId(m_result.parentDuplicate);

    m_searching = false;
    Q_EMIT searchingChanged();
}

#include "moc_duplicatemodel.cpp"
