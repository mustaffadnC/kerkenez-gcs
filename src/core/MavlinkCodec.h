#pragma once

#include <QByteArray>
#include <QObject>

#include "core/MavlinkDefs.h"

namespace kerkenez {

// Turns a raw byte stream into decoded MAVLink messages and tracks link
// quality statistics (CRC failures, sequence gaps).
class MavlinkCodec : public QObject
{
    Q_OBJECT
public:
    explicit MavlinkCodec(QObject *parent = nullptr);

    quint64 packetsReceived() const { return m_packetsReceived; }
    quint64 crcErrors() const { return m_crcErrors; }
    quint64 packetsLost() const { return m_packetsLost; }

    // Serializes a message for transmission.
    static QByteArray pack(const mavlink_message_t &msg);

public slots:
    void feed(const QByteArray &bytes);
    void reset();

signals:
    void messageReceived(const mavlink_message_t &msg);

private:
    mavlink_message_t m_rxBuffer{};
    mavlink_status_t m_rxBufferStatus{};
    quint64 m_packetsReceived = 0;
    quint64 m_crcErrors = 0;
    quint64 m_packetsLost = 0;
    int m_lastSeq = -1;
};

} // namespace kerkenez
