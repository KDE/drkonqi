/******************************************************************
 *
 * kdbgwin - Helper application for DrKonqi
 *
 * This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2010 Ilie Halip <lupuroshu@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *****************************************************************/

#include "msvc_generator.h"
#include "mingw_generator.h"
#include "outputters.h"
#include "kdbgwin_process.h"
#include "common.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kdbgwin"));

    if (argc != 3)
    {
        qCCritical(DRKONQI_LOG) << "Parameters are incorrect";
        return -1;
    }

    if (!Process::EnableDebugPrivilege())
    {
        qCCritical(DRKONQI_LOG) << "Cannot enable debug privilege, exiting";
        return -1;
    }

    // ok, argv[1] is the pid of the failing process,
    // and argv[2] the current thread id - let's get the info we need
    Process proc;
    if (!proc.GetInfo(argv[1], argv[2]))
    {
        qCCritical(DRKONQI_LOG) << "Cannot attach to process, exiting";
        return -1;
    }

#if defined(Q_CC_MSVC)
    MsvcGenerator generator(proc);
#elif defined(Q_CC_GNU)
    MingwGenerator generator(proc);
#endif

    Outputter outputter;

    QObject::connect(&generator, &MingwGenerator::DebugLine, &outputter, &Outputter::OnDebugLine);

    TThreadsMap::const_iterator it;
    for (it = proc.GetThreads().constBegin(); it != proc.GetThreads().constEnd(); it++)
    {
        generator.Run(it.value(), (it.key() == proc.GetThreadId())? true : false);
    }
}
