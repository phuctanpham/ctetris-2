#!/usr/bin/env bash
# =============================================================================
# build.sh -- Bash build script cho cTetris (macOS / Linux)
# =============================================================================

set -e

# -----------------------------------------------------------------------------
# Detect OS
# -----------------------------------------------------------------------------
detect_os() {
    case "$(uname -s)" in
        Darwin) echo "macos" ;;
        Linux)
            if [ -r /etc/os-release ]; then
                . /etc/os-release
                case "${ID:-unknown}" in
                    ubuntu|debian|linuxmint|pop)        echo "ubuntu" ;;
                    fedora|rhel|centos|rocky|almalinux) echo "fedora" ;;
                    arch|manjaro|endeavouros)           echo "arch" ;;
                    alpine)                             echo "alpine" ;;
                    opensuse*|sles)                     echo "opensuse" ;;
                    *)                                  echo "linux" ;;
                esac
            else
                echo "linux"
            fi
            ;;
        *) echo "unknown" ;;
    esac
}

OS_NAME="$(detect_os)"
APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBS_DIR="$APP_DIR/libs/$OS_NAME"
DOWNLOAD_DIR="$LIBS_DIR/downloads"
BUILD_NATIVE_DIR="$APP_DIR/build/desktop/$OS_NAME"
BUILD_WASM_DIR="$APP_DIR/build/wasm/$OS_NAME"
BRANDKIT_DIR="$APP_DIR/brandkit"
BRAND_LOGO_SVG="$BRANDKIT_DIR/logo.svg"
WEB_DIR="$APP_DIR/web"

CMAKE_MIN_VERSION="3.16"
EMSDK_VERSION="3.1.72"
SDL3_VERSION_MIN="3.2.0"
SDL3_VERSION="3.2.18"
NLOHMANN_VERSION="3.11.3" # 
SQLITE_VERSION="3460100"
SQLITE_YEAR="2024"
DETECTED_SDL3_VERSION=""
BUILD_MODE="${1:-}"

# -----------------------------------------------------------------------------
# Logging & Helpers
# -----------------------------------------------------------------------------
log_info()  { echo -e "\033[0;36m[INFO]\033[0m  $*"; }
log_warn()  { echo -e "\033[0;33m[WARN]\033[0m  $*"; }
log_error() { echo -e "\033[0;31m[ERROR]\033[0m $*"; }
log_ok()    { echo -e "\033[0;32m[OK]\033[0m    $*"; }

version_ge() {
    [ -z "$2" ] && return 0
    [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | head -n1)" = "$2" ]
}

