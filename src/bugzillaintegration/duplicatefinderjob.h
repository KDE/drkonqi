/*******************************************************************
* duplicatefinderjob.h
* Copyright 2011    Matthias Fuchs <mat69@gmx.net>
* Copyright 2019    Harald Sitter <sitter@kde.org>
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

#ifndef DUPLICATE_FINDER_H
#define DUPLICATE_FINDER_H

#include <QList>

#include <KJob>

#include "bugzillalib.h"

/**
 * Looks if of the current backtrace is a
 * duplicate of any of the specified bug ids.
 * If a duplicate is found result is emitted instantly
 */
class DuplicateFinderJob : public KJob
{
    Q_OBJECT
    public:
        struct Result
        {
            /**
             * First duplicate that was found, it might be that
             * this one is a duplicate itself, though this is still
             * useful for example to inform the user that their
             * backtrace is a duplicate of this bug, which is
             * tracked at another number though.
             *
             * @note 0 means that there is no duplicate
             * @see parrentDuplicate
             */
            int duplicate = 0;

            /**
             * This always points to the parent bug, i.e.
             * the bug that has no duplicates itself.
             * If this is 0 it means that there are no duplicates
             */
            int parentDuplicate = 0;

            Bugzilla::Bug::Status status = Bugzilla::Bug::Status::Unknown;
            Bugzilla::Bug::Resolution resolution = Bugzilla::Bug::Resolution::Unknown;
        };

        DuplicateFinderJob(const QList<Bugzilla::Bug::Ptr> &bugs, BugzillaManager *manager, QObject *parent = nullptr);
        ~DuplicateFinderJob() override;

        void start() override;

        /**
         * Call this after result has been emitted to
         * get the result
         */
        Result result() const;

    private Q_SLOTS:
        void slotBugReportFetched(const Bugzilla::Bug::Ptr &bug, QObject *owner);
        void slotCommentsFetched(const QList<Bugzilla::Comment::Ptr> &comments, QObject *owner);

        void slotError(const QString &message, QObject *owner);

    private:
        void analyzeNextBug();
        void fetchBug(int bugId);

    private:
        BugzillaManager *m_manager = nullptr;
        Result m_result;

        Bugzilla::Bug::Ptr m_bug = nullptr;

        QList<Bugzilla::Bug::Ptr> m_bugs;
};
#endif
