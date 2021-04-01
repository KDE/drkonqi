/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
*/

#include "linuxprocmapsparser.h"

#include <QByteArray>
#include <QByteArrayList>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "drkonqi_debug.h"

bool LinuxProc::isLibraryPath(const QString &path)
{
    // This intentionally matches potential suffixes, i.e. "/usr/lib/foo.so.0" but also "foo.so (deleted)"
    static QRegularExpression soExpression(QStringLiteral("(?<path>.+\\.(so|py)([^/]*))$"));
    const auto soMatch = soExpression.match(path);
    return soMatch.isValid() && soMatch.hasMatch() && !soMatch.captured(u"path").isEmpty();
}

bool LinuxProc::hasMapsDeletedFiles(const QString &exePathString, const QByteArray &maps, Check check)
{
    const QByteArray exePath = QFile::encodeName(exePathString);
    const QByteArrayList lines = maps.split('\n');
    for (const auto &line : lines) {
        if (line.isEmpty()) {
            continue;
        }
        // Walk string by tokens. This is by far the easiest way to parse the format as anything after
        // the first 5 fields (minus the tokens) is the pathname. The pathname may be nothing, or contain more
        // spaces in turn. Qt has no convenient API for this, use strtok.

        QByteArray mutableLine = line;
        // address
        strtok(mutableLine.data(), " ");
        // perms
        strtok(nullptr, " ");
        // offset
        strtok(nullptr, " ");
        // dev
        strtok(nullptr, " ");
        // inode
        const QByteArray inode(strtok(nullptr, " "));
        // remainder is the pathname
        const QByteArray pathname = QByteArray(strtok(nullptr, "\n")).simplified(); // simplify to make evaluation easier

        if (pathname.isEmpty() || pathname.at(0) != QLatin1Char('/')) {
            // Could be pseudo entry like [heap] or anonymous region.
            continue;
        }

        if (pathname.startsWith(QByteArrayLiteral("/memfd"))) {
            // Qml.so's JIT shows up under memfd. This is a false positive as it isn't a real path in the
            // file system. Skip over it.
            continue;
        }

        const QByteArray deletedMarker = QByteArrayLiteral(" (deleted)");
        // We filter only .so files to ensure that we don't trip over cache files or the like.
        // NB: includes .so* and .py* since we also implicitly support snakes to
        //   a degree
        // As a result we need to explicitly look for the main executable.
        if (pathname == exePath + deletedMarker) {
            return true;
        }

        if (pathname != exePath && !isLibraryPath(QFile::decodeName(pathname))) {
            continue; // not exe and not a library.
        }

        // Deleted marker always declares something missing. Even when we perform additional stat checks on it.
        if (pathname.endsWith(deletedMarker)) {
            return true;
        }

        switch (check) {
        case Check::DeletedMarker: {
            // If we get here the file hasn't been marked deleted.
            break;
        }
        case Check::Stat: {
            struct stat info {
            };
            const int ret = stat(pathname.constData(), &info);
            if (ret == -1) {
                qCWarning(DRKONQI_LOG) << "Couldn't stat file, assuming it was deleted" << pathname << strerror(errno);
                return true;
                break;
            }

            if (info.st_ino != inode.toULongLong()) {
                qCWarning(DRKONQI_LOG) << "Found mismatching inode on" << pathname << info.st_ino << inode;
                return true;
                break;
            }

            // It's very awkward but st_dev seems dodgy at least with btrfs. The dev_t the kernel has is not the one
            // stat has and what's more the kernel has one that solid doesn't know about either. That may simply be
            // because btrfs makes up fake dev_ts since multiple btrfs subvolumes may be on the same block device.
            // Anyway, it's unfortunate but I guess we had best not look at the device.
        } break;
        }
    }

    return false;
}
