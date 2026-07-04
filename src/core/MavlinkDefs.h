#pragma once

// Single include point for the vendored MAVLink headers. The generated code
// uses packed structs; GCC warns about taking addresses of packed members,
// which is harmless on x86 — suppress only around this include.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

// ardupilotmega extends common with ArduPilot-specific messages (AHRS,
// SIMSTATE, MEMINFO, ...). Without them the parser would flag every unknown
// message id as a CRC failure and the sequence-loss statistics would drift.
#include <ardupilotmega/mavlink.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <QMetaType>

Q_DECLARE_METATYPE(mavlink_message_t)
