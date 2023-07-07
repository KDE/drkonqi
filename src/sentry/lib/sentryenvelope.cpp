// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#include "sentryenvelope.h"

#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

static constexpr auto EVENT_ID = "event_id"_L1;
static constexpr auto SDK = "sdk"_L1;

SentryEvent::SentryEvent(const QByteArray &payload)
{
    m_headers.insert("type"_L1, "event"_L1);
    m_headers.insert("length"_L1, payload.size());

    m_payload = payload;
}

SentryUserFeedback::SentryUserFeedback(const QByteArray &payload)
{
    m_headers.insert("type"_L1, "user_report"_L1);
    m_headers.insert("length"_L1, payload.size());

    m_payload = payload;
}

QByteArray SentryItem::toEnvelopePayload() const
{
    QJsonDocument doc(QJsonObject::fromVariantHash(m_headers));
    const auto header = doc.toJson(QJsonDocument::Compact);
    return "%1\n%2"_L1.arg(QString::fromUtf8(header), QString::fromUtf8(m_payload)).toUtf8();
}

QByteArray SentryEnvelope::toEnvelope() const
{
    QJsonDocument doc(QJsonObject::fromVariantHash(m_headers));
    const auto header = doc.toJson(QJsonDocument::Compact);
    QList<QByteArray> payloads;
    payloads.reserve(m_items.size());
    for (const auto &item : m_items) {
        payloads.push_back(item.toEnvelopePayload());
    }
    return "%1\n%2"_L1.arg(QString::fromUtf8(header), QString::fromUtf8(payloads.join("\n"_qba))).toUtf8();
}

void SentryEnvelope::addItem(const SentryItem &item)
{
    const auto document = QJsonDocument::fromJson(item.m_payload);

    for (const auto &globalHeader : {EVENT_ID, SDK}) {
        const auto weHaveHeader = !m_headers.value(globalHeader).toString().isEmpty();
        const auto incomingHeader = document[globalHeader].toString();
        const auto incomingHasHeader = !incomingHeader.isEmpty();
        if (!weHaveHeader && incomingHasHeader) {
            m_headers.insert(globalHeader, incomingHeader);
        }
    }

    m_items.push_back(item);
}

QString SentryEnvelope::eventId() const
{
    return m_headers.value(EVENT_ID).toString();
}

void SentryEnvelope::setDSN(const QUrl &dsn)
{
    m_headers.insert("dsn"_L1, dsn.toString());
}

bool SentryEnvelope::isEmpty() const
{
    return m_items.isEmpty() || m_headers.value(EVENT_ID).toString().isEmpty();
}