check_cmd_version() {
    local cmd="$1" min="$2" cur
    command -v "$cmd" >/dev/null 2>&1 || return 1
    cur=$("$cmd" --version 2>&1 | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
    [ -z "$cur" ] && return 0
    version_ge "$cur" "$min"
}

# =============================================================================
# Dependencies Management
# =============================================================================

ensure_nanosvg() {
    local nano_dir="$DOWNLOAD_DIR/nanosvg"
    if [ -f "$nano_dir/nanosvg.h" ]; then return 0; fi

    log_info "Tai nanosvg headers..."
    mkdir -p "$nano_dir"
    curl -fsSL "https://raw.githubusercontent.com/memononen/nanosvg/master/src/nanosvg.h" -o "$nano_dir/nanosvg.h"
    curl -fsSL "https://raw.githubusercontent.com/memononen/nanosvg/master/src/nanosvgrast.h" -o "$nano_dir/nanosvgrast.h"
}

# [C.1] nlohmann/json integration [cite: 21, 26]
ensure_nlohmann() {
    local n_root="$DOWNLOAD_DIR/nlohmann"
    local n_file="$n_root/nlohmann/json.hpp"

    if [ -f "$APP_DIR/src/gameStory/include/nlohmann/json.hpp" ]; then
        log_ok "nlohmann/json da co trong source tree (vendored)" [cite: 27]
        return 0
    fi

    if [ -f "$n_file" ]; then
        log_ok "nlohmann/json da co tai $n_file" [cite: 28]
        return 0
    fi

    log_info "Tai nlohmann/json $NLOHMANN_VERSION vao $n_file..."
    mkdir -p "$n_root/nlohmann"
    local url="https://raw.githubusercontent.com/nlohmann/json/v${NLOHMANN_VERSION}/single_include/nlohmann/json.hpp"
    curl -fsSL "$url" -o "$n_file"

    local size
    size=$(stat -c%s "$n_file" 2>/dev/null || stat -f%z "$n_file" 2>/dev/null)
    if [ -z "$size" ] || [ "$size" -lt 102400 ]; then
        log_error "nlohmann/json.hpp tai ve nho bat thuong ($size bytes) -- xoa va abort" [cite: 29]
        rm -f "$n_file"
        return 1
    fi
    log_ok "nlohmann/json $NLOHMANN_VERSION san sang tai $n_file ($size bytes)"
}

# SQLite amalgamation -- download zip, extract sqlite3.c + sqlite3.h
ensure_sqlite() {
    local sqlite_dir="$DOWNLOAD_DIR/sqlite"
    local sqlite_c="$sqlite_dir/sqlite3.c"
    local sqlite_h="$sqlite_dir/sqlite3.h"

    if [ -f "$sqlite_c" ] && [ -f "$sqlite_h" ]; then
        local size
        size=$(stat -c%s "$sqlite_c" 2>/dev/null || stat -f%z "$sqlite_c" 2>/dev/null)
        if [ -n "$size" ] && [ "$size" -gt 5242880 ]; then
            log_ok "SQLite amalgamation da co tai $sqlite_dir ($((size/1048576)) MB)"
            return 0
        fi
        log_warn "sqlite3.c kich thuoc bat thuong ($size bytes) -- re-download"
    fi

    log_info "Tai SQLite amalgamation $SQLITE_VERSION vao $sqlite_dir..."
    mkdir -p "$sqlite_dir"

    local zip_url="https://sqlite.org/$SQLITE_YEAR/sqlite-amalgamation-$SQLITE_VERSION.zip"
    local zip_path="$sqlite_dir/sqlite-amalgamation.zip"
    local tmp_extract="$sqlite_dir/tmp_extract"

    curl -fsSL "$zip_url" -o "$zip_path" || {
        log_error "SQLite download fail tu $zip_url"
        return 1
    }

    rm -rf "$tmp_extract"
    mkdir -p "$tmp_extract"
    unzip -q "$zip_path" -d "$tmp_extract" || {
        log_error "SQLite unzip fail"
        return 1
    }

    cp -f "$tmp_extract/sqlite-amalgamation-$SQLITE_VERSION/sqlite3.c" "$sqlite_c"
    cp -f "$tmp_extract/sqlite-amalgamation-$SQLITE_VERSION/sqlite3.h" "$sqlite_h"

    rm -f "$zip_path"
    rm -rf "$tmp_extract"

    local size
    size=$(stat -c%s "$sqlite_c" 2>/dev/null || stat -f%z "$sqlite_c" 2>/dev/null)
    if [ -z "$size" ] || [ "$size" -lt 5242880 ]; then
        log_error "sqlite3.c tai ve nho bat thuong ($size bytes) -- abort"
        rm -f "$sqlite_c" "$sqlite_h"
        return 1
    fi
    log_ok "SQLite amalgamation $SQLITE_VERSION san sang ($((size/1048576)) MB)"
}

# =============================================================================
# libcurl -- native HTTP sync (manifest, leaderboard, record submit).
# Detection first; install via OS package manager only when missing.
# find_package(CURL) in CMakeLists.txt picks it up on its own (system installs).
# macOS brew-keg-only path is appended to CMAKE_PREFIX_PATH inside build_native.
# =============================================================================
ensure_curl() {
    if command -v curl-config >/dev/null 2>&1; then
        local ver
        ver=$(curl-config --version 2>/dev/null | awk '{print $2}')
        log_ok "libcurl ${ver:-detected} via curl-config"
        return 0
    fi
    if command -v pkg-config >/dev/null 2>&1 && \
       pkg-config --exists libcurl 2>/dev/null; then
        log_ok "libcurl detected via pkg-config"
        return 0
    fi

    log_warn "libcurl dev headers not found -- attempting install (OS=$OS_NAME)..."
    case "$OS_NAME" in
        macos)
            if command -v brew >/dev/null 2>&1; then
                brew install curl || log_warn "brew install curl failed"
            else
                log_error "Install Homebrew (https://brew.sh), then: brew install curl"
                return 1
            fi
            ;;
        ubuntu)
            if command -v apt-get >/dev/null 2>&1; then
                sudo apt-get update -qq
                sudo apt-get install -y libcurl4-openssl-dev || \
                    log_warn "apt-get install libcurl4-openssl-dev failed"
            fi
            ;;
        fedora)
            sudo dnf install -y libcurl-devel || \
                log_warn "dnf install libcurl-devel failed"
            ;;
        arch)
            sudo pacman -S --noconfirm curl || \
                log_warn "pacman -S curl failed"
            ;;
        alpine)
            sudo apk add curl-dev || log_warn "apk add curl-dev failed"
            ;;
        opensuse)
            sudo zypper install -y libcurl-devel || \
                log_warn "zypper install libcurl-devel failed"
            ;;
        *)
            log_error "Unknown OS '$OS_NAME' -- install libcurl dev package manually"
            return 1
            ;;
    esac

    if command -v curl-config >/dev/null 2>&1 || \
       (command -v pkg-config >/dev/null 2>&1 && \
        pkg-config --exists libcurl 2>/dev/null); then
        log_ok "libcurl installed successfully"
    else
        log_warn "libcurl install finished but detection still failed -- find_package(CURL) may not see it"
    fi
    return 0   # do not hard-fail; CMakeLists.txt warns + falls back to offline
}

