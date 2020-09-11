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

#define KDBGWIN_CALL_TYPE __stdcall

/**
 * \brief StackWalk64 callbacks.
 *
 * This class encapsulates 3 callbacks used by StackWalk64. ReadProcessMemory is the only
 * one really needed here, but I kept all three for debugging purposes.
 */
class Callbacks
{
public:
    static BOOL    KDBGWIN_CALL_TYPE ReadProcessMemory(HANDLE, DWORD64, PVOID, DWORD, LPDWORD);
    static PVOID   KDBGWIN_CALL_TYPE SymFunctionTableAccess64(HANDLE, DWORD64);
    static DWORD64 KDBGWIN_CALL_TYPE SymGetModuleBase64(HANDLE, DWORD64);
};
