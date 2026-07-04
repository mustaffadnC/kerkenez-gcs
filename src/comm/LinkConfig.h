#pragma once

#include <QString>

namespace kerkenez {

struct LinkConfig
{
    enum class Type { Tcp, Udp, Serial };

    Type type = Type::Tcp;

    // Tcp: remote host/port. Udp: local bind port (host/port optionally a fixed peer).
    QString host = QStringLiteral("127.0.0.1");
    quint16 port = 5760;

    QString serialPortName;
    qint32 baudRate = 115200;

    QString description() const
    {
        switch (type) {
        case Type::Tcp:
            return QStringLiteral("tcp://%1:%2").arg(host).arg(port);
        case Type::Udp:
            return QStringLiteral("udp://0.0.0.0:%1").arg(port);
        case Type::Serial:
            return QStringLiteral("serial://%1@%2").arg(serialPortName).arg(baudRate);
        }
        return {};
    }
};

} // namespace kerkenez
