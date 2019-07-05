/*******************************************************************
* bugzillalib.cpp
* Copyright  2009, 2011  Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "bugzillalib.h"

#include <QReadWriteLock>

#include "libbugzilla/clients/commentclient.h"
#include "libbugzilla/connection.h"
#include "libbugzilla/bugzilla.h"
#include "drkonqi_debug.h"

static const char showBugUrl[] = "show_bug.cgi?id=%1";

// Extra filter rigging. We don't want to leak secrets via qdebug, so install
// a message handler which does nothing more than replace secrets in debug
// messages with placeholders.
// This is used as a global static (since message handlers are meant to be
// static) and is slightly synchronizing across threads WRT the filter hash.
struct QMessageFilterContainer {
    QMessageFilterContainer();
    ~QMessageFilterContainer();
    void insert(const QString &needle, const QString &replace);
    void clear();

    QString filter(const QString &msg);

    // Message handler is called across threads. Syncronize for good meassure.
    QReadWriteLock lock;
    QtMessageHandler handler;

private:
    QHash<QString, QString> filters;
};

Q_GLOBAL_STATIC(QMessageFilterContainer, s_messageFilter)

QMessageFilterContainer::QMessageFilterContainer()
{
    handler =
            qInstallMessageHandler([](QtMsgType type,
                                   const QMessageLogContext &context,
                                   const QString &msg) {
            s_messageFilter->handler(type, context, s_messageFilter->filter(msg));
});
}

QMessageFilterContainer::~QMessageFilterContainer()
{
    qInstallMessageHandler(handler);
}

void QMessageFilterContainer::insert(const QString &needle, const QString &replace)
{
    if (needle.isEmpty()) {
        return;
    }

    QWriteLocker locker(&lock);
    filters[needle] = replace;
}

QString QMessageFilterContainer::filter(const QString &msg)
{
    QReadLocker locker(&lock);
    QString filteredMsg = msg;
    for (auto it = filters.constBegin(); it != filters.constEnd(); ++it) {
        filteredMsg.replace(it.key(), it.value());
    }
    return filteredMsg;
}

void QMessageFilterContainer::clear()
{
    QWriteLocker locker(&lock);
    filters.clear();
}

BugzillaManager::BugzillaManager(const QString &bugTrackerUrl, QObject *parent)
        : QObject(parent)
        , m_bugTrackerUrl(bugTrackerUrl)
        , m_logged(false)
        , m_searchJob(nullptr)
{
    Q_ASSERT(bugTrackerUrl.endsWith(QLatin1Char('/')));
    Bugzilla::setConnection(new Bugzilla::HTTPConnection(QUrl(m_bugTrackerUrl + QStringLiteral("rest"))));
}

void BugzillaManager::lookupVersion()
{
    KJob *job = Bugzilla::version();
    connect(job, &KJob::finished, this, [this](KJob *job) {
        try {
            QString version = Bugzilla::version(job);
            setFeaturesForVersion(version);
            emit bugzillaVersionFound();
        } catch (Bugzilla::Exception &e) {
            // Version detection problems simply mean we'll not mark the version
            // found and the UI will not allow reporting.
            qCWarning(DRKONQI_LOG) << e.whatString();
            emit bugzillaVersionError(e.whatString());
        }
    });
}

void BugzillaManager::setFeaturesForVersion(const QString& version)
{
    // A procedure to change Dr Konqi behaviour automatically when Bugzilla
    // software versions change.
    //
    // Changes should be added to Dr Konqi AHEAD of when the corresponding
    // Bugzilla software changes are released into bugs.kde.org, so that
    // Dr Konqi can continue to operate smoothly, without bug reports and a
    // reactive KDE software release.
    //
    // If Bugzilla announces a change to its software that affects Dr Konqi,
    // add executable code to implement the change automatically when the
    // Bugzilla software version changes. It goes at the end of this procedure
    // and elsewhere in this class (BugzillaManager) and/or other classes where
    // the change should actually be implemented.

    const int nVersionParts = 3;
    QString seps = QLatin1String("[._-]");
    QStringList digits = version.split(QRegExp(seps), QString::SkipEmptyParts);
    while (digits.count() < nVersionParts) {
        digits << QLatin1String("0");
    }
    if (digits.count() > nVersionParts) {
        qCWarning(DRKONQI_LOG) << QStringLiteral("Current Bugzilla version %1 has more than %2 parts. Check that this is not a problem.").arg(version).arg(nVersionParts);
    }

    qCDebug(DRKONQI_LOG) << "VERSION" << version;
}

void BugzillaManager::tryLogin(const QString &username, const QString &password)
{
    m_username = username;
    m_password = password;
    refreshToken();
}

void BugzillaManager::refreshToken()
{
    Q_ASSERT(!m_username.isEmpty());
    Q_ASSERT(!m_password.isEmpty());
    m_logged = false;

    // Rest token and qdebug filters
    Bugzilla::connection().setToken(QString());
    s_messageFilter->clear();
    s_messageFilter->insert(m_password, QStringLiteral("PASSWORD"));

    KJob *job = Bugzilla::login(m_username, m_password);
    connect(job, &KJob::finished, this, [this](KJob *job) {
        try {
            auto details = Bugzilla::login(job);
            m_token = details.token;
            if (m_token.isEmpty()) {
                throw Bugzilla::RuntimeException(QStringLiteral("Did not receive a token"));
            }

            s_messageFilter->insert(m_token, QStringLiteral("TOKEN"));
            Bugzilla::connection().setToken(m_token);
            m_logged = true;

            emit loginFinished(true);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            // Version detection problems simply mean we'll not mark the version
            // found and the UI will not allow reporting.
            emit loginError(e.whatString());
        }
    });
}

bool BugzillaManager::getLogged() const
{
    return m_logged;
}

QString BugzillaManager::getUsername() const
{
    return m_username;
}

void BugzillaManager::fetchBugReport(int bugnumber, QObject *jobOwner)
{
    Bugzilla::BugSearch search;
    search.id = bugnumber;

    Bugzilla::BugClient client;
    auto job = m_searchJob = client.search(search);
    connect(job, &KJob::finished, this, [this, &client, jobOwner](KJob *job) {
        try {
            auto list = client.search(job);
            if (list.size() != 1) {
                throw Bugzilla::RuntimeException(QStringLiteral("Unexpected bug amount returned: %1").arg(list.size()));
            }
            auto bug = list.at(0);
            m_searchJob = nullptr;
            emit bugReportFetched(bug, jobOwner);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            emit bugReportError(e.whatString(), jobOwner);
        }
    });
}

void BugzillaManager::fetchComments(const Bugzilla::Bug::Ptr &bug, QObject *jobOwner)
{
    Bugzilla::CommentClient client;
    auto job = client.getFromBug(bug->id());
    connect(job, &KJob::finished, this, [this, &client, jobOwner](KJob *job) {
        try {
            auto comments = client.getFromBug(job);
            emit commentsFetched(comments, jobOwner);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            emit commentsError(e.whatString(), jobOwner);
        }
    });
}

// TODO: This would kinda benefit from an actual pagination class,
// currently this implicitly relies on the caller to handle offests correctly.
// Fortunately we only have one caller so it makes no difference.
void BugzillaManager::searchBugs(const QStringList &products,
                                 const QString &severity,
                                 const QString &comment,
                                 int offset)
{
    Bugzilla::BugSearch search;
    search.products = products;
    search.severity = severity;
    search.longdesc = comment;
    // Order descedingly by bug_id. This allows us to offset through the results
    // from newest to oldest.
    // The UI will later order our data anyway, so the order at which we receive
    // the data is not important for the UI (outside the fact that we want
    // to step through pages of data)
    search.order << QStringLiteral("bug_id DESC");
    search.limit = 25;
    search.offset = offset;

    stopCurrentSearch();

    Bugzilla::BugClient client;
    auto job = m_searchJob = Bugzilla::BugClient().search(search);
    connect(job, &KJob::finished, this, [this, &client](KJob *job) {
        try {
            auto list = client.search(job);
            m_searchJob = nullptr;
            emit searchFinished(list);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            emit searchError(e.whatString());
        }
    });
}

void BugzillaManager::sendReport(const Bugzilla::NewBug &bug)
{
    auto job = Bugzilla::BugClient().create(bug);
    connect(job, &KJob::finished, this, [this](KJob *job) {
        try {
            int id = Bugzilla::BugClient().create(job);
            Q_ASSERT(id > 0);
            emit reportSent(id);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            emit sendReportError(e.whatString());
        }
    });
}

void BugzillaManager::attachTextToReport(const QString & text, const QString & filename,
    const QString & summary, int bugId, const QString & comment)
{
    Bugzilla::NewAttachment attachment;
    attachment.ids = QList<int> { bugId };
    attachment.data = text;
    attachment.file_name = filename;
    attachment.summary = summary;
    attachment.comment = comment;
    attachment.content_type = QLatin1Literal("text/plain");

    auto job = Bugzilla::AttachmentClient().createAttachment(bugId, attachment);
    connect(job, &KJob::finished, this, [this](KJob *job) {
        try {
            QList<int> ids = Bugzilla::AttachmentClient().createAttachment(job);
            Q_ASSERT(ids.size() == 1);
            emit attachToReportSent(ids.at(0));
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            emit attachToReportError(e.whatString());
        }
    });
}

void BugzillaManager::addMeToCC(int bugId)
{
    Bugzilla::BugUpdate update;
    Q_ASSERT(!m_username.isEmpty());
    update.cc->add << m_username;

    auto job = Bugzilla::BugClient().update(bugId, update);
    connect(job, &KJob::finished, this, [this](KJob *job) {
        try {
            const auto bugId = Bugzilla::BugClient().update(job);
            Q_ASSERT(bugId != 0);
            emit addMeToCCFinished(bugId);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            emit addMeToCCError(e.whatString());
        }
    });
}

void BugzillaManager::fetchProductInfo(const QString &product)
{
    auto job = Bugzilla::ProductClient().get(product);
    connect(job, &KJob::finished, this, [this](KJob *job) {
        try {
            auto ptr = Bugzilla::ProductClient().get(job);
            Q_ASSERT(ptr);
            productInfoFetched(ptr);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            // This doesn't have a string because it is actually not used for
            // anything...
            emit productInfoError();
        }
    });
}

QString BugzillaManager::urlForBug(int bug_number) const
{
    return QString(m_bugTrackerUrl) + QString::fromLatin1(showBugUrl).arg(bug_number);
}

void BugzillaManager::stopCurrentSearch()
{
    if (m_searchJob) { //Stop previous searchJob
        m_searchJob->disconnect();
        m_searchJob->kill();
        m_searchJob = nullptr;
    }
}
