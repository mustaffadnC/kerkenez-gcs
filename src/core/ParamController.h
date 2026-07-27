#pragma once

#include <QByteArray>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QTimer>

#include "core/MavlinkDefs.h"

namespace kerkenez {

class Vehicle;

// Parameter protocol. A full download is a burst of a thousand-odd
// PARAM_VALUE messages over a lossy link, so gaps are expected: once the burst
// goes quiet, the missing indices are re-requested individually.
class ParamController : public QObject
{
    Q_OBJECT
public:
    explicit ParamController(Vehicle *vehicle, QObject *parent = nullptr);

    QMap<QString, float> parameters() const { return m_values; }
    bool isLoading() const { return m_loading; }
    int expectedCount() const { return m_expectedCount; }

    void setQuietPeriodMs(int ms) { m_quietPeriodMs = ms; }
    void setMaxGapPasses(int passes) { m_maxGapPasses = passes; }

public slots:
    void handleMessage(const mavlink_message_t &msg);
    void refresh();
    void setParameter(const QString &name, float value);

signals:
    void sendMessage(const QByteArray &bytes);
    void progress(int received, int total);
    void parameterChanged(const QString &name, float value);
    void refreshFinished(int count);
    void failed(const QString &reason);

private:
    void requestMissing();
    uint8_t target() const;

    Vehicle *m_vehicle;
    QMap<QString, float> m_values;
    QHash<QString, uint8_t> m_types;
    QSet<int> m_receivedIndices;
    int m_expectedCount = 0;
    bool m_loading = false;
    int m_gapPass = 0;
    int m_maxGapPasses = 3;
    int m_quietPeriodMs = 1500;
    QTimer m_quietTimer;
};

} // namespace kerkenez
