# Scripted demo: SITL + autonomous flight (tools/demo_flight.py) + Kerkenez GCS.
# Optionally records GCS window frames (-GrabDir) for README GIFs.
#
# SITL quirk: the serial1/serial2 TCP listeners (5762/5763) are only created
# after a first client connects to serial0 (5760) — boot blocks until then.
# So the flight script owns serial0 (connecting early, which unblocks boot)
# and the GCS watches serial1 (5762).

param(
    [string]$GrabDir = '',
    [int]$GrabFrames = 40,
    [int]$GrabIntervalMs = 600,
    [int]$WatchSeconds = 30,
    [switch]$MapOffline
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$gcsExe = Join-Path $root 'build\mingw-debug\src\app\kerkenez.exe'
$sitlDir = Join-Path $root 'tools\sitl\copter'
$env:PATH = "C:\Qt\6.10.3\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:PATH"

$sitl = Start-Process -FilePath (Join-Path $sitlDir 'ArduCopter.exe') `
    -ArgumentList '--model', '+', '--home', '39.925533,32.866287,850,0', `
    '--defaults', 'copter.parm', '--serial1', 'tcp:2', '-I0' `
    -WorkingDirectory $sitlDir -WindowStyle Hidden -PassThru
Start-Sleep 5

$demoLog = Join-Path $root 'build\demo_flight.log'
Remove-Item $demoLog -ErrorAction SilentlyContinue
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

$gcsArgs = @('--connect', 'tcp:127.0.0.1:5762')
if ($GrabDir) { $gcsArgs += @('--grab', "$GrabDir,$GrabFrames,$GrabIntervalMs") }
if ($MapOffline) { $gcsArgs += '--map-offline' }
$gcs = Start-Process -FilePath $gcsExe -ArgumentList $gcsArgs -PassThru

Write-Host "watching the flight for $WatchSeconds s..."
Start-Sleep $WatchSeconds

foreach ($p in @($gcs, $demo, $sitl)) { try { Stop-Process $p -Force -Confirm:$false } catch {} }
Write-Host '--- flight log:'
Get-Content $demoLog
