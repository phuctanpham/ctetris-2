# =============================================================================
# build.ps1 -- PowerShell build script cho cTetris (Windows)
# =============================================================================
# THAY DOI v3 (validation + os-driven paths):
#   - OS_NAME = "windows" -- moi clone & download vao app\libs\windows\downloads\
#   - Build artifact: app\build\desktop\windows\ (native), app\build\wasm\windows\ (WASM)
#   - Validation truoc khi cai dat:
#       1. Check em++/cmake da o PATH chua + version >= min
#       2. Check ~\emsdk (user install)
#       3. Check libs\windows\downloads\emsdk\ (managed)
#       4. Cuoi cung moi clone moi
#   - Uu tien winget hoac choco truoc khi download/clone thu cong.
#   - KHONG sinh file co content cung tu script -- file *_svg.h va *_layout.h
#     phai duoc commit san trong repo.
# =============================================================================

[CmdletBinding()]
param(
    [ValidateSet('native','wasm','all','clean','deepclean')]
    [string]$Mode = 'native'
)

$ErrorActionPreference = 'Stop'

# -----------------------------------------------------------------------------
# Paths -- Windows luon co OS_NAME = "windows"
# -----------------------------------------------------------------------------
$OS_NAME       = 'windows'
$AppDir        = Split-Path -Parent $MyInvocation.MyCommand.Path
$LibsDir       = Join-Path $AppDir "libs\$OS_NAME"
$DownloadDir   = Join-Path $LibsDir 'downloads'
$BuildNativeDir= Join-Path $AppDir "build\desktop\$OS_NAME"
$BuildWasmDir  = Join-Path $AppDir "build\wasm\$OS_NAME"
$BrandkitDir   = Join-Path $AppDir 'brandkit'
$BrandLogoSvg  = Join-Path $BrandkitDir 'logo.svg'
$WebDir        = Join-Path $AppDir 'web'

# Version pinning
$CmakeMinVersion   = '3.16'
$EmsdkVersion      = '3.1.72'
$Sdl3VersionMin    = '3.2.0'
$Sdl3Version       = '3.2.18'   # default pin
[cite_start]$NlohmannVersion   = '3.11.3'   # 
$DetectedSdl3Version = ''        

