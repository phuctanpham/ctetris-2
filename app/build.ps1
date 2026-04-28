$ErrorActionPreference = "Stop"

# ================================================================
# Resolve script dir va cd ve do, dam bao chay tu bat ky cwd nao
# $PSScriptRoot la built-in cua PowerShell, tro toi thu muc chua .ps1
# ================================================================
$ScriptDir = $PSScriptRoot
if (-not $ScriptDir) { $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path }
Set-Location $ScriptDir

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  ctetris -- Build Script (SDL3 + PWA brandkit)"  -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  Working dir: $ScriptDir"                         -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

$EmsdkVersion = "3.1.72"
$EmsdkDir     = Join-Path $env:USERPROFILE "emsdk"
$PlatformDir  = "windows"
$CiMode       = $env:CI -or $env:GITHUB_ACTIONS

# ----------------------------------------------------------------
# Step 1: nanosvg headers
# ----------------------------------------------------------------
function Ensure-NanoSVG {
    $IncludeDir = "src/gameStory/include"
    $Base       = "https://raw.githubusercontent.com/memononen/nanosvg/master/src"
    if (-not (Test-Path $IncludeDir)) {
        New-Item -ItemType Directory $IncludeDir -Force | Out-Null
    }
    foreach ($f in @("nanosvg.h", "nanosvgrast.h")) {
        $tgt = Join-Path $IncludeDir $f
        if ((-not (Test-Path $tgt)) -or ((Get-Item $tgt).Length -eq 0)) {
            Write-Host "Tai $f ..." -ForegroundColor Yellow
            try { Invoke-WebRequest "$Base/$f" -OutFile $tgt -UseBasicParsing }
            catch { Remove-Item $tgt -Force -EA SilentlyContinue; throw }
        }
    }
}

# ----------------------------------------------------------------
# Step 2: sinh gameStory_logo_svg.h va gameStory_corp_svg.h
# Refactor: tach helper Generate-SvgHeader de tai su dung cho ca
# logo va corp (cung pattern: doc SVG -> embed thanh raw string literal)
# ----------------------------------------------------------------
function Generate-SvgHeader {
    param(
        [string]$SvgFile,
        [string]$HeaderFile,
        [string]$VarName
    )
    if (-not (Test-Path $SvgFile)) { throw "Khong tim thay $SvgFile" }
    # Skip neu header da moi hon SVG (giong make timestamp dependency)
    if ((Test-Path $HeaderFile) -and
        (Get-Item $HeaderFile).LastWriteTime -gt (Get-Item $SvgFile).LastWriteTime) {
        return
    }

    Write-Host "Sinh $HeaderFile ..." -ForegroundColor Yellow
    $content = Get-Content $SvgFile -Raw
    $svgBaseName = Split-Path $SvgFile -Leaf
    $out = @"
#pragma once
// File nay duoc sinh tu dong tu $svgBaseName boi build.ps1
// KHONG sua tay -- moi thay doi se bi ghi de o lan build tiep theo.
static const char* $VarName = R"SVG_RAW_DATA(
$content
)SVG_RAW_DATA";
"@
    [System.IO.File]::WriteAllText(
        (Join-Path (Get-Location) $HeaderFile), $out,
        (New-Object System.Text.UTF8Encoding $false))
}

function Generate-LogoHeader {
    Generate-SvgHeader `
        -SvgFile    "src/gameStory/gameStory_logo.svg" `
        -HeaderFile "src/gameStory/include/gameStory_logo_svg.h" `
        -VarName    "LOGO_SVG_DATA"
    Generate-SvgHeader `
        -SvgFile    "src/gameStory/gameStory_corp.svg" `
        -HeaderFile "src/gameStory/include/gameStory_corp_svg.h" `
        -VarName    "CORP_SVG_DATA"
}

# ----------------------------------------------------------------
# Step 3-4: rasterizer + brandkit
# ----------------------------------------------------------------
function Get-Rasterizer {
    if (Get-Command rsvg-convert -EA SilentlyContinue) { return "rsvg" }
    if (Get-Command magick       -EA SilentlyContinue) { return "magick" }
    if (Get-Command convert      -EA SilentlyContinue) { return "convert" }
    if (Get-Command inkscape     -EA SilentlyContinue) { return "inkscape" }
    return $null
}

function Svg-To-Png {
    param([string]$Tool, [string]$Src, [int]$Size, [string]$Dst)
    switch ($Tool) {
        "rsvg"     { & rsvg-convert -w $Size -h $Size $Src -o $Dst }
        "magick"   { & magick -background none -size "${Size}x${Size}" $Src -resize "${Size}x${Size}" $Dst }
        "convert"  { & convert -background none -size "${Size}x${Size}" $Src -resize "${Size}x${Size}" $Dst }
        "inkscape" { & inkscape -w $Size -h $Size $Src -o $Dst }
    }
}

