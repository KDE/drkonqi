// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

#pragma once

#include "DumpTruckInterface.h"

#include <QObject>

// Issues KNotifications with the option of opening konsole with gdb.
// It also sports no i18n because it must be opted into and is only intended
// for developers.
class DevNotifierTruck : public QObject, public DumpTruckInterface
{
    Q_OBJECT
public:
    using QObject::QObject;
    [[nodiscard]] bool handle(const Coredump &dump) override;
};