# =============================================================================
# validate_endpoints -- check MANIFEST_GIST_URL + CTETRIS_API_URL + media
# Mirrors the game's loading-bar progress: shows [N/total] for each file.
# Stops the build (exit 1) on first failure.
# =============================================================================
validate_endpoints() {
    log_info "=== Endpoint & Media Validation ==="

    local env_file="$APP_DIR/.env"
    if [ ! -f "$env_file" ]; then
        log_error ".env not found at $env_file — create it with MANIFEST_GIST_URL and CTETRIS_API_URL"
        exit 1
    fi

    local manifest_url api_url
    manifest_url=$(grep -E '^MANIFEST_GIST_URL=' "$env_file" | cut -d'=' -f2- | tr -d '[:space:]')
    api_url=$(grep -E '^CTETRIS_API_URL=' "$env_file" | cut -d'=' -f2- | tr -d '[:space:]')

    # ── 1/3: MANIFEST_GIST_URL ────────────────────────────────────────────
    log_info "[1/3] MANIFEST_GIST_URL: $manifest_url"
    if [ -z "$manifest_url" ] || echo "$manifest_url" | grep -qE 'GIST_ID|OWNER|<'; then
        log_error "MANIFEST_GIST_URL is a placeholder. Set real Gist URL in app/.env and push sync-chapters.yml."
        exit 1
    fi
    local code
    code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 15 "$manifest_url" 2>/dev/null || echo "000")
    if [ "$code" != "200" ]; then
        log_error "MANIFEST_GIST_URL returned HTTP $code — Gist may be private or URL wrong."
        log_error "  URL: $manifest_url"
        exit 1
    fi
    log_ok "MANIFEST_GIST_URL OK (HTTP $code)"

    # ── 2/3: CTETRIS_API_URL ──────────────────────────────────────────────
    log_info "[2/3] CTETRIS_API_URL: $api_url"
    if [ -z "$api_url" ] || echo "$api_url" | grep -qE 'OWNER|<your'; then
        log_error "CTETRIS_API_URL is a placeholder. Deploy Cloudflare Worker and set real URL in app/.env."
        exit 1
    fi
    code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 15 "$api_url/health" 2>/dev/null || echo "000")
    if [ "$code" != "200" ]; then
        log_error "CTETRIS_API_URL/health returned HTTP $code — Worker may not be deployed."
        log_error "  URL: $api_url/health"
        exit 1
    fi
    log_ok "CTETRIS_API_URL OK (HTTP $code)"

    # ── 3/3: Media files — HEAD check on GitHub raw ───────────────────────
    log_info "[3/3] Checking media files online..."

    # Derive owner/repo from git remote (https or ssh)
    local remote owner repo
    remote=$(git -C "$APP_DIR" remote get-url origin 2>/dev/null || echo "")
    if [ -z "$remote" ]; then
        log_warn "No git remote detected — skipping media file checks"
        return 0
    fi
    owner=$(echo "$remote" | sed -E 's|.*github\.com[:/]([^/]+)/.*|\1|')
    repo=$(echo "$remote"  | sed -E 's|.*github\.com[:/][^/]+/([^/.]+)(\.git)?$|\1|')
    if [ -z "$owner" ] || [ -z "$repo" ]; then
        log_warn "Cannot parse owner/repo from remote '$remote' — skipping media checks"
        return 0
    fi

    local chapters_dir="$APP_DIR/../chapters/src"
    if [ ! -d "$chapters_dir" ]; then
        log_warn "chapters/src not found at $chapters_dir — skipping media checks"
        return 0
    fi

    # Build list: "<url> <chapter_id/filename>" lines into temp file
    local tmp_list
    tmp_list=$(mktemp)
    local json_file chapter_id media_base filename

    for json_file in $(find "$chapters_dir" -maxdepth 2 -name "c*.json" \
                        | grep -E 'c[0-9]+/c[0-9]+\.json$' | sort); do
        chapter_id=$(basename "$(dirname "$json_file")")
        media_base="https://raw.githubusercontent.com/$owner/$repo/main/chapters/src/$chapter_id/media/"

        # Extract thumbnailPath values (non-empty, ending in image extension)
        grep -oE '"thumbnailPath"[[:space:]]*:[[:space:]]*"[^"]+"' "$json_file" \
            | sed -E 's/.*"thumbnailPath"[[:space:]]*:[[:space:]]*"([^"]+)".*/\1/' \
            | grep -E '\.(png|jpg|jpeg|gif|webp|svg)$' \
            | sort -u \
            | while IFS= read -r fn; do
                echo "${media_base}${fn} ${chapter_id}/${fn}"
              done >> "$tmp_list"

        # Extract imageUrl values (non-empty, ending in image extension)
        grep -oE '"imageUrl"[[:space:]]*:[[:space:]]*"[^"]+"' "$json_file" \
            | sed -E 's/.*"imageUrl"[[:space:]]*:[[:space:]]*"([^"]+)".*/\1/' \
            | grep -E '\.(png|jpg|jpeg|gif|webp|svg)$' \
            | sort -u \
            | while IFS= read -r fn; do
                echo "${media_base}${fn} ${chapter_id}/${fn}"
              done >> "$tmp_list"
    done

    # Deduplicate
    sort -u "$tmp_list" -o "$tmp_list"

    local total done_n fail_n
    total=$(grep -c . "$tmp_list" 2>/dev/null || echo 0)

    if [ "$total" -eq 0 ]; then
        log_warn "No media file references found in chapter JSONs — skipping"
        rm -f "$tmp_list"
        return 0
    fi

    done_n=0
    fail_n=0

    while IFS=' ' read -r url label; do
        done_n=$((done_n + 1))
        printf "  [%d/%d] %s " "$done_n" "$total" "$label"
        code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 -I "$url" 2>/dev/null || echo "000")
        if [ "$code" = "200" ]; then
            printf "OK\n"
        else
            printf "MISSING (HTTP %s)\n" "$code"
            log_error "  -> $url"
            fail_n=$((fail_n + 1))
        fi
    done < "$tmp_list"

    rm -f "$tmp_list"

    if [ "$fail_n" -gt 0 ]; then
        log_error "$fail_n/$total media files not found on GitHub."
        log_error "Commit and push the missing files to chapters/src/<chapter>/media/ before building."
        exit 1
    fi

    log_ok "All $total media files confirmed online ($owner/$repo)"
}

