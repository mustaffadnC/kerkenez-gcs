# MAVLink Usage Notes

Dialect: `ardupilotmega` (vendored from [mavlink/c_library_v2](https://github.com/mavlink/c_library_v2),
commit recorded in `third_party/mavlink/VERSION.txt`). MAVLink v2 framing.

Why not plain `common`: ArduPilot also emits its own dialect messages (AHRS,
SIMSTATE, MEMINFO, ...). A parser that only knows `common` cannot look up their
`crc_extra` and misclassifies every one of them as a CRC failure — and the
sequence-loss statistics drift because the "failed" frames still consume
sequence numbers. Discovered via the recorded-stream fixture test.

GCS identity: `sysid 255`, `compid MAV_COMP_ID_MISSIONPLANNER` (what ArduPilot
expects from a ground station).

## Messages consumed

| Message | Use |
|---|---|
| HEARTBEAT | presence, vehicle type, armed flag, custom_mode (flight mode) |
| SYS_STATUS | battery voltage/current/remaining, sensor health |
| GLOBAL_POSITION_INT | fused lat/lon/alt + relative alt (map, HUD) |
| ATTITUDE | roll/pitch/yaw for the artificial horizon |
| VFR_HUD | airspeed, groundspeed, climb, throttle, heading |
| GPS_RAW_INT | fix type, satellite count |
| STATUSTEXT | autopilot messages → alert panel (severity-colored) |
| HOME_POSITION | home marker on map |
| MISSION_* , PARAM_VALUE, COMMAND_ACK | protocol responses |

## Messages sent

| Message | Use |
|---|---|
| HEARTBEAT (1 Hz) | GCS presence (ArduPilot failsafe expects it) |
| REQUEST_DATA_STREAM | enable telemetry streams (4 Hz); ArduPilot still honors this legacy request — simpler than per-message SET_MESSAGE_INTERVAL |
| COMMAND_LONG | ARM/DISARM, NAV_TAKEOFF, RTL, LAND, DO_SET_MODE |
| SET_POSITION_TARGET_GLOBAL_INT | guided-mode "fly here" |
| MISSION_COUNT / MISSION_ITEM_INT / MISSION_REQUEST_INT / MISSION_ACK | mission upload/download (INT variants only — float lat/lon loses precision) |
| PARAM_REQUEST_LIST / PARAM_SET | parameter screen |

## Protocol notes

- **Mission protocol** is a strict request/response ladder; the state machine in
  `MissionController` must tolerate retransmits and out-of-order requests.
  Reference: https://mavlink.io/en/services/mission.html
- **ArduPilot flight modes** live in `custom_mode` (e.g. Copter: 0=Stabilize,
  3=Auto, 4=Guided, 5=Loiter, 6=RTL, 9=Land). Mode names differ per vehicle type —
  map them from HEARTBEAT `type`.
- **SITL specifics:** serial0 listens on TCP `5760 + 10 × instance`. The vehicle
  boots disarmed; arming requires GPS lock (~10 s after start) and
  `MAV_CMD_COMPONENT_ARM_DISARM`.
