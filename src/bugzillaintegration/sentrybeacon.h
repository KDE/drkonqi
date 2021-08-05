// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <memory>

struct SentryDSNContext {
    QString project;
    QString key;
    QString index;
};

class SentryBeacon : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    void sendEvent();
    void sendUserFeedback(const QString &feedback);

Q_SIGNALS:
    void eventSent();
    void userFeedbackSent();

private Q_SLOTS:
    void getDSNs();
    void onDSNsReceived();

    bool maybePostStore(const QJsonValue &value);

    void postStore(const SentryDSNContext &context);
    void onStoreSent();

    void postUserFeedback();
    void onUserFeedbackSent();

private:
    QString m_userFeedback;
    std::unique_ptr<QNetworkAccessManager> m_manager = std::make_unique<QNetworkAccessManager>();
    SentryDSNContext m_context;
    QByteArray m_eventPayload;
    QVariant m_eventID;
    bool m_started = false;
};
