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

# Version pinning -- NEU detect duoc SDL3 native qua winget/choco/manual,
# WASM build se MATCH dung version do (tranh dij ban). Neu khong, dung
# Sdl3Version pin.
$CmakeMinVersion   = '3.16'
$EmsdkVersion      = '3.1.72'
$Sdl3VersionMin    = '3.2.0'
$Sdl3Version       = '3.2.18'   # default pin
$DetectedSdl3Version = ''        # se duoc set boi Initialize-Sdl3Native

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


# (Removed all package manager wrappers and install logic)

# =============================================================================
# nanosvg -- check committed -> check libs\downloads -> download
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
# emsdk -- multi-source detection priority
#   1. em++ on PATH + version OK
#   2. $env:USERPROFILE\emsdk (user install)
#   3. $DownloadDir\emsdk (managed)
#   4. Clone moi (last resort)
# =============================================================================
function Initialize-Emsdk {
    # Priority 1: em++ tren PATH
    if (Get-Command em++ -ErrorAction SilentlyContinue) {
        $cur = (em++ --version 2>$null | Select-Object -First 1) `
                -replace '.*?(\d+\.\d+\.\d+).*','$1'
        if ((Test-VersionGE $cur $EmsdkVersion)) {
            Write-Ok "em++ tren PATH version $cur >= $EmsdkVersion (skip)"
            return
        }
        Write-Warn "em++ tren PATH version $cur < $EmsdkVersion"
    }

    # Priority 2: ~/emsdk
    $userEmsdk = Join-Path $env:USERPROFILE 'emsdk'
    $userEnv   = Join-Path $userEmsdk 'emsdk_env.ps1'
    if (Test-Path $userEnv) {
        Write-Info "Phat hien $userEmsdk -- dung emsdk cua user"
        & $userEnv | Out-Null
        if (Get-Command em++ -ErrorAction SilentlyContinue) {
            $cur = (em++ --version 2>$null | Select-Object -First 1) `
                    -replace '.*?(\d+\.\d+\.\d+).*','$1'
            if (Test-VersionGE $cur $EmsdkVersion) {
                Write-Ok "~/emsdk version $cur >= $EmsdkVersion (skip)"
                return
            }
            Write-Info "Update ~/emsdk len $EmsdkVersion..."
            Push-Location $userEmsdk
            try {
                & .\emsdk.bat install $EmsdkVersion
                & .\emsdk.bat activate $EmsdkVersion
            } finally { Pop-Location }
            & $userEnv | Out-Null
            return
        }
    }

    # Priority 3: managed download
    $managed    = Join-Path $DownloadDir 'emsdk'
    $managedEnv = Join-Path $managed 'emsdk_env.ps1'
    if (Test-Path $managedEnv) {
        Write-Info "Phat hien managed emsdk tai $managed"
        & $managedEnv | Out-Null
        if (Get-Command em++ -ErrorAction SilentlyContinue) {
            $cur = (em++ --version 2>$null | Select-Object -First 1) `
                    -replace '.*?(\d+\.\d+\.\d+).*','$1'
            if (Test-VersionGE $cur $EmsdkVersion) {
                Write-Ok "managed emsdk version $cur >= $EmsdkVersion (skip)"
                return
            }
        }
        Write-Info "Update managed emsdk len $EmsdkVersion..."
        Push-Location $managed
        try {
            & .\emsdk.bat install $EmsdkVersion
            & .\emsdk.bat activate $EmsdkVersion
        } finally { Pop-Location }
        & $managedEnv | Out-Null
        return
    }

    # Priority 4: clone fresh
    Write-Info "Khong tim thay emsdk, clone vao $managed..."
    New-Item -ItemType Directory -Force -Path $DownloadDir | Out-Null
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git $managed
    Push-Location $managed
    try {
        & .\emsdk.bat install $EmsdkVersion
        & .\emsdk.bat activate $EmsdkVersion
    } finally { Pop-Location }
    & $managedEnv | Out-Null
    Write-Ok "emsdk active: $(em++ --version | Select-Object -First 1)"
}

# =============================================================================
# SDL3 native (Windows)
#   1. pkg-config --exists sdl3 (neu user co MSYS2 / vcpkg setup pkg-config)
#   2. winget / choco install sdl3 (rare -- thuong khong co)
#   3. Build tu source vao $DownloadDir\sdl3-native\
# =============================================================================
function Initialize-Sdl3Native {
    Write-Info 'Checking SDL3 cho native Windows (priority chain)...'

    # Priority 1: pkg-config (neu user co MSYS2 setup)
    Write-Info '  [1/4] Try pkg-config --exists sdl3...'
    if (Get-Command pkg-config -ErrorAction SilentlyContinue) {
        & pkg-config --exists sdl3 2>$null
        if ($LASTEXITCODE -eq 0) {
            $v = & pkg-config --modversion sdl3
            $script:DetectedSdl3Version = $v
            Write-Ok "Found via pkg-config (version $v) -- skip install"
            Write-Info "WASM build se khop dung version $v de tranh dij ban"
            return
        }
    }
    Write-Info '  ... pkg-config khong co SDL3'

    # Priority 2: vcpkg
    Write-Info '  [2/4] Skip vcpkg (chua implement)'

    # Priority 3: winget / choco install (rare cho SDL3)
    Write-Info '  [3/4] Try winget Library.SDL3 (best effort)...'
    if (Get-Command winget -ErrorAction SilentlyContinue) {
        # SDL3 chua co winget package chinh thuc, skip
        Write-Info '  ... winget chua co SDL3 package'
    }

    # Priority 4: Build tu source
    Write-Info "  [4/4] Build SDL3 $Sdl3Version tu source..."
    $script:DetectedSdl3Version = $Sdl3Version
    Build-Sdl3FromSource -InstallPrefix (Join-Path $DownloadDir 'sdl3-native') `
                         -Target 'native' `
                         -Version $Sdl3Version
}

function Initialize-Sdl3Wasm {
    # Match version voi native (DetectedSdl3Version). Fallback ve Sdl3Version pin.
    $targetVersion = if ($script:DetectedSdl3Version) { $script:DetectedSdl3Version } else { $Sdl3Version }
    Write-Info "WASM SDL3 target version: $targetVersion"

    $installDir = Join-Path $DownloadDir "sdl3-wasm-$targetVersion"
    $cmakeConf  = Join-Path $installDir 'lib\cmake\SDL3'
    $cmakeConf64= Join-Path $installDir 'lib64\cmake\SDL3'

    Write-Info "Checking SDL3 WASM cache tai $installDir..."

    if ((Test-Path $cmakeConf) -or (Test-Path $cmakeConf64)) {
        $hasLib = Get-ChildItem -Path $installDir -Recurse -Filter 'libSDL3*.a' `
                                -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($hasLib) {
            Write-Ok "SDL3 WASM cache HIT version $targetVersion -- skip rebuild"
            return
        }
        Write-Warn 'Cache co cmake config nhung thieu .a -- rebuild'
    } else {
        Write-Info "Cache MISS version $targetVersion -- chua build SDL3 cho WASM"
    }

    Write-Info 'System SDL3 (neu co) la native arch -- KHONG dung duoc cho wasm32'
    Write-Info "Build SDL3 $targetVersion cho WASM target (lan dau, ~1-2 phut)..."

    Build-Sdl3FromSource -InstallPrefix $installDir -Target 'wasm' -Version $targetVersion
}

