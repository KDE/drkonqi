// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

#include "Patient.h"

#include <QDebug>
#include <QFileInfo>
#include <QProcess>

#include <KShell>
#include <KTerminalLauncherJob>

#include "../coredump/coredump.h"

Patient::Patient(const Coredump &dump)
    : m_signal(dump.m_rawData["COREDUMP_SIGNAL"].toInt())
    , m_appName(QFileInfo(dump.exe).fileName())
    , m_pid(dump.pid)
    , m_canDebug(QFileInfo::exists(QString::fromUtf8(dump.m_rawData.value("COREDUMP_FILENAME"))))
    , m_timestamp(dump.m_rawData["COREDUMP_TIMESTAMP"].toLong())
    , m_coredumpExe(dump.m_rawData["COREDUMP_EXE"])
    , m_coredumpCom(dump.m_rawData["COREDUMP_COMM"])
{
}

QStringList Patient::coredumpctlArguments(const QString &command) const
{
    return {command, QString::number(m_pid), QString::fromUtf8(m_coredumpExe), QString::fromUtf8(m_coredumpCom)};
}

void Patient::debug() const
{
    const QString arguments = KShell::joinArgs(coredumpctlArguments(QStringLiteral("debug")));
    auto job = new KTerminalLauncherJob(QStringLiteral("coredumpctl %1").arg(arguments));
    connect(job, &KJob::result, this, [job] {
        job->deleteLater();
        if (job->error() != KJob::NoError) {
            qWarning() << job->errorText();
        }
    });
    job->start();
}

QString Patient::dateTime() const
{
    QDateTime time;
    time.setMSecsSinceEpoch(m_timestamp / 1000);
    return time.toString();
}
