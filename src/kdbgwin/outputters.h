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

#pragma once

#include "common.h"
#include <QTextStream>

/**
 * \brief Basic outputter.
 *
 * This class will output the information from a backtrace generator.
 */
class Outputter : public QObject
{
    Q_OBJECT

protected:
    QTextStream stream;

public:
    Outputter();

public Q_SLOTS:
    /// The slot that will be called for each line emitted by the generator. This method
    /// sends the string to stdout and qCDebug(DRKONQI_LOG)
    void OnDebugLine(const QString &);
};
