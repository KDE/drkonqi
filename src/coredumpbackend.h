/*
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "drkonqibackends.h"

#include <QHash>
#include <QTemporaryDir>

#include <memory>

class QProcess;

class CoredumpBackend : public AbstractDrKonqiBackend
{
    Q_OBJECT
public:
    using AbstractDrKonqiBackend::AbstractDrKonqiBackend;
    bool init() override;
    void prepareForDebugger() override;

protected:
    CrashedApplication *constructCrashedApplication() override;
    DebuggerManager *constructDebuggerManager() override;

private:
    std::unique_ptr<QTemporaryDir> m_coreDir;
    std::unique_ptr<CrashedApplication> m_crashedApplication;
    DebuggerManager *m_debuggerManager; // parented
    QHash<QByteArray, QByteArray> m_journalEntry;
    const QByteArray m_cursor;
    std::unique_ptr<QProcess> m_preparationProc;
};
