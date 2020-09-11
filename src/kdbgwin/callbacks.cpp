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

#include "callbacks.h"

BOOL Callbacks::ReadProcessMemory(HANDLE hProcess, DWORD64 qwBaseAddr, PVOID lpBuf, DWORD nSize, LPDWORD lpBytesRead)
{
    SIZE_T st;
    BOOL bRet = ::ReadProcessMemory(hProcess, (LPVOID) qwBaseAddr, (LPVOID) lpBuf, nSize, &st);
    *lpBytesRead = (DWORD) st;
    //qCDebug(DRKONQI_LOG) << "bytes read=" << st << "; bRet=" << bRet << "; LastError: " << GetLastError();
    return bRet;
}

PVOID Callbacks::SymFunctionTableAccess64(HANDLE hProcess, DWORD64 qwAddr)
{
    PVOID ret = ::SymFunctionTableAccess64(hProcess, qwAddr);
    //qCDebug(DRKONQI_LOG) << "ret=" << ret << "; LastError: " << GetLastError();
    return ret;
}

DWORD64 Callbacks::SymGetModuleBase64(HANDLE hProcess, DWORD64 qwAddr)
{
    DWORD64 ret = ::SymGetModuleBase64(hProcess, qwAddr);
    //qCDebug(DRKONQI_LOG) << "ret=" << ret << "; LastError: " << GetLastError();
    return ret;
}
