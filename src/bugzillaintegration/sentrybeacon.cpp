// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "sentrybeacon.h"

#include "backtracegenerator.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqi_debug.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTemporaryDir>

void SentryBeacon::sendEvent()
{
    if (m_started) {
        return;
    }
    m_started = true;

    // We grab the payload here to prevent data races, it could change between now and when the actual submission happens.
    m_eventPayload = DrKonqi::debuggerManager()->backtraceGenerator()->sentryPayload();
    m_manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    getDSNs();
}

void SentryBeacon::sendUserFeedback(const QString &feedback)
{
    m_userFeedback = feedback;
    if (!m_eventID.isNull() && !m_userFeedback.isEmpty()) {
        postUserFeedback();
    } else {
        Q_EMIT userFeedbackSent();
    }
}

void SentryBeacon::getDSNs()
{
    QNetworkRequest request(QUrl::fromUserInput(QStringLiteral("https://errors-eval.kde.org/_drkonqi_static/0/dsns.json")));
    auto reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &SentryBeacon::onDSNsReceived);
}

void SentryBeacon::onDSNsReceived()
{
    auto reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    const auto application = DrKonqi::crashedApplication()->fakeExecutableBaseName();
    const auto document = QJsonDocument::fromJson(reply->readAll());
    const auto object = document.object();

    qDebug() << document << document.isObject() << object;
    if (maybePostStore(object.value(application))) {
        return;
    }
    qCWarning(DRKONQI_LOG) << "Failed to post to application" << application;
    if (maybePostStore(object.value(QStringLiteral("fallthrough")))) {
        return;
    }
    qCWarning(DRKONQI_LOG) << "Failed to post to dynamic fallthrough";
    // final fallback is a hardcoded fallthrough, this isn't ideal because we can't change this after releases
    postStore({QStringLiteral("fallthrough"), QStringLiteral("456f53a71a074438bbb786d6add63241"), QStringLiteral("11")});
}

bool SentryBeacon::maybePostStore(const QJsonValue &value)
{
    if (value.isObject()) {
        const auto object = value.toObject();
        postStore({
            object.value(QStringLiteral("project")).toString(),
            object.value(QStringLiteral("key")).toString(),
            object.value(QStringLiteral("index")).toString(),
        });
        return true;
    }
    return false;
}

void SentryBeacon::postStore(const SentryDSNContext &context)
{
    m_context = context;

    // https://develop.sentry.dev/sdk/store/
    QNetworkRequest request(QUrl::fromUserInput(QStringLiteral("https://%1@errors-eval.kde.org/api/%2/store/").arg(context.key, context.index)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("DrKonqi"));
    request.setRawHeader(QByteArrayLiteral("X-Sentry-Auth"),
                         QStringLiteral("Sentry sentry_version=7,sentry_timestamp=%1,sentry_client=sentry-curl/1.0,sentry_key=%2")
                             .arg(QString::number(std::time(nullptr)), context.key)
                             .toUtf8());

    auto reply = m_manager->post(request, m_eventPayload);
    connect(reply, &QNetworkReply::finished, this, &SentryBeacon::onStoreSent);
}

void SentryBeacon::onStoreSent()
{
    auto reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    const auto replyBlob = QJsonDocument::fromJson(reply->readAll());
    m_eventID = replyBlob.object().value(QStringLiteral("id"));

    Q_EMIT eventSent();
    if (!m_userFeedback.isEmpty()) { // a feedback was set in the meantime, apply it; otherwise we wait for sendFeedback()
        postUserFeedback();
    } else {
        Q_EMIT userFeedbackSent();
    }
}

void SentryBeacon::postUserFeedback()
{
    qDebug() << m_context.key << m_context.index;
    const QJsonObject feedbackObject = {
        {QStringLiteral("event_id"), QJsonValue::fromVariant(m_eventID)},
        {QStringLiteral("name"), QStringLiteral("Anonymous")},
        {QStringLiteral("email"), QStringLiteral("anonymous@kde.org")},
        {QStringLiteral("comments"), m_userFeedback},
    };
    // TODO we could back reference the bug report, but that needs some reshuffling

    // https://docs.sentry.io/api/projects/submit-user-feedback/
    QNetworkRequest request(QUrl::fromUserInput(QStringLiteral("https://errors-eval.kde.org/api/0/projects/kde/%1/user-feedback/").arg(m_context.project)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("DrKonqi"));
    request.setRawHeader(QByteArrayLiteral("Authorization"),
                         QStringLiteral("DSN https://%1@errors-eval.kde.org/%2").arg(m_context.key, m_context.index).toUtf8());
    auto feedbackReply = m_manager->post(request, QJsonDocument(feedbackObject).toJson());
    connect(feedbackReply, &QNetworkReply::finished, this, &SentryBeacon::onUserFeedbackSent);
}

void SentryBeacon::onUserFeedbackSent()
{
    auto reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    qDebug() << reply->readAll();
    Q_EMIT userFeedbackSent();
}
