# =============================================================================
# build.ps1 -- PowerShell build script cho cTetris (Windows)
# =============================================================================
# FIXED ISSUES (auto-applied on clone + build):
#
# 1. SDL3 Runtime Deployment (v4 fix):
#    - Automatic SDL3.dll copy to build output post-build
#    - Eliminates "SDL3.dll not found" error at runtime
#    - Searches: libs\windows\downloads\sdl3-native\bin\ or \lib\
#    - Status: ENABLED - no manual deployment needed
#
# 2. Duplicate Symbol Linker Error (v6 fix in CMakeLists.txt):
#    - Fixed LNK2005 "already defined" for nanosvg symbols
#    - Root cause: Both gameStory/app.cpp and gameConsole/app.cpp
#      defined NANOSVG_IMPLEMENTATION, causing duplicate public symbols
#    - Solution: compile a single shared TU: src/shared/nanosvg_impl.cpp
#      and keep module .cpp files declaration-only
#    - Status: ENABLED - no duplicate object files in final link
#
# 3. Console Window Hidden on Windows (v5 fix - NEW):
#    - Automatic validation and auto-patching of CMakeLists.txt
#    - Adds WIN32 flag to add_executable()
#    - Adds /SUBSYSTEM:WINDOWS and /ENTRY:mainCRTStartup linker flags
#    - Configures Windows SDK library paths for x64
#    - Result: .exe shows only GUI, no console window on Windows
#    - Status: ENABLED - validated/auto-patched on every build
#
# 4. File Logging System (v5 NEW):
#    - Header-only Logger class in src/logger.h
#    - Automatic validation of logger integration
#    - Logs to game.log in same directory as executable
#    - Thread-safe singleton pattern
#    - Printf-style formatting with auto-timestamp
#    - Status: ENABLED - validated on every build
#
# 5. Dependency Consolidation (v2-v3 fixes):
#    - All downloads centralized to libs\windows\downloads\
#    - SDL3, nanosvg, nlohmann/json follow OS-driven paths
#    - build.ps1 + build.sh kept in sync for parallel workflows
#    - Status: ENABLED - consistent caching across platforms
#
# DEPLOYMENT GUARANTEE (Fresh Clone Workflow):
#   New laptop -> clone repo -> cd app -> .\build.ps1 native
#   => All validation + auto-fixes applied automatically
#   => Console hidden on Windows, logger integrated
#   => No manual intervention needed
#
# =============================================================================
# CONFIGURATION NOTES (v3):
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
$SqliteVersion     = '3460100'   # SQLite 3.46.1
$SqliteYear        = '2024'

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

function Get-PythonCommandVersion {
    param([string]$Cmd)
    if (-not (Get-Command $Cmd -ErrorAction SilentlyContinue)) { return $null }
    try {
        $out = & $Cmd --version 2>&1 | Select-Object -First 1
        if ($out -match '(\d+\.\d+(?:\.\d+)?)') { return $Matches[1] }
    } catch {}
    return $null
}

function Test-PythonForEmsdk {
    $pyVersion = Get-PythonCommandVersion 'py'
    $pythonVersion = Get-PythonCommandVersion 'python'

    if ($pyVersion) {
        Write-Info "py --version = $pyVersion"
    } else {
        Write-Info 'py --version = not available'
    }

    if ($pythonVersion) {
        Write-Info "python --version = $pythonVersion"
        return $true
    }

    if ($pyVersion) {
        Write-Warn "py co san nhung python khong co trong PATH; emsdk can python de chay. Cai thu cong python va chay lai."
    } else {
        Write-Warn "Khong co ca py lan python; emsdk can python de chay. Cai thu cong python va chay lai."
    }

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
# Package installation helpers for winget and choco
# =============================================================================
function Install-WingetPackagesIfMissing {
    param([string[]]$Ids)
    foreach ($id in $Ids) {
        Write-Info "Installing $id via winget..."
        & winget install -e --id $id --accept-source-agreements --accept-package-agreements 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "winget install $id may have failed (exit $LASTEXITCODE)"
        }
    }
}

function Install-ChocoPackagesIfMissing {
    param([string[]]$Packages)
    foreach ($pkg in $Packages) {
        Write-Info "Installing $pkg via choco..."
        & choco install $pkg -y 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "choco install $pkg may have failed (exit $LASTEXITCODE)"
        }
    }
}

