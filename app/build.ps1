$ErrorActionPreference = "Stop"

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  ctetris — Build Script (SDL3 Native) - Windows" -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

$targetChoice = Read-Host "Ban muon build gi? (1: Tat ca, 2: gameStory, 3: gameConsole, 4: gameCore)"
switch ($targetChoice) {
    '2' { $Target = "gameStory" }
    '3' { $Target = "gameConsole" }
    '4' { $Target = "gameCore" }
    default { $Target = "ctetris" }
}

$platChoice = Read-Host "Chon nen tang (1: Windows, 2: WASM)"

if ($platChoice -eq '2') {
    $BuildDir = "build/wasm"
    if (-not (Test-Path -Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
    Set-Location -Path $BuildDir
    Write-Host "Tien hanh cau hinh Emscripten WASM..." -ForegroundColor Yellow
    emcmake cmake ../..
    cmake --build . --target $Target
    Write-Host "Build thanh cong! Dung live-server de test nhe." -ForegroundColor Green
    exit
}

$BuildDir = "build/local"
if (-not (Test-Path -Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
Set-Location -Path $BuildDir

Write-Host "Tien hanh cau hinh CMake cho Windows..." -ForegroundColor Yellow
cmake ../..

Write-Host "Tien hanh bien dich ma nguon..." -ForegroundColor Yellow
cmake --build . --config Release --target $Target

Write-Host "Build thanh cong! File nam trong $BuildDir/Release/" -ForegroundColor Green