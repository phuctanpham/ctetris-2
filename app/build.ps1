# =============================================================================
# build.ps1 -- PowerShell build script cho cTetris (Windows)
# =============================================================================
# THAY DOI v1:
#   - Drop hoan toan pipeline rasterize SVG -> PNG -> ICO cho desktop.
#   - WASM build chi can mot file favicon.svg duy nhat (SVG-driven).
#
# THAY DOI v2 (smart-install):
#   - KHONG con install toan bo dependency mu quang nhu cu.
#   - Quy trinh: KIEM TRA -> DIFF -> INSTALL CHI THIEU
#       1. Kiem tra cmake/git/python/curl: ton tai? version >= min?
#       2. Kiem tra package qua choco / winget (neu co)
#       3. CHI install nhung package thuc su thieu / qua cu
#       4. emsdk: kiem tra version active, chi re-activate khi != target
#       5. nanosvg: chi download neu file thieu
# =============================================================================

[CmdletBinding()]
param(
    [ValidateSet('native','wasm','all','clean')]
    [string]$Mode = 'native'
)

$ErrorActionPreference = 'Stop'

# -----------------------------------------------------------------------------
# Cau hinh duong dan + version yeu cau
# -----------------------------------------------------------------------------
$AppDir       = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir     = Join-Path $AppDir 'build'
$ThirdPartyDir= Join-Path $AppDir 'thirdparty'
$EmsdkDir     = Join-Path $AppDir '.emsdk'
$BrandkitDir  = Join-Path $AppDir 'brandkit'
$BrandLogoSvg = Join-Path $BrandkitDir 'logo.svg'

$CmakeMinVersion = '3.16'
$EmsdkVersion    = '3.1.72'
$SDL3_VERSION    = '3.2.18'

# -----------------------------------------------------------------------------
# Logging helpers
# -----------------------------------------------------------------------------
function Write-Info ($msg) { Write-Host "[INFO]  $msg" -ForegroundColor Cyan }
function Write-Warn ($msg) { Write-Host "[WARN]  $msg" -ForegroundColor Yellow }
function Write-Err  ($msg) { Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Ok   ($msg) { Write-Host "[OK]    $msg" -ForegroundColor Green }

# -----------------------------------------------------------------------------
# Helper: so sanh 2 phien ban semver. Tra ve $true neu $A >= $B.
# Dung [version] cua .NET de parse, an toan voi format "X.Y.Z" / "X.Y".
# -----------------------------------------------------------------------------
function Test-VersionGE {
    param([string]$A, [string]$B)
    if ([string]::IsNullOrEmpty($B)) { return $true }
    try {
        # Pad ve dang day du de tranh exception khi compare "3" voi "3.0.0"
        function _normalize($v) {
            $parts = $v.Split('.')
            while ($parts.Count -lt 3) { $parts += '0' }
            return ($parts[0..2] -join '.')
        }
        return [version](_normalize $A) -ge [version](_normalize $B)
    } catch {
        return $true  # khong parse duoc -> coi nhu OK, tranh false-negative
    }
}

# -----------------------------------------------------------------------------
# Helper: lay version cua command qua "<cmd> --version"
# -----------------------------------------------------------------------------
function Get-CommandVersion {
    param([string]$Cmd)
    try {
        $out = & $Cmd --version 2>&1 | Select-Object -First 1
        if ($out -match '(\d+\.\d+(?:\.\d+)?)') { return $Matches[1] }
    } catch {}
    return $null
}

# -----------------------------------------------------------------------------
# Helper: kiem tra command co + version >= min
# -----------------------------------------------------------------------------
function Test-CommandVersion {
    param([string]$Cmd, [string]$MinVersion)
    if (-not (Get-Command $Cmd -ErrorAction SilentlyContinue)) {
        return $false
    }
    $cur = Get-CommandVersion $Cmd
    if (-not $cur) { return $true }  # co nhung khong parse duoc -> OK
    if (Test-VersionGE $cur $MinVersion) { return $true }
    Write-Warn "$Cmd version $cur < $MinVersion (yeu cau)"
    return $false
}

# =============================================================================
# CHOCOLATEY: kiem tra package roi install chi cai thieu
# =============================================================================
function Test-ChocoPackageInstalled {
    param([string]$Name)
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) { return $false }
    # "choco list --local-only --exact <name>" tra ve dong matching neu installed
    $out = & choco list --local-only --exact --limit-output $Name 2>$null
    return ($out -match "^$([regex]::Escape($Name))\|")
}

