#!/bin/sh
# Tập lệnh build tích hợp — SDL3 & WASM
set -e

echo "================================================="
echo "  ctetris — Build Script (SDL3)"
echo "================================================="

# --- Hỏi người dùng build đơn lẻ hay tích hợp ---
echo "Ban muon build gi?"
echo "  1) Toan bo chuong trinh tich hop (ctetris)"
echo "  2) Rieng gameStory"
echo "  3) Rieng gameConsole"
echo "  4) Rieng gameCore"
printf "Lua chon [1-4]: "; read build_choice

case "$build_choice" in
    1) TARGET="ctetris" ;;
    2) TARGET="gameStory" ;;
    3) TARGET="gameConsole" ;;
    4) TARGET="gameCore" ;;
    *) TARGET="ctetris" ;;
esac

# --- Hỏi nền tảng ---
echo "Chon nen tang build:"
echo "  1) MacDinh (macOS/Ubuntu)"
echo "  2) WebAssembly (WASM)"
printf "Lua chon [1-2]: "; read plat_choice

CURRENT_OS=$(uname -s)

if [ "$plat_choice" = "2" ]; then
    echo "Dang chuan bi moi truong WASM (Yeu cau emscripten)..."
    BUILD_DIR="build/wasm"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    # Giả lập build WASM qua emcc
    echo "Chay emcmake ninja..."
    emcmake cmake -G Ninja ../..
    ninja $TARGET
    echo "Build thanh cong tai build/wasm. Chay thu bang lenh: python3 -m http.server"
    exit 0
fi

# --- Cài đặt công cụ nền tảng cục bộ ---
if [ "$CURRENT_OS" = "Linux" ]; then
    echo "Kiem tra dependencies cho Linux (SDL3)..."
    if ! command -v cmake &> /dev/null; then sudo apt-get update && sudo apt-get install -y cmake ninja-build g++ libsdl3-dev; fi
elif [ "$CURRENT_OS" = "Darwin" ]; then
    echo "Kiem tra dependencies cho macOS (SDL3)..."
    if ! command -v cmake &> /dev/null; then brew install cmake; fi
    if ! command -v ninja &> /dev/null; then brew install ninja; fi
    if ! brew list sdl3 &> /dev/null; then brew install sdl3; fi
fi

BUILD_DIR="build/local"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Tien hanh cau hinh CMake..."
cmake -G Ninja ../..

echo "Tien hanh bien dich bang Ninja target: $TARGET ..."
ninja $TARGET

echo "Build thanh cong! Xem ket qua trong thu muc $BUILD_DIR"