ensure_emsdk() {
    if command -v emcmake >/dev/null 2>&1 && command -v em++ >/dev/null 2>&1; then
        return 0
    fi

    local emsdk_root="${EMSDK:-$HOME/emsdk}"
    if [ -f "$emsdk_root/emsdk_env.sh" ]; then
        # shellcheck disable=SC1090
        . "$emsdk_root/emsdk_env.sh" >/dev/null 2>&1 || true
    fi

    if command -v emcmake >/dev/null 2>&1 && command -v em++ >/dev/null 2>&1; then
        return 0
    fi

    log_error "emsdk khong san sang -- can emcmake/em++ de build WASM"
    return 1
}

build_sdl3_from_source() {
    local target="$1"
    local version="$2"
    local install_prefix="$3"

    local sdl_src="$DOWNLOAD_DIR/SDL-$version"
    local sdl_build="$sdl_src/build-$target"

    mkdir -p "$DOWNLOAD_DIR"
    if [ ! -d "$sdl_src/.git" ]; then
        rm -rf "$sdl_src"
        log_info "Clone SDL3 release-$version vao $sdl_src..."
        git clone --depth 1 --branch "release-$version" https://github.com/libsdl-org/SDL "$sdl_src"
    fi

    rm -rf "$sdl_build"
    mkdir -p "$sdl_build"

    local -a cmake_args=(
        -S "$sdl_src"
        -B "$sdl_build"
        -G Ninja
        -DCMAKE_INSTALL_PREFIX="$install_prefix"
        -DSDL_INSTALL=ON
        -DSDL_TESTS=OFF
        -DSDL_EXAMPLES=OFF
    )

    if [ "$target" = "wasm" ]; then
        cmake_args+=(-DSDL_SHARED=OFF -DSDL_STATIC=ON)
        emcmake cmake "${cmake_args[@]}"
    else
        cmake_args+=(-DSDL_SHARED=ON -DSDL_STATIC=ON)
        cmake "${cmake_args[@]}"
    fi

    cmake --build "$sdl_build" -j
    cmake --install "$sdl_build"
}

