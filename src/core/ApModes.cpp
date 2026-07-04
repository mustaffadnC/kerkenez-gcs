#include "core/ApModes.h"

namespace kerkenez {

namespace {

QString copterModeName(uint32_t mode)
{
    switch (mode) {
    case 0: return QStringLiteral("Stabilize");
    case 1: return QStringLiteral("Acro");
    case 2: return QStringLiteral("AltHold");
    case 3: return QStringLiteral("Auto");
    case 4: return QStringLiteral("Guided");
    case 5: return QStringLiteral("Loiter");
    case 6: return QStringLiteral("RTL");
    case 7: return QStringLiteral("Circle");
    case 9: return QStringLiteral("Land");
    case 11: return QStringLiteral("Drift");
    case 13: return QStringLiteral("Sport");
    case 14: return QStringLiteral("Flip");
    case 15: return QStringLiteral("AutoTune");
    case 16: return QStringLiteral("PosHold");
    case 17: return QStringLiteral("Brake");
    case 18: return QStringLiteral("Throw");
    case 19: return QStringLiteral("Avoid_ADSB");
    case 20: return QStringLiteral("Guided_NoGPS");
    case 21: return QStringLiteral("SmartRTL");
    case 22: return QStringLiteral("FlowHold");
    case 23: return QStringLiteral("Follow");
    case 24: return QStringLiteral("ZigZag");
    case 25: return QStringLiteral("SystemID");
    case 26: return QStringLiteral("AutoRotate");
    case 27: return QStringLiteral("Auto_RTL");
    default: return QStringLiteral("Mode(%1)").arg(mode);
    }
}

QString planeModeName(uint32_t mode)
{
    switch (mode) {
    case 0: return QStringLiteral("Manual");
    case 1: return QStringLiteral("Circle");
    case 2: return QStringLiteral("Stabilize");
    case 3: return QStringLiteral("Training");
    case 4: return QStringLiteral("Acro");
    case 5: return QStringLiteral("FBWA");
    case 6: return QStringLiteral("FBWB");
    case 7: return QStringLiteral("Cruise");
    case 8: return QStringLiteral("AutoTune");
    case 10: return QStringLiteral("Auto");
    case 11: return QStringLiteral("RTL");
    case 12: return QStringLiteral("Loiter");
    case 13: return QStringLiteral("Takeoff");
    case 14: return QStringLiteral("Avoid_ADSB");
    case 15: return QStringLiteral("Guided");
    case 17: return QStringLiteral("QStabilize");
    case 18: return QStringLiteral("QHover");
    case 19: return QStringLiteral("QLoiter");
    case 20: return QStringLiteral("QLand");
    case 21: return QStringLiteral("QRTL");
    case 22: return QStringLiteral("QAutoTune");
    case 23: return QStringLiteral("QAcro");
    case 24: return QStringLiteral("Thermal");
    default: return QStringLiteral("Mode(%1)").arg(mode);
    }
}

bool isCopterType(uint8_t mavType)
{
    switch (mavType) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_DODECAROTOR:
        return true;
    default:
        return false;
    }
}

} // namespace

QString apModeName(uint8_t mavType, uint32_t customMode)
{
    if (isCopterType(mavType))
        return copterModeName(customMode);
    if (mavType == MAV_TYPE_FIXED_WING || mavType == MAV_TYPE_VTOL_TILTROTOR
        || mavType == MAV_TYPE_VTOL_FIXEDROTOR || mavType == MAV_TYPE_VTOL_TAILSITTER)
        return planeModeName(customMode);
    return QStringLiteral("Mode(%1)").arg(customMode);
}

} // namespace kerkenez
