# Architecture

## Layers

```
┌────────────────────────────────────────────────┐
│ src/app     kerkenez.exe / poc_telemetry.exe   │  wiring, DI
├────────────────────────────────────────────────┤
│ src/ui      QtWidgets                          │  PFD, map, mission editor
├────────────────────────────────────────────────┤
│ src/core    QtCore only (headless-testable)    │  codec, vehicle state, protocols
├────────────────────────────────────────────────┤
│ src/comm    QtNetwork + QtSerialPort           │  TCP / UDP / serial links
├────────────────────────────────────────────────┤
│ third_party/mavlink  (c_library_v2, common)    │  generated MAVLink v2 headers
└────────────────────────────────────────────────┘
```

Dependency rule: `comm` depends on `core`; `ui` depends on `core` and on
`comm`'s link abstractions (`LinkConfig`, `ILink` state, `LinkManager`);
`core` depends only on QtCore + MAVLink headers. Nothing depends on `app`.

## Data flow (telemetry, Phase 1 target)

```
SITL/vehicle ──TCP/UDP/serial──► ILink ──bytesReceived(QByteArray)──► MavlinkCodec
                                                                        │
                                                   messageReceived(mavlink_message_t)
                                                                        │
                                              Vehicle (typed state + change signals)
                                                                        │
                                 ┌──────────────┬──────────────┬────────┴──────┐
                               PfdWidget   MapWidget      StatusPanel    TelemetryRecorder
```

Command flow is the reverse: UI → controller (Command/Mission/Param) → `MavlinkCodec::pack` → `ILink::send`.

## Planned class inventory

| Layer | Class | Responsibility |
|---|---|---|
| comm | `ILink` | transport abstraction (state, bytes in/out) |
| comm | `TcpLink` / `UdpLink` / `SerialLink` | concrete transports |
| comm | `LinkManager` | active link, auto-reconnect, statistics |
| core | `MavlinkCodec` | byte stream ⇆ messages, CRC/seq stats |
| core | `Vehicle` | central vehicle state, Qt signals per field group |
| core | `CommandController` | COMMAND_LONG + ACK matching, timeout/retry |
| core | `MissionController` | mission protocol state machine (upload/download) |
| core | `ParamController` | parameter list/set with progress |
| core | `TelemetryRecorder` / `TlogPlayer` | timestamped tlog record + replay |
| ui | `PfdWidget`, `CompassWidget` | QPainter flight instruments |
| ui | `MapWidget` + `TileCache` + `TileFetcher` | offline-capable slippy map |
| ui | `MissionEditor`, `ParamTable`, `ConnectDialog`, `AlertPanel` | operator UI |

`TlogPlayer` emits the same signals as the live pipeline, so every UI component
works identically during replay — that is why recording lives in `core`.

## Testing strategy

- `core` is GUI-free: unit tests run headless in CI (`QT_QPA_PLATFORM=offscreen`).
- Codec tests feed hand-packed frames (valid, split, corrupted CRC) — see
  `tests/tst_mavlinkcodec.cpp`.
- Phase 1 adds recorded SITL byte streams under `tests/data/` as parser fixtures.
- Integration testing happens against ArduPilot SITL (see `docs/setup.md`).