# =============================================================================
# nlohmann/json -- check committed -> check libs\downloads -> download
# Same pattern as nanosvg: vendored in source tree takes priority.
# =============================================================================
$NlohmannVersion = '3.11.3'

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
# SQLite amalgamation -- check vendored -> check libs\downloads -> download zip
# Compile with -DSQLITE_THREADSAFE=0 for WASM (set later in CMakeLists.txt).
# =============================================================================
function Initialize-Sqlite {
    $sqliteDir = Join-Path $DownloadDir 'sqlite'
    $sqliteC   = Join-Path $sqliteDir 'sqlite3.c'
    $sqliteH   = Join-Path $sqliteDir 'sqlite3.h'

    if ((Test-Path $sqliteC) -and (Test-Path $sqliteH)) {
        $size = (Get-Item $sqliteC).Length
        if ($size -gt 5MB) {
            Write-Ok "SQLite amalgamation da co tai $sqliteDir ($([math]::Round($size/1MB,1)) MB)"
            return
        }
        Write-Warn "sqlite3.c kich thuoc bat thuong ($size bytes) -- re-download"
    }

    Write-Info "Tai SQLite amalgamation $SqliteVersion vao $sqliteDir..."
    New-Item -ItemType Directory -Force -Path $sqliteDir | Out-Null

    $zipUrl = "https://sqlite.org/$SqliteYear/sqlite-amalgamation-$SqliteVersion.zip"
    $zipPath = Join-Path $sqliteDir 'sqlite-amalgamation.zip'
    $tmpExtract = Join-Path $sqliteDir 'tmp_extract'

    try {
        Invoke-WebRequest -Uri $zipUrl -OutFile $zipPath -UseBasicParsing
        if (Test-Path $tmpExtract) { Remove-Item -Recurse -Force $tmpExtract }
        Expand-Archive -Path $zipPath -DestinationPath $tmpExtract -Force

        $innerDir = Join-Path $tmpExtract "sqlite-amalgamation-$SqliteVersion"
        Copy-Item -Force (Join-Path $innerDir 'sqlite3.c') $sqliteC
        Copy-Item -Force (Join-Path $innerDir 'sqlite3.h') $sqliteH

        Remove-Item -Force $zipPath
        Remove-Item -Recurse -Force $tmpExtract
    } catch {
        Write-Err "SQLite download/extract failed: $_"
        throw "SQLite acquisition failed"
    }

    $size = (Get-Item $sqliteC).Length
    if ($size -lt 5MB) {
        Write-Err "sqlite3.c tai ve nho bat thuong ($size bytes) -- abort"
        Remove-Item -Force $sqliteC, $sqliteH -ErrorAction SilentlyContinue
        throw "SQLite amalgamation truncated"
    }
    Write-Ok "SQLite amalgamation $SqliteVersion san sang tai $sqliteDir ($([math]::Round($size/1MB,1)) MB)"
}

