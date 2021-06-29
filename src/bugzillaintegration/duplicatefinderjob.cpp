/*******************************************************************
 * duplicatefinderjob.cpp
 * SPDX-FileCopyrightText: 2011 Matthias Fuchs <mat69@gmx.net>
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "duplicatefinderjob.h"

#include <QtConcurrent>

#include "backtracegenerator.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqi_debug.h"
#include "parser/backtraceparser.h"

DuplicateFinderJob::DuplicateFinderJob(const QList<Bugzilla::Bug::Ptr> &bugs, BugzillaManager *manager, QObject *parent)
    : KJob(parent)
    , m_manager(manager)
    , m_bugs(bugs)
{
    qCDebug(DRKONQI_LOG) << "Possible duplicates:" << m_bugs.size();
    connect(m_manager, &BugzillaManager::bugReportFetched, this, &DuplicateFinderJob::slotBugReportFetched);
    connect(m_manager, &BugzillaManager::bugReportError, this, &DuplicateFinderJob::slotError);

    connect(m_manager, &BugzillaManager::commentsFetched, this, &DuplicateFinderJob::slotCommentsFetched);
    connect(m_manager, &BugzillaManager::commentsError, this, &DuplicateFinderJob::slotError);
}

DuplicateFinderJob::~DuplicateFinderJob() = default;

void DuplicateFinderJob::start()
{
    analyzeNextBug();
}

DuplicateFinderJob::Result DuplicateFinderJob::result() const
{
    return m_result;
}

void DuplicateFinderJob::analyzeNextBug()
{
    if (m_bugs.isEmpty()) {
        emitResult();
        return;
    }

    m_bug = m_bugs.takeFirst();
    qCDebug(DRKONQI_LOG) << "Fetching:" << m_bug->id();
    m_manager->fetchComments(m_bug, this);
}

void DuplicateFinderJob::fetchBug(int bugId)
{
    if (bugId > 0) {
        qCDebug(DRKONQI_LOG) << "Fetching:" << bugId;
        m_manager->fetchBugReport(bugId, this);
    } else {
        qCDebug(DRKONQI_LOG) << "Bug id not valid:" << bugId;
        analyzeNextBug();
    }
}

void DuplicateFinderJob::slotBugReportFetched(const Bugzilla::Bug::Ptr &bug, QObject *owner)
{
    if (this != owner) {
        return;
    }

    m_bug = bug;
    qCDebug(DRKONQI_LOG) << "Fetching:" << m_bug->id();
    m_manager->fetchComments(m_bug, this);
}

void DuplicateFinderJob::slotCommentsFetched(const QList<Bugzilla::Comment::Ptr> &comments, QObject *owner)
{
    if (this != owner) {
        return;
    }

    // NOTE: we do not hold the comments in our bug object, once they go out
    //   of scope they are gone again. We have no use for keeping them in memory
    //   a user might look at 3 out of 20 bugs, and for those we can simply
    //   request the comments again instead of holding the potentially very large
    //   comments in memory.

    BacktraceGenerator *btGenerator = DrKonqi::debuggerManager()->backtraceGenerator();
    const QList<BacktraceLine> ourTraceLines = btGenerator->parser()->parsedBacktraceLines();

    // QFuture the parsing. We'll not want to block the GUI thread with this nonesense.
    auto watcher = new QFutureWatcher<ParseBugBacktraces::DuplicateRating>(this);
    connect(watcher, &std::remove_pointer_t<decltype(watcher)>::finished, this, [this, watcher] {
        // runs on our thread again
        watcher->deleteLater();
        commentsParsed(watcher->result());
    });
    auto future = QtConcurrent::run([comments, ourTraceLines]() -> ParseBugBacktraces::DuplicateRating {
        ParseBugBacktraces parse(comments);
        parse.parse();
        return parse.findDuplicate(ourTraceLines);
    });
    watcher->setFuture(future);
}

void DuplicateFinderJob::commentsParsed(ParseBugBacktraces::DuplicateRating rating)
{
    qCDebug(DRKONQI_LOG) << "Duplicate rating:" << rating;

    // TODO handle more cases here
    if (rating != ParseBugBacktraces::PerfectDuplicate) {
        qCDebug(DRKONQI_LOG) << "Bug" << m_bug->id() << "most likely not a duplicate:" << rating;
        analyzeNextBug();
        return;
    }

    bool unknownStatus = (m_bug->status() == Bugzilla::Bug::Status::Unknown);
    bool unknownResolution = (m_bug->resolution() == Bugzilla::Bug::Resolution::Unknown);

    // The Bug is a duplicate, now find out the status and resolution of the existing report
    if (m_bug->resolution() == Bugzilla::Bug::Resolution::DUPLICATE) {
        qCDebug(DRKONQI_LOG) << "Found duplicate is a duplicate itself.";
        if (!m_result.duplicate) {
            m_result.duplicate = m_bug->id();
        }
        fetchBug(m_bug->dupe_of());
    } else if (unknownStatus || unknownResolution) {
        // A resolution is unknown when the bug is unresolved.
        // Status generally is never unknown.
        qCDebug(DRKONQI_LOG) << "Either the status or the resolution is unknown.";
        qCDebug(DRKONQI_LOG) << "Status \"" << m_bug->status() << "\" known:" << !unknownStatus;
        qCDebug(DRKONQI_LOG) << "Resolution \"" << m_bug->resolution() << "\" known:" << !unknownResolution;
        analyzeNextBug();
    } else {
        if (!m_result.duplicate) {
            m_result.duplicate = m_bug->id();
        }
        m_result.parentDuplicate = m_bug->id();
        m_result.status = m_bug->status();
        m_result.resolution = m_bug->resolution();
        qCDebug(DRKONQI_LOG) << "Found duplicate information (id/status/resolution):" << m_bug->id() << m_bug->status() << m_bug->resolution();
        emitResult();
    }
}

void DuplicateFinderJob::slotError(const QString &message, QObject *owner)
{
    if (this != owner) {
        return;
    }
    qCDebug(DRKONQI_LOG) << "Error fetching bug:" << message;
    analyzeNextBug();
}
