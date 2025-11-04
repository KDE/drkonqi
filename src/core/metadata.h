// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2019-2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QJsonObject>
#include <QLatin1StringView>

// Please note that we have two types of metadata. The incoming kcrash data and the internal drkonqi data.
// The principal difference is that the drkonqi data is more comprehensive and persistent. KCrash data gets
// consumed and deleted once it got turned into a drkonqi variant.
// Generally speaking after coredump-launcher ran the drkonqi data is available, before that only kcrash is available.
namespace Metadata
{
constexpr auto DRKONQI_KEY = QLatin1StringView("drkonqi");
constexpr auto PICKED_UP_KEY = QLatin1StringView("PickedUp");
constexpr auto SENTRY_EVENT_ID_KEY = QLatin1StringView("sentryEventId");

constexpr auto KCRASH_KEY = QLatin1StringView("kcrash");
constexpr auto KCRASH_TAGS_KEY = QLatin1StringView("kcrash-tags");
constexpr auto KCRASH_EXTRA_DATA_KEY = QLatin1StringView("kcrash-extra-data");
constexpr auto KCRASH_GPU_KEY = QLatin1StringView("kcrash-gpu");

[[nodiscard]] QString drkonqiMetadataPath(const QString &exe, const QString &bootId, const QString &timestamp, int pid);
[[nodiscard]] QString resolveKCrashMetadataPath(const QString &exe, const QString &bootId, int pid);
[[nodiscard]] QJsonObject readFromDisk(const QString &drkonqiMetadataPath);
[[nodiscard]] bool isPickedUp(const QJsonObject &metadata);
[[nodiscard]] QStringList metadataArguments(const QVariantHash &kcrash);
} // namespace Metadata
