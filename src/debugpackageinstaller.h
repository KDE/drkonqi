/*******************************************************************
 * debugpackageinstaller.h
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/
#ifndef DEBUGPACKAGEINSTALLER__H
#define DEBUGPACKAGEINSTALLER__H

#include <QObject>
#include <QProcess>

class KProcess;
class QProgressDialog;

class DebugPackageInstaller : public QObject
{
    Q_OBJECT

    enum Results {
        ResultInstalled = 0,
        ResultError = 1,
        ResultSymbolsNotFound = 2,
        ResultCanceled = 3,
    };

public:
    explicit DebugPackageInstaller(QObject *parent = nullptr);
    bool canInstallDebugPackages() const;
    void setMissingLibraries(const QStringList &);
    void installDebugPackages();

private Q_SLOTS:
    void processFinished(int, QProcess::ExitStatus);
    void progressDialogCanceled();

Q_SIGNALS:
    void packagesInstalled();
    void error(const QString &);
    void canceled();

private:
    KProcess *m_installerProcess = nullptr;
    QProgressDialog *m_progressDialog = nullptr;
    QString m_executablePath;
    QStringList m_missingLibraries;
};

#endif
