// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "sentryscope.h"

#include <QHash>
#include <QList>
#include <QUuid>

using namespace Qt::StringLiterals;

SentryScope *SentryScope::instance()
{
    static SentryScope scope = [] {
        SentryScope scope;
        scope.traceId = QUuid::createUuid().toString(QUuid::Id128);
        constexpr auto spanIdLength = 16; // per the spec
        scope.spanId = QUuid::createUuid().toString(QUuid::Id128).mid(0, spanIdLength);
        return scope;
    }();
    return &scope;
}

QHash<QString, QString> SentryScope::environment()
{
    QHash<QString, QString> env;
    // These variables are used to transfer context from one process to another;
    // in absence of on-the-wire context propagation.
    // WARNING: this is our context, not the one of the crashed app. Keep them separate!
    // This one is used to tie problems in our preamble to us.
    // https://develop.sentry.dev/sdk/telemetry/traces/distributed-tracing/
    // https://develop.sentry.dev/sdk/telemetry/traces/dynamic-sampling-context/#baggage-header
    env.insert(u"SENTRY_TRACE"_s, traceId + '-'_L1 + spanId);
    env.insert(u"SENTRY_BAGGAGE"_s, [this]() -> QString {
        QList<QString> baggage;
        baggage.append("sentry-trace_id="_L1 + traceId);
        baggage.append("sentry-release="_L1 + release);
        baggage.append(u"sentry-public_key=d6d53bb0121041dd97f59e29051a1781"_s); // our DSN key
        baggage.append(u"sentry-environment=production"_s);
        return baggage.join(','_L1);
    }());
    return env;
}