function Install-ChocoPackagesIfMissing {
    param([string[]]$Packages)
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
        Write-Warn 'Khong co Chocolatey -- skip Install-ChocoPackagesIfMissing'
        return $false
    }

    $missing = @()
    foreach ($p in $Packages) {
        if (-not (Test-ChocoPackageInstalled $p)) { $missing += $p }
    }
    if ($missing.Count -eq 0) {
        Write-Ok "Tat ca $($Packages.Count) choco packages da co san"
        return $true
    }
    Write-Info "Thieu $($missing.Count)/$($Packages.Count) choco packages: $($missing -join ', ')"
    & choco install -y --no-progress @missing
    return $?
}

# =============================================================================
# WINGET: kiem tra package roi install chi cai thieu
# =============================================================================
function Test-WingetPackageInstalled {
    param([string]$Id)
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) { return $false }
    $out = & winget list --id $Id --exact 2>$null
    return ($LASTEXITCODE -eq 0 -and $out -match $Id)
}

function Install-WingetPackagesIfMissing {
    param([string[]]$Ids)
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        Write-Warn 'Khong co winget -- skip Install-WingetPackagesIfMissing'
        return $false
    }
    $missing = @()
    foreach ($id in $Ids) {
        if (-not (Test-WingetPackageInstalled $id)) { $missing += $id }
    }
    if ($missing.Count -eq 0) {
        Write-Ok "Tat ca $($Ids.Count) winget packages da co san"
        return $true
    }
    Write-Info "Thieu $($missing.Count)/$($Ids.Count) winget packages: $($missing -join ', ')"
    foreach ($id in $missing) {
        & winget install --id $id --silent --accept-package-agreements --accept-source-agreements
    }
    return $true
}

# =============================================================================
# nanosvg + SVG header generation
# =============================================================================
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

