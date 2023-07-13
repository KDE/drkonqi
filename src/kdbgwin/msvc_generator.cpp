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

#include "msvc_generator.h"

MsvcGenerator::MsvcGenerator(const Process &process)
    : AbstractBTGenerator(process)
{
}

bool MsvcGenerator::Init()
{
    return true;
}

void MsvcGenerator::UnInit()
{
}

QString MsvcGenerator::GetFunctionName()
{
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)malloc(sizeof(SYMBOL_INFO) + MAX_SYMBOL_NAME);
    ZeroMemory(symbol, sizeof(symbol) + MAX_SYMBOL_NAME);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYMBOL_NAME;

    DWORD64 dwDisplacement = 0;

    if (!SymFromAddr(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &dwDisplacement, symbol)) {
        qCCritical(DRKONQI_LOG) << "SymFromAddr() failed: " << GetLastError();
        return QString::fromLatin1(DEFAULT_FUNC);
    }

    char undecoratedName[MAX_PATH] = {0};
    if (!UnDecorateSymbolName(symbol->Name, undecoratedName, MAX_PATH, UNDNAME_COMPLETE)) {
        // if this fails, show the decorated name anyway, don't fail
        qCCritical(DRKONQI_LOG) << "UnDecorateSymbolName() failed: " << GetLastError();
        return QString::fromLatin1(symbol->Name);
    }

    return QString::fromLatin1(undecoratedName);
}

QString MsvcGenerator::GetFile()
{
    IMAGEHLP_LINE64 line;
    ZeroMemory(&line, sizeof(line));
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD dwDisplacement = 0;

    if (!SymGetLineFromAddr64(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &dwDisplacement, &line)) {
        qCCritical(DRKONQI_LOG) << "SymGetLineFromAddr64 failed: " << GetLastError();
        return QString::fromLatin1(DEFAULT_FILE);
    }

    return QString::fromLatin1(const_cast<char *>(line.FileName));
}

int MsvcGenerator::GetLine()
{
    IMAGEHLP_LINE64 line;
    ZeroMemory(&line, sizeof(line));
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD dwDisplacement = 0;

    if (!SymGetLineFromAddr64(m_process.GetHandle(), m_currentFrame.AddrPC.Offset, &dwDisplacement, &line)) {
        // qCCritical(DRKONQI_LOG) << "SymGetLineFromAddr64 failed: " << GetLastError();
        return DEFAULT_LINE;
    }

    return (int)line.LineNumber;
}

void MsvcGenerator::LoadSymbol(const QString &module, DWORD64 dwBaseAddr)
{
    QString strOutput;

    IMAGEHLP_MODULE64 moduleInfo;
    ZeroMemory(&moduleInfo, sizeof(moduleInfo));
    moduleInfo.SizeOfStruct = sizeof(moduleInfo);
    SymGetModuleInfo64(m_process.GetHandle(), dwBaseAddr, &moduleInfo);

    m_symbolsMap[module] = false; // default
    QString symbolType;
    switch (moduleInfo.SymType) {
    case SymNone:
        symbolType = QString::fromLatin1("no symbols loaded");
        break;
    case SymCoff:
    case SymCv:
    case SymPdb:
    case SymSym:
    case SymDia:
        symbolType = QString::fromLatin1("symbols loaded");
        m_symbolsMap[module] = true;
        break;
    case SymExport:
        symbolType = QString::fromLatin1("export table only");
        break;
    case SymDeferred:
        symbolType = QString::fromLatin1("deferred (not loaded currently)");
        break;
    case SymVirtual:
        symbolType = QString::fromLatin1("virtual");
        break;
    }

    strOutput = QString::fromLatin1("Loaded %1 (%2)").arg(module).arg(symbolType);

    Q_EMIT DebugLine(strOutput);
}

#include "moc_msvc_generator.cpp"