# =============================================================================
# Validate-Endpoints -- check MANIFEST_GIST_URL + CTETRIS_API_URL + media
# Mirrors game loading-bar progress: [N/total] per media file check (HEAD).
# Stops the build (throw) on first failure.
# =============================================================================
function Validate-Endpoints {
    Write-Info "=== Endpoint & Media Validation ==="

    $envFile = Join-Path $AppDir '.env'
    if (-not (Test-Path $envFile)) {
        Write-Err ".env not found at $envFile — create it with MANIFEST_GIST_URL and CTETRIS_API_URL"
        throw "Missing app/.env"
    }

    $envContent = Get-Content $envFile -Raw

    $manifestUrl = ''
    $apiUrl = ''
    if ($envContent -match '(?m)^MANIFEST_GIST_URL=(.+)$') { $manifestUrl = $Matches[1].Trim() }
    if ($envContent -match '(?m)^CTETRIS_API_URL=(.+)$') { $apiUrl = $Matches[1].Trim() }

    Write-Info ('[1/3] MANIFEST_GIST_URL: ' + $manifestUrl)
    if ([string]::IsNullOrWhiteSpace($manifestUrl) -or $manifestUrl -match 'GIST_ID|OWNER|<') {
        Write-Err 'MANIFEST_GIST_URL is a placeholder. Set real Gist URL in app/.env.'
        throw 'MANIFEST_GIST_URL not configured'
    }
    try {
        $r = Invoke-WebRequest -Uri $manifestUrl -Method Get -TimeoutSec 15 -UseBasicParsing -ErrorAction Stop
        if ($r.StatusCode -ne 200) { throw ('HTTP ' + $r.StatusCode) }
        Write-Ok ('MANIFEST_GIST_URL OK (HTTP ' + $r.StatusCode + ')')
    } catch {
        Write-Err ('MANIFEST_GIST_URL unreachable: ' + $_)
        Write-Err ('  URL: ' + $manifestUrl)
        throw 'MANIFEST_GIST_URL validation failed'
    }

    Write-Info ('[2/3] CTETRIS_API_URL: ' + $apiUrl)
    if ([string]::IsNullOrWhiteSpace($apiUrl) -or $apiUrl -match 'OWNER|<your') {
        Write-Err 'CTETRIS_API_URL is a placeholder. Deploy Cloudflare Worker and set real URL.'
        throw 'CTETRIS_API_URL not configured'
    }
    try {
        $apiHealthUrl = $apiUrl + '/health'
        $r = Invoke-WebRequest -Uri $apiHealthUrl -Method Get -TimeoutSec 15 -UseBasicParsing -ErrorAction Stop
        if ($r.StatusCode -ne 200) { throw ('HTTP ' + $r.StatusCode) }
        Write-Ok ('CTETRIS_API_URL OK (HTTP ' + $r.StatusCode + ')')
    } catch {
        Write-Err ('CTETRIS_API_URL/health unreachable: ' + $_)
        Write-Err ('  URL: ' + $apiUrl + '/health')
        throw 'CTETRIS_API_URL validation failed'
    }

    Write-Info '[3/3] Checking media files online...'

    $remoteUrl = & git -C $AppDir remote get-url origin 2>$null
    if ([string]::IsNullOrWhiteSpace($remoteUrl)) {
        Write-Warn 'No git remote - skipping media checks'
        return
    }

    $owner = ''
    $repo = ''
    if ($remoteUrl -match 'github\.com[:/]([^/]+)/([^/.]+)(\.git)?$') {
        $owner = $Matches[1]
        $repo = $Matches[2]
    }
    if (-not $owner -or -not $repo) {
        Write-Warn ('Cannot parse owner/repo from ' + $remoteUrl + ' - skipping media checks')
        return
    }

    $chaptersDir = Join-Path $AppDir '..\chapters\src'
    if (-not (Test-Path $chaptersDir)) {
        Write-Warn 'chapters\src not found - skipping media checks'
        return
    }

    $mediaItems = @()
    foreach ($jsonFile in Get-ChildItem -Path $chaptersDir -Recurse -Filter 'c*.json' | Where-Object { $_.FullName -match 'c\d+\\c\d+\.json$' }) {
        $chapterId = $jsonFile.Directory.Name
        $mediaBase = 'https://raw.githubusercontent.com/' + $owner + '/' + $repo + '/main/chapters/src/' + $chapterId + '/media/'
        $content = Get-Content $jsonFile.FullName -Raw
        $pattern = '"(?:thumbnailPath|imageUrl)"\s*:\s*"([^"]+\.(png|jpg|jpeg|gif|webp|svg))"'

        [regex]::Matches($content, $pattern) | ForEach-Object {
            $fn = $_.Groups[1].Value
            if ($fn) {
                $url = $mediaBase + $fn
                if (-not ($mediaItems | Where-Object { $_.Url -eq $url })) {
                    $mediaItems += [PSCustomObject]@{ Url = $url; Label = $chapterId + '/' + $fn }
                }
            }
        }
    }

    $total = $mediaItems.Count
    if ($total -eq 0) {
        Write-Warn 'No media file references found - skipping'
        return
    }

    $doneN = 0
    $failN = 0
    foreach ($item in $mediaItems) {
        $doneN++
        Write-Host ('  [{0}/{1}] {2} ' -f $doneN, $total, $item.Label) -NoNewline
        try {
            $r = Invoke-WebRequest -Uri $item.Url -Method Head -TimeoutSec 10 -UseBasicParsing -ErrorAction Stop
            if ($r.StatusCode -eq 200) {
                Write-Host 'OK' -ForegroundColor Green
            } else {
                Write-Host ('HTTP ' + $r.StatusCode) -ForegroundColor Red
                $failN++
            }
        } catch {
            Write-Host 'MISSING' -ForegroundColor Red
            Write-Warn ('  -> ' + $item.Url)
            $failN++
        }
    }

    if ($failN -gt 0) {
        Write-Err ($failN.ToString() + '/' + $total + ' media files not found on GitHub (' + $owner + '/' + $repo + ').')
        Write-Err 'Commit and push missing files to chapters/src/<chapter>/media/ before building.'
        throw ('Media validation failed: ' + $failN + ' missing files')
    }

    Write-Ok ('All ' + $total + ' media files confirmed online (' + $owner + '/' + $repo + ')')
}

