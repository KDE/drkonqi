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

const static DWORD MAX_SYMBOL_NAME = 256 * sizeof(TCHAR);

/**
 * \brief Generator for MSVC.
 *
 * This class implements a backtrace generator for executables created with Microsoft's
 * Visual C++. The fundamental difference of executables created with MSVC and MinGW is
 * the debugging format. MSVC uses a proprietary debugging format called PDB (Program
 * Database), which can be manipulated with DbgHelp API.
 *
 */
class MsvcGenerator : public AbstractBTGenerator
{
    Q_OBJECT
public:
    /// Constructor
    MsvcGenerator(const Process &process);

    virtual bool Init();
    virtual void UnInit();

    virtual void FrameChanged(){};

    virtual QString GetFunctionName();
    virtual QString GetFile();
    virtual int GetLine();

    virtual void LoadSymbol(const QString &module, DWORD64 dwBaseAddr);
};
