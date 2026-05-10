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


# =============================================================================
# FIX: Import-VsEnv -- Tim va load MSVC compiler environment vao PowerShell
# session hien tai. Can thiet de cmake tim duoc cl.exe / nmake / link.exe.
# Thu tu uu tien:
#   1. vswhere.exe (chi co neu co VS hoac Build Tools)
#   2. Fallback: tim vcvarsall.bat o cac duong dan pho bien
# =============================================================================
function Import-VsEnv {
    # Kiem tra cl.exe da co chua (co the da duoc load tu Developer PS)
    if (Get-Command cl -ErrorAction SilentlyContinue) {
        Write-Ok "MSVC cl.exe da co trong PATH (skip Import-VsEnv)"
        return
    }

    Write-Info "Tim MSVC compiler environment..."

    # Priority 1: dung vswhere de tim tat ca VS / Build Tools (KHONG dung -requires)
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vcvars  = $null

    if (Test-Path $vswhere) {
        # -all: bao gom ca preview; -products *: ca BuildTools lan Community/Pro/Ent
        $vsPaths = (& $vswhere -all -products * -property installationPath 2>$null) -split "`n" |
                   ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
        foreach ($vsPath in $vsPaths) {
            $candidate = Join-Path $vsPath 'VC\Auxiliary\Build\vcvarsall.bat'
            if (Test-Path $candidate) { $vcvars = $candidate; break }
        }
        if ($vcvars) { Write-Info "vswhere tim thay: $vcvars" }
    }

    # Priority 2: fallback cung voi ${env:ProgramFiles(x86)} de match chinh xac
    if (-not $vcvars) {
        $x86 = ${env:ProgramFiles(x86)}
        $pf  = $env:ProgramFiles
        $fallbacks = @(
            "$x86\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat",
            "$pf\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat",
            "$pf\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat",
            "$pf\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
            "$x86\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat",
            "$pf\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
        )
        foreach ($fb in $fallbacks) {
            if (Test-Path $fb) { $vcvars = $fb; break }
        }
        if ($vcvars) { Write-Info "Fallback tim thay: $vcvars" }
    }

    if (-not $vcvars) {
        Write-Warn "Khong tim thay vcvarsall.bat -- cmake co the that bai neu chua co compiler trong PATH."
        Write-Warn "Hay mo 'Developer PowerShell for VS 2022' roi chay lai script."
        return
    }

    Write-Info "Load MSVC env tu: $vcvars"
    cmd /c "call `"$vcvars`" x64 > nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
        }
    }

    if (Get-Command cl -ErrorAction SilentlyContinue) {
        # cl.exe in version ra stderr -- dung 2>&1 va wrap trong try/catch
        # de tranh $ErrorActionPreference = Stop lam dung script
        try {
            $clOut = (cl 2>&1 | Select-Object -First 1).ToString()
            $clVer = if ($clOut -match '(\d+\.\d+\.\d+)') { $Matches[1] } else { 'unknown' }
        } catch { $clVer = 'unknown' }
        Write-Ok "MSVC cl.exe san sang (version $clVer)"
    } else {
        Write-Warn "Sau khi load vcvarsall, van khong tim thay cl.exe. Kiem tra lai cai dat VS."
    }

    # Kiem tra Ninja (can thiet vi script dung -G Ninja cho ca native va WASM)
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        Write-Info "Ninja chua co trong PATH -- thu tim trong VS install..."
        # Dung vswhere de lay paths, sau do thu cac subfolder Ninja
        $ninjaDirs = @()
        if (Test-Path $vswhere) {
            $vsPaths2 = (& $vswhere -all -products * -property installationPath 2>$null) -split "`n" |
                        ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
            foreach ($vp in $vsPaths2) {
                $ninjaDirs += Join-Path $vp 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
            }
        }
        # Fallback cung voi env vars (tranh hardcode C:\)
        $x86pf = ${env:ProgramFiles(x86)}
        $pf    = $env:ProgramFiles
        $ninjaDirs += @(
            "$x86pf\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
            "$pf\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
            "$pf\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
            "$pf\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
            "$x86pf\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
        )
        $ninjaFound = $false
        foreach ($nd in $ninjaDirs) {
            if ($nd -and (Test-Path (Join-Path $nd 'ninja.exe'))) {
                $env:PATH = "$nd;$env:PATH"
                Write-Ok "Ninja tim thay: $nd"
                $ninjaFound = $true
                break
            }
        }
        if (-not $ninjaFound) {
            Write-Warn "Khong tim thay ninja.exe. Cai Ninja: winget install Ninja-build.Ninja"
        }
    } else {
        Write-Ok "Ninja da co trong PATH"
    }
}


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
# FIX: Import-EmsdkEnv -- source emsdk_env.bat vao PowerShell session hien tai.
# emsdk chi ship emsdk_env.bat (KHONG co emsdk_env.ps1). Phai chay qua
# cmd /c de bat ENV vars tu .bat vao process PowerShell hien tai.
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
    # FIX: kiem tra emsdk_env.bat (khong phai .ps1 -- file do khong ton tai)
    $userEmsdk = Join-Path $env:USERPROFILE 'emsdk'
    if (Test-Path (Join-Path $userEmsdk 'emsdk_env.bat')) {
        Write-Info "Phat hien $userEmsdk -- dung emsdk cua user"
        Import-EmsdkEnv $userEmsdk
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
            Import-EmsdkEnv $userEmsdk
            return
        }
    }

    # Priority 3: managed download
    # FIX: kiem tra emsdk_env.bat (khong phai .ps1)
    $managed = Join-Path $DownloadDir 'emsdk'
    if (Test-Path (Join-Path $managed 'emsdk_env.bat')) {
        Write-Info "Phat hien managed emsdk tai $managed"
        Import-EmsdkEnv $managed
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
        Import-EmsdkEnv $managed
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
    Import-EmsdkEnv $managed   # FIX: dung Import-EmsdkEnv thay vi & .ps1
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
            if (Test-VersionGE $v $Sdl3VersionMin) {
                $script:DetectedSdl3Version = $v
                Write-Ok "Found via pkg-config (version $v >= $Sdl3VersionMin) -- skip install"
                Write-Info "WASM build se khop dung version $v de tranh dij ban"
                return
            }
            Write-Warn "pkg-config bao SDL3 $v < min $Sdl3VersionMin -- bo qua, build tu source"
        }
    }
    Write-Info '  ... pkg-config khong co SDL3 (hoac version qua cu)'

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
# SDL3 build tu source -- dung Ninja cho ca native va WASM
# (Ninja di kem VS Build Tools, khong phu thuoc nmake)
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
        "-DCMAKE_INSTALL_PREFIX=$InstallPrefix",
        '-DSDL_TESTS=OFF', '-DSDL_TEST_LIBRARY=OFF',
        '-G', 'Ninja'    # Ninja: co san trong VS Build Tools, khong can nmake
    )

    if ($Target -eq 'wasm') {
        $cfg += @('-DSDL_SHARED=OFF', '-DSDL_STATIC=ON')
        & emcmake cmake @cfg
    } else {
        # Native: dam bao MSVC env da duoc load truoc khi configure
        Import-VsEnv
        $cfg += @('-DSDL_SHARED=ON')
        & cmake @cfg
    }
    if ($LASTEXITCODE -ne 0) { throw "SDL3 $Target configure failed" }
    & cmake --build $sdlBuild -j
    if ($LASTEXITCODE -ne 0) { throw "SDL3 $Target build failed" }
    & cmake --install $sdlBuild
    if ($LASTEXITCODE -ne 0) { throw "SDL3 $Target install failed" }
    Write-Ok "SDL3 $Version ($Target) da install vao $InstallPrefix"
}


