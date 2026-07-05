# Kerkenez GCS — İHA Yer Kontrol İstasyonu

Savunma sanayi portföyü için C++20/Qt6 + MAVLink v2 + ArduPilot SITL yer kontrol istasyonu.
Portföy hikayesi: HavaKarakolu-Firmware (STM32 aviyonik) + roket-yer-istasyonu (Python YKİ) + Kerkenez (endüstri standardı C++/Qt YKİ).

## Doğrulanmış Altyapı (4 Tem 2026 — değiştirme, kuruldu ve test edildi)

- **Qt 6.10.3 win64_mingw** → `C:\Qt\6.10.3\mingw_64` (+ qtserialport, qtcharts, qtimageformats)
- **Derleyici: Qt'nin MinGW 13.1'i** → `C:\Qt\Tools\mingw1310_64` — WinLibs GCC 16 ile DERLEME; runtime uyumsuzluğu riski. Presetler doğru derleyiciyi pinliyor.
- **Neden 6.12 değil:** aqtinstall 3.3.0 (en yeni) 6.11/6.12 depo düzenini çözemiyor; 6.10.x aqt'nin desteklediği en yeni seri. aqt güncellenince yükseltilebilir.
- **SITL:** `tools/get_sitl.ps1` → Mission Planner'ın prebuilt Windows binary'leri. İki tuzak çözüldü: `.elf` uzantısı `.exe` olarak kaydediliyor (Windows .elf'i çalıştırmıyor) + `Unblock-File` (MOTW engeli). Çalıştırma: `.\ArduCopter.exe --model + --home 39.925533,32.866287,850,0 --defaults copter.parm -I0` → TCP 5760.
- **Python:** her zaman `py` launcher (`python` Store stub'ı). pymavlink + aqtinstall + pillow kurulu.
- **SITL tuzakları (Faz 2'de öğrenildi):** (1) serial1/serial2 TCP listener'ları ancak serial0'a ilk istemci bağlanınca açılır — demo bu yüzden 5760'ı uçuş scriptine, 5762'yi GCS'e verir. (2) GUIDED, boot erken safhasında "kabul edilip" init bitince Stabilize'a geri döner — önce 3D fix bekle, sonra modu HEARTBEAT'ten teyit et, sonra arm. (3) Telemetri stream'leri her port için ayrı ayrı REQUEST_DATA_STREAM ister.
- **MAVLink:** `third_party/mavlink` — c_library_v2 **ardupilotmega** dialect'i (common yetmez: AP'nin kendi mesajları bilinmeyince BAD_CRC sayılıyor + seq istatistiği kayıyor). Include her zaman `core/MavlinkDefs.h` üzerinden (pragma'lı sarmalayıcı).

## Build

```powershell
cmake --preset mingw-debug && cmake --build --preset mingw-debug && ctest --preset mingw-debug
```

## Mimari Kuralları

- Katman bağımlılığı: `ui`/`comm` → `core` → (QtCore + mavlink). `core`'da GUI YOK — testler headless koşar.
- Replay, canlı akışla aynı sinyalleri üretir (`TlogPlayer` → `Vehicle`); UI replay'den habersizdir.
- Mission protokolünde yalnızca *_INT varyantları (float lat/lon hassasiyet kaybettirir).
- GCS kimliği: sysid 255, compid MAV_COMP_ID_MISSIONPLANNER.
- Sınıf envanteri ve veri akışı: `docs/architecture.md`. MAVLink mesaj seti: `docs/mavlink-notes.md`.

## Fazlar

- **Faz 0 ✅** — toolchain, SITL hattı, MavlinkCodec + 4 test, TcpLink, poc_telemetry, CI, dokümanlar
- **Faz 1 ✅** — UdpLink/SerialLink + LinkManager (3 sn auto-reconnect, SITL restart ile doğrulandı), Vehicle modeli (sysid kilidi, mod adları, heartbeat watchdog), ConnectDialog + TelemetryPanel + status bar istatistikleri, gerçek SITL kaydı fixture (`tests/data/sitl_stream.bin`, `tools/record_stream.py`), 4 test hedefi
- **Faz 2 ✅** — PfdWidget (yapay ufuk, pitch merdiveni, roll yayı, hız/irtifa şeritleri; görünür pitch ±35°), CompassWidget, StatusPanel, AlertPanel (banner + severity log), Raw telemetry dock, `--connect`/`--grab` CLI, `tools/demo_flight.py` + `tools/run_demo.ps1`, README'de canlı uçuş GIF'i (docs/img/)
- **Faz 3** — MapWidget + TileCache/TileFetcher (OSM, UA: `KerkenezGCS/1.0 (github.com/conny0506/kerkenez-gcs)`, ≤2 paralel, disk cache → offline); kabul: internet kesik cache'li bölgede çalışır
- **Faz 4** — CommandController (ACK/timeout/retry), guided "buraya git", MissionEditor + MissionController, ParamTable; kabul: haritada çizilen 6+ WP görevi Copter+Plane SITL'de uçar, video
- **Faz 5** — TelemetryRecorder/TlogPlayer, uçuş özeti + CSV; kabul: kayıt kapat-aç replay birebir
- **Faz 6** — EN README cilası, windeployqt release zip, demo video, CV entegrasyonu
- **Stretch** — çoklu araç (-I1 → 5770), SDL2 joystick, geofence

## Kurallar

- Commit'lerde Claude izi / Co-Authored-By YOK (kullanıcı tercihi, kesin kural).
- Kod + commit mesajları EN; dokümanlar EN (README'de TR özet bölümü).
- Her faz sonunda README'ye o fazın GIF/ekran görüntüsü — repo her an gösterilebilir kalır.
- `tools/sitl/`, `build/`, tlog'lar git'e girmez (.gitignore'da).
