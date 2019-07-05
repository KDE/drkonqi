/*******************************************************************
* bugzillalib.h
* Copyright  2009, 2011   Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright  2012  George Kiagiadakis <kiagiadakis.george@gmail.com>
* Copyright  2019  Harald Sitter <sitter@kde.org>
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

#ifndef BUGZILLALIB__H
#define BUGZILLALIB__H

#include <QObject>
#include <QString>

#include "libbugzilla/clients/bugclient.h"
#include "libbugzilla/clients/productclient.h"
#include "libbugzilla/clients/attachmentclient.h"

namespace KIO {
class KJob;
}

class BugzillaManager : public QObject
{
    Q_OBJECT

public:
    // Note: it expect the bugTrackerUrl parameter to contain the trailing slash.
    // so it should be "https://bugs.kde.org/", not "https://bugs.kde.org"
    explicit BugzillaManager(const QString &bugTrackerUrl, QObject *parent = nullptr);

    /* Login methods */
    void tryLogin(const QString &username, const QString &password);
    void refreshToken();
    bool getLogged() const;

    QString getUsername() const;

    /* Bugzilla Action methods */
    void fetchBugReport(int, QObject *jobOwner = nullptr);
    void searchBugs(const QStringList &products, const QString &severity, const QString &comment, int offset);
    void sendReport(const Bugzilla::NewBug &bug);
    void attachTextToReport(const QString &text, const QString &filename,
                            const QString &description, int bugId,
                            const QString &comment);
    void addMeToCC(int bugId);
    void fetchProductInfo(const QString &);

    /* Misc methods */
    QString urlForBug(int bug_number) const;
    void stopCurrentSearch();

    void fetchComments(const Bugzilla::Bug::Ptr &bug, QObject *jobOwner);
    void lookupVersion();

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
    void loginError(const QString &errorMsg, const QString & xtendedErrorMsg = QString());
    void bugReportError(const QString &errorMsg, QObject *jobOwner);
    void commentsError(const QString &errorMsg, QObject *jobOwner);
    void searchError(const QString &errorMsg);
    void sendReportError(const QString &errorMsg, const QString &extendedErrorMsg = QString());
    void sendReportErrorInvalidValues(); //To use default values
    void attachToReportError(const QString &errorMsg, const QString &extendedErrorMsg = QString());
    void addMeToCCError(const QString &errorMsg, const QString &extendedErrorMsg = QString());
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
