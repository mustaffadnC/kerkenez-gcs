# Architecture

## Layers

```
┌────────────────────────────────────────────────┐
│ src/app     kerkenez.exe / poc_telemetry.exe   │  wiring, DI
├────────────────────────────────────────────────┤
│ src/ui      QtWidgets                          │  PFD, map widget, panels
├──────────────────────────┬─────────────────────┤
│ src/core  QtCore only    │ src/map  no GUI     │  codec, vehicle state │ tiles
├──────────────────────────┴─────────────────────┤
│ src/comm    QtNetwork + QtSerialPort           │  TCP / UDP / serial links
├────────────────────────────────────────────────┤
│ third_party/mavlink (c_library_v2, ardupilot)  │  generated MAVLink v2 headers
└────────────────────────────────────────────────┘
```

Dependency rule: `comm` depends on `core`; `ui` depends on `core`, `map` and on
`comm`'s link abstractions (`LinkConfig`, `ILink` state, `LinkManager`);
`core` depends only on QtCore + MAVLink headers and `map` on QtCore/Gui/Network.
Nothing depends on `app`.

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

Controllers never touch a link themselves — they emit `sendMessage(QByteArray)`
and consume decoded messages through `handleMessage`. That keeps the protocol
state machines testable with nothing but synthetic messages, which is how the
mission upload/download, acknowledgement matching and retry paths are covered
in CI.

## Planned class inventory

| Layer | Class | Responsibility |
|---|---|---|
| comm | `ILink` | transport abstraction (state, bytes in/out) |
| comm | `TcpLink` / `UdpLink` / `SerialLink` | concrete transports |
| comm | `LinkManager` | active link, auto-reconnect, statistics |
| core | `MavlinkCodec` | byte stream ⇆ messages, CRC/seq stats |
| core | `Vehicle` | central vehicle state, Qt signals per field group |
| core | `CommandController` | COMMAND_LONG + ACK matching, one command at a time, timeout/retry |
| core | `MissionController` | mission protocol state machine (upload/download/clear) |
| core | `ParamController` | parameter list/set, re-requests indices the link dropped |
| core | `TelemetryRecorder` / `TlogPlayer` | timestamped tlog record + replay |
| ui | `PfdWidget`, `CompassWidget` | QPainter flight instruments |
| map | `TileMath` | Web Mercator conversions, ground resolution, distances |
| map | `TileCache` | RAM LRU over a `<z>/<x>/<y>.png` disk tree — the offline backing store |
| map | `TileFetcher` | OSM downloads: identifying User-Agent, 2 concurrent, offline switch |
| ui | `MapWidget` | slippy map painting, pan/zoom, vehicle icon, trail, home marker |
| ui | `MissionEditor`, `ParamTable`, `ConnectDialog`, `AlertPanel` | operator UI |

`TlogPlayer` emits the same signals as the live pipeline, so every UI component
works identically during replay — that is why recording lives in `core`.

## Testing strategy

- `core` is GUI-free: unit tests run headless in CI (`QT_QPA_PLATFORM=offscreen`).
- Codec tests feed hand-packed frames (valid, split, corrupted CRC) — see
  `tests/tst_mavlinkcodec.cpp`.
- Recorded SITL byte streams under `tests/data/` serve as parser fixtures.
- Widget tests render into a `QImage` and assert exact pixel colors, which is
  how a leaked painter brush tinting the whole map was caught.
- Map tests never touch the network: the fetcher is switched offline and the
  cache is pre-seeded, so CI proves the offline path rather than assuming it.
- Integration testing happens against ArduPilot SITL (see `docs/setup.md`).