ensure_sdl3_native() {
    local sdl_install="$DOWNLOAD_DIR/sdl3-native"
    local sdl_config=""

    for cand in \
        "$sdl_install/cmake" \
        "$sdl_install/lib/cmake/SDL3" \
        "$sdl_install/lib64/cmake/SDL3"; do
        if [ -f "$cand/SDL3Config.cmake" ]; then
            sdl_config="$cand"
            break
        fi
    done

    if [ -n "$sdl_config" ]; then
        log_ok "SDL3 native da co tai $sdl_config"
        return 0
    fi

    local target_version="${DETECTED_SDL3_VERSION:-$SDL3_VERSION}"
    DETECTED_SDL3_VERSION="$target_version"
    log_info "Build SDL3 $target_version cho native vao $sdl_install..."
    build_sdl3_from_source native "$target_version" "$sdl_install"
}

ensure_sdl3_wasm() {
    local target_version="${DETECTED_SDL3_VERSION:-$SDL3_VERSION}"
    local sdl_install="$DOWNLOAD_DIR/sdl3-wasm-$target_version"
    local sdl_config=""

    for cand in \
        "$sdl_install/cmake" \
        "$sdl_install/lib/cmake/SDL3" \
        "$sdl_install/lib64/cmake/SDL3"; do
        if [ -f "$cand/SDL3Config.cmake" ]; then
            sdl_config="$cand"
            break
        fi
    done

    if [ -n "$sdl_config" ]; then
        log_ok "SDL3 wasm da co tai $sdl_config"
        return 0
    fi

    log_info "Build SDL3 $target_version cho WASM vao $sdl_install..."
    build_sdl3_from_source wasm "$target_version" "$sdl_install"
}

# =============================================================================
# Build entry points
# =============================================================================

