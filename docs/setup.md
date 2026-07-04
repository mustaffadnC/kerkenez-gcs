# Development Environment Setup (Windows)

Verified on Windows 11, July 2026.

## 1. Qt 6.10.3 (MinGW) + MinGW 13.1 toolchain

Installed with [aqtinstall](https://github.com/miurahr/aqtinstall) (no Qt account needed):

```powershell
py -m pip install aqtinstall pymavlink
py -m aqt install-qt  windows desktop 6.10.3 win64_mingw -m qtserialport qtcharts qtimageformats -O C:\Qt
py -m aqt install-tool windows desktop tools_mingw1310 qt.tools.win64_mingw1310 -O C:\Qt
```

Results:
- Qt: `C:\Qt\6.10.3\mingw_64`
- Compiler: `C:\Qt\Tools\mingw1310_64` (GCC 13.1)

**Why 6.10.3 and not newer:** aqtinstall 3.3.0 (latest as of July 2026) cannot parse
the repository layout of Qt 6.11/6.12 (it looks for `qt6_6120/qt6_6120/Updates.xml`,
which does not exist — the real path is `qt6_6120/qt6_6120_mingw/`). 6.10.x is the
newest series aqt fully supports. The CI action uses aqtinstall internally, so local
and CI versions stay identical. Revisit when aqtinstall ships 6.11+ support.

**Why the Qt-provided MinGW instead of an existing GCC:** the Qt binaries are built
with GCC 13.1; mixing them with another MinGW distribution (e.g. WinLibs GCC 16)
risks `libstdc++`/runtime DLL mismatches. `CMakePresets.json` pins the compiler and
prepends both `bin` directories to `PATH`, so no global PATH changes are needed.

## 2. ArduPilot SITL (no WSL required)

```powershell
powershell -ExecutionPolicy Bypass -File tools\get_sitl.ps1
```

Downloads the prebuilt Windows SITL binaries that Mission Planner uses
(`firmware.ardupilot.org/Tools/MissionPlanner/sitl/`, Stable channels) plus the
cygwin runtime DLLs and `copter.parm` defaults into `tools/sitl/` (git-ignored).

Two Windows quirks the script handles:
1. Upstream names the binaries `*.elf`, but they are PE executables — Windows
   refuses to run the `.elf` extension, so they are saved as `.exe`.
2. Downloaded files carry the Mark-of-the-Web and will not execute until
   unblocked (`Unblock-File`).

Start and verify:

```powershell
cd tools\sitl\copter
.\ArduCopter.exe --model + --home 39.925533,32.866287,850,0 --defaults copter.parm -I0
# serial0 listens on TCP 5760 (+10 per -I instance)

py tools\sitl_smoke.py    # expects a HEARTBEAT within 15 s
```

## 3. Build and test

```powershell
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --preset mingw-debug
```

The presets hard-code the local Qt/MinGW paths above. CI (`.github/workflows/ci.yml`)
uses the same Qt version/arch via `jurplel/install-qt-action`.
