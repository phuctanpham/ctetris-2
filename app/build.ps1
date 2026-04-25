$ErrorActionPreference = "Stop"

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  ctetris -- Build Script (SDL3) - Windows"        -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

# --- Hoi target ---
$targetChoice = Read-Host "Ban muon build gi? (1: ctetris, 2: gameStory, 3: gameConsole, 4: gameCore)"
switch ($targetChoice) {
    '2' { $Target = "gameStory" }
    '3' { $Target = "gameConsole" }
    '4' { $Target = "gameCore" }
    default { $Target = "ctetris" }
}

# --- Hoi nen tang ---
$platChoice = Read-Host "Chon nen tang (1: Windows, 2: WASM)"

# Ham kiem tra cong cu
function Require-Tool($name, $hint) {
    if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
        Write-Host "[LOI] Thieu cong cu '$name'. $hint" -ForegroundColor Red
        exit 1
    }
}

# =========== Nhanh WASM ===========
if ($platChoice -eq '2') {
    Write-Host "Kiem tra moi truong WASM..." -ForegroundColor Yellow
    Require-Tool "emcmake" "Cai Emscripten SDK va activate emsdk truoc khi chay lai."
    Require-Tool "ninja"   "Cai Ninja: choco install ninja"

    $BuildDir = "build/wasm"
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
    Push-Location $BuildDir

    Write-Host "Cau hinh CMake voi emcmake..." -ForegroundColor Yellow
    emcmake cmake -G Ninja ../..

    Write-Host "Bien dich target: $Target ..." -ForegroundColor Yellow
    cmake --build . --target $Target

    Pop-Location
    Write-Host ""
    Write-Host "Build WASM thanh cong tai $BuildDir." -ForegroundColor Green
    Write-Host "Cach chay tren localhost:"          -ForegroundColor Green
    Write-Host "  cd $BuildDir; python -m http.server 8000" -ForegroundColor Green
    Write-Host "  Mo: http://localhost:8000/$Target.html"   -ForegroundColor Green
    exit
}

# =========== Nhanh Windows native ===========
Require-Tool "cmake" "Cai CMake >=3.16 tu https://cmake.org/download/"

# Goi y vcpkg neu chua co SDL3
if (-not $env:VCPKG_ROOT -and -not $env:CMAKE_PREFIX_PATH) {
    Write-Host "[CANH BAO] Khong tim thay VCPKG_ROOT hoac CMAKE_PREFIX_PATH cho SDL3." -ForegroundColor Yellow
    Write-Host "Cach 1: vcpkg install sdl3 va set `$env:VCPKG_ROOT"                    -ForegroundColor Yellow
    Write-Host "Cach 2: Tai SDL3 binary va set `$env:CMAKE_PREFIX_PATH chi den thu muc giai nen" -ForegroundColor Yellow
}

$BuildDir = "build/local"
if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
Push-Location $BuildDir

Write-Host "Cau hinh CMake cho Windows..." -ForegroundColor Yellow
if ($env:VCPKG_ROOT) {
    cmake ../.. -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
} else {
    cmake ../..
}

Write-Host "Bien dich..." -ForegroundColor Yellow
cmake --build . --config Release --target $Target

Pop-Location
Write-Host ""
Write-Host "Build thanh cong! File trong $BuildDir/Release/" -ForegroundColor Green