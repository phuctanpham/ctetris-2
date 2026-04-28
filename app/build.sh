#!/bin/sh
# Build script tich hop: SDL3 native + WASM + PWA icon brandkit
# Co the chay tu bat ky thu muc nao -- script tu cd ve thu muc app/.
set -e

# ================================================================
# Resolve SCRIPT_DIR roi cd ve do, dam bao moi path tuong doi
# trong script deu hieu dung. realpath/readlink khong co tren mac
# co macOS nen dung pattern POSIX cd-pwd.
# ================================================================
SCRIPT_PATH="$0"
# Resolve symlink (neu user goi qua symlink trong PATH)
while [ -h "$SCRIPT_PATH" ]; do
    DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
    SCRIPT_PATH="$(readlink "$SCRIPT_PATH")"
    case "$SCRIPT_PATH" in
        /*) ;;
        *)  SCRIPT_PATH="$DIR/$SCRIPT_PATH" ;;
    esac
done
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================="
echo "  ctetris -- Build Script (SDL3 + PWA brandkit)"
echo "================================================="
echo "  Working dir: $SCRIPT_DIR"
echo "================================================="

# Phien ban emsdk on dinh nhat voi SDL3 native port
EMSDK_VERSION="3.1.72"
EMSDK_DIR="${HOME}/emsdk"

# CI mode: GITHUB_ACTIONS=true -> bo qua moi prompt
CI_MODE="${CI:-${GITHUB_ACTIONS:-}}"

CURRENT_OS=$(uname -s)
case "$CURRENT_OS" in
    Darwin) PLATFORM_DIR="macos"   ;;
    Linux)  PLATFORM_DIR="ubuntu"  ;;
    *)      PLATFORM_DIR="unknown" ;;
esac

# ================================================================
# Step 1: dam bao nanosvg headers
# ================================================================
ensure_nanosvg() {
    INCLUDE_DIR="src/gameStory/include"
    BASE="https://raw.githubusercontent.com/memononen/nanosvg/master/src"
    mkdir -p "$INCLUDE_DIR"
    if ! command -v curl >/dev/null 2>&1; then
        echo "[LOI] Thieu 'curl'."; exit 1
    fi
    for f in nanosvg.h nanosvgrast.h; do
        if [ ! -s "$INCLUDE_DIR/$f" ]; then
            echo "Tai $f ..."
            curl -fsSL -o "$INCLUDE_DIR/$f" "$BASE/$f" || {
                echo "[LOI] Khong tai duoc $f"; rm -f "$INCLUDE_DIR/$f"; exit 1; }
        fi
    done
}

# ================================================================
# Step 2: sinh gameStory_logo_svg.h va gameStory_corp_svg.h
# Tu dong embed SVG content thanh raw string literal cua C++
# (plug-and-play: khong can copy file SVG di kem .exe/.app)
# ================================================================
generate_svg_header() {
    SVG_FILE="$1"
    HEADER_FILE="$2"
    VAR_NAME="$3"   # ten bien C++ chua noi dung SVG (vd LOGO_SVG_DATA)

    [ ! -f "$SVG_FILE" ] && { echo "[LOI] Khong tim thay $SVG_FILE"; exit 1; }
    # Skip neu header da moi hon SVG
    [ -f "$HEADER_FILE" ] && [ "$HEADER_FILE" -nt "$SVG_FILE" ] && return 0

    echo "Sinh $HEADER_FILE ..."
    {
        echo "#pragma once"
        echo "// File nay duoc sinh tu dong tu $(basename $SVG_FILE) boi build.sh"
        echo "// KHONG sua tay -- moi thay doi se bi ghi de o lan build tiep theo."
        echo "static const char* $VAR_NAME = R\"SVG_RAW_DATA("
        cat "$SVG_FILE"
        echo ")SVG_RAW_DATA\";"
    } > "$HEADER_FILE"
}

generate_logo_header() {
    generate_svg_header \
        "src/gameStory/gameStory_logo.svg" \
        "src/gameStory/include/gameStory_logo_svg.h" \
        "LOGO_SVG_DATA"
    generate_svg_header \
        "src/gameStory/gameStory_corp.svg" \
        "src/gameStory/include/gameStory_corp_svg.h" \
        "CORP_SVG_DATA"
}

# ================================================================
# Step 3: detect rasterizer
# ================================================================
detect_rasterizer() {
    RASTERIZE=""
    if   command -v rsvg-convert >/dev/null 2>&1; then RASTERIZE="rsvg"
    elif command -v magick       >/dev/null 2>&1; then RASTERIZE="magick"
    elif command -v convert      >/dev/null 2>&1; then RASTERIZE="convert"
    elif command -v inkscape     >/dev/null 2>&1; then RASTERIZE="inkscape"
    fi
}

ensure_rasterizer() {
    detect_rasterizer
    [ -n "$RASTERIZE" ] && return 0
    echo "[ICON] Khong co rasterizer SVG."
    if [ -z "$CI_MODE" ]; then
        printf "Tu dong cai 'librsvg'? [y/N]: "
        read ans
        case "$ans" in [Yy]*) ;; *) return 1 ;; esac
    fi
    case "$PLATFORM_DIR" in
        macos)
            command -v brew >/dev/null 2>&1 || { echo "[LOI] Cai brew truoc."; return 1; }
            brew install librsvg ;;
        ubuntu)
            sudo apt-get update
            sudo apt-get install -y librsvg2-bin imagemagick ;;
    esac
    detect_rasterizer
    [ -z "$RASTERIZE" ] && { echo "[LOI] Cai rasterizer that bai."; return 1; }
    return 0
}

svg_to_png() {
    src="$1"; size="$2"; dst="$3"
    case "$RASTERIZE" in
        rsvg)     rsvg-convert -w "$size" -h "$size" "$src" -o "$dst" ;;
        magick)   magick -background none -size "${size}x${size}" "$src" \
                      -resize "${size}x${size}" "$dst" ;;
        convert)  convert -background none -size "${size}x${size}" "$src" \
                      -resize "${size}x${size}" "$dst" ;;
        inkscape) inkscape -w "$size" -h "$size" "$src" -o "$dst" ;;
    esac
}

# ================================================================
# Step 4: brandkit
# Approach inline: doc viewBox cua logo.svg, sinh wrapper SVG voi
# noi dung logo INLINE (khong dung <image href>) -- cach duy nhat
# bao dam moi rasterizer (rsvg/magick/inkscape) deu render dung.
# ================================================================
generate_brandkit() {
    OUT_DIR="$1"
    SOURCE_SVG="$SCRIPT_DIR/brandkit/logo.svg"

    [ ! -f "$SOURCE_SVG" ] && { echo "[LOI] Thieu $SOURCE_SVG"; return 1; }

    # Cache check: chi skip neu da co du file QUAN TRONG
    if [ -f "$OUT_DIR/icon-512.png" ] && \
       [ -f "$OUT_DIR/icon-192.png" ] && \
       [ -f "$OUT_DIR/icon-maskable-512.png" ] && \
       [ -f "$OUT_DIR/favicon.svg" ]; then
        echo "[ICON] Cache hit -- $OUT_DIR da co du brandkit, skip."
        return 0
    fi

    ensure_rasterizer || {
        echo "[ICON] Khong co rasterizer -- bo qua, build van chay."
        return 0
    }

    mkdir -p "$OUT_DIR"
    echo "[ICON] Sinh brandkit vao $OUT_DIR (rasterizer: $RASTERIZE) ..."

    TMP=$(mktemp -d)
    trap "rm -rf $TMP" EXIT

    # Buoc 1: trich xuat viewBox tu logo.svg de tinh ti le scale chinh xac.
    # Format viewBox cua SVG: "minX minY width height" (cach nhau bang space).
    # Dung grep+sed don gian, tranh phu thuoc xmllint/python.
    VIEWBOX=$(grep -oE 'viewBox="[^"]*"' "$SOURCE_SVG" | head -1 \
              | sed -e 's/viewBox="//' -e 's/"//')
    if [ -z "$VIEWBOX" ]; then
        # SVG khong co viewBox -> fallback dung width/height
        W=$(grep -oE 'width="[0-9.]+"' "$SOURCE_SVG" | head -1 \
            | sed -e 's/width="//' -e 's/"//')
        H=$(grep -oE 'height="[0-9.]+"' "$SOURCE_SVG" | head -1 \
            | sed -e 's/height="//' -e 's/"//')
        [ -z "$W" ] && W=1024
        [ -z "$H" ] && H=1024
        VB_X=0; VB_Y=0; VB_W=$W; VB_H=$H
    else
        VB_X=$(echo "$VIEWBOX" | awk '{print $1}')
        VB_Y=$(echo "$VIEWBOX" | awk '{print $2}')
        VB_W=$(echo "$VIEWBOX" | awk '{print $3}')
        VB_H=$(echo "$VIEWBOX" | awk '{print $4}')
    fi

    # Buoc 2: trich xuat noi dung BEN TRONG <svg>...</svg> cua logo
    # de inline vao wrapper. Dung sed range giua dau dong co <svg va </svg>.
    LOGO_INNER="$TMP/logo_inner.txt"
    awk '
        /<svg/   { capture = 1; sub(/.*<svg[^>]*>/, ""); }
        capture  { print }
        /<\/svg>/ { exit }
    ' "$SOURCE_SVG" | sed 's|</svg>.*||' > "$LOGO_INNER"

    # Buoc 3: helper sinh wrapper SVG voi padding va inline logo
    # CANVAS = 1024 (kich thuoc chuan PWA). Logo duoc:
    #   - translate sang (pad, pad) de can giua trong vung INNER
    #   - scale = INNER / max(VB_W, VB_H) de fit theo canh dai nhat
    #   - translate them de cancel viewBox offset (VB_X, VB_Y)
    render_wrapper() {
        bg="$1"; pad_ratio="$2"; out_svg="$3"
        pad=$(awk "BEGIN { printf \"%.4f\", 1024 * $pad_ratio }")
        inner=$(awk "BEGIN { printf \"%.4f\", 1024 - 2 * $pad }")
        # Scale dong nhat hai truc de giu ti le; chon chieu dai hon lam mau so
        scale=$(awk "BEGIN { vbw=$VB_W; vbh=$VB_H;
                              maxd = (vbw>vbh) ? vbw : vbh;
                              printf \"%.6f\", $inner / maxd }")
        # Offset can giua trong vung INNER neu logo khong vuong
        scaled_w=$(awk "BEGIN { printf \"%.4f\", $VB_W * $scale }")
        scaled_h=$(awk "BEGIN { printf \"%.4f\", $VB_H * $scale }")
        offset_x=$(awk "BEGIN { printf \"%.4f\", $pad + ($inner - $scaled_w) / 2 }")
        offset_y=$(awk "BEGIN { printf \"%.4f\", $pad + ($inner - $scaled_h) / 2 }")
        # Cancel viewBox origin truoc khi scale
        neg_vb_x=$(awk "BEGIN { printf \"%.4f\", -1 * $VB_X }")
        neg_vb_y=$(awk "BEGIN { printf \"%.4f\", -1 * $VB_Y }")

        {
            echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            echo "<svg xmlns=\"http://www.w3.org/2000/svg\""
            echo "     xmlns:xlink=\"http://www.w3.org/1999/xlink\""
            echo "     viewBox=\"0 0 1024 1024\" width=\"1024\" height=\"1024\">"
            echo "  <rect width=\"1024\" height=\"1024\" fill=\"$bg\"/>"
            # Outer translate -> can giua trong canvas
            # Inner scale + translate -> normalize viewBox cua logo
            echo "  <g transform=\"translate($offset_x $offset_y) scale($scale) translate($neg_vb_x $neg_vb_y)\">"
            cat "$LOGO_INNER"
            echo "  </g>"
            echo "</svg>"
        } > "$out_svg"
    }

    # Standard purpose: padding 10% (logo chiem 80% canvas)
    render_wrapper "#ffffff" "0.10" "$TMP/standard.svg"
    # Maskable purpose: padding 20% (logo chiem 60%, an toan moi mask shape)
    render_wrapper "#ffffff" "0.20" "$TMP/maskable.svg"

    svg_to_png "$TMP/standard.svg" 192  "$OUT_DIR/icon-192.png"
    svg_to_png "$TMP/standard.svg" 512  "$OUT_DIR/icon-512.png"
    svg_to_png "$TMP/standard.svg" 1024 "$OUT_DIR/icon-1024.png"
    svg_to_png "$TMP/maskable.svg" 192  "$OUT_DIR/icon-maskable-192.png"
    svg_to_png "$TMP/maskable.svg" 512  "$OUT_DIR/icon-maskable-512.png"
    svg_to_png "$TMP/standard.svg" 180  "$OUT_DIR/apple-touch-icon.png"

    # Favicon: copy nguyen logo SVG (browser tab tu render sac net)
    cp "$SOURCE_SVG" "$OUT_DIR/favicon.svg"
    # Favicon PNG 32x32 fallback cho browser cu khong support SVG favicon
    svg_to_png "$TMP/standard.svg" 32 "$OUT_DIR/favicon.png"

    # Windows ICO multi-resolution (chi voi ImageMagick)
    if [ "$RASTERIZE" = "magick" ] || [ "$RASTERIZE" = "convert" ]; then
        TOOL="$RASTERIZE"
        for s in 16 32 48 64 128 256; do
            svg_to_png "$TMP/standard.svg" "$s" "$TMP/_ico_${s}.png"
        done
        "$TOOL" "$TMP"/_ico_16.png "$TMP"/_ico_32.png "$TMP"/_ico_48.png \
                "$TMP"/_ico_64.png "$TMP"/_ico_128.png "$TMP"/_ico_256.png \
                "$OUT_DIR/icon.ico"
    fi

    # macOS ICNS (chi tren Darwin)
    if [ "$CURRENT_OS" = "Darwin" ] && command -v iconutil >/dev/null 2>&1; then
        ICONSET="$TMP/icon.iconset"
        mkdir -p "$ICONSET"
        svg_to_png "$TMP/standard.svg" 16   "$ICONSET/icon_16x16.png"
        svg_to_png "$TMP/standard.svg" 32   "$ICONSET/icon_16x16@2x.png"
        svg_to_png "$TMP/standard.svg" 32   "$ICONSET/icon_32x32.png"
        svg_to_png "$TMP/standard.svg" 64   "$ICONSET/icon_32x32@2x.png"
        svg_to_png "$TMP/standard.svg" 128  "$ICONSET/icon_128x128.png"
        svg_to_png "$TMP/standard.svg" 256  "$ICONSET/icon_128x128@2x.png"
        svg_to_png "$TMP/standard.svg" 256  "$ICONSET/icon_256x256.png"
        svg_to_png "$TMP/standard.svg" 512  "$ICONSET/icon_256x256@2x.png"
        svg_to_png "$TMP/standard.svg" 512  "$ICONSET/icon_512x512.png"
        svg_to_png "$TMP/standard.svg" 1024 "$ICONSET/icon_512x512@2x.png"
        iconutil -c icns "$ICONSET" -o "$OUT_DIR/icon.icns"
    fi

    echo "[ICON] Done. $(ls -1 "$OUT_DIR" | wc -l | tr -d ' ') file."
}

# ================================================================
# Step 5: emsdk auto-setup
# ================================================================
ensure_emsdk() {
    if command -v emcmake >/dev/null 2>&1; then
        echo "[OK] Emscripten san sang: $(emcc --version | head -1)"
        return 0
    fi
    if [ -f "$EMSDK_DIR/emsdk_env.sh" ]; then
        # shellcheck disable=SC1091
        . "$EMSDK_DIR/emsdk_env.sh" >/dev/null 2>&1 || true
        if command -v emcmake >/dev/null 2>&1; then
            echo "[OK] Emscripten kich hoat: $(emcc --version | head -1)"
            return 0
        fi
    fi
    echo "[INFO] Emscripten chua co trong PATH."
    if [ -z "$CI_MODE" ]; then
        printf "Cai emsdk %s vao %s? [y/N]: " "$EMSDK_VERSION" "$EMSDK_DIR"
        read ans
        case "$ans" in [Yy]*) ;; *) echo "Huy."; exit 1 ;; esac
    fi
    if [ ! -d "$EMSDK_DIR" ]; then
        command -v git >/dev/null 2>&1 || { echo "[LOI] Thieu git."; exit 1; }
        git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
    fi
    ( cd "$EMSDK_DIR" && ./emsdk install "$EMSDK_VERSION" \
                       && ./emsdk activate "$EMSDK_VERSION" )
    # shellcheck disable=SC1091
    . "$EMSDK_DIR/emsdk_env.sh"
    command -v emcmake >/dev/null 2>&1 || { echo "[LOI] emcmake van thieu."; exit 1; }
    echo "[OK] Emscripten san sang: $(emcc --version | head -1)"
}

# ================================================================
# Step 6: native deps
# ================================================================
ensure_native_deps() {
    case "$PLATFORM_DIR" in
        ubuntu)
            command -v cmake >/dev/null 2>&1 || sudo apt-get install -y cmake
            command -v ninja >/dev/null 2>&1 || sudo apt-get install -y ninja-build
            if ! dpkg -s libsdl3-dev >/dev/null 2>&1; then
                echo "[INFO] libsdl3-dev khong co tren apt -- CMake se FetchContent SDL3."
                export FORCE_FETCH_SDL=1
            fi ;;
        macos)
            command -v brew  >/dev/null 2>&1 || { echo "[LOI] Cai brew truoc."; exit 1; }
            command -v cmake >/dev/null 2>&1 || brew install cmake
            command -v ninja >/dev/null 2>&1 || brew install ninja
            brew list sdl3   >/dev/null 2>&1 || brew install sdl3 ;;
    esac
}

# ================================================================
# Main flow
# ================================================================
ensure_nanosvg
generate_logo_header

if [ -n "$CI_MODE" ]; then
    TARGET="${TARGET:-ctetris}"
    PLAT_CHOICE="${PLAT_CHOICE:-2}"
    echo "[CI] TARGET=$TARGET, PLAT_CHOICE=$PLAT_CHOICE"
else
    echo "Ban muon build gi?"
    echo "  1) Toan bo (ctetris)   2) gameStory   3) gameConsole   4) gameCore"
    printf "Lua chon [1-4]: "; read build_choice
    case "$build_choice" in
        2) TARGET="gameStory" ;;
        3) TARGET="gameConsole" ;;
        4) TARGET="gameCore" ;;
        *) TARGET="ctetris" ;;
    esac
    echo "Chon nen tang:"
    echo "  1) Native ($PLATFORM_DIR)"
    echo "  2) WebAssembly (WASM)"
    printf "Lua chon [1-2]: "; read PLAT_CHOICE
fi

# WASM branch
if [ "$PLAT_CHOICE" = "2" ]; then
    # Tat ca path tu day deu ABSOLUTE de cmake hieu dung khi cd vao build dir
    BUILD_DIR="$SCRIPT_DIR/build/wasm/$PLATFORM_DIR"
    ICON_DIR="$BUILD_DIR/icons"

    command -v ninja >/dev/null 2>&1 || {
        echo "[LOI] Thieu ninja. macOS: brew install ninja"
        exit 1
    }
    ensure_emsdk
    generate_brandkit "$ICON_DIR"

    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    ( cd "$BUILD_DIR" && \
      emcmake cmake -G Ninja \
          -DICON_DIR="$ICON_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          "$SCRIPT_DIR" )

    ( cd "$BUILD_DIR" && cmake --build . --target "$TARGET" )

    # Dam bao moi PWA asset deu nam canh HTML output
    for asset in icon-192.png icon-512.png icon-maskable-192.png \
                 icon-maskable-512.png apple-touch-icon.png favicon.svg; do
        [ -f "$ICON_DIR/$asset" ] && cp "$ICON_DIR/$asset" "$BUILD_DIR/"
    done

    echo ""
    echo "Build WASM xong: $BUILD_DIR"
    [ -z "$CI_MODE" ] && {
        echo "Chay localhost:"
        echo "  cd $BUILD_DIR && python3 -m http.server 8000"
        echo "  Mo: http://localhost:8000/${TARGET}.html"
    }
    exit 0
fi

# Native branch
ensure_native_deps
BUILD_DIR="$SCRIPT_DIR/build/local/$PLATFORM_DIR"
ICON_DIR="$BUILD_DIR/icons"
generate_brandkit "$ICON_DIR"

mkdir -p "$BUILD_DIR"
( cd "$BUILD_DIR" && cmake -G Ninja \
                           -DICON_DIR="$ICON_DIR" \
                           -DCMAKE_BUILD_TYPE=Release \
                           "$SCRIPT_DIR" )
( cd "$BUILD_DIR" && ninja "$TARGET" )

echo ""
echo "Build native xong: $BUILD_DIR"
[ "$CURRENT_OS" = "Darwin" ] && echo "Mo: open $BUILD_DIR/${TARGET}.app"