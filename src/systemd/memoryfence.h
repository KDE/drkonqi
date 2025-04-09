// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusConnection>
#include <QObject>

class MemoryFence : public QObject
{
    Q_OBJECT
public:
    enum class Size {
        Spacious,
        Some,
        Little,
        Cramped,
    };
    Q_ENUM(Size)

    Q_PROPERTY(Size size MEMBER m_size NOTIFY sizeChanged)

    using QObject::QObject;

    void surroundMe();
    [[nodiscard]] Size size() const;

Q_SIGNALS:
    void loaded();
    void sizeChanged();

private:
    bool registerDBusTypes();
    void getUnit();
    [[nodiscard]] std::optional<qulonglong> freeRAM();
    void getMemory();
    void applyProperties(qulonglong memoryCurrent, qulonglong memoryAvailable);

    QDBusConnection m_bus = QDBusConnection::sessionBus();
    inline const static QString s_service = QStringLiteral("org.freedesktop.systemd1");
    inline const static QString s_manager = QStringLiteral("org.freedesktop.systemd1.Manager");
    inline const static QString s_unit = QStringLiteral("org.freedesktop.systemd1.Unit");
    QString m_unitPath;
    std::optional<quint64> m_memory;
    Size m_size = Size::Spacious;
};
