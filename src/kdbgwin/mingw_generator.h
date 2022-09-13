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

#include "abstract_generator.h"

/**
 * \brief Generator for MinGW.
 *
 * This class allows generating backtraces for executables created with the MinGW compiler.
 * It assumes that symbols are stored in .sym files in the same directory as the corresponding
 * binary (eg. N:\\kde\\bin\\libkdecore.dll and N:\\kde\\bin\\libkdecore.sym). It uses libbfd to
 * find and extract the information it needs from the symbols it loads.
 */
class MingwGenerator : public AbstractBTGenerator
{
    Q_OBJECT
protected:
    /// The current file
    const char *file;
    /// The current function
    const char *func;
    /// The current line
    int line;

public:
    /// Constructor
    MingwGenerator(const Process &process);

    bool Init() override;
    void UnInit() override;

    void FrameChanged() override;

    QString GetFunctionName() override;
    QString GetFile() override;
    int GetLine() override;

    void LoadSymbol(const QString &module, DWORD64 dwBaseAddr) override;
};
