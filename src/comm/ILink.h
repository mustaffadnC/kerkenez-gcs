#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace kerkenez {

// Transport abstraction: TCP, UDP or serial connection carrying MAVLink bytes.
class ILink : public QObject
{
    Q_OBJECT
public:
    enum class State { Disconnected, Connecting, Connected };
    Q_ENUM(State)

    using QObject::QObject;

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void send(const QByteArray &bytes) = 0;
    virtual State state() const = 0;
    virtual QString description() const = 0;

signals:
    void bytesReceived(const QByteArray &bytes);
    void stateChanged(kerkenez::ILink::State state);
    void errorOccurred(const QString &message);
};

} // namespace kerkenez