# =============================================================================
# SDL3 WASM -- tu build static lib, KHONG dung -sUSE_SDL=3
# =============================================================================
function Build-Sdl3FromSource {
    param([string]$InstallPrefix, [string]$Target, [string]$Version)

    $sdlSrc   = Join-Path $DownloadDir "SDL-$Version"
    $sdlBuild = Join-Path $sdlSrc "build-$Target"

    New-Item -ItemType Directory -Force -Path $DownloadDir | Out-Null
    if (-not (Test-Path $sdlSrc)) {
        Write-Info "Clone SDL3 release-$Version vao $sdlSrc..."
        git clone --depth 1 --branch "release-$Version" `
            https://github.com/libsdl-org/SDL $sdlSrc
    } else {
        Write-Ok "SDL3 source $Version da co tai $sdlSrc"
    }

    $cfg = @(
        '-S', $sdlSrc, '-B', $sdlBuild,
        '-DCMAKE_BUILD_TYPE=Release',
        "-DCMAKE_INSTALL_PREFIX=$InstallPrefix"
    )
    if ($Target -eq 'wasm') {
        $cfg += @('-DSDL_SHARED=OFF', '-DSDL_STATIC=ON',
                  '-DSDL_TESTS=OFF', '-DSDL_TEST_LIBRARY=OFF')
        & emcmake cmake @cfg
    } else {
        $cfg += @('-DSDL_SHARED=ON')
        & cmake @cfg
    }
    & cmake --build $sdlBuild -j
    & cmake --install $sdlBuild
    Write-Ok "SDL3 $Version ($Target) da install vao $InstallPrefix"
}


# Icon: Only copy pre-created icon from brandkit to build output
function Copy-DesktopIcon {
    param([string]$OutDir)
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    $icoSrc = Join-Path $BrandkitDir 'cTetris.ico'
    $icoDst = Join-Path $OutDir 'cTetris.ico'
    if (Test-Path $icoSrc) {
        Copy-Item -Force $icoSrc $icoDst
        Write-Ok "Copied icon: $icoDst"
    } else {
        Write-Warn "No $icoSrc found, using default OS icon."
    }
}
function Initialize-WindowsTools {
    $needCmake  = -not (Test-CommandVersion 'cmake' $CmakeMinVersion)
    $needGit    = -not (Get-Command git    -ErrorAction SilentlyContinue)
    $needPython = -not (Get-Command python -ErrorAction SilentlyContinue) -and `
                  -not (Get-Command python3 -ErrorAction SilentlyContinue)
    $needCurl   = -not (Get-Command curl   -ErrorAction SilentlyContinue)

    if (-not ($needCmake -or $needGit -or $needPython -or $needCurl)) {
        Write-Ok 'Basic tools deu co san'
        return
    }

    Write-Info ('Thieu: ' + (@(
        if ($needCmake)  { 'cmake' }
        if ($needGit)    { 'git' }
        if ($needPython) { 'python' }
        if ($needCurl)   { 'curl' }
    ) -join ', '))

    if (Get-Command winget -ErrorAction SilentlyContinue) {
        $ids = @()
        if ($needCmake)  { $ids += 'Kitware.CMake' }
        if ($needGit)    { $ids += 'Git.Git' }
        if ($needPython) { $ids += 'Python.Python.3.12' }
        if ($needCurl)   { $ids += 'cURL.cURL' }
        Install-WingetPackagesIfMissing -Ids $ids
        return
    }

    if (Get-Command choco -ErrorAction SilentlyContinue) {
        $pkgs = @()
        if ($needCmake)  { $pkgs += 'cmake' }
        if ($needGit)    { $pkgs += 'git' }
        if ($needPython) { $pkgs += 'python' }
        if ($needCurl)   { $pkgs += 'curl' }
        Install-ChocoPackagesIfMissing -Packages $pkgs
        return
    }

    Write-Err 'Khong co winget va khong co choco. Cai thu cong:'
    Write-Err '  cmake  : https://cmake.org/download/'
    Write-Err '  git    : https://git-scm.com/download/win'
    Write-Err '  python : https://www.python.org/downloads/'
    throw 'Missing required tools'
}

# =============================================================================
# Validate sources -- file phai duoc commit san, KHONG sinh tu script
# =============================================================================
function Test-Sources {
    $required = @(
        'main.cpp',
        'src\gameStory\app.cpp',
        'src\gameConsole\app.cpp',
        'src\gameCore\app.cpp',
        'src\gameStory\include\gameStory_layout.h',
        'src\gameStory\include\gameStory_logo_svg.h',
        'src\gameStory\include\gameStory_corp_svg.h',
        'src\gameConsole\include\gameConsole_layout.h',
        'src\gameCore\include\gameCore_layout.h',
        'CMakeLists.txt'
    )
    $missing = @()
    foreach ($rel in $required) {
        $abs = Join-Path $AppDir $rel
        if (-not (Test-Path $abs)) { $missing += $abs }
    }
    if ($missing.Count -gt 0) {
        Write-Err 'Thieu source file (phai duoc commit san):'
        foreach ($f in $missing) { Write-Err "  - $f" }
        throw 'Source validation failed'
    }
    Write-Ok 'Source files validated'
}

# =============================================================================
# Build entry points
# =============================================================================
function Build-Native {
    Write-Info "Build NATIVE -> $BuildNativeDir"
    Test-Sources
    Initialize-Sdl3Native
    Initialize-Nanosvg

    # Copy icon from brandkit to build output
    Copy-DesktopIcon -OutDir $BuildNativeDir

    # Tim path SDL3Config.cmake
    $sdlInstall = Join-Path $DownloadDir 'sdl3-native'
    $sdlDirArgs = @()
    foreach ($cand in @(
        (Join-Path $sdlInstall 'lib\cmake\SDL3'),
        (Join-Path $sdlInstall 'lib64\cmake\SDL3')
    )) {
        if (Test-Path (Join-Path $cand 'SDL3Config.cmake')) {
            $sdlDirArgs = @("-DSDL3_DIR=$cand")
            Write-Info "SDL3_DIR = $cand"
            break
        }
    }

    # Truyen icon path cho CMake (neu da copy)
    $iconArg = @()
    $ico = Join-Path $BuildNativeDir 'cTetris.ico'
    if (Test-Path $ico) {
        $iconArg = @("-DCTETRIS_ICON_PATH=$ico")
        Write-Info "CTETRIS_ICON_PATH = $ico"
    }

    New-Item -ItemType Directory -Force -Path $BuildNativeDir | Out-Null
    cmake -S $AppDir -B $BuildNativeDir `
          -DCMAKE_BUILD_TYPE=Release `
          -DBUILD_WASM=OFF `
          -DCMAKE_PREFIX_PATH=$sdlInstall `
          -DNANOSVG_INCLUDE_DIR=(Join-Path $DownloadDir 'nanosvg') `
          @sdlDirArgs `
          @iconArg
    cmake --build $BuildNativeDir --config Release -j
    Write-Ok "Native build hoan tat: $BuildNativeDir"
}

function Build-Wasm {
    Write-Info "Build WASM -> $BuildWasmDir"
    Test-Sources
    Initialize-WindowsTools

    # Detect version SDL3 native truoc -- WASM build se MATCH version do
    Write-Info 'Detect SDL3 native version de match cho WASM...'
    Initialize-Sdl3Native

    Initialize-Emsdk
    Initialize-Sdl3Wasm
    Initialize-Nanosvg

    # Derive sdl_install path tu version detect duoc
    $targetVersion = if ($script:DetectedSdl3Version) { $script:DetectedSdl3Version } else { $Sdl3Version }
    $sdlInstall = Join-Path $DownloadDir "sdl3-wasm-$targetVersion"

    $sdlDir = $null
    foreach ($cand in @(
        (Join-Path $sdlInstall 'lib\cmake\SDL3'),
        (Join-Path $sdlInstall 'lib64\cmake\SDL3')
    )) {
        if (Test-Path (Join-Path $cand 'SDL3Config.cmake')) {
            $sdlDir = $cand
            break
        }
    }
    if (-not $sdlDir) {
        Write-Err "Khong tim thay SDL3Config.cmake trong $sdlInstall\lib*\cmake\SDL3\"
        throw 'SDL3 WASM build incomplete'
    }
    Write-Info "SDL3_DIR = $sdlDir"

    New-Item -ItemType Directory -Force -Path $BuildWasmDir | Out-Null
    emcmake cmake -S $AppDir -B $BuildWasmDir `
                  -DCMAKE_BUILD_TYPE=Release `
                  -DBUILD_WASM=ON `
                  -DSDL3_DIR=$sdlDir `
                  -DCMAKE_PREFIX_PATH=$sdlInstall `
                  -DNANOSVG_INCLUDE_DIR=(Join-Path $DownloadDir 'nanosvg')
    cmake --build $BuildWasmDir -j

    if (Test-Path $BrandLogoSvg) {
        Copy-Item -Force $BrandLogoSvg (Join-Path $BuildWasmDir 'favicon.svg')
        Write-Ok 'Copy favicon.svg (SVG-driven)'
    } else {
        Write-Warn "Khong co $BrandLogoSvg"
    }
    Write-Ok "WASM build hoan tat: $BuildWasmDir"
}

switch ($Mode) {
    'native'    { Build-Native }
    'wasm'      { Build-Wasm }
    'all'       { Build-Native; Build-Wasm }
    'clean'     {
        Write-Info 'Don dep build/ (giu lai libs/ cache)...'
        $b = Join-Path $AppDir 'build'
        if (Test-Path $b) { Remove-Item -Recurse -Force $b }
        Write-Ok "Da xoa build/ (libs\$OS_NAME\downloads\ van con)"
    }
    'deepclean' {
        Write-Info 'Don dep TOAN BO (build/ + libs/)...'
        foreach ($d in @('build', 'libs')) {
            $p = Join-Path $AppDir $d
            if (Test-Path $p) { Remove-Item -Recurse -Force $p }
        }
        Write-Ok 'Da xoa build\ va libs\'
    }
}