# Icon: Only copy pre-created icon from brandkit to build output
function Copy-DesktopIcon {
    param([string]$OutDir)
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    # FIX: dung 'logo.ico' (ten file trong brandkit), khong phai 'cTetris.ico'
    $icoSrc = Join-Path $BrandkitDir 'logo.ico'
    $icoDst = Join-Path $OutDir 'cTetris.ico'
    if (Test-Path $icoSrc) {
        Copy-Item -Force $icoSrc $icoDst
        Write-Ok "Copied icon: $icoDst"
    } else {
        Write-Warn "No $icoSrc found, using default OS icon."
    }
}

# PWA assets -- copy tu web/ (da commit san, khong sinh on-the-fly)
# manifest.webmanifest + sw.js can thiet de browser hien nut Install
# va Service Worker hoat dong (offline + full-height standalone mode).
function Copy-PwaAssets {
    param([string]$OutDir)
    foreach ($asset in @('manifest.webmanifest', 'sw.js')) {
        $src = Join-Path $WebDir $asset
        $dst = Join-Path $OutDir $asset
        if (Test-Path $src) {
            Copy-Item -Force $src $dst
            Write-Ok "Copy PWA asset: $asset"
        } else {
            Write-Warn "Khong co $src -- PWA se thieu $asset"
        }
    }
}

# =============================================================================
# Package install helpers -- called by Initialize-WindowsTools
# =============================================================================
function Install-WingetPackagesIfMissing {
    param([string[]]$Ids)
    foreach ($id in $Ids) {
        Write-Info "winget install $id..."
        winget install --id $id --silent --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "winget install $id thoat voi code $LASTEXITCODE -- co the da co hoac can restart shell"
        } else {
            Write-Ok "$id da cai xong qua winget"
        }
    }
    # Refresh PATH sau khi cai (winget khong tu refresh process PATH)
    $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine') + ';' +
                [System.Environment]::GetEnvironmentVariable('PATH', 'User')
}

