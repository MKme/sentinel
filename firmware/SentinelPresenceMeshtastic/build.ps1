$ErrorActionPreference = 'Stop'
$savedEnvironment = @{}
foreach ($name in @('PIP_NO_INDEX','HTTP_PROXY','HTTPS_PROXY','ALL_PROXY','PLATFORMIO_SETTING_ENABLE_TELEMETRY','PLATFORMIO_SETTING_CHECK_PLATFORMIO_INTERVAL','PLATFORMIO_SETTING_CHECK_PRUNE_SYSTEM_THRESHOLD')) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
try {
$env:PIP_NO_INDEX = '1'
$env:HTTP_PROXY = 'http://127.0.0.1:9'
$env:HTTPS_PROXY = 'http://127.0.0.1:9'
$env:ALL_PROXY = 'http://127.0.0.1:9'
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = 'No'
$env:PLATFORMIO_SETTING_CHECK_PLATFORMIO_INTERVAL = '99999'
$env:PLATFORMIO_SETTING_CHECK_PRUNE_SYSTEM_THRESHOLD = '0'
$BuildRoot = Join-Path $PSScriptRoot '.build/meshtastic'
if (-not (Test-Path -LiteralPath $BuildRoot)) {
    python "$PSScriptRoot/prepare.py"
    if ($LASTEXITCODE -ne 0) { throw 'Local preparation failed' }
}
Copy-Item -Path "$PSScriptRoot/overlay/*" -Destination $BuildRoot -Recurse -Force
python "$PSScriptRoot/patch_local_input.py"
if ($LASTEXITCODE -ne 0) { throw 'Local observation ingress hook failed' }
pio run -d $BuildRoot -e sentinel-xiao-s3
if ($LASTEXITCODE -ne 0) { throw 'Firmware build failed; no artifacts exported' }
$Artifacts = Join-Path $PSScriptRoot 'artifacts'
New-Item -ItemType Directory -Force -Path $Artifacts | Out-Null
Get-ChildItem -LiteralPath "$BuildRoot/.pio/build/sentinel-xiao-s3" -File |
    Where-Object { $_.Extension -in '.bin', '.elf', '.json' } |
    Copy-Item -Destination $Artifacts
Get-ChildItem -LiteralPath $Artifacts -File | Where-Object Name -ne 'sha256.json' | Get-FileHash -Algorithm SHA256 |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$Artifacts/sha256.json"
} finally {
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}
