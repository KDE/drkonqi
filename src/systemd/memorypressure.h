// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>

class SDPressureMonitor;

class MemoryPressure : public QObject
{
    Q_OBJECT
public:
    enum class Level {
        // Low memory pressure
        Low,
        // Memory pressure is high
        High,
    };

    [[nodiscard]] static MemoryPressure *instance();

    ~MemoryPressure() override;
    Q_DISABLE_COPY_MOVE(MemoryPressure)

    void reset();

    [[nodiscard]] Level level() const;

Q_SIGNALS:
    void levelChanged();
    void monitoring(pid_t pid);
    void pidReset();

private:
    explicit MemoryPressure(QObject *parent = nullptr);

    Level m_level = Level::Low;
    SDPressureMonitor *m_monitor = nullptr;
};
