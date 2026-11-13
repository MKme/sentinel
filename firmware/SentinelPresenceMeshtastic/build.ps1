param([switch]$Power,[string]$OutputDirectory)
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
$BuildRoot = Join-Path $PSScriptRoot '.build-ground/meshtastic'
if (-not (Test-Path -LiteralPath $BuildRoot)) {
    python "$PSScriptRoot/prepare.py"
    if ($LASTEXITCODE -ne 0) { throw 'Local preparation failed' }
}
Copy-Item -Path "$PSScriptRoot/overlay/*" -Destination $BuildRoot -Recurse -Force
python "$PSScriptRoot/patch_local_input.py"
if ($LASTEXITCODE -ne 0) { throw 'Local observation ingress hook failed' }
$target = if ($Power) { 'sentinel-ground-power-xiao-s3' } else { 'sentinel-ground-xiao-s3' }
$configPath = Join-Path $BuildRoot 'platformio.ini'
if (-not (Select-String -LiteralPath $configPath -SimpleMatch '[env:sentinel-ground-power-xiao-s3]' -Quiet)) {
@'

[env:sentinel-ground-power-xiao-s3]
extends = env:sentinel-ground-xiao-s3
build_flags =
  ${env:sentinel-ground-xiao-s3.build_flags}
  -DSENTINEL_POWER_ENABLED=1
'@ | Add-Content -LiteralPath $configPath
}
pio run -d $BuildRoot -e $target
if ($LASTEXITCODE -ne 0) { throw 'Firmware build failed; no artifacts exported' }
$Artifacts = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $PSScriptRoot $(if ($Power) { 'artifacts-ground-power' } else { 'artifacts-ground' }) }
New-Item -ItemType Directory -Force -Path $Artifacts | Out-Null
@{ product = 'Sentinel Ground'; target = $target; bootIdentity = 'sentinel-ground-v1' } |
    ConvertTo-Json | Set-Content (Join-Path $Artifacts 'edition.json')
Get-ChildItem -LiteralPath "$BuildRoot/.pio/build/$target" -File |
    Where-Object { $_.Extension -in '.bin', '.elf', '.json' } |
    Copy-Item -Destination $Artifacts
Get-ChildItem -LiteralPath $Artifacts -File | Where-Object Name -ne 'sha256.json' | Get-FileHash -Algorithm SHA256 |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$Artifacts/sha256.json"
} finally {
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}
