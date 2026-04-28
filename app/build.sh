#!/usr/bin/env bash
# =============================================================================
# build.sh -- Bash build script cho cTetris (macOS / Linux)
# =============================================================================
# THAY DOI v1:
#   - Drop hoan toan pipeline rasterize SVG -> PNG -> ICO/ICNS cho desktop.
#     Desktop app (.app / executable) khong con embed icon binary nua,
#     OS se dung icon mac dinh. Thay vao do, file SVG goc duoc bundle nguyen
#     ve nhu mot resource (de hien thi trong runtime neu can).
#   - WASM build chi can mot file favicon.svg duy nhat (SVG-driven), khong
#     can sinh array PNG nhieu kich thuoc va khong can manifest PWA.
#   - Cac function generate_brandkit / detect_rasterizer / svg_to_png da
#     bi loai bo. Chi giu lai generate_svg_header (embed SVG goc thanh
#     C string header de runtime ve logo bang nanosvg).
# =============================================================================

set -e  # Bao loi ngay khi co lenh fail (tranh build dang do)

# -----------------------------------------------------------------------------
# Cau hinh duong dan co ban
# -----------------------------------------------------------------------------
APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$APP_DIR/build"
THIRDPARTY_DIR="$APP_DIR/thirdparty"
EMSDK_DIR="$APP_DIR/.emsdk"

# Brandkit chi con 1 file SVG goc duy nhat (logo.svg) -- khong co PNG/ICO/ICNS
BRANDKIT_DIR="$APP_DIR/brandkit"
BRAND_LOGO_SVG="$BRANDKIT_DIR/logo.svg"

# Build mode: native (mac default) hoac wasm (truyen "wasm" lam tham so 1)
BUILD_MODE="${1:-native}"

# -----------------------------------------------------------------------------
# Helper: in log co mau de de doc trong CI/CD
# -----------------------------------------------------------------------------
log_info()  { echo -e "\033[0;36m[INFO]\033[0m  $*"; }
log_warn()  { echo -e "\033[0;33m[WARN]\033[0m  $*"; }
log_error() { echo -e "\033[0;31m[ERROR]\033[0m $*"; }
log_ok()    { echo -e "\033[0;32m[OK]\033[0m    $*"; }

# -----------------------------------------------------------------------------
# Tai header nanosvg (parser + rasterizer) neu chua co.
# Chi can 2 file header don le -- khong can build library rieng.
# -----------------------------------------------------------------------------
ensure_nanosvg() {
    local include_dir="$APP_DIR/src/gameStory/include"
    local nano="$include_dir/nanosvg.h"
    local nano_rast="$include_dir/nanosvgrast.h"

    if [[ -f "$nano" && -f "$nano_rast" ]]; then
        log_ok "nanosvg headers da ton tai, bo qua download"
        return 0
    fi

    log_info "Tai nanosvg headers..."
    mkdir -p "$include_dir"
    local base="https://raw.githubusercontent.com/memononen/nanosvg/master/src"
    curl -fsSL "$base/nanosvg.h"     -o "$nano"
    curl -fsSL "$base/nanosvgrast.h" -o "$nano_rast"
    log_ok "nanosvg headers da san sang"
}

# -----------------------------------------------------------------------------
# Sinh header C tu file SVG goc.
#   $1 = path den file SVG nguon
#   $2 = path den header dau ra
#   $3 = ten bien C (vi du LOGO_SVG_DATA)
# Sinh ra mot raw string literal R"SVG_RAW_DATA(...)SVG_RAW_DATA";
# -- ASCII-safe, khong can escape ky tu dac biet.
# -----------------------------------------------------------------------------
generate_svg_header() {
    local svg_path="$1"
    local header_path="$2"
    local var_name="$3"

    if [[ ! -f "$svg_path" ]]; then
        log_error "Khong tim thay SVG nguon: $svg_path"
        return 1
    fi

    mkdir -p "$(dirname "$header_path")"
    local header_name=$(basename "$header_path")

    {
        echo "#pragma once"
        echo "// File nay duoc sinh tu dong tu $(basename "$svg_path") boi build.sh"
        echo "// KHONG sua tay -- moi thay doi se bi ghi de o lan build tiep theo."
        echo "static const char* ${var_name} = R\"SVG_RAW_DATA("
        cat "$svg_path"
        echo ")SVG_RAW_DATA\";"
    } > "$header_path"

    log_ok "Sinh header: $header_name (tu $(basename "$svg_path"))"
}

# -----------------------------------------------------------------------------
# Sinh ca 2 header SVG (logo cua game + logo UIT) neu file SVG goc ton tai.
# -----------------------------------------------------------------------------
generate_logo_headers() {
    local story_dir="$APP_DIR/src/gameStory"
    local include_dir="$story_dir/include"

    # Logo cua GAME cTetris (3x3 o vuong vang/teal)
    if [[ -f "$story_dir/gameStory_logo.svg" ]]; then
        generate_svg_header "$story_dir/gameStory_logo.svg" \
                            "$include_dir/gameStory_logo_svg.h" \
                            "LOGO_SVG_DATA"
    else
        log_warn "Khong co gameStory_logo.svg, bo qua sinh header"
    fi

    # Logo UIT (corp credit "Powered up by ...")
    if [[ -f "$story_dir/gameStory_corp.svg" ]]; then
        generate_svg_header "$story_dir/gameStory_corp.svg" \
                            "$include_dir/gameStory_corp_svg.h" \
                            "CORP_SVG_DATA"
    else
        log_warn "Khong co gameStory_corp.svg, bo qua sinh header"
    fi
}

