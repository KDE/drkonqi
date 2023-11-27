// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "coredumpexcavator.h"

#include <QDebug>
#include <QProcess>

using namespace Qt::StringLiterals;

void CoredumpExcavator::excavateFromTo(const QString &coreFile, const QString &coreFileTarget)
{
    auto proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::ForwardedChannels);
    proc->setProgram("coredumpctl"_L1);
    proc->setArguments({"--output"_L1, coreFileTarget, "dump"_L1, "COREDUMP_FILENAME=%1"_L1.arg(coreFile)});
    qDebug() << "excavating" << proc->arguments();
    QObject::connect(proc, &QProcess::finished, this, [this, coreFileTarget, proc](int exitCode, QProcess::ExitStatus exitStatus) mutable {
        qDebug() << "Core dump excavation complete" << exitCode << exitStatus << coreFileTarget;
        proc->deleteLater();
        Q_EMIT excavated(exitCode);
    });
    proc->start();
}