# -----------------------------------------------------------------------------
# Logging
# -----------------------------------------------------------------------------
function Write-Info ($msg) { Write-Host "[INFO]  $msg" -ForegroundColor Cyan }
function Write-Warn ($msg) { Write-Host "[WARN]  $msg" -ForegroundColor Yellow }
function Write-Err  ($msg) { Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Ok   ($msg) { Write-Host "[OK]    $msg" -ForegroundColor Green }

Write-Info "OS detected: $OS_NAME"
Write-Info "Downloads dir: $DownloadDir"

# -----------------------------------------------------------------------------
# Version helpers
# -----------------------------------------------------------------------------
function Test-VersionGE {
    param([string]$A, [string]$B)
    if ([string]::IsNullOrEmpty($B)) { return $true }
    function _norm($v) {
        $parts = $v.Split('.')
        while ($parts.Count -lt 3) { $parts += '0' }
        return ($parts[0..2] -join '.')
    }
    try { return [version](_norm $A) -ge [version](_norm $B) } catch { return $true }
}

function Get-CommandVersion {
    param([string]$Cmd)
    try {
        $out = & $Cmd --version 2>&1 | Select-Object -First 1
        if ($out -match '(\d+\.\d+(?:\.\d+)?)') { return $Matches[1] }
    } catch {}
    return $null
}

function Test-CommandVersion {
    param([string]$Cmd, [string]$MinVersion)
    if (-not (Get-Command $Cmd -ErrorAction SilentlyContinue)) { return $false }
    $cur = Get-CommandVersion $Cmd
    if (-not $cur) { return $true }
    if (Test-VersionGE $cur $MinVersion) { return $true }
    Write-Warn "$Cmd version $cur < $MinVersion"
    return $false
}

# =============================================================================
# nanosvg
# =============================================================================
function Initialize-Nanosvg {
    $vendored = Join-Path $AppDir 'src\gameStory\include\nanosvg.h'
    if (Test-Path $vendored) {
        Write-Ok 'nanosvg da co trong source tree (vendored)'
        return
    }

    $nanoDir   = Join-Path $DownloadDir 'nanosvg'
    $nano      = Join-Path $nanoDir 'nanosvg.h'
    $nanoRast  = Join-Path $nanoDir 'nanosvgrast.h'

    if ((Test-Path $nano) -and (Test-Path $nanoRast)) {
        Write-Ok "nanosvg da co tai $nanoDir"
        return
    }

    Write-Info "Tai nanosvg headers vao $nanoDir..."
    New-Item -ItemType Directory -Force -Path $nanoDir | Out-Null
    $base = 'https://raw.githubusercontent.com/memononen/nanosvg/master/src'
    Invoke-WebRequest -Uri "$base/nanosvg.h"     -OutFile $nano
    Invoke-WebRequest -Uri "$base/nanosvgrast.h" -OutFile $nanoRast
    Write-Ok "nanosvg san sang tai $nanoDir"
}

# =============================================================================
# nlohmann/json 
# =============================================================================
function Initialize-Nlohmann {
    $vendored = Join-Path $AppDir 'src\gameStory\include\nlohmann\json.hpp'
    if (Test-Path $vendored) {
        Write-Ok 'nlohmann/json da co trong source tree (vendored)'
        return
    }

    $nRoot = Join-Path $DownloadDir 'nlohmann'
    $nFile = Join-Path $nRoot 'nlohmann\json.hpp'

    if (Test-Path $nFile) {
        Write-Ok "nlohmann/json da co tai $nFile"
        return
    }

    Write-Info "Tai nlohmann/json $NlohmannVersion vao $nFile..."
    New-Item -ItemType Directory -Force -Path (Join-Path $nRoot 'nlohmann') | Out-Null
    $url = "https://raw.githubusercontent.com/nlohmann/json/v$NlohmannVersion/single_include/nlohmann/json.hpp"
    Invoke-WebRequest -Uri $url -OutFile $nFile

    $size = (Get-Item $nFile).Length
    if ($size -lt 102400) {
        Write-Err "nlohmann/json.hpp tai ve nho bat thuong ($size bytes) -- xoa va abort"
        Remove-Item -Force $nFile
        throw "nlohmann download truncated"
    }
    Write-Ok "nlohmann/json $NlohmannVersion san sang tai $nFile ($size bytes)"
}

# =============================================================================
# emsdk helper
# =============================================================================
function Import-EmsdkEnv {
    param([string]$EmsdkRoot)
    $bat = Join-Path $EmsdkRoot 'emsdk_env.bat'
    if (-not (Test-Path $bat)) {
        Write-Warn "Khong tim thay $bat -- bo qua source env"
        return
    }
    Write-Info "Source emsdk env tu $bat..."
    cmd /c "call `"$bat`" > nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
        }
    }
}

function Initialize-Emsdk {
    if (Get-Command em++ -ErrorAction SilentlyContinue) {
        $cur = (em++ --version 2>$null | Select-Object -First 1) `
                -replace '.*?(\d+\.\d+\.\d+).*','$1'
        if ((Test-VersionGE $cur $EmsdkVersion)) {
            Write-Ok "em++ tren PATH version $cur >= $EmsdkVersion (skip)"
            return
        }
    }

    $userEmsdk = Join-Path $env:USERPROFILE 'emsdk'
    if (Test-Path (Join-Path $userEmsdk 'emsdk_env.bat')) {
        Write-Info "Phat hien $userEmsdk -- dung emsdk cua user"
        Import-EmsdkEnv $userEmsdk
        return
    }

    $managed = Join-Path $DownloadDir 'emsdk'
    if (Test-Path (Join-Path $managed 'emsdk_env.bat')) {
        Write-Info "Phat hien managed emsdk tai $managed"
        Import-EmsdkEnv $managed
        return
    }

    Write-Info "Khong tim thay emsdk, clone vao $managed..."
    New-Item -ItemType Directory -Force -Path $DownloadDir | Out-Null
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git $managed
    Push-Location $managed
    try {
        & .\emsdk.bat install $EmsdkVersion
        & .\emsdk.bat activate $EmsdkVersion
    } finally { Pop-Location }
    Import-EmsdkEnv $managed
}

# =============================================================================
# SDL3 
# =============================================================================
function Initialize-Sdl3Native {
    Write-Info 'Checking SDL3 cho native Windows...'
    if (Get-Command pkg-config -ErrorAction SilentlyContinue) {
        & pkg-config --exists sdl3 2>$null
        if ($LASTEXITCODE -eq 0) {
            $v = & pkg-config --modversion sdl3
            $script:DetectedSdl3Version = $v
            Write-Ok "Found via pkg-config (version $v) -- skip install"
            return
        }
    }
    $script:DetectedSdl3Version = $Sdl3Version
    Build-Sdl3FromSource -InstallPrefix (Join-Path $DownloadDir 'sdl3-native') -Target 'native' -Version $Sdl3Version
}

function Initialize-Sdl3Wasm {
    $targetVersion = if ($script:DetectedSdl3Version) { $script:DetectedSdl3Version } else { $Sdl3Version }
    $installDir = Join-Path $DownloadDir "sdl3-wasm-$targetVersion"
    if (Test-Path (Join-Path $installDir 'lib\cmake\SDL3')) {
        Write-Ok "SDL3 WASM cache HIT version $targetVersion"
        return
    }
    Build-Sdl3FromSource -InstallPrefix $installDir -Target 'wasm' -Version $targetVersion
}