function New-SvgHeader {
    param([string]$SvgPath, [string]$HeaderPath, [string]$VarName)
    if (-not (Test-Path $SvgPath)) {
        Write-Err "Khong tim thay SVG: $SvgPath"; return
    }
    $headerDir = Split-Path -Parent $HeaderPath
    New-Item -ItemType Directory -Force -Path $headerDir | Out-Null

    # Skip neu header da fresh hon SVG nguon
    if (Test-Path $HeaderPath) {
        $hdrTime = (Get-Item $HeaderPath).LastWriteTime
        $svgTime = (Get-Item $SvgPath).LastWriteTime
        if ($hdrTime -gt $svgTime) {
            Write-Ok "Header $(Split-Path -Leaf $HeaderPath) da fresh, bo qua regenerate"
            return
        }
    }

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

function New-LogoHeaders {
    $storyDir   = Join-Path $AppDir 'src\gameStory'
    $includeDir = Join-Path $storyDir 'include'

    $gameLogo = Join-Path $storyDir 'gameStory_logo.svg'
    if (Test-Path $gameLogo) {
        New-SvgHeader -SvgPath $gameLogo `
                      -HeaderPath (Join-Path $includeDir 'gameStory_logo_svg.h') `
                      -VarName 'LOGO_SVG_DATA'
    }
    $corpLogo = Join-Path $storyDir 'gameStory_corp.svg'
    if (Test-Path $corpLogo) {
        New-SvgHeader -SvgPath $corpLogo `
                      -HeaderPath (Join-Path $includeDir 'gameStory_corp_svg.h') `
                      -VarName 'CORP_SVG_DATA'
    }
}

# =============================================================================
# Basic Windows tools: cmake/git/python/curl. Smart install qua winget hoac choco.
# =============================================================================
function Initialize-WindowsTools {
    # Build danh sach tools thieu
    $needCmake  = -not (Test-CommandVersion 'cmake' $CmakeMinVersion)
    $needGit    = -not (Get-Command git    -ErrorAction SilentlyContinue)
    $needPython = -not (Get-Command python -ErrorAction SilentlyContinue) -and
                  -not (Get-Command python3 -ErrorAction SilentlyContinue)
    $needCurl   = -not (Get-Command curl   -ErrorAction SilentlyContinue)

    if (-not $needCmake -and -not $needGit -and -not $needPython -and -not $needCurl) {
        Write-Ok 'Basic Windows tools (cmake, git, python, curl) deu co san'
        return
    }

    Write-Info ('Thieu: ' + (@(
        if ($needCmake)  { 'cmake' }
        if ($needGit)    { 'git' }
        if ($needPython) { 'python' }
        if ($needCurl)   { 'curl' }
    ) -join ', '))

    # Uu tien winget vi co san tu Windows 10 1809+ va khong yeu cau admin
    if (Get-Command winget -ErrorAction SilentlyContinue) {
        $ids = @()
        if ($needCmake)  { $ids += 'Kitware.CMake' }
        if ($needGit)    { $ids += 'Git.Git' }
        if ($needPython) { $ids += 'Python.Python.3.12' }
        if ($needCurl)   { $ids += 'cURL.cURL' }
        Install-WingetPackagesIfMissing -Ids $ids
        return
    }

    # Fallback: Chocolatey
    if (Get-Command choco -ErrorAction SilentlyContinue) {
        $pkgs = @()
        if ($needCmake)  { $pkgs += 'cmake' }
        if ($needGit)    { $pkgs += 'git' }
        if ($needPython) { $pkgs += 'python' }
        if ($needCurl)   { $pkgs += 'curl' }
        Install-ChocoPackagesIfMissing -Packages $pkgs
        return
    }

    Write-Err 'Khong co winget va khong co Chocolatey. Vui long cai thu cong:'
    Write-Err '  - cmake  : https://cmake.org/download/'
    Write-Err '  - git    : https://git-scm.com/download/win'
    Write-Err '  - python : https://www.python.org/downloads/'
    throw 'Missing required tools'
}

# =============================================================================
# emsdk: chi install/activate khi version active != target
# =============================================================================
function Initialize-Emsdk {
    if (-not (Test-Path (Join-Path $EmsdkDir 'emsdk.bat'))) {
        Write-Info 'Cai emsdk lan dau tu GitHub...'
        git clone https://github.com/emscripten-core/emsdk.git $EmsdkDir
    } else {
        Write-Ok 'emsdk thu muc da ton tai'
    }

    # Kich hoat moi truong de kiem tra version
    $envScript = Join-Path $EmsdkDir 'emsdk_env.ps1'
    if (Test-Path $envScript) {
        & $envScript | Out-Null
    }

    $needActivate = $true
    if (Get-Command em++ -ErrorAction SilentlyContinue) {
        $cur = (em++ --version 2>$null | Select-Object -First 1) -replace '.*?(\d+\.\d+\.\d+).*','$1'
        if ($cur -eq $EmsdkVersion) {
            Write-Ok "emsdk $EmsdkVersion da active, bo qua install/activate"
            $needActivate = $false
        } else {
            Write-Warn "emsdk active $cur != target $EmsdkVersion, se re-activate"
        }
    }

    if ($needActivate) {
        Push-Location $EmsdkDir
        try {
            & .\emsdk.bat install $EmsdkVersion
            & .\emsdk.bat activate $EmsdkVersion
        } finally { Pop-Location }
        & $envScript | Out-Null
    }
    Write-Ok "emsdk active: $(em++ --version | Select-Object -First 1)"
}

# =============================================================================
# SDL3 self-built cho WASM
# -----------------------------------------------------------------------------
# Tai sao tu build:
#   - Emscripten port "-sUSE_SDL=3" yeu cau emsdk version moi va khong
#     on dinh tren 3.1.72 / cac ban gan day.
#   - Build SDL3 tinh tai dia phuong dam bao chinh xac SDL3 version.
# Skip neu .a + cmake config files da ton tai.
# =============================================================================
function Initialize-Sdl3Wasm {
    $sdlInstall = Join-Path $AppDir '.sdl3-wasm'
    $cmakeConfDir = Join-Path $sdlInstall 'lib\cmake\SDL3'
    $cmakeConfDir64 = Join-Path $sdlInstall 'lib64\cmake\SDL3'

    # Skip neu da ton tai san pham
    if ((Test-Path $cmakeConfDir) -or (Test-Path $cmakeConfDir64)) {
        $hasLib = Get-ChildItem -Path $sdlInstall -Recurse -Filter 'libSDL3*.a' `
                                -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($hasLib) {
            Write-Ok "SDL3 WASM static da co tai $sdlInstall (skip rebuild)"
            return
        }
    }

    Write-Info "Build SDL3 $SDL3_VERSION cho WASM (lan dau, ~1-2 phut)..."
    New-Item -ItemType Directory -Force -Path $ThirdPartyDir | Out-Null

    $sdlSrc = Join-Path $ThirdPartyDir 'SDL'
    if (-not (Test-Path $sdlSrc)) {
        git clone --depth 1 --branch "release-$SDL3_VERSION" `
            https://github.com/libsdl-org/SDL $sdlSrc
    }

    $sdlBuild = Join-Path $sdlSrc 'build-wasm'
    # SDL3 tu nhan Emscripten platform khi chay qua emcmake
    emcmake cmake -S $sdlSrc -B $sdlBuild `
        -DCMAKE_BUILD_TYPE=Release `
        -DSDL_SHARED=OFF `
        -DSDL_STATIC=ON `
        -DSDL_TESTS=OFF `
        -DSDL_TEST_LIBRARY=OFF `
        -DCMAKE_INSTALL_PREFIX=$sdlInstall
    cmake --build $sdlBuild -j
    cmake --install $sdlBuild

    Write-Ok "SDL3 WASM static da install vao $sdlInstall"
}

# =============================================================================
# Build entry points
# =============================================================================
function Build-Native {
    Write-Info 'Build NATIVE mode (Windows .exe)'
    Initialize-WindowsTools
    Test-Nanosvg
    New-LogoHeaders

    # SDL3 tren Windows: gia su user da co qua vcpkg hoac install thu cong.
    # CMake tu tim qua find_package(SDL3 REQUIRED CONFIG).
    # Neu khong co, CMake configure se fail voi message ro rang.

    $nativeBuild = Join-Path $BuildDir 'native'
    New-Item -ItemType Directory -Force -Path $nativeBuild | Out-Null

    cmake -S $AppDir -B $nativeBuild `
          -DCMAKE_BUILD_TYPE=Release `
          -DBUILD_WASM=OFF
    cmake --build $nativeBuild --config Release -j

    Write-Ok "Native build hoan tat: $nativeBuild"
}

function Build-Wasm {
    Write-Info 'Build WASM mode'
    Initialize-WindowsTools
    Initialize-Emsdk
    Initialize-Sdl3Wasm    # Build SDL3 tinh cho WASM neu chua co
    Test-Nanosvg
    New-LogoHeaders

    $wasmBuild = Join-Path $BuildDir 'wasm'
    New-Item -ItemType Directory -Force -Path $wasmBuild | Out-Null

    # Truyen CMAKE_PREFIX_PATH den thu muc SDL3 da install de
    # find_package(SDL3 CONFIG) trong CMakeLists.txt cua app tim duoc.
    $sdlPrefix = Join-Path $AppDir '.sdl3-wasm'

    emcmake cmake -S $AppDir -B $wasmBuild `
                  -DCMAKE_BUILD_TYPE=Release `
                  -DBUILD_WASM=ON `
                  -DCMAKE_PREFIX_PATH=$sdlPrefix
    cmake --build $wasmBuild -j

    if (Test-Path $BrandLogoSvg) {
        Copy-Item -Force $BrandLogoSvg (Join-Path $wasmBuild 'favicon.svg')
        Write-Ok 'Copy favicon.svg (SVG-driven)'
    } else {
        Write-Warn 'Khong co brandkit\logo.svg cho favicon'
    }

    Write-Ok "WASM build hoan tat: $wasmBuild"
}

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