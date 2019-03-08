/*
    Copyright 2019 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef APIJOB_H
#define APIJOB_H

#include <QJsonArray> // Not used here but included so clients don't have to
#include <QJsonDocument>
#include <QJsonObject>

#include <KJob>

namespace KIO {
class TransferJob;
}

namespace Bugzilla {

class APIJob : public KJob
{
    Q_OBJECT
public:
    using KJob::KJob;

    /**
     * @throws ProtocolException on unexpected HTTP statuses or KIO errors
     * @throws APIException when the API returned an error object
     * @return json document returned by api request
     */
    QJsonDocument document() const;

    /**
     * This is a convenience function on top of document() and may throw all
     * the same exceptions.
     * @return json object of document (may be in valid if the doc has none)
     */
    QJsonObject object() const;

    void setAutoStart(bool start);

public:
    // Should be protected but since we call it for testing I don't care.
    virtual QByteArray data() const = 0;

private:
    virtual void start() override {}
    virtual void connectNotify(const QMetaMethod &signal) override;

    bool m_autostart = true;
};

class TransferAPIJob : public APIJob
{
    Q_OBJECT
    friend class HTTPConnection; // constructs us, ctor is private though
public:
    virtual QByteArray data() const override { return m_data; }

private:
    explicit TransferAPIJob(KIO::TransferJob *transferJob, QObject *parent = nullptr);

    void setPutData(const QByteArray &data);
    void addMetaData(const QString &key, const QString &value);

    KIO::TransferJob *m_transferJob = nullptr;
    QByteArray m_data;
    QByteArray m_putData;
    QList<QByteArray> m_dataSegments;
};

} // namespace Bugzilla

#endif // APIJOB_H
