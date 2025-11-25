// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>

class DrKonqiDialog : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    enum class GoTo { Main, Sentry };
    void show(GoTo to);
};