function Generate-Brandkit {
    param([string]$OutDir)

    $Source = Join-Path $ScriptDir "brandkit/logo.svg"
    if (-not (Test-Path $Source)) { throw "Thieu $Source" }

    $required = @("icon-192.png","icon-512.png","icon-maskable-512.png","favicon.svg","icon.ico")
    $allExist = $true
    foreach ($a in $required) {
        if (-not (Test-Path (Join-Path $OutDir $a))) { $allExist = $false; break }
    }
    if ($allExist) {
        Write-Host "[ICON] Cache hit -- $OutDir, skip." -ForegroundColor Green
        return
    }

    $tool = Get-Rasterizer
    if (-not $tool) {
        Write-Host "[ICON] Khong co rasterizer SVG." -ForegroundColor Yellow
        if (-not $CiMode) {
            $ans = Read-Host "Cai 'rsvg-convert' qua choco? [y/N]"
            if ($ans -notmatch '^[Yy]') { return }
        }
        if (Get-Command choco -EA SilentlyContinue) {
            choco install -y rsvg-convert imagemagick
        } else {
            Write-Host "[LOI] Khong co choco." -ForegroundColor Red; return
        }
        $tool = Get-Rasterizer
        if (-not $tool) { return }
    }

    if (-not (Test-Path $OutDir)) {
        New-Item -ItemType Directory $OutDir -Force | Out-Null
    }
    Write-Host "[ICON] Sinh brandkit vao $OutDir (rasterizer: $tool) ..." -ForegroundColor Cyan

    # Doc viewBox tu logo.svg de tinh scale chinh xac
    $svgRaw = Get-Content $Source -Raw
    $vbX = 0; $vbY = 0; $vbW = 1024; $vbH = 1024
    if ($svgRaw -match 'viewBox="([^"]+)"') {
        $parts = $Matches[1] -split '\s+'
        if ($parts.Count -ge 4) {
            $vbX = [double]$parts[0]; $vbY = [double]$parts[1]
            $vbW = [double]$parts[2]; $vbH = [double]$parts[3]
        }
    } elseif ($svgRaw -match 'width="([0-9.]+)"' -and $Matches[1]) {
        $vbW = [double]$Matches[1]
        if ($svgRaw -match 'height="([0-9.]+)"') { $vbH = [double]$Matches[1] }
    }

    # Trich xuat phan inner cua <svg>...</svg>
    if ($svgRaw -match '(?s)<svg[^>]*>(.*?)</svg>') {
        $logoInner = $Matches[1]
    } else { throw "logo.svg khong co the <svg>" }

    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("ctetris-icons-" + [guid]::NewGuid().Guid)
    New-Item -ItemType Directory $tmp -Force | Out-Null

    function Render-Wrapper {
        param([string]$Bg, [double]$PadRatio, [string]$OutSvg)
        $pad   = 1024.0 * $PadRatio
        $inner = 1024.0 - 2 * $pad
        $maxD  = [Math]::Max($vbW, $vbH)
        $scale = $inner / $maxD
        $scaledW = $vbW * $scale
        $scaledH = $vbH * $scale
        $offX = $pad + ($inner - $scaledW) / 2
        $offY = $pad + ($inner - $scaledH) / 2
        $negX = -1 * $vbX
        $negY = -1 * $vbY
        $wrapper = @"
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"
     viewBox="0 0 1024 1024" width="1024" height="1024">
    <rect width="1024" height="1024" fill="$Bg"/>
    <g transform="translate($offX $offY) scale($scale) translate($negX $negY)">
$logoInner
    </g>
</svg>
"@
        Set-Content -Path $OutSvg -Value $wrapper -Encoding UTF8
    }

    try {
        Render-Wrapper "#ffffff" 0.10 (Join-Path $tmp "standard.svg")
        Render-Wrapper "#ffffff" 0.20 (Join-Path $tmp "maskable.svg")

        Svg-To-Png $tool (Join-Path $tmp "standard.svg") 192  (Join-Path $OutDir "icon-192.png")
        Svg-To-Png $tool (Join-Path $tmp "standard.svg") 512  (Join-Path $OutDir "icon-512.png")
        Svg-To-Png $tool (Join-Path $tmp "standard.svg") 1024 (Join-Path $OutDir "icon-1024.png")
        Svg-To-Png $tool (Join-Path $tmp "maskable.svg") 192  (Join-Path $OutDir "icon-maskable-192.png")
        Svg-To-Png $tool (Join-Path $tmp "maskable.svg") 512  (Join-Path $OutDir "icon-maskable-512.png")
        Svg-To-Png $tool (Join-Path $tmp "standard.svg") 180  (Join-Path $OutDir "apple-touch-icon.png")
        Svg-To-Png $tool (Join-Path $tmp "standard.svg") 32   (Join-Path $OutDir "favicon.png")

        Copy-Item $Source (Join-Path $OutDir "favicon.svg") -Force

        if ($tool -eq "magick" -or $tool -eq "convert") {
            $tmpFiles = @()
            foreach ($s in @(16,32,48,64,128,256)) {
                $f = Join-Path $tmp "_ico_$s.png"
                Svg-To-Png $tool (Join-Path $tmp "standard.svg") $s $f
                $tmpFiles += $f
            }
            & $tool ($tmpFiles + (Join-Path $OutDir "icon.ico"))
        }
    } finally {
        Remove-Item $tmp -Recurse -Force -EA SilentlyContinue
    }
    Write-Host "[ICON] Done." -ForegroundColor Green
}

