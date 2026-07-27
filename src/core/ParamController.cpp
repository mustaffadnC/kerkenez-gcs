#include "core/ParamController.h"

#include "core/GcsMessages.h"
#include "core/MavlinkCodec.h"
#include "core/Vehicle.h"

namespace kerkenez {

namespace {

// param_id is a fixed 16-byte field that is only null-terminated when shorter.
QString paramName(const char id[16])
{
    char buffer[17] = {};
    memcpy(buffer, id, 16);
    return QString::fromLatin1(buffer);
}

void fillParamId(char out[16], const QString &name)
{
    memset(out, 0, 16);
    const QByteArray latin = name.toLatin1();
    memcpy(out, latin.constData(), size_t(qMin(latin.size(), 16)));
}

} // namespace

ParamController::ParamController(Vehicle *vehicle, QObject *parent)
    : QObject(parent)
    , m_vehicle(vehicle)
{
    m_quietTimer.setSingleShot(true);
    connect(&m_quietTimer, &QTimer::timeout, this, &ParamController::requestMissing);
}

uint8_t ParamController::target() const
{
    return uint8_t(m_vehicle->systemId());
}

void ParamController::refresh()
{
    m_values.clear();
    m_types.clear();
    m_receivedIndices.clear();
    m_expectedCount = 0;
    m_gapPass = 0;
    m_loading = true;

    mavlink_message_t msg;
    mavlink_msg_param_request_list_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                                        MAV_COMP_ID_AUTOPILOT1);
    emit sendMessage(MavlinkCodec::pack(msg));
    m_quietTimer.start(m_quietPeriodMs);
}

void ParamController::setParameter(const QString &name, float value)
{
    mavlink_message_t msg;
    char id[16];
    fillParamId(id, name);
    mavlink_msg_param_set_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                               MAV_COMP_ID_AUTOPILOT1, id, value,
                               m_types.value(name, MAV_PARAM_TYPE_REAL32));
    emit sendMessage(MavlinkCodec::pack(msg));
}

void ParamController::handleMessage(const mavlink_message_t &msg)
{
    if (msg.msgid != MAVLINK_MSG_ID_PARAM_VALUE)
        return;

    mavlink_param_value_t value;
    mavlink_msg_param_value_decode(&msg, &value);

    const QString name = paramName(value.param_id);
    if (name.isEmpty())
        return;

    m_values.insert(name, value.param_value);
    m_types.insert(name, value.param_type);
    emit parameterChanged(name, value.param_value);

    if (!m_loading)
        return;

    m_expectedCount = value.param_count;
    m_receivedIndices.insert(value.param_index);
    emit progress(m_receivedIndices.size(), m_expectedCount);

    if (m_expectedCount > 0 && m_receivedIndices.size() >= m_expectedCount) {
        m_loading = false;
        m_quietTimer.stop();
        emit refreshFinished(m_values.size());
        return;
    }
    m_quietTimer.start(m_quietPeriodMs);
}

void ParamController::requestMissing()
{
    if (!m_loading)
        return;

    if (m_expectedCount <= 0) {
        m_loading = false;
        emit failed(tr("no parameters received"));
        return;
    }

    QVector<int> missing;
    for (int index = 0; index < m_expectedCount; ++index) {
        if (!m_receivedIndices.contains(index))
            missing.append(index);
    }

    if (missing.isEmpty()) {
        m_loading = false;
        emit refreshFinished(m_values.size());
        return;
    }

    if (++m_gapPass > m_maxGapPasses) {
        m_loading = false;
        emit failed(tr("%1 of %2 parameters never arrived")
                        .arg(missing.size())
                        .arg(m_expectedCount));
        return;
    }

    char emptyId[16] = {};
    for (const int index : missing) {
        mavlink_message_t msg;
        mavlink_msg_param_request_read_pack(kGcsSystemId, kGcsComponentId, &msg, target(),
                                            MAV_COMP_ID_AUTOPILOT1, emptyId, int16_t(index));
        emit sendMessage(MavlinkCodec::pack(msg));
    }
    m_quietTimer.start(m_quietPeriodMs);
}

} // namespace kerkenez
