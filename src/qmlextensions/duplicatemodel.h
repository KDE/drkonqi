// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QAbstractListModel>

#include "bugzillaintegration/duplicatefinderjob.h"
#include "bugzillaintegration/libbugzilla/models/bug.h"

class BugzillaManager;
class ReportInterface;

class DuplicateModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool searching MEMBER m_searching NOTIFY searchingChanged)
    Q_PROPERTY(bool atEnd MEMBER m_atEnd NOTIFY atEndChanged)
public:
    enum class Role {
        Title = Qt::UserRole + 1,
        Number,
        Object,
    };
    Q_ENUM(Role)

    using QAbstractListModel::QAbstractListModel;

    Q_PROPERTY(BugzillaManager *manager MEMBER m_manager WRITE setManager NOTIFY managerChanged)
    BugzillaManager *m_manager = nullptr;
    Q_SIGNAL void managerChanged();
    void setManager(BugzillaManager *manager);

    Q_PROPERTY(ReportInterface *iface MEMBER m_reportInterface NOTIFY reportInterfaceChanged)
    ReportInterface *m_reportInterface = nullptr;
    Q_SIGNAL void reportInterfaceChanged();

    Q_SIGNAL void searchingChanged();
    Q_SIGNAL void atEndChanged();

    Q_INVOKABLE void searchBugs(const QStringList &products, const QString &severity, const QString &comment);

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int intRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
    void searchFinished(const QList<Bugzilla::Bug::Ptr> &list);
    void analyzedDuplicates(KJob *j);

    int m_offset = 0;
    bool m_atEnd = false;
    QList<Bugzilla::Bug::Ptr> m_list;
    bool m_searching = false;
    int m_foundDuplicate = false;
    DuplicateFinderJob::Result m_result;
};
