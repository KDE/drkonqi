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

#include "outputters.h"

Outputter::Outputter()
    : stream(stdout)
{
}

void Outputter::OnDebugLine(const QString &line)
{
    stream << line << '\n';
}

#include "moc_outputters.cpp"
