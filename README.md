# Kerkenez GCS

[![CI](https://github.com/conny0506/kerkenez-gcs/actions/workflows/ci.yml/badge.svg)](https://github.com/conny0506/kerkenez-gcs/actions/workflows/ci.yml)

A UAV ground control station written in **C++20 / Qt 6**, speaking **MAVLink v2** and tested against **ArduPilot SITL** — no hardware required.

> 🇹🇷 *Türkçe özet aşağıda.*

## Why

Most hobby GCS projects wrap a web view around someone else's stack. Kerkenez is built from the transport layer up:

- **Raw MAVLink v2** framing over TCP/UDP/serial — own codec on top of the official `c_library_v2` headers, with CRC and sequence-loss tracking
- **Custom flight instruments** — artificial horizon, tapes and compass drawn with QPainter (no web view)
- **Offline-capable slippy map** — own tile engine with disk cache, built for environments where you cannot assume connectivity
- **Simulation-first workflow** — every feature is validated against ArduPilot SITL (Copter + Plane)

## Status — Phase 0 (skeleton) ✅

| Phase | Scope | Status |
|---|---|---|
| 0 | Toolchain, SITL pipeline, MAVLink codec + tests, CI | ✅ done |
| 1 | Link layer (TCP/UDP/serial), vehicle model, reconnect | ⬜ |
| 2 | Telemetry panel / PFD (artificial horizon, tapes, alerts) | ⬜ |
| 3 | Map with offline tile cache, live tracking | ⬜ |
| 4 | Commands (ARM/Takeoff/RTL), waypoint mission editor, params | ⬜ |
| 5 | tlog recording + replay, flight summary | ⬜ |
| 6 | Release packaging, demo video | ⬜ |

Working today: `poc_telemetry` connects to SITL on TCP 5760, requests streams and prints live HEARTBEAT / ATTITUDE / GLOBAL_POSITION_INT.

## Build

Requirements: Qt 6.10 (MinGW), MinGW 13.1, CMake ≥ 3.21, Ninja. See [docs/setup.md](docs/setup.md) for one-command installs via `aqtinstall`.

```powershell
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --preset mingw-debug
```

## Run against SITL

```powershell
# one-time: download ArduPilot SITL prebuilt binaries (~25 MB)
powershell -ExecutionPolicy Bypass -File tools\get_sitl.ps1

# terminal 1 — start the simulated quad (Ankara home position)
cd tools\sitl\copter
.\ArduCopter.exe --model + --home 39.925533,32.866287,850,0 --defaults copter.parm -I0

# terminal 2 — live telemetry
.\build\mingw-debug\src\app\poc_telemetry.exe
```

## Architecture

```
src/core  — QtCore only: MAVLink codec, vehicle state, protocols (testable headless)
src/comm  — transports: TCP / UDP / serial links
src/ui    — QtWidgets: instruments, map, mission editor
src/app   — application wiring + PoC tools
```

Details in [docs/architecture.md](docs/architecture.md), MAVLink usage notes in [docs/mavlink-notes.md](docs/mavlink-notes.md).

---

## 🇹🇷 Türkçe Özet

Kerkenez, **C++20 / Qt 6** ile yazılmış, **MAVLink v2** konuşan ve **ArduPilot SITL** ile donanımsız test edilen bir İHA yer kontrol istasyonudur. Hazır web bileşeni gömmek yerine ulaşım katmanından itibaren kendi yazılmıştır: özel MAVLink codec'i (CRC + paket kaybı takibi), QPainter ile çizilen uçuş göstergeleri ve **offline çalışabilen** disk cache'li harita motoru. Şu an Faz 0 (iskelet + SITL hattı + testler + CI) tamamlandı; yol haritası yukarıdaki tabloda.

## License

[MIT](LICENSE). Vendored MAVLink headers ([third_party/mavlink](third_party/mavlink)) are MIT-licensed by the MAVLink project.
