#pragma once

// Single include point for the vendored MAVLink headers. The generated code
// uses packed structs; GCC warns about taking addresses of packed members,
// which is harmless on x86 — suppress only around this include.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include <common/mavlink.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <QMetaType>

Q_DECLARE_METATYPE(mavlink_message_t)