# ----------------------------------------------------------------
# Step 5: emsdk auto-setup
# ----------------------------------------------------------------
function Import-EmsdkEnv {
    param([string]$EnvBat)
    if (-not (Test-Path $EnvBat)) { return $false }
    $out = & cmd /c "`"$EnvBat`" >nul 2>&1 && set"
    foreach ($l in $out) {
        if ($l -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], "Process")
        }
    }
    return $true
}

function Ensure-Emsdk {
    if (Get-Command emcmake -EA SilentlyContinue) {
        Write-Host "[OK] Emscripten san sang." -ForegroundColor Green
        return
    }
    $envBat = Join-Path $EmsdkDir "emsdk_env.bat"
    if (Test-Path $envBat) {
        Import-EmsdkEnv $envBat | Out-Null
        if (Get-Command emcmake -EA SilentlyContinue) {
            Write-Host "[OK] Emscripten kich hoat." -ForegroundColor Green
            return
        }
    }
    if (-not $CiMode) {
        $ans = Read-Host "Cai emsdk $EmsdkVersion? [y/N]"
        if ($ans -notmatch '^[Yy]') { exit 1 }
    }
    if (-not (Test-Path $EmsdkDir)) {
        if (-not (Get-Command git -EA SilentlyContinue)) { throw "Thieu git" }
        git clone https://github.com/emscripten-core/emsdk.git $EmsdkDir
    }
    Push-Location $EmsdkDir
    & ".\emsdk.bat" install  $EmsdkVersion
    & ".\emsdk.bat" activate $EmsdkVersion
    Pop-Location
    Import-EmsdkEnv $envBat | Out-Null
    if (-not (Get-Command emcmake -EA SilentlyContinue)) { throw "emcmake van thieu" }
    Write-Host "[OK] Emscripten san sang." -ForegroundColor Green
}

# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------
Ensure-NanoSVG
Generate-LogoHeader

if ($CiMode) {
    $Target     = if ($env:TARGET)      { $env:TARGET }      else { "ctetris" }
    $platChoice = if ($env:PLAT_CHOICE) { $env:PLAT_CHOICE } else { "2" }
    Write-Host "[CI] TARGET=$Target PLAT_CHOICE=$platChoice"
} else {
    $tc = Read-Host "Build gi? (1: ctetris, 2: gameStory, 3: gameConsole, 4: gameCore)"
    switch ($tc) {
        '2' { $Target = "gameStory" }
        '3' { $Target = "gameConsole" }
        '4' { $Target = "gameCore" }
        default { $Target = "ctetris" }
    }
    $platChoice = Read-Host "Nen tang? (1: Windows native, 2: WASM)"
}

# WASM branch
if ($platChoice -eq '2') {
    if (-not (Get-Command ninja -EA SilentlyContinue)) {
        throw "Thieu ninja: choco install ninja"
    }
    Ensure-Emsdk
    $BuildDir = Join-Path $ScriptDir "build/wasm/$PlatformDir"
    $IconDir  = Join-Path $BuildDir "icons"
    Generate-Brandkit $IconDir

    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    New-Item -ItemType Directory $BuildDir -Force | Out-Null
    Push-Location $BuildDir
    emcmake cmake -G Ninja -DICON_DIR="$IconDir" -DCMAKE_BUILD_TYPE=Release $ScriptDir
    cmake --build . --target $Target
    Pop-Location

    foreach ($a in @("icon-192.png","icon-512.png","icon-maskable-192.png",
                     "icon-maskable-512.png","apple-touch-icon.png","favicon.svg")) {
        $src = Join-Path $IconDir $a
        if (Test-Path $src) { Copy-Item $src $BuildDir -Force }
    }
    Write-Host "Build WASM xong: $BuildDir" -ForegroundColor Green
    exit
}

# Native Windows branch
if (-not (Get-Command cmake -EA SilentlyContinue)) {
    throw "Thieu cmake: https://cmake.org"
}
$BuildDir = Join-Path $ScriptDir "build/local/$PlatformDir"
$IconDir  = Join-Path $BuildDir "icons"
Generate-Brandkit $IconDir

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory $BuildDir -Force | Out-Null
}
Push-Location $BuildDir
if ($env:VCPKG_ROOT) {
    cmake -DICON_DIR="$IconDir" `
          -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" $ScriptDir
} else {
    cmake -DICON_DIR="$IconDir" $ScriptDir
}
cmake --build . --config Release --target $Target
Pop-Location
Write-Host "Build native xong: $BuildDir/Release/" -ForegroundColor Green