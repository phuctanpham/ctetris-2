#!/bin/sh
# Tap lenh build tich hop -- SDL3 native + WASM
set -e

echo "================================================="
echo "  ctetris -- Build Script (SDL3)"
echo "================================================="

# --- Hoi target ---
echo "Ban muon build gi?"
echo "  1) Toan bo chuong trinh tich hop (ctetris)"
echo "  2) Rieng gameStory"
echo "  3) Rieng gameConsole"
echo "  4) Rieng gameCore"
printf "Lua chon [1-4]: "; read build_choice

case "$build_choice" in
    2) TARGET="gameStory" ;;
    3) TARGET="gameConsole" ;;
    4) TARGET="gameCore" ;;
    *) TARGET="ctetris" ;;
esac

# --- Hoi nen tang ---
echo "Chon nen tang build:"
echo "  1) Native (macOS / Ubuntu)"
echo "  2) WebAssembly (WASM)"
printf "Lua chon [1-2]: "; read plat_choice

CURRENT_OS=$(uname -s)

# Ham kiem tra cong cu, fail-fast neu thieu
require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "[LOI] Thieu cong cu '$1'."
        echo "      $2"
        exit 1
    fi
}

# =========== Nhanh WASM ===========
if [ "$plat_choice" = "2" ]; then
    echo "Kiem tra moi truong WASM..."
    require_tool emcmake "Cai Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html ; sau do 'source ./emsdk_env.sh'."
    require_tool ninja   "macOS: brew install ninja  |  Ubuntu: sudo apt-get install ninja-build"

    BUILD_DIR="build/wasm"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    echo "Cau hinh CMake voi emcmake..."
    emcmake cmake -G Ninja ../..

    echo "Bien dich target: $TARGET ..."
    cmake --build . --target "$TARGET"

    echo ""
    echo "Build WASM thanh cong tai $BUILD_DIR."
    echo "Cach chay tren localhost:"
    echo "  cd $BUILD_DIR && python3 -m http.server 8000"
    echo "  Mo trinh duyet: http://localhost:8000/${TARGET}.html"
    exit 0
fi

# =========== Nhanh Native ===========
if [ "$CURRENT_OS" = "Linux" ]; then
    echo "Kiem tra dependencies cho Linux (SDL3)..."
    if ! command -v cmake >/dev/null 2>&1; then sudo apt-get update && sudo apt-get install -y cmake; fi
    if ! command -v ninja >/dev/null 2>&1; then sudo apt-get install -y ninja-build; fi
    if ! dpkg -s libsdl3-dev >/dev/null 2>&1; then
        echo "[LOI] libsdl3-dev khong co tren apt mac dinh."
        echo "      Build SDL3 thu cong tu: https://github.com/libsdl-org/SDL"
        exit 1
    fi
elif [ "$CURRENT_OS" = "Darwin" ]; then
    echo "Kiem tra dependencies cho macOS (SDL3)..."
    require_tool brew "Cai Homebrew tu https://brew.sh truoc."
    if ! command -v cmake >/dev/null 2>&1; then brew install cmake; fi
    if ! command -v ninja >/dev/null 2>&1; then brew install ninja; fi
    if ! brew list sdl3 >/dev/null 2>&1; then brew install sdl3; fi
fi

BUILD_DIR="build/local"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Cau hinh CMake..."
cmake -G Ninja ../..

echo "Bien dich Ninja target: $TARGET ..."
ninja "$TARGET"

echo ""
echo "Build thanh cong! Ket qua trong $BUILD_DIR/"
if [ "$CURRENT_OS" = "Darwin" ]; then
    echo "Tren macOS: open $BUILD_DIR/${TARGET}.app"
fi