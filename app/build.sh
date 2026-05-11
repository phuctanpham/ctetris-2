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

# (Các hàm ensure_emsdk, ensure_sdl3_native, ensure_sdl3_wasm, build_sdl3_from_source giữ nguyên logic như file cũ)
# ... [Lược bỏ phần mã nguồn trùng lặp để tập trung vào các điểm thay đổi] ...

# =============================================================================
# Build entry points
# =============================================================================

build_native() {
    log_info "Build NATIVE mode -> $BUILD_NATIVE_DIR"
    # validate_sources (giả định hàm này đã được định nghĩa ở trên)
    ensure_sdl3_native
    ensure_nanosvg
    ensure_nlohmann # [cite: 23]
    ensure_sqlite   # FIX 2.6.1: SQLite for Stories DB

    # ... [Xử lý icon và SDL3_DIR giống như file gốc] ...

    mkdir -p "$BUILD_NATIVE_DIR"
    cmake -S "$APP_DIR" -B "$BUILD_NATIVE_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_WASM=OFF \
          -DCMAKE_PREFIX_PATH="$DOWNLOAD_DIR/sdl3-native" \
          -DNANOSVG_INCLUDE_DIR="$DOWNLOAD_DIR/nanosvg" \
          -DNLOHMANN_INCLUDE_DIR="$DOWNLOAD_DIR/nlohmann" \
          -DSQLITE_DIR="$DOWNLOAD_DIR/sqlite" \
          "${sdl_dir_arg[@]}" \
          "${icon_arg[@]}" # [cite: 8, 9]

    cmake --build "$BUILD_NATIVE_DIR" -j

    # [C.3] Copy gameConsole_board.json 
    if [ -f "$APP_DIR/src/gameConsole/gameConsole_board.json" ]; then
        cp "$APP_DIR/src/gameConsole/gameConsole_board.json" "$BUILD_NATIVE_DIR/"
        log_ok "Copy gameConsole_board.json -> $BUILD_NATIVE_DIR/"
    else
        log_warn "Khong co gameConsole_board.json -- runtime se fall back hardcoded array"
    fi
}

build_wasm() {
    log_info "Build WASM mode -> $BUILD_WASM_DIR"
    ensure_sdl3_native || true
    ensure_emsdk
    ensure_sdl3_wasm
    ensure_nanosvg
    ensure_nlohmann # [cite: 30]
    ensure_sqlite   # FIX 2.6.1: SQLite for Stories DB

    # ... [Xử lý sdl_dir giống như file gốc] ...

    mkdir -p "$BUILD_WASM_DIR"
    emcmake cmake -S "$APP_DIR" -B "$BUILD_WASM_DIR" \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DBUILD_WASM=ON \
                  -DSDL3_DIR="$sdl_dir" \
                  -DCMAKE_PREFIX_PATH="$sdl_install" \
                  -DNANOSVG_INCLUDE_DIR="$DOWNLOAD_DIR/nanosvg" \
                  -DNLOHMANN_INCLUDE_DIR="$DOWNLOAD_DIR/nlohmann" \
                  -DSQLITE_DIR="$DOWNLOAD_DIR/sqlite" # [cite: 10, 11]
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