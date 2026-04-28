# =============================================================================
# build.ps1 -- PowerShell build script cho cTetris (Windows)
# =============================================================================
# THAY DOI v1:
#   - Drop hoan toan pipeline rasterize SVG -> PNG -> ICO cho desktop.
#     Windows .exe khong con embed icon binary nua, OS dung icon mac dinh.
#   - WASM build chi can mot file favicon.svg duy nhat (SVG-driven), khong
#     can sinh array PNG nhieu kich thuoc va khong can manifest PWA.
#   - Cac function Generate-Brandkit / ConvertTo-PngFromSvg da bi loai bo.
#     Chi giu lai New-SvgHeader (embed SVG goc thanh C string header).
# =============================================================================

[CmdletBinding()]
param(
    # Build mode: native (Windows exe) / wasm / all / clean
    [ValidateSet('native','wasm','all','clean')]
    [string]$Mode = 'native'
)

$ErrorActionPreference = 'Stop'  # Bao loi ngay khi co lenh fail

# -----------------------------------------------------------------------------
# Cau hinh duong dan
# -----------------------------------------------------------------------------
$AppDir       = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir     = Join-Path $AppDir 'build'
$ThirdPartyDir= Join-Path $AppDir 'thirdparty'
$EmsdkDir     = Join-Path $AppDir '.emsdk'
$BrandkitDir  = Join-Path $AppDir 'brandkit'
$BrandLogoSvg = Join-Path $BrandkitDir 'logo.svg'