# -----------------------------------------------------------------------------
# Cai dat emsdk (Emscripten SDK) cho build WASM, chi cai khi can.
# -----------------------------------------------------------------------------
ensure_emsdk() {
    if [[ -d "$EMSDK_DIR" && -f "$EMSDK_DIR/emsdk_env.sh" ]]; then
        log_ok "emsdk da ton tai, kich hoat moi truong"
    else
        log_info "Cai emsdk lan dau (mat vai phut)..."
        git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
        ( cd "$EMSDK_DIR"
          ./emsdk install 3.1.72
          ./emsdk activate 3.1.72 )
    fi
    # shellcheck disable=SC1091
    source "$EMSDK_DIR/emsdk_env.sh"
    log_ok "emsdk active: $(em++ --version | head -n1)"
}

# -----------------------------------------------------------------------------
# Cai SDL3 cho build native. Tren macOS dung Homebrew, Linux build tu source.
# -----------------------------------------------------------------------------
ensure_native_deps() {
    if command -v sdl3-config &>/dev/null; then
        log_ok "SDL3 da co tren he thong"
        return 0
    fi

    if [[ "$(uname)" == "Darwin" ]]; then
        log_info "Cai SDL3 qua Homebrew (macOS)..."
        brew install sdl3 || log_warn "brew install sdl3 fail, can install thu cong"
    else
        log_info "Build SDL3 tu source (Linux)..."
        mkdir -p "$THIRDPARTY_DIR"
        if [[ ! -d "$THIRDPARTY_DIR/SDL" ]]; then
            git clone --depth 1 --branch release-3.2.18 \
                https://github.com/libsdl-org/SDL "$THIRDPARTY_DIR/SDL"
        fi
        cmake -S "$THIRDPARTY_DIR/SDL" -B "$THIRDPARTY_DIR/SDL/build" \
              -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=ON
        cmake --build "$THIRDPARTY_DIR/SDL/build" -j
        sudo cmake --install "$THIRDPARTY_DIR/SDL/build" || true
    fi
}

# -----------------------------------------------------------------------------
# Build native (macOS .app / Linux executable) qua CMake.
# -----------------------------------------------------------------------------
build_native() {
    log_info "Build NATIVE mode"
    ensure_native_deps
    ensure_nanosvg
    generate_logo_headers

    mkdir -p "$BUILD_DIR/native"
    cmake -S "$APP_DIR" -B "$BUILD_DIR/native" \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_WASM=OFF
    cmake --build "$BUILD_DIR/native" -j

    log_ok "Native build hoan tat: $BUILD_DIR/native"
}

# -----------------------------------------------------------------------------
# Build WASM (cho web). Sinh ra cTetris.html / .js / .wasm + favicon.svg.
# Khong sinh manifest PWA va khong dang ky service worker -- yeu cau v1
# la disable PWA install prompt o thanh dia chi browser.
# -----------------------------------------------------------------------------
build_wasm() {
    log_info "Build WASM mode"
    ensure_emsdk
    ensure_nanosvg
    generate_logo_headers

    mkdir -p "$BUILD_DIR/wasm"
    emcmake cmake -S "$APP_DIR" -B "$BUILD_DIR/wasm" \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DBUILD_WASM=ON
    cmake --build "$BUILD_DIR/wasm" -j

    # Copy SVG goc lam favicon de browser hien thi tab icon.
    # Khong rasterize ra PNG -- yeu cau v1 la SVG-driven.
    if [[ -f "$BRAND_LOGO_SVG" ]]; then
        cp "$BRAND_LOGO_SVG" "$BUILD_DIR/wasm/favicon.svg"
        log_ok "Copy favicon.svg (SVG-driven, khong PNG)"
    else
        log_warn "Khong co brandkit/logo.svg cho favicon"
    fi

    log_ok "WASM build hoan tat: $BUILD_DIR/wasm"
    log_info "Mo browser: cd $BUILD_DIR/wasm && python3 -m http.server 8080"
}

# -----------------------------------------------------------------------------
# Entry point: dispatch theo BUILD_MODE
# -----------------------------------------------------------------------------
case "$BUILD_MODE" in
    native) build_native ;;
    wasm)   build_wasm ;;
    all)    build_native; build_wasm ;;
    clean)
        log_info "Don dep build artifacts..."
        rm -rf "$BUILD_DIR"
        log_ok "Da xoa $BUILD_DIR"
        ;;
    *)
        echo "Usage: $0 [native|wasm|all|clean]"
        exit 1
        ;;
esac