build_native() {
    validate_endpoints
    log_info "Build NATIVE mode -> $BUILD_NATIVE_DIR"
    # validate_sources (giả định hàm này đã được định nghĩa ở trên)
    ensure_sdl3_native
    ensure_nanosvg
    ensure_nlohmann # [cite: 23]
    ensure_sqlite   # FIX 2.6.1: SQLite for Stories DB
    ensure_curl     # libcurl for native HTTP sync (manifest + API)

    # ... [Xử lý icon và SDL3_DIR giống như file gốc] ...

    local sdl_install="$DOWNLOAD_DIR/sdl3-native"
    local sdl_dir=""
    local -a sdl_dir_arg=()
    for cand in \
        "$sdl_install/cmake" \
        "$sdl_install/lib/cmake/SDL3" \
        "$sdl_install/lib64/cmake/SDL3"; do
        if [ -f "$cand/SDL3Config.cmake" ]; then
            sdl_dir="$cand"
            sdl_dir_arg=(-DSDL3_DIR="$sdl_dir")
            break
        fi
    done

    # Build CMAKE_PREFIX_PATH (';'-separated CMake list).
    # macOS Homebrew curl is keg-only; append its opt prefix when present
    # so find_package(CURL) discovers it without env vars.
    local prefix_list="$sdl_install"
    if [ "$OS_NAME" = "macos" ]; then
        for cand in /opt/homebrew/opt/curl /usr/local/opt/curl; do
            if [ -d "$cand" ]; then
                prefix_list="$prefix_list;$cand"
                log_info "macOS brew-keg curl: $cand"
                break
            fi
        done
    fi

    mkdir -p "$BUILD_NATIVE_DIR"
    cmake -S "$APP_DIR" -B "$BUILD_NATIVE_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_WASM=OFF \
          -DCMAKE_PREFIX_PATH="$prefix_list" \
          -DNANOSVG_INCLUDE_DIR="$DOWNLOAD_DIR/nanosvg" \
          -DNLOHMANN_INCLUDE_DIR="$DOWNLOAD_DIR/nlohmann" \
          -DSQLITE_DIR="$DOWNLOAD_DIR/sqlite" \
          "${sdl_dir_arg[@]}" \
          "${icon_arg[@]}" # [cite: 8, 9]

    cmake --build "$BUILD_NATIVE_DIR" -j
}

build_wasm() {
    validate_endpoints
    log_info "Build WASM mode -> $BUILD_WASM_DIR"
    ensure_sdl3_native || true
    ensure_emsdk
    ensure_sdl3_wasm
    ensure_nanosvg
    ensure_nlohmann # [cite: 30]
    ensure_sqlite   # FIX 2.6.1: SQLite for Stories DB

    # ... [Xử lý sdl_dir giống như file gốc] ...

    local target_version="${DETECTED_SDL3_VERSION:-$SDL3_VERSION}"
    local sdl_install="$DOWNLOAD_DIR/sdl3-wasm-$target_version"
    local sdl_dir=""
    local -a sdl_dir_arg=()
    for cand in \
        "$sdl_install/cmake" \
        "$sdl_install/lib/cmake/SDL3" \
        "$sdl_install/lib64/cmake/SDL3"; do
        if [ -f "$cand/SDL3Config.cmake" ]; then
            sdl_dir="$cand"
            sdl_dir_arg=(-DSDL3_DIR="$sdl_dir")
            break
        fi
    done

    mkdir -p "$BUILD_WASM_DIR"
    emcmake cmake -S "$APP_DIR" -B "$BUILD_WASM_DIR" \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DBUILD_WASM=ON \
                  -DCMAKE_PREFIX_PATH="$sdl_install" \
                  -DNANOSVG_INCLUDE_DIR="$DOWNLOAD_DIR/nanosvg" \
                  -DNLOHMANN_INCLUDE_DIR="$DOWNLOAD_DIR/nlohmann" \
                  -DSQLITE_DIR="$DOWNLOAD_DIR/sqlite" \
                  "${sdl_dir_arg[@]}" # [cite: 10, 11]
    cmake --build "$BUILD_WASM_DIR" -j

    # ... [Copy favicon và PWA assets giống như file gốc] ...
}

# -----------------------------------------------------------------------------
# Execution
# -----------------------------------------------------------------------------
case "$BUILD_MODE" in
    native)    build_native ;;
    wasm)      build_wasm ;;
    all)       build_native; build_wasm ;;
    clean)     rm -rf "$APP_DIR/build" ;;
    deepclean) rm -rf "$APP_DIR/build" "$APP_DIR/libs" ;;
    *)         echo "Usage: $0 [native|wasm|all|clean|deepclean]"; exit 1 ;;
esac