function Build-Sdl3FromSource {
    param([string]$InstallPrefix, [string]$Target, [string]$Version)
    $sdlSrc   = Join-Path $DownloadDir "SDL-$Version"
    $sdlBuild = Join-Path $sdlSrc "build-$Target"

    if (-not (Test-Path $sdlSrc)) {
        git clone --depth 1 --branch "release-$Version" https://github.com/libsdl-org/SDL $sdlSrc
    }

    $cfg = @('-S', $sdlSrc, '-B', $sdlBuild, '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_INSTALL_PREFIX=$InstallPrefix")
    if ($Target -eq 'wasm') {
        $cfg += @('-DSDL_SHARED=OFF', '-DSDL_STATIC=ON', '-G', 'Ninja')
        & emcmake cmake @cfg
    } else {
        $cfg += @('-DSDL_SHARED=ON')
        & cmake @cfg
    }
    & cmake --build $sdlBuild --config Release -j
    & cmake --install $sdlBuild --config Release
}

# =============================================================================
# Assets Helpers
# =============================================================================
function Copy-DesktopIcon {
    param([string]$OutDir)
    $icoSrc = Join-Path $BrandkitDir 'logo.ico'
    $icoDst = Join-Path $OutDir 'cTetris.ico'
    if (Test-Path $icoSrc) { Copy-Item -Force $icoSrc $icoDst }
}

function Copy-PwaAssets {
    param([string]$OutDir)
    foreach ($asset in @('manifest.webmanifest', 'sw.js')) {
        $src = Join-Path $WebDir $asset
        if (Test-Path $src) { Copy-Item -Force $src (Join-Path $OutDir $asset) }
    }
}

# =============================================================================
# Main Build Logic
# =============================================================================
function Build-Native {
    Write-Info "Build NATIVE -> $BuildNativeDir"
    Initialize-Sdl3Native
    Initialize-Nanosvg
    [cite_start]Initialize-Nlohmann  # 
    
    Copy-DesktopIcon -OutDir $BuildNativeDir

    $sdlInstall = Join-Path $DownloadDir 'sdl3-native'
    $sdlDirArgs = @()
    foreach ($cand in @((Join-Path $sdlInstall 'lib\cmake\SDL3'), (Join-Path $sdlInstall 'lib64\cmake\SDL3'))) {
        if (Test-Path (Join-Path $cand 'SDL3Config.cmake')) { $sdlDirArgs = @("-DSDL3_DIR=$cand"); break }
    }

    New-Item -ItemType Directory -Force -Path $BuildNativeDir | Out-Null
    $nativeArgs = @(
        '-S', $AppDir, '-B', $BuildNativeDir,
        '-DCMAKE_BUILD_TYPE=Release',
        '-DBUILD_WASM=OFF',
        "-DCMAKE_PREFIX_PATH=$sdlInstall",
        "-DNANOSVG_INCLUDE_DIR=$(Join-Path $DownloadDir 'nanosvg')",
        [cite_start]"-DNLOHMANN_INCLUDE_DIR=$(Join-Path $DownloadDir 'nlohmann')" # [cite: 10]
    ) + $sdlDirArgs
    
    & cmake @nativeArgs
    & cmake --build $BuildNativeDir --config Release -j
    
    # Copy board JSON 
    $jsonSrc = Join-Path $AppDir 'src\gameConsole\gameConsole_board.json'
    if (Test-Path $jsonSrc) {
        Copy-Item -Force $jsonSrc (Join-Path $BuildNativeDir 'gameConsole_board.json')
        Write-Ok "Copy gameConsole_board.json -> $BuildNativeDir"
    }
}

function Build-Wasm {
    Write-Info "Build WASM -> $BuildWasmDir"
    try { Initialize-Sdl3Native } catch { }
    Initialize-Emsdk
    Initialize-Sdl3Wasm
    Initialize-Nanosvg
    [cite_start]Initialize-Nlohmann  # 

    $targetVersion = if ($script:DetectedSdl3Version) { $script:DetectedSdl3Version } else { $Sdl3Version }
    $sdlInstall = Join-Path $DownloadDir "sdl3-wasm-$targetVersion"

    New-Item -ItemType Directory -Force -Path $BuildWasmDir | Out-Null
    $wasmArgs = @(
        '-S', $AppDir, '-B', $BuildWasmDir,
        '-DCMAKE_BUILD_TYPE=Release',
        '-DBUILD_WASM=ON',
        '-G', 'Ninja',
        "-DCMAKE_PREFIX_PATH=$sdlInstall",
        "-DNANOSVG_INCLUDE_DIR=$(Join-Path $DownloadDir 'nanosvg')",
        [cite_start]"-DNLOHMANN_INCLUDE_DIR=$(Join-Path $DownloadDir 'nlohmann')" # [cite: 11]
    )
    & emcmake cmake @wasmArgs
    & cmake --build $BuildWasmDir --parallel

    Copy-PwaAssets -OutDir $BuildWasmDir
    Write-Ok "WASM build hoan tat."
}

# -----------------------------------------------------------------------------
# Execution
# -----------------------------------------------------------------------------
switch ($Mode) {
    'native'    { Build-Native }
    'wasm'      { Build-Wasm }
    'all'       { Build-Native; Build-Wasm }
    'clean'     { Remove-Item -Recurse -Force (Join-Path $AppDir 'build') -ErrorAction SilentlyContinue }
    'deepclean' { 
        Remove-Item -Recurse -Force (Join-Path $AppDir 'build') -ErrorAction SilentlyContinue
        Remove-Item -Recurse -Force (Join-Path $AppDir 'libs') -ErrorAction SilentlyContinue
    }
}