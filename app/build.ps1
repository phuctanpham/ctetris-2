$ErrorActionPreference = "Stop"

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  ctetris -- Build Script (SDL3) - Windows"        -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

# --- Tu dong dam bao nanosvg headers (single-header lib cho gameStory) ---
# Kiem tra app/src/gameStory/include co nanosvg.h va nanosvgrast.h chua;
# neu thieu hoac file rong thi tai lai tu GitHub bang Invoke-WebRequest.
function Ensure-NanoSVG {
    $IncludeDir = "src/gameStory/include"
    $BaseUrl    = "https://raw.githubusercontent.com/memononen/nanosvg/master/src"

    # Tao thu muc include neu chua ton tai
    if (-not (Test-Path $IncludeDir)) {
        New-Item -ItemType Directory -Path $IncludeDir -Force | Out-Null
    }

    # Duyet qua tung file can dam bao co mat
    $files = @("nanosvg.h", "nanosvgrast.h")
    foreach ($file in $files) {
        $target = Join-Path $IncludeDir $file
        $needsDownload = $true

        # Chi bo qua download neu file ton tai va co kich thuoc > 0
        if (Test-Path $target) {
            if ((Get-Item $target).Length -gt 0) {
                $needsDownload = $false
            }
        }

        if ($needsDownload) {
            Write-Host "Thieu $file -- dang tai ve $IncludeDir ..." -ForegroundColor Yellow
            try {
                # -UseBasicParsing de tuong thich PowerShell core va Windows PowerShell
                Invoke-WebRequest -Uri "$BaseUrl/$file" -OutFile $target -UseBasicParsing
            } catch {
                Write-Host "[LOI] Khong tai duoc $file. Kiem tra ket noi internet roi thu lai." -ForegroundColor Red
                if (Test-Path $target) { Remove-Item $target -Force }
                exit 1
            }
        }
    }
}

Ensure-NanoSVG

# --- Tu dong sinh gameStory_logo_svg.h tu gameStory_logo.svg ---
# Embed noi dung SVG vao raw string literal de plug-and-play (khong can
# copy file SVG di kem .exe). Chi regenerate khi .svg moi hon .h.
function Generate-LogoHeader {
    $SvgFile    = "src/gameStory/gameStory_logo.svg"
    $HeaderFile = "src/gameStory/include/gameStory_logo_svg.h"

    if (-not (Test-Path $SvgFile)) {
        Write-Host "[LOI] Khong tim thay $SvgFile" -ForegroundColor Red
        exit 1
    }

    # Skip neu header da ton tai va moi hon SVG (giong make dependency)
    if (Test-Path $HeaderFile) {
        $svgTime    = (Get-Item $SvgFile).LastWriteTime
        $headerTime = (Get-Item $HeaderFile).LastWriteTime
        if ($headerTime -gt $svgTime) { return }
    }

    Write-Host "Sinh $HeaderFile tu $SvgFile ..." -ForegroundColor Yellow

    # Doc toan bo noi dung SVG (raw, giu nguyen ky tu xuong dong)
    $svgContent = Get-Content -Path $SvgFile -Raw

    # Dung delimiter SVG_RAW_LOGO de tranh trung lap voi noi dung SVG
    $headerContent = @"
#pragma once
// File nay duoc sinh tu dong tu gameStory_logo.svg boi build.ps1
// KHONG sua tay -- moi thay doi se bi ghi de o lan build tiep theo.
static const char* LOGO_SVG_DATA = R"SVG_RAW_LOGO(
$svgContent
)SVG_RAW_LOGO";
"@

    # Ghi UTF-8 khong BOM (tranh compiler tren cac platform hieu sai)
    $absPath = Join-Path (Get-Location) $HeaderFile
    [System.IO.File]::WriteAllText(
        $absPath,
        $headerContent,
        (New-Object System.Text.UTF8Encoding $false)
    )
}

Generate-LogoHeader

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