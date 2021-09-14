/*
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "apijob.h"

#include <QMetaMethod>
#include <QTimer>

#include <KIOCore/KIO/TransferJob>

#include "bugzilla_debug.h"
#include "exceptions.h"

namespace Bugzilla
{
TransferAPIJob::TransferAPIJob(KIO::TransferJob *transferJob, QObject *parent)
    : APIJob(parent)
    , m_transferJob(transferJob)
{
    // Required for every request type.
    addMetaData(QStringLiteral("content-type"), QStringLiteral("application/json"));
    addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
    addMetaData(QStringLiteral("UserAgent"), QStringLiteral("DrKonqi"));
    // We don't want HTML blobs but proper job errors + text!
    addMetaData(QStringLiteral("errorPage"), QStringLiteral("false"));
    // Disable automatic cookie injection. We don't need cookies but they
    // can mess up requests (supposedly by being unexpected or invalid or outdated ...)
    // https://bugs.kde.org/show_bug.cgi?id=419646
    addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));

    connect(m_transferJob, &KIO::TransferJob::data, this, [this](KIO::Job *, const QByteArray &data) {
        m_data += data;
    });

    connect(m_transferJob, &KIO::TransferJob::finished, this, [this](KJob *job) {
        // Set errors, they are read by document() when the consumer reads
        // the data and possibly raised as exception.
        setError(job->error());
        setErrorText(job->errorText());

        Q_ASSERT(!((KIO::TransferJob *)job)->isErrorPage());

        // Force a delay on all API actions if configured. This allows
        // simulation of slow connections.
        static int delay = qEnvironmentVariableIntValue("DRKONQI_HTTP_DELAY_MS");
        if (delay > 0) {
            QTimer::singleShot(delay, this, [this] {
                emitResult();
            });
            return;
        }

        emitResult();
    });
}

void TransferAPIJob::addMetaData(const QString &key, const QString &value)
{
    m_transferJob->addMetaData(key, value);
}

void TransferAPIJob::setPutData(const QByteArray &data)
{
    m_putData = data;

    // This is really awkward, does it need to be this way? Why can't we just
    // push the entire array in?

    // dataReq says we shouldn't send data >1mb, so segment the incoming data
    // accordingly and generate QBAs wrapping the raw data (zero-copy).
    int segmentSize = 1024 * 1024; // 1 mb per segment maximum
    int segments = qMax(data.size() / segmentSize, 1);
    m_dataSegments.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        int offset = i * segmentSize;
        const char *buf = data.constData() + offset;
        int segmentLength = qMin(offset + segmentSize, data.size());
        m_dataSegments.append(QByteArray::fromRawData(buf, segmentLength));
    }

    // TODO: throw away, only here to make sure I don't mess up the
    // segmentation.
    int allLengths = 0;
    for (const auto &a : qAsConst(m_dataSegments)) {
        allLengths += a.size();
    }
    Q_ASSERT(allLengths == data.size());

    connect(m_transferJob, &KIO::TransferJob::dataReq, this, [this](KIO::Job *, QByteArray &dataForSending) {
        if (m_dataSegments.isEmpty()) {
            return;
        }
        dataForSending = m_dataSegments.takeFirst();
    });
}

QJsonDocument APIJob::document() const
{
    ProtocolException::maybeThrow(this);
    Q_ASSERT(error() == KJob::NoError);

    auto document = QJsonDocument::fromJson(data());
    APIException::maybeThrow(document);
    return document;
}

QJsonObject APIJob::object() const
{
    return document().object();
}

void APIJob::setAutoStart(bool start)
{
    m_autostart = start;
}

void APIJob::connectNotify(const QMetaMethod &signal)
{
    if (m_autostart && signal == QMetaMethod::fromSignal(&KJob::finished)) {
        qCDebug(BUGZILLA_LOG) << "auto starting";
        start();
    }
    KJob::connectNotify(signal);
}

} // namespace Bugzilla
