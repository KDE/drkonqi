// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022-2023 Harald Sitter <sitter@kde.org>

#include "sentrypostbox.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

#include "sentryconnection.h"
#include "sentrypaths.h"

SentryPostbox::SentryPostbox(const QString &applicationName, std::shared_ptr<SentryConnection> connection, QObject *parent)
    : QObject(parent)
    , m_dsns(std::move(connection))
    , m_applicationName(applicationName)
{
    connect(&m_dsns, &SentryDSNs::loaded, this, &SentryPostbox::onDSNsLoaded);
    m_dsns.load();
}

void SentryPostbox::onDSNsLoaded()
{
    m_dsnContext = m_dsns.context(m_applicationName);

    m_envelope.setDSN(m_dsnContext.dsnUrl());
    m_dsnSet = true;
}

void SentryPostbox::addEventPayload(const SentryEvent &event)
{
    m_envelope.addItem(event);
}

void SentryPostbox::addUserFeedback(const QString &feedbackString)
{
    const QJsonObject feedbackObject = {
        {QStringLiteral("name"), QStringLiteral("Anonymous")},
        {QStringLiteral("email"), QStringLiteral("anonymous@kde.org")},
        {QStringLiteral("comments"), feedbackString},
    };

    m_envelope.addItem(SentryUserFeedback(QJsonDocument(feedbackObject).toJson(QJsonDocument::Compact)));
}

void SentryPostbox::deliver()
{
    Q_ASSERT(!m_envelope.eventId().isEmpty()); // requirement for path construction
    QFile file(SentryPaths::payloadPath(m_envelope.eventId()));
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        file.write(m_envelope.toEnvelope());
    }
    m_hasDelivered = true;
    Q_EMIT hasDeliveredChanged();
}

bool SentryPostbox::hasDelivered() const
{
    return m_hasDelivered;
}

bool SentryPostbox::isReadyToDeliver() const
{
    return m_dsnSet && !m_envelope.isEmpty();
}