# -----------------------------------------------------------------------------
# Helper log
# -----------------------------------------------------------------------------
function Write-Info  ($msg) { Write-Host "[INFO]  $msg" -ForegroundColor Cyan }
function Write-Warn  ($msg) { Write-Host "[WARN]  $msg" -ForegroundColor Yellow }
function Write-Err   ($msg) { Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Ok    ($msg) { Write-Host "[OK]    $msg" -ForegroundColor Green }

# -----------------------------------------------------------------------------
# Tai header nanosvg neu chua co
# -----------------------------------------------------------------------------
function Test-Nanosvg {
    $includeDir = Join-Path $AppDir 'src\gameStory\include'
    $nano       = Join-Path $includeDir 'nanosvg.h'
    $nanoRast   = Join-Path $includeDir 'nanosvgrast.h'

    if ((Test-Path $nano) -and (Test-Path $nanoRast)) {
        Write-Ok 'nanosvg headers da ton tai, bo qua download'
        return
    }

    Write-Info 'Tai nanosvg headers...'
    New-Item -ItemType Directory -Force -Path $includeDir | Out-Null
    $base = 'https://raw.githubusercontent.com/memononen/nanosvg/master/src'
    Invoke-WebRequest -Uri "$base/nanosvg.h"     -OutFile $nano
    Invoke-WebRequest -Uri "$base/nanosvgrast.h" -OutFile $nanoRast
    Write-Ok 'nanosvg headers da san sang'
}

# -----------------------------------------------------------------------------
# Sinh header C tu file SVG goc, chua trong raw string literal R"SVG_RAW_DATA(...)"
# -----------------------------------------------------------------------------
function New-SvgHeader {
    param(
        [string]$SvgPath,
        [string]$HeaderPath,
        [string]$VarName
    )
    if (-not (Test-Path $SvgPath)) {
        Write-Err "Khong tim thay SVG nguon: $SvgPath"
        return
    }

    $headerDir = Split-Path -Parent $HeaderPath
    New-Item -ItemType Directory -Force -Path $headerDir | Out-Null

    $svgContent = Get-Content -Raw $SvgPath
    $svgFile    = Split-Path -Leaf $SvgPath

    $sb = [System.Text.StringBuilder]::new()
    [void]$sb.AppendLine('#pragma once')
    [void]$sb.AppendLine("// File nay duoc sinh tu dong tu $svgFile boi build.ps1")
    [void]$sb.AppendLine('// KHONG sua tay -- moi thay doi se bi ghi de o lan build tiep theo.')
    [void]$sb.AppendLine("static const char* $VarName = R`"SVG_RAW_DATA(")
    [void]$sb.AppendLine($svgContent.TrimEnd())
    [void]$sb.AppendLine(')SVG_RAW_DATA";')

    Set-Content -Path $HeaderPath -Value $sb.ToString() -NoNewline
    Write-Ok "Sinh header: $(Split-Path -Leaf $HeaderPath) (tu $svgFile)"
}

# -----------------------------------------------------------------------------
# Sinh ca 2 header SVG (logo cua game + logo UIT)
# -----------------------------------------------------------------------------
function New-LogoHeaders {
    $storyDir   = Join-Path $AppDir 'src\gameStory'
    $includeDir = Join-Path $storyDir 'include'

    # Logo cua GAME cTetris
    $gameLogo = Join-Path $storyDir 'gameStory_logo.svg'
    if (Test-Path $gameLogo) {
        New-SvgHeader -SvgPath $gameLogo `
                      -HeaderPath (Join-Path $includeDir 'gameStory_logo_svg.h') `
                      -VarName 'LOGO_SVG_DATA'
    } else {
        Write-Warn 'Khong co gameStory_logo.svg, bo qua sinh header'
    }

    # Logo UIT (corp credit)
    $corpLogo = Join-Path $storyDir 'gameStory_corp.svg'
    if (Test-Path $corpLogo) {
        New-SvgHeader -SvgPath $corpLogo `
                      -HeaderPath (Join-Path $includeDir 'gameStory_corp_svg.h') `
                      -VarName 'CORP_SVG_DATA'
    } else {
        Write-Warn 'Khong co gameStory_corp.svg, bo qua sinh header'
    }
}

# -----------------------------------------------------------------------------
# Cai dat emsdk cho build WASM
# -----------------------------------------------------------------------------
function Initialize-Emsdk {
    if ((Test-Path $EmsdkDir) -and (Test-Path (Join-Path $EmsdkDir 'emsdk.bat'))) {
        Write-Ok 'emsdk da ton tai, kich hoat moi truong'
    } else {
        Write-Info 'Cai emsdk lan dau (mat vai phut)...'
        git clone https://github.com/emscripten-core/emsdk.git $EmsdkDir
        Push-Location $EmsdkDir
        & .\emsdk.bat install 3.1.72
        & .\emsdk.bat activate 3.1.72
        Pop-Location
    }
    # Goi script kich hoat moi truong (set PATH, EMSCRIPTEN_ROOT...)
    & (Join-Path $EmsdkDir 'emsdk_env.ps1')
    Write-Ok 'emsdk active'
}

# -----------------------------------------------------------------------------
# Build native (Windows .exe). Yeu cau co Visual Studio / MSVC + CMake.
# SDL3 phai duoc cai san o thidparty hoac thong qua vcpkg.
# -----------------------------------------------------------------------------
function Build-Native {
    Write-Info 'Build NATIVE mode (Windows .exe)'
    Test-Nanosvg
    New-LogoHeaders

    $nativeBuild = Join-Path $BuildDir 'native'
    New-Item -ItemType Directory -Force -Path $nativeBuild | Out-Null

    cmake -S $AppDir -B $nativeBuild `
          -DCMAKE_BUILD_TYPE=Release `
          -DBUILD_WASM=OFF
    cmake --build $nativeBuild --config Release -j

    Write-Ok "Native build hoan tat: $nativeBuild"
}

# -----------------------------------------------------------------------------
# Build WASM cho web. Sinh ra cTetris.html / .js / .wasm + favicon.svg
# -----------------------------------------------------------------------------
function Build-Wasm {
    Write-Info 'Build WASM mode'
    Initialize-Emsdk
    Test-Nanosvg
    New-LogoHeaders

    $wasmBuild = Join-Path $BuildDir 'wasm'
    New-Item -ItemType Directory -Force -Path $wasmBuild | Out-Null

    emcmake cmake -S $AppDir -B $wasmBuild `
                  -DCMAKE_BUILD_TYPE=Release `
                  -DBUILD_WASM=ON
    cmake --build $wasmBuild -j

    # Copy SVG goc lam favicon (SVG-driven, khong rasterize ra PNG)
    if (Test-Path $BrandLogoSvg) {
        Copy-Item -Force $BrandLogoSvg (Join-Path $wasmBuild 'favicon.svg')
        Write-Ok 'Copy favicon.svg (SVG-driven, khong PNG)'
    } else {
        Write-Warn 'Khong co brandkit\logo.svg cho favicon'
    }

    Write-Ok "WASM build hoan tat: $wasmBuild"
    Write-Info "Mo browser: cd $wasmBuild; python -m http.server 8080"
}

# -----------------------------------------------------------------------------
# Entry point: dispatch theo Mode
# -----------------------------------------------------------------------------
switch ($Mode) {
    'native' { Build-Native }
    'wasm'   { Build-Wasm }
    'all'    { Build-Native; Build-Wasm }
    'clean'  {
        Write-Info 'Don dep build artifacts...'
        if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
        Write-Ok "Da xoa $BuildDir"
    }
}
