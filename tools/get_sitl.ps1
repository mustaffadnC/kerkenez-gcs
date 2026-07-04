# Downloads ArduPilot SITL Windows binaries (Stable) + cygwin runtime DLLs + default params.
# Source: https://firmware.ardupilot.org/Tools/MissionPlanner/sitl/
# Usage:  powershell -ExecutionPolicy Bypass -File tools\get_sitl.ps1 [-Force]

param([switch]$Force)

$ErrorActionPreference = 'Stop'
$base = 'https://firmware.ardupilot.org/Tools/MissionPlanner/sitl'
$paramBase = 'https://raw.githubusercontent.com/ArduPilot/ardupilot/master/Tools/autotest/default_params'
$root = Join-Path $PSScriptRoot 'sitl'

$dlls = @(
    'cygwin1.dll', 'cygstdc++-6.dll', 'cyggcc_s-seh-1.dll', 'cygatomic-1.dll',
    'cyggomp-1.dll', 'cygiconv-2.dll', 'cygintl-8.dll', 'cygquadmath-0.dll', 'cygssp-0.dll'
)

# Notes:
# - default_params has no plain "plane.parm" — the base plane model needs no
#   defaults file; only variants (elevons, vtail, ...) have one.
# - Upstream names the binaries *.elf but they are Windows PE executables
#   (cygwin builds). Windows refuses to execute the .elf extension, so we save
#   them as .exe locally.
$vehicles = @(
    @{ Name = 'copter'; Folder = 'CopterStable'; Elf = 'ArduCopter.elf'; Exe = 'ArduCopter.exe'; Parm = 'copter.parm' },
    @{ Name = 'plane';  Folder = 'PlaneStable';  Elf = 'ArduPlane.elf';  Exe = 'ArduPlane.exe';  Parm = $null }
)

function Get-File($url, $dest) {
    if ((Test-Path $dest) -and -not $Force) {
        Write-Host "  [skip] $(Split-Path $dest -Leaf) (already exists)"
        return
    }
    Write-Host "  [get ] $url"
    Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing
}

foreach ($v in $vehicles) {
    $dir = Join-Path $root $v.Name
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    Write-Host "== $($v.Name) → $dir"

    Get-File "$base/$($v.Folder)/$($v.Elf)" (Join-Path $dir $v.Exe)
    if ($v.Parm) {
        Get-File "$paramBase/$($v.Parm)" (Join-Path $dir $v.Parm)
    }

    foreach ($dll in $dlls) {
        $dest = Join-Path $dir $dll
        try {
            Get-File "$base/$($v.Folder)/$dll" $dest
        } catch {
            # some DLLs only exist at the repository root listing
            Get-File "$base/$dll" $dest
        }
    }
}

# Downloaded files carry the Mark-of-the-Web which blocks execution — clear it.
Get-ChildItem $root -Recurse -File | Unblock-File

Write-Host ''
Write-Host 'Done. Start SITL with e.g.:'
Write-Host '  cd tools\sitl\copter'
Write-Host '  .\ArduCopter.exe --model + --home 39.925533,32.866287,850,0 --defaults copter.parm -I0'
Write-Host '  (serial0 listens on TCP 5760; verify with: py tools\sitl_smoke.py)'
