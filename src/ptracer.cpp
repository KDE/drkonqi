/*
    Copyright (C) 2019 Christoph Roick <chrisito@gmx.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ptracer.h"

#ifdef Q_OS_LINUX

#include "drkonqi_debug.h"

#include <QFile>
#include <QStandardPaths>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

void setPtracer(qint64 debuggerpid, qint64 debuggeepid)
{

    int sfd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sfd < 0) {
        qCWarning(DRKONQI_LOG) << "socket to set ptracer not accessible";
        return;
    }

    static struct sockaddr_un server;
    static socklen_t sl = sizeof(server);
    server.sun_family = AF_UNIX;
    const QString socketPath =
        QStringLiteral("%1/kcrash_%2").arg(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation))
                                      .arg(debuggeepid);

    if (socketPath.size() >= static_cast<int>(sizeof(server.sun_path))) {
        qCWarning(DRKONQI_LOG) << "socket path is too long";
        close(sfd);
        return;
    }
    strcpy(server.sun_path, QFile::encodeName(socketPath).constData());

    if (::connect(sfd, (struct sockaddr *)&server, sl) == 0) {
        static const int msize = 21; // most digits in a 64bit int (+sign +'\0')
        char msg[msize];
        sprintf(msg, "%lld", debuggerpid);

        int r, bytes = 0;
        while (bytes < msize) {
            r = write(sfd, msg + bytes, msize - bytes);
            if (r > 0)
                bytes += r;
            else if (r == -1 && errno != EINTR)
                break;
        }
        if (bytes == msize) {
            struct pollfd fd;
            fd.fd = sfd;
            fd.events = POLLIN;
            while ((r = poll(&fd, 1, 1000)) == -1 && errno == EINTR) {}
            if (r > 0 && (fd.revents & POLLIN)) {
                char rmsg[msize];
                bytes = 0;
                while (bytes < msize) {
                    r = read(sfd, rmsg + bytes, msize - bytes);
                    if (r > 0)
                        bytes += r;
                    else if (r == -1 && errno != EINTR)
                        break;
                }
                if (bytes == msize && memcmp(msg, rmsg, msize) == 0)
                    qCInfo(DRKONQI_LOG) << "ptracer set to" << debuggerpid << "by debugged process";
                else
                    qCWarning(DRKONQI_LOG) << "debugged process did not acknowledge setting ptracer to" << debuggerpid;
                close(sfd);
                return;
            }
        }
    }

    qCWarning(DRKONQI_LOG) << "unable to set ptracer to" << debuggerpid;
    close(sfd);
}

#else

void setPtracer(qint64, qint64)
{
}

#endif