function Install-ChocoPackagesIfMissing {
    param([string[]]$Packages)
    foreach ($pkg in $Packages) {
        Write-Info "choco install $pkg..."
        choco install $pkg -y --no-progress
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "choco install $pkg thoat voi code $LASTEXITCODE"
        } else {
            Write-Ok "$pkg da cai xong qua choco"
        }
    }
    # Refresh PATH (choco installs to new folders not yet in process PATH)
    $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine') + ';' +
                [System.Environment]::GetEnvironmentVariable('PATH', 'User')
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
    Initialize-WindowsTools   # FIX: parity voi Build-Wasm -- dam bao cmake/git/python co san

    # FIX: Load MSVC compiler environment truoc khi lam bat cu thu gi voi cmake
    Import-VsEnv

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
    $nativeArgs = @(
        '-S', $AppDir,
        '-B', $BuildNativeDir,
        '-DCMAKE_BUILD_TYPE=Release',
        '-DBUILD_WASM=OFF',
        '-G', 'Ninja',
        "-DCMAKE_PREFIX_PATH=$sdlInstall",
        "-DNANOSVG_INCLUDE_DIR=$(Join-Path $DownloadDir 'nanosvg')"
    ) + $sdlDirArgs + $iconArg
    & cmake @nativeArgs
    if ($LASTEXITCODE -ne 0) { throw "Native configure failed (exit $LASTEXITCODE)" }
    & cmake --build $BuildNativeDir --config Release -j
    if ($LASTEXITCODE -ne 0) { throw "Native build failed (exit $LASTEXITCODE)" }
    Write-Ok "Native build hoan tat: $BuildNativeDir"
}

function Build-Wasm {
    Write-Info "Build WASM -> $BuildWasmDir"
    Test-Sources
    Initialize-WindowsTools

    # Detect version SDL3 native truoc -- WASM build se MATCH version do
    # FIX: wrap try/catch -- neu khong co native SDL3 thi dung pin $Sdl3Version
    Write-Info 'Detect SDL3 native version de match cho WASM (best effort)...'
    try { Initialize-Sdl3Native }
    catch { Write-Warn "Khong detect duoc SDL3 native -- dung version pin $Sdl3Version" }

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
    $wasmArgs = @(
        '-S', $AppDir,
        '-B', $BuildWasmDir,
        '-DCMAKE_BUILD_TYPE=Release',
        '-DBUILD_WASM=ON',
        '-G', 'Ninja',
        "-DSDL3_DIR=$sdlDir",
        "-DCMAKE_PREFIX_PATH=$sdlInstall",
        "-DNANOSVG_INCLUDE_DIR=$(Join-Path $DownloadDir 'nanosvg')"
    )
    & emcmake cmake @wasmArgs
    if ($LASTEXITCODE -ne 0) { throw "emcmake configure failed (exit $LASTEXITCODE)" }

    # FIX: build step (parity voi build.sh line 699: `cmake --build "$BUILD_WASM_DIR" -j`)
    & cmake --build $BuildWasmDir --parallel
    if ($LASTEXITCODE -ne 0) { throw "WASM cmake --build failed (exit $LASTEXITCODE)" }

    # Verify deploy-pages.yml expected artifacts
    foreach ($f in @('cTetris.html','cTetris.js','cTetris.wasm')) {
        $p = Join-Path $BuildWasmDir $f
        if (-not (Test-Path $p)) { throw "Missing build artifact: $p" }
    }

    # Favicon SVG-driven
    if (Test-Path $BrandLogoSvg) {
        Copy-Item -Force $BrandLogoSvg (Join-Path $BuildWasmDir 'favicon.svg')
        Write-Ok 'Copy favicon.svg (SVG-driven)'
    } else {
        Write-Warn "Khong co $BrandLogoSvg"
    }

    # FIX: Fallback favicon.ico cho browser khong ho tro SVG favicon
    $brandIco = Join-Path $BrandkitDir 'logo.ico'
    if (Test-Path $brandIco) {
        Copy-Item -Force $brandIco (Join-Path $BuildWasmDir 'favicon.ico')
        Write-Ok 'Copy favicon.ico (ICO fallback)'
    } else {
        Write-Warn "Khong co $brandIco, favicon.ico fallback se thieu"
    }

    # PWA assets -- copy tu web/ (da commit san, khong sinh on-the-fly)
    Copy-PwaAssets -OutDir $BuildWasmDir

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