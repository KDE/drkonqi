/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
*/

#pragma once

#include <QtPlugin>

class Coredump;

class DumpTruckInterface
{
public:
    DumpTruckInterface() = default;
    virtual ~DumpTruckInterface() = default;
    Q_DISABLE_COPY_MOVE(DumpTruckInterface)
    [[nodiscard]] virtual bool handle(const Coredump &dump) = 0;
};
