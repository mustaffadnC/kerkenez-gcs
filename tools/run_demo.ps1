# Runs SITL together with the ground station for a scripted demo.
#
#   -Mission   the GCS itself builds, uploads and flies a mission
#              (--demo-mission); nothing else talks to the autopilot
#   default    tools/demo_flight.py flies via pymavlink while the GCS watches
#
# SITL quirk: the serial1/serial2 TCP listeners (5762/5763) only appear after a
# first client connects to serial0 (5760) — boot blocks until then. In the
# default mode the flight script owns serial0 and the GCS watches serial1; with
# -Mission the GCS owns serial0 on its own.

param(
    [switch]$Mission,
    [string]$GrabDir = '',
    [int]$GrabFrames = 40,
    [int]$GrabIntervalMs = 600,
    [int]$WatchSeconds = 0,
    [switch]$MapOffline
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$gcsExe = Join-Path $root 'build\mingw-debug\src\app\kerkenez.exe'
$sitlDir = Join-Path $root 'tools\sitl\copter'
$env:PATH = "C:\Qt\6.10.3\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:PATH"
if ($WatchSeconds -le 0) { $WatchSeconds = if ($Mission) { 210 } else { 30 } }

$sitl = Start-Process -FilePath (Join-Path $sitlDir 'ArduCopter.exe') `
    -ArgumentList '--model', '+', '--home', '39.925533,32.866287,850,0', `
    '--defaults', 'copter.parm', '--serial1', 'tcp:2', '-I0' `
    -WorkingDirectory $sitlDir -WindowStyle Hidden -PassThru
Start-Sleep 5

$gcsArgs = @()
$demo = $null
$demoLog = Join-Path $root 'build\demo_flight.log'
$gcsLog = Join-Path $root 'build\gcs.log'
Remove-Item $demoLog, $gcsLog -ErrorAction SilentlyContinue

if ($Mission) {
    $gcsArgs += @('--connect', 'tcp:127.0.0.1:5760', '--demo-mission')
} else {
    $demo = Start-Process -FilePath 'py' `
        -ArgumentList (Join-Path $root 'tools\demo_flight.py'), 'tcp:127.0.0.1:5760' `
        -RedirectStandardOutput $demoLog -WindowStyle Hidden -PassThru

    Write-Host 'waiting until airborne (EKF needs ~40 s)...'
    $deadline = (Get-Date).AddSeconds(240)
    $airborne = $false
    while ((Get-Date) -lt $deadline) {
        if ((Test-Path $demoLog) -and (Select-String -Path $demoLog -Pattern 'AIRBORNE' -Quiet)) {
            $airborne = $true
            break
        }
        if ($demo.HasExited) { break }
        Start-Sleep 2
    }
    if (-not $airborne) {
        Get-Content $demoLog -ErrorAction SilentlyContinue
        foreach ($p in @($demo, $sitl)) { try { Stop-Process $p -Force -Confirm:$false } catch {} }
        throw 'flight did not get airborne'
    }
    $gcsArgs += @('--connect', 'tcp:127.0.0.1:5762')
}

if ($GrabDir) { $gcsArgs += @('--grab', "$GrabDir,$GrabFrames,$GrabIntervalMs") }
if ($MapOffline) { $gcsArgs += '--map-offline' }

$gcs = Start-Process -FilePath $gcsExe -ArgumentList $gcsArgs -PassThru `
    -RedirectStandardError $gcsLog

Write-Host "watching for $WatchSeconds s..."
Start-Sleep $WatchSeconds

foreach ($p in @($gcs, $demo, $sitl)) {
    if ($p) { try { Stop-Process $p -Force -Confirm:$false } catch {} }
}

if ($Mission) {
    Write-Host '--- ground station demo log:'
    Get-Content (Join-Path $root 'demo-mission.log') -ErrorAction SilentlyContinue
} else {
    Write-Host '--- flight log:'
    Get-Content $demoLog
}