# =============================================================================
# nanosvg -- shared headers in libs\downloads\nanosvg -> download if missing
# =============================================================================
function Initialize-Nanosvg {
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
    if (-not (Test-PythonForEmsdk)) {
        throw 'Python 3 is required for emsdk. Install python manually and ensure `python --version` works.'
    }

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
                if ($LASTEXITCODE -ne 0) { 
                    throw "emsdk install failed (exit $LASTEXITCODE). Ensure Python 3 is installed and in PATH."
                }
                & .\emsdk.bat activate $EmsdkVersion
                if ($LASTEXITCODE -ne 0) { 
                    throw "emsdk activate failed (exit $LASTEXITCODE). Ensure Python 3 is installed and in PATH."
                }
            } finally { Pop-Location }
            Import-EmsdkEnv $userEmsdk
            if (-not (Get-Command emcmake -ErrorAction SilentlyContinue)) {
                throw "emcmake not available after emsdk activation. emsdk may not be properly installed."
            }
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
            if ($LASTEXITCODE -ne 0) { 
                throw "emsdk install failed (exit $LASTEXITCODE). Ensure Python 3 is installed and in PATH."
            }
            & .\emsdk.bat activate $EmsdkVersion
            if ($LASTEXITCODE -ne 0) { 
                throw "emsdk activate failed (exit $LASTEXITCODE). Ensure Python 3 is installed and in PATH."
            }
        } finally { Pop-Location }
        Import-EmsdkEnv $managed
        if (-not (Get-Command emcmake -ErrorAction SilentlyContinue)) {
            throw "emcmake not available after emsdk activation. emsdk may not be properly installed."
        }
        return
    }

    # Priority 4: clone fresh
    Write-Info "Khong tim thay emsdk, clone vao $managed..."
    New-Item -ItemType Directory -Force -Path $DownloadDir | Out-Null
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git $managed
    Push-Location $managed
    try {
        & .\emsdk.bat install $EmsdkVersion
        if ($LASTEXITCODE -ne 0) { 
            throw "emsdk install failed (exit $LASTEXITCODE). Ensure Python 3 is installed and in PATH."
        }
        & .\emsdk.bat activate $EmsdkVersion
        if ($LASTEXITCODE -ne 0) { 
            throw "emsdk activate failed (exit $LASTEXITCODE). Ensure Python 3 is installed and in PATH."
        }
    } finally { Pop-Location }
    Import-EmsdkEnv $managed   # FIX: dung Import-EmsdkEnv thay vi & .ps1
    if (-not (Get-Command emcmake -ErrorAction SilentlyContinue)) {
        throw "emcmake not available after emsdk activation. emsdk may not be properly installed."
    }
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

    # Remove stale CMakeCache to avoid generator mismatch on re-runs
    if (Test-Path $sdlBuild) {
        Remove-Item -Recurse -Force $sdlBuild
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
        if ($needCurl)   { $ids += 'cURL.cURL' }
        Install-WingetPackagesIfMissing -Ids $ids
        if ($needPython) {
            Write-Warn 'Python khong co trong PATH; cai thu cong truoc khi build WASM/emsdk.'
        }
        return
    }

    if (Get-Command choco -ErrorAction SilentlyContinue) {
        $pkgs = @()
        if ($needCmake)  { $pkgs += 'cmake' }
        if ($needGit)    { $pkgs += 'git' }
        if ($needCurl)   { $pkgs += 'curl' }
        Install-ChocoPackagesIfMissing -Packages $pkgs
        if ($needPython) {
            Write-Warn 'Python khong co trong PATH; cai thu cong truoc khi build WASM/emsdk.'
        }
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
        'CMakeLists.txt',
        'src\logger.h'
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
# Validate Windows console-hiding fixes in CMakeLists.txt
# (auto-applied on fresh clone to ensure console hidden on Windows)
# =============================================================================
function Validate-WindowsFixes {
    $cmakelists = Join-Path $AppDir 'CMakeLists.txt'
    $content = Get-Content $cmakelists -Raw
    
    $hasWin32Flag = $content -match 'add_executable\s*\(\s*cTetris\s+WIN32'
    $hasSubsystemFlag = $content -match '/SUBSYSTEM:WINDOWS'
    $hasEntryPoint = $content -match '/ENTRY:mainCRTStartup'
    $hasSdkPaths = $content -match 'Windows Kits/10/Lib'
    
    if ($hasWin32Flag -and $hasSubsystemFlag -and $hasEntryPoint -and $hasSdkPaths) {
        Write-Ok 'Windows console-hiding fixes validated in CMakeLists.txt'
        return
    }
    
    Write-Info 'Detecting Windows console-hiding fixes status:'
    if (-not $hasWin32Flag) { Write-Warn '  - WIN32 flag missing in add_executable()' }
    if (-not $hasSubsystemFlag) { Write-Warn '  - /SUBSYSTEM:WINDOWS linker flag missing' }
    if (-not $hasEntryPoint) { Write-Warn '  - /ENTRY:mainCRTStartup linker flag missing' }
    if (-not $hasSdkPaths) { Write-Warn '  - Windows SDK paths not configured' }
    
    Write-Info 'Auto-applying Windows console-hiding fixes to CMakeLists.txt...'
    
    # Fix 1: Add WIN32 flag to add_executable if missing
    if (-not $hasWin32Flag) {
        Write-Info '  Applying: add_executable WIN32 flag...'
        $content = $content -replace `
            '(add_executable\s*\(\s*cTetris)\s+(\$\{GAME_SOURCES\})', `
            '${1} WIN32 ${2}'
    }
    
    # Fix 2: Add Windows-specific linker configuration after target_include_directories
    if (-not ($hasSubsystemFlag -and $hasEntryPoint)) {
        Write-Info '  Applying: Windows linker flags (/SUBSYSTEM, /ENTRY)...'
        $linkerConfig = @'
# Add Windows SDK paths for linking on Windows
if(WIN32)
    target_link_directories(cTetris PRIVATE
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64"
    )
    # Use main() as entry point even with Windows subsystem (hides console)
    target_link_options(cTetris PRIVATE 
        /SUBSYSTEM:WINDOWS 
        /ENTRY:mainCRTStartup
    )
endif()
'@
        # Insert after target_include_directories line
        $content = $content -replace `
            '(target_include_directories\(cTetris PRIVATE \$\{GAME_INCLUDE_DIRS\})', `
            "`$1`n`n$linkerConfig"
    }
    
    Set-Content -Path $cmakelists -Value $content -Encoding UTF8
    Write-Ok 'CMakeLists.txt updated with Windows console-hiding fixes'
}

# =============================================================================
# Validate logger.h exists and is properly integrated
# =============================================================================
function Validate-LoggerIntegration {
    $loggerHeader = Join-Path $AppDir 'src\logger.h'
    if (-not (Test-Path $loggerHeader)) {
        Write-Err 'logger.h not found - logging system missing'
        throw 'Logger integration incomplete'
    }
    
    $mainCpp = Join-Path $AppDir 'main.cpp'
    $mainContent = Get-Content $mainCpp -Raw
    
    if ($mainContent -match '#include\s+"logger\.h"' -and $mainContent -match 'Logger::getInstance') {
        Write-Ok 'Logger integration validated in main.cpp'
        return
    }
    
    Write-Warn 'Logger not fully integrated in main.cpp'
    Write-Info 'Ensure main.cpp includes: #include "logger.h"'
    Write-Info 'And initializes: Logger& logger = Logger::getInstance();'
}

# =============================================================================
# Show Python server guide for running WASM build on localhost
# =============================================================================
function Show-PythonServerGuide {
    param([string]$BuildDir)
    
    Write-Host ""
    Write-Host "================================================================================" -ForegroundColor Cyan
    Write-Host "WASM BUILD COMPLETE! Run on localhost with Python HTTP server:" -ForegroundColor Cyan
    Write-Host "================================================================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Step 1: OPEN NEW PowerShell terminal and navigate to build directory" -ForegroundColor Yellow
    Write-Host "  cd `"$BuildDir`"" -ForegroundColor White
    Write-Host ""
    Write-Host "  Full path (if needed):" -ForegroundColor Gray
    Write-Host "  cd D:\projects\ctetris-2\app\build\wasm\windows" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Step 2: Verify files exist in current directory" -ForegroundColor Yellow
    Write-Host "  dir" -ForegroundColor White
    Write-Host "  (You should see: cTetris.html, cTetris.js, cTetris.wasm)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Step 3: Start Python HTTP server (pick ONE command):" -ForegroundColor Yellow
    Write-Host "  python -m http.server 8000" -ForegroundColor White
    Write-Host "  OR" -ForegroundColor Gray
    Write-Host "  py -m http.server 8000" -ForegroundColor White
    Write-Host ""
    Write-Host "  Expected output: 'Serving HTTP on :: port 8000'" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Step 4: Open browser and navigate to:" -ForegroundColor Yellow
    Write-Host "  http://localhost:8000/cTetris.html" -ForegroundColor White
    Write-Host ""
    Write-Host "Step 5: Stop server when done" -ForegroundColor Yellow
    Write-Host "  Press Ctrl+C in the server terminal" -ForegroundColor White
    Write-Host ""
    Write-Host "================================================================================" -ForegroundColor Cyan
    Write-Host ""
}

# =============================================================================
# Show Windows guidance when Smart App Control blocks the native EXE
# =============================================================================
function Show-SmartAppControlGuide {
    param([string]$ExePath)

    Write-Host ""
    Write-Host "================================================================================" -ForegroundColor Yellow
    Write-Host "WINDOWS SMART APP CONTROL NOTICE:" -ForegroundColor Yellow
    Write-Host "================================================================================" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "If Windows blocks $ExePath with Smart App Control or SmartScreen, the real fix is code signing." -ForegroundColor White
    Write-Host "This build is unsigned unless a code-signing certificate was configured, so Windows may not trust it yet." -ForegroundColor White
    Write-Host ""
    Write-Host "Recommended next steps:" -ForegroundColor Yellow
    Write-Host "  1. Configure a real Authenticode code-signing certificate and rebuild." -ForegroundColor White
    Write-Host "  2. For local development only, run the EXE from this workspace on a trusted machine." -ForegroundColor White
    Write-Host "  3. If you are testing your own build, use the file directly from the build output folder." -ForegroundColor White
    Write-Host ""
    Write-Host "Build output location:" -ForegroundColor Yellow
    Write-Host "  $ExePath" -ForegroundColor White
    Write-Host ""
    Write-Host "================================================================================" -ForegroundColor Yellow
    Write-Host ""
}

# =============================================================================
# Optional Windows code signing for Smart App Control / SmartScreen trust.
# Requires a real code-signing certificate.
# Supported inputs:
#   CTETRIS_SIGN_CERT_PATH : path to a .pfx file
#   CTETRIS_SIGN_CERT_PWD   : password for the .pfx file
#   CTETRIS_SIGN_TIMESTAMP  : timestamp server URL
# =============================================================================
function Sign-WindowsBinary {
    param([string]$ExePath)

    if ($env:OS -ne 'Windows_NT') { return }
    if (-not (Test-Path $ExePath)) {
        Write-Warn "Signing skipped: EXE not found at $ExePath"
        return
    }

    $certPath = $env:CTETRIS_SIGN_CERT_PATH
    if ([string]::IsNullOrWhiteSpace($certPath)) {
        Write-Warn 'No CTETRIS_SIGN_CERT_PATH configured; native EXE remains unsigned.'
        return
    }
    if (-not (Test-Path $certPath)) {
        Write-Warn "Signing skipped: certificate not found at $certPath"
        return
    }

    $signtool = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if (-not $signtool) {
        Write-Warn 'Signing skipped: signtool.exe not found in PATH. Install Windows SDK signing tools.'
        return
    }

    $timestamp = $env:CTETRIS_SIGN_TIMESTAMP
    if ([string]::IsNullOrWhiteSpace($timestamp)) {
        $timestamp = 'http://timestamp.digicert.com'
    }

    $args = @('sign', '/fd', 'SHA256', '/f', $certPath)
    if (-not [string]::IsNullOrWhiteSpace($env:CTETRIS_SIGN_CERT_PWD)) {
        $args += @('/p', $env:CTETRIS_SIGN_CERT_PWD)
    }
    if (-not [string]::IsNullOrWhiteSpace($timestamp)) {
        $args += @('/tr', $timestamp, '/td', 'SHA256')
    }
    $args += $ExePath

    Write-Info "Signing native EXE with Authenticode certificate: $certPath"
    & $signtool.Path @args
    if ($LASTEXITCODE -ne 0) {
        throw "signtool sign failed (exit $LASTEXITCODE)"
    }

    Write-Ok 'Native EXE signed successfully'
}

# =============================================================================
# Build entry points
# =============================================================================
function Build-Native {
    Validate-Endpoints
    Write-Info "Build NATIVE -> $BuildNativeDir"
    Test-Sources
    Validate-WindowsFixes
    Validate-LoggerIntegration

    # FIX: Load MSVC compiler environment truoc khi lam bat cu thu gi voi cmake
    Import-VsEnv

    Initialize-Sdl3Native
    Initialize-Nanosvg
    Initialize-Nlohmann   # FIX: gameConsole/app.cpp requires nlohmann/json.hpp
    Initialize-Sqlite   # FIX 2.6.1: SQLite for Stories DB

    # Copy icon from brandkit to build output
    Copy-DesktopIcon -OutDir $BuildNativeDir

    # Tim path SDL3Config.cmake -- Windows installs to cmake\ OR lib\cmake\SDL3
    $sdlInstall = Join-Path $DownloadDir 'sdl3-native'
    $sdlDirArgs = @()
    foreach ($cand in @(
        (Join-Path $sdlInstall 'cmake'),
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
        "-DNANOSVG_INCLUDE_DIR=$(Join-Path $DownloadDir 'nanosvg')",
        "-DNLOHMANN_INCLUDE_DIR=$(Join-Path $DownloadDir 'nlohmann')",
        "-DSQLITE_DIR=$(Join-Path $DownloadDir 'sqlite')"
    ) + $sdlDirArgs + $iconArg
    & cmake @nativeArgs
    if ($LASTEXITCODE -ne 0) { throw "Native configure failed (exit $LASTEXITCODE)" }
    & cmake --build $BuildNativeDir --config Release -j
    if ($LASTEXITCODE -ne 0) { throw "Native build failed (exit $LASTEXITCODE)" }


    Write-Ok "Native build hoan tat: $BuildNativeDir"
    Sign-WindowsBinary -ExePath (Join-Path $BuildNativeDir 'cTetris.exe')
    Show-SmartAppControlGuide -ExePath (Join-Path $BuildNativeDir 'cTetris.exe')
}

function Build-Wasm {
    Validate-Endpoints
    Write-Info "Build WASM -> $BuildWasmDir"
    Test-Sources
    Validate-LoggerIntegration
    Initialize-WindowsTools

    # Detect version SDL3 native truoc -- WASM build se MATCH version do
    # FIX: wrap try/catch -- neu khong co native SDL3 thi dung pin $Sdl3Version
    Write-Info 'Detect SDL3 native version de match cho WASM (best effort)...'
    try { Initialize-Sdl3Native }
    catch { Write-Warn "Khong detect duoc SDL3 native -- dung version pin $Sdl3Version" }

    Initialize-Emsdk
    Initialize-Sdl3Wasm
    Initialize-Nanosvg
    Initialize-Nlohmann   # FIX: gameConsole/app.cpp requires nlohmann/json.hpp
    Initialize-Sqlite   # FIX 2.6.1: SQLite for Stories DB

    # Derive sdl_install path tu version detect duoc
    $targetVersion = if ($script:DetectedSdl3Version) { $script:DetectedSdl3Version } else { $Sdl3Version }
    $sdlInstall = Join-Path $DownloadDir "sdl3-wasm-$targetVersion"

    $sdlDir = $null
    foreach ($cand in @(
        (Join-Path $sdlInstall 'cmake'),
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
        "-DNANOSVG_INCLUDE_DIR=$(Join-Path $DownloadDir 'nanosvg')",
        "-DNLOHMANN_INCLUDE_DIR=$(Join-Path $DownloadDir 'nlohmann')",
        "-DSQLITE_DIR=$(Join-Path $DownloadDir 'sqlite')"
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
    Show-PythonServerGuide -BuildDir $BuildWasmDir
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