// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include "DumpTruckInterface.h"

#include <QObject>

class GlobalNotifierTruck : public QObject, public DumpTruckInterface
{
    Q_OBJECT
public:
    using QObject::QObject;
    [[nodiscard]] bool handle(const Coredump &dump) override;
};
