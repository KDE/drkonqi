/*******************************************************************
 * bugzillalib.h
 * SPDX-FileCopyrightText: 2009, 2011 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>
 * SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef BUGZILLALIB__H
#define BUGZILLALIB__H

#include <QObject>
#include <QString>

#include "libbugzilla/clients/attachmentclient.h"
#include "libbugzilla/clients/bugclient.h"
#include "libbugzilla/clients/productclient.h"

#include "../drkonqi_globals.h"

namespace KIO
{
class KJob;
}

class BugzillaManager : public QObject
{
    Q_OBJECT

public:
    // Note: it expect the bugTrackerUrl parameter to contain the trailing slash.
    // so it should be "https://bugs.kde.org/", not "https://bugs.kde.org"
    explicit BugzillaManager(const QString &bugTrackerUrl = QString(), QObject *parent = nullptr);

    /* Login methods */
    Q_SCRIPTABLE void tryLogin(const QString &username, const QString &password);
    Q_INVOKABLE void refreshToken();
    bool getLogged() const;

    QString getUsername() const;

    /* Bugzilla Action methods */
    void fetchBugReport(int, QObject *jobOwner = nullptr);
    Q_INVOKABLE void searchBugs(const QStringList &products, const QString &severity, const QString &comment, int offset);
    void sendReport(const Bugzilla::NewBug &bug);
    void attachTextToReport(const QString &text, const QString &filename, const QString &description, int bugId, const QString &comment);
    void addMeToCC(int bugId);
    void fetchProductInfo(const QString &);

    /* Misc methods */
    QString urlForBug(int bug_number) const;
    void stopCurrentSearch();

    void fetchComments(const Bugzilla::Bug::Ptr &bug, QObject *jobOwner);
    Q_SCRIPTABLE void lookupVersion();

Q_SIGNALS:
    /* Bugzilla actions finished successfully */
    void loginFinished(bool logged);
    void bugReportFetched(Bugzilla::Bug::Ptr bug, QObject *jobOwner);
    void commentsFetched(QList<Bugzilla::Comment::Ptr> comments, QObject *jobOwner);
    void searchFinished(const QList<Bugzilla::Bug::Ptr> &bug);
    void reportSent(int bugId);
    void attachToReportSent(int bugId);
    void addMeToCCFinished(int bugId);
    void productInfoFetched(const Bugzilla::Product::Ptr &product);
    void bugzillaVersionFound();

    /* Bugzilla actions had errors */
    void loginError(const QString &errorMsg);
    void bugReportError(const QString &errorMsg, QObject *jobOwner);
    void commentsError(const QString &errorMsg, QObject *jobOwner);
    void searchError(const QString &errorMsg);
    void sendReportError(const QString &errorMsg);
    void sendReportErrorInvalidValues(); // To use default values
    void attachToReportError(const QString &errorMsg);
    void addMeToCCError(const QString &errorMsg);
    void productInfoError();
    void bugzillaVersionError(const QString &errorMsg);

private:
    QString m_bugTrackerUrl;
    QString m_username;
    QString m_token;
    QString m_password;
    bool m_logged = false;

    KJob *m_searchJob = nullptr;

    void setFeaturesForVersion(const QString &version);
};

#endif
