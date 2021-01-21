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

// the compiler only provides UNICODE. tchar.h checks for the _UNICODE macro
#if defined(MSC_VER) && defined(UNICODE)
#define _UNICODE
#endif

// clang-format off
// first: windows & compiler includes
#include <tchar.h>
#include <windows.h>
#include <dbghelp.h>
#include <assert.h>
#include <shlwapi.h>
#include <psapi.h>
#include <tlhelp32.h>

#include "drkonqi_debug.h"
// clang-format on

// second: Qt includes

// third: KDE includes

// common defines
#define SafeCloseHandle(h)                                                                                                                                     \
    CloseHandle(h);                                                                                                                                            \
    h = NULL;

#define ArrayCount(x) (sizeof(x) / sizeof(x[0]))

// Documentation
/**
\mainpage KDbgWin

KDbgWin (KDE Debugger for Windows) is a helper application for DrKonqi. Because KDE-Windows supports
2 compilers (MSVC and MinGW), and there is no debugger that supports them both, a simple debugger was needed
to make DrKonqi able to generate backtraces - Windows only.

MSVC generates .pdb files for its binaries, and GNU GCC embeds debugging information in executables. However,
with MinGW, debugging information can be stripped into external files and then loaded on demand. So the only
difference between the two is how symbols are handled. DbgHelp and LibBfd were used for manipulating and getting
the required information from each debugging format.
*/
