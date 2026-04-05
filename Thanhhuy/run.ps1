Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    Write-Error "Không tìm thấy 'pio'. Hãy cài PlatformIO CLI và đảm bảo có trong PATH."
}

Write-Host "[1/2] Upload firmware..."
pio run -t upload

Write-Host "[2/2] Upload filesystem (LittleFS)..."
pio run -t uploadfs

Write-Host "Done. Hãy reset ESP32 và mở lại webserver."
