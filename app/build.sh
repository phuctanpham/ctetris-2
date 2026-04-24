#!/bin/sh
# phucpt: app/build.sh
# Script build hợp nhất — VRSFML + WASM (Emscripten)
# Chạy từ thư mục app/: sh build.sh
set -e

BUILD_SCRIPT_VERSION="1.8.0"

# =========================
# Tiện ích
# =========================
command_exists() { command -v "$1" >/dev/null 2>&1; }

print_header() {
    echo ""
    echo "================================================="
    echo "  ctetris — Build Script (VRSFML + WASM)"
    echo "  Version: $BUILD_SCRIPT_VERSION"
    echo "================================================="
    echo ""
}

detect_os() {
    case "$(uname -s)" in
        Darwin*)              echo "macos"   ;;
        Linux*)               echo "linux"   ;;
        CYGWIN*|MINGW*|MSYS*) echo "windows" ;;
        *)                    echo "unknown" ;;
    esac
}

CURRENT_OS=$(detect_os)
EMSDK_VERSION="3.1.72"
EMSDK_DIR="$HOME/emsdk"
IS_WASM=0

# =========================
# Kiểm tra CMake >= 3.16
# Dùng awk để parse version — tương thích BSD sed (macOS) và GNU sed (Linux)
# =========================
check_cmake() {
    if command_exists cmake; then
        CMAKE_VER=$(cmake --version 2>/dev/null | head -1 | awk '{print $3}')
        CMAKE_MAJ=$(echo "$CMAKE_VER" | awk -F. '{print $1}' | tr -dc '0-9')
        CMAKE_MIN=$(echo "$CMAKE_VER" | awk -F. '{print $2}' | tr -dc '0-9')

        if [ -n "$CMAKE_MAJ" ] && [ -n "$CMAKE_MIN" ]; then
            if [ "$CMAKE_MAJ" -gt 3 ] || \
               ([ "$CMAKE_MAJ" -eq 3 ] && [ "$CMAKE_MIN" -ge 16 ]); then
                echo "[OK] CMake $CMAKE_VER đã cài đặt và đủ phiên bản."
                return 0
            fi
            echo "[WARN] CMake $CMAKE_VER quá cũ (yêu cầu >= 3.16)."
        else
            echo "[WARN] Không đọc được phiên bản CMake."
        fi
    else
        echo "[INFO] CMake chưa được cài đặt."
    fi

    printf "Cài/nâng cấp CMake không? [y/N]: "; read ans
    case "$ans" in [Yy]*) ;; *) echo "Huỷ."; exit 1 ;; esac

    if   [ "$CURRENT_OS" = "macos" ]; then brew install cmake || brew upgrade cmake
    elif [ "$CURRENT_OS" = "linux" ]; then sudo apt-get update && sudo apt-get install -y cmake
    else echo "[ERROR] Cài CMake thủ công: https://cmake.org/download/"; exit 1
    fi
}

# =========================
# Kiểm tra Ninja
# =========================
check_ninja() {
    if command_exists ninja; then
        echo "[OK] Ninja $(ninja --version) đã cài đặt."
        return 0
    fi
    echo "[INFO] Ninja chưa cài (tăng tốc build VRSFML đáng kể)."
    printf "Cài Ninja không? [y/N]: "; read ans
    case "$ans" in
        [Yy]*)
            # Chỉ update khi thực sự cài — tắt auto-update khi chỉ kiểm tra
            [ "$CURRENT_OS" = "macos" ] && HOMEBREW_NO_AUTO_UPDATE=1 brew install ninja
            [ "$CURRENT_OS" = "linux" ] && sudo apt-get install -y ninja-build
            ;;
    esac
}

# =========================
# Kiểm tra giflib — chỉ cần cho Desktop
# =========================
check_giflib() {
    if [ "$IS_WASM" = "1" ]; then
        echo "[INFO] WASM build: giflib không cần (GIF được bundle qua --preload-file)."
        return 0
    fi

    local found=0
    if command_exists pkg-config && pkg-config --exists giflib 2>/dev/null; then
        found=1
    fi
    [ "$found" = "0" ] && [ -f /opt/homebrew/include/gif_lib.h ] && found=1
    [ "$found" = "0" ] && [ -f /usr/local/include/gif_lib.h    ] && found=1
    [ "$found" = "0" ] && [ -f /usr/include/gif_lib.h          ] && found=1

    if [ "$found" = "1" ]; then
        echo "[OK] giflib đã cài đặt."
        return 0
    fi

    echo "[INFO] giflib chưa cài (cần cho gameStory intro GIF decode)."
    printf "Cài giflib không? [y/N]: "; read ans
    case "$ans" in
        [Yy]*)
            [ "$CURRENT_OS" = "macos" ] && brew install giflib
            [ "$CURRENT_OS" = "linux" ] && sudo apt-get install -y libgif-dev
            ;;
    esac
}

# =========================
# Cài dependencies Linux cho VRSFML
# =========================
install_linux_deps() {
    echo "[INFO] Cài dependencies Linux cho VRSFML..."
    sudo apt-get update
    sudo apt-get install -y \
        g++ git cmake ninja-build pkg-config \
        libx11-dev libxrandr-dev libxi-dev libxcursor-dev \
        libudev-dev libopengl-dev libfreetype-dev \
        libflac-dev libvorbis-dev libopenal-dev \
        libogg-dev libharfbuzz-dev libgif-dev \
        libgl1-mesa-dev libglu1-mesa-dev
}

# =========================
# Cài dependencies macOS qua Homebrew
# =========================
install_macos_deps() {
    if ! command_exists brew; then
        echo "[INFO] Homebrew chưa cài — đang cài..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        if [ -f "/opt/homebrew/bin/brew" ]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        elif [ -f "/usr/local/bin/brew" ]; then
            eval "$(/usr/local/bin/brew shellenv)"
        fi
    fi

    brew update

    # pkg-config và pkgconf là cùng một công cụ, tên formula khác nhau
    # Homebrew hiện dùng pkgconf; kiểm tra cả hai trước khi kết luận thiếu
    DEPS="cmake ninja giflib"
    MISSING=""
    for d in $DEPS; do
        brew list --formula 2>/dev/null | grep -q "^${d}$" || MISSING="$MISSING $d"
    done

    # Kiểm tra pkg-config / pkgconf riêng (chấp nhận một trong hai)
    if ! brew list --formula 2>/dev/null | grep -qE "^pkg-config$|^pkgconf$"; then
        MISSING="$MISSING pkgconf"
    fi

    if [ -n "$MISSING" ]; then
        echo "[INFO] Các gói chưa cài:$MISSING"
        printf "Cài không? [y/N]: "; read ans
        case "$ans" in
            [Yy]*) brew install $MISSING ;;
            *)     echo "Huỷ."; exit 1   ;;
        esac
    else
        echo "[OK] Tất cả dependencies macOS đã có."
    fi
}

# =========================
# Kiểm tra và cài emsdk cho WASM
# VRSFML tương thích xác nhận với emsdk 3.1.72
# emsdk 4.x có lỗi wasm-ld link với VRSFML
# =========================
check_emsdk() {
    # Bước 1: Nếu emsdk đã cài ở ~/emsdk nhưng chưa source vào shell hiện tại,
    # tự động source trước khi kiểm tra emcc — tránh hỏi cài lại không cần thiết
    if [ -f "$EMSDK_DIR/emsdk_env.sh" ]; then
        # shellcheck disable=SC1090
        . "$EMSDK_DIR/emsdk_env.sh" > /dev/null 2>&1 || true
    fi

    # Bước 2: Kiểm tra emcc sau khi source
    if command_exists emcc; then
        EMCC_VER=$(emcc --version 2>/dev/null | head -1 \
                   | grep -o '[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*' | head -1)
        EMCC_MAJ=$(echo "$EMCC_VER" | awk -F. '{print $1}' | tr -dc '0-9')
        EMCC_MIN=$(echo "$EMCC_VER" | awk -F. '{print $2}' | tr -dc '0-9')

        if [ -n "$EMCC_MAJ" ] && [ "$EMCC_MAJ" -eq 3 ] && [ "$EMCC_MIN" -eq 1 ]; then
            echo "[OK] Emscripten $EMCC_VER (3.1.x) đã kích hoạt — tương thích tốt với VRSFML."
            return 0
        fi

        if [ -n "$EMCC_MAJ" ] && [ "$EMCC_MAJ" -ge 4 ]; then
            echo "[WARN] Emscripten $EMCC_VER (4.x) — VRSFML có thể lỗi wasm-ld."
            echo "       Khuyến nghị dùng emsdk $EMSDK_VERSION."
            printf "Tiếp tục với $EMCC_VER? [y/N]: "; read ans
            case "$ans" in [Yy]*) return 0 ;; esac
        fi

        echo "[INFO] emcc $EMCC_VER — không đủ điều kiện, sẽ cài lại."
    else
        echo "[INFO] emcc không tìm thấy trong PATH sau khi source emsdk_env.sh."
    fi

    # Bước 3: Cài hoặc kích hoạt lại emsdk
    printf "Cài/kích hoạt emsdk $EMSDK_VERSION không? [y/N]: "; read ans
    case "$ans" in [Yy]*) ;; *) echo "Huỷ."; exit 1 ;; esac

    if [ ! -d "$EMSDK_DIR" ]; then
        echo "[INFO] Clone emsdk vào $EMSDK_DIR ..."
        git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
    fi

    cd "$EMSDK_DIR"
    git pull
    ./emsdk install  "$EMSDK_VERSION"
    ./emsdk activate "$EMSDK_VERSION"
    # shellcheck disable=SC1090
    . "$EMSDK_DIR/emsdk_env.sh"
    cd -

    echo "[OK] Emscripten $EMSDK_VERSION đã kích hoạt."
}

# =========================
# Tạo app/emscripten/shell.html nếu chưa có
# VRSFML hardcode --shell-file trỏ đến file này khi compile WASM.
# File phải tồn tại tại đúng đường dẫn trước khi ninja chạy.
# =========================
ensure_shell_html() {
    SHELL_DIR="$(pwd)/emscripten"
    SHELL_FILE="$SHELL_DIR/shell.html"

    # Chỉ tạo nếu file thực sự chưa tồn tại — không bao giờ ghi đè
    if [ -f "$SHELL_FILE" ]; then
        echo "[OK] emscripten/shell.html đã tồn tại — giữ nguyên."
        return 0
    fi

    echo "[INFO] emscripten/shell.html chưa có — đang tạo..."
    mkdir -p "$SHELL_DIR"

    printf '%s\n' '<!doctype html>' > "$SHELL_FILE"
    printf '%s\n' '<!-- phucpt: app/emscripten/shell.html' >> "$SHELL_FILE"
    printf '%s\n' '     Emscripten shell template required by VRSFML for WASM builds.' >> "$SHELL_FILE"
    printf '%s\n' '     {{{ SCRIPT }}} is replaced by Emscripten with the loader <script> tag. -->' >> "$SHELL_FILE"
    printf '%s\n' '<html lang="en">' >> "$SHELL_FILE"
    printf '%s\n' '<head>' >> "$SHELL_FILE"
    printf '%s\n' '  <meta charset="utf-8">' >> "$SHELL_FILE"
    printf '%s\n' '  <meta name="viewport" content="width=device-width, initial-scale=1">' >> "$SHELL_FILE"
    printf '%s\n' '  <title>ctetris</title>' >> "$SHELL_FILE"
    printf '%s\n' '  <style>' >> "$SHELL_FILE"
    printf '%s\n' '    * { margin: 0; padding: 0; box-sizing: border-box; }' >> "$SHELL_FILE"
    printf '%s\n' '    body { background: #0d0d1a; display: flex; flex-direction: column;' >> "$SHELL_FILE"
    printf '%s\n' '           align-items: center; justify-content: center; min-height: 100vh;' >> "$SHELL_FILE"
    printf '%s\n' '           font-family: sans-serif; color: #ccc; }' >> "$SHELL_FILE"
    printf '%s\n' '    #canvas { display: block; max-height: 100vh; aspect-ratio: 9/16;' >> "$SHELL_FILE"
    printf '%s\n' '              background: #000; cursor: default; user-select: none; }' >> "$SHELL_FILE"
    printf '%s\n' '    #status { margin-top: 8px; font-size: 13px; color: #666; min-height: 18px; }' >> "$SHELL_FILE"
    printf '%s\n' '    #progress-bar { width: 300px; height: 6px; background: #222;' >> "$SHELL_FILE"
    printf '%s\n' '                    border-radius: 3px; margin-top: 6px; overflow: hidden; }' >> "$SHELL_FILE"
    printf '%s\n' '    #progress-inner { height: 100%; width: 0%; background: #4a9eff;' >> "$SHELL_FILE"
    printf '%s\n' '                      border-radius: 3px; transition: width 0.1s; }' >> "$SHELL_FILE"
    printf '%s\n' '  </style>' >> "$SHELL_FILE"
    printf '%s\n' '</head>' >> "$SHELL_FILE"
    printf '%s\n' '<body>' >> "$SHELL_FILE"
    printf '%s\n' '  <canvas id="canvas" oncontextmenu="event.preventDefault()"></canvas>' >> "$SHELL_FILE"
    printf '%s\n' '  <div id="status">Đang tải...</div>' >> "$SHELL_FILE"
    printf '%s\n' '  <div id="progress-bar"><div id="progress-inner"></div></div>' >> "$SHELL_FILE"
    printf '%s\n' '  <script>' >> "$SHELL_FILE"
    printf '%s\n' '    var Module = {' >> "$SHELL_FILE"
    printf '%s\n' '      canvas: document.getElementById("canvas"),' >> "$SHELL_FILE"
    printf '%s\n' '      setStatus: function(t) {' >> "$SHELL_FILE"
    printf '%s\n' '        var s = document.getElementById("status");' >> "$SHELL_FILE"
    printf '%s\n' '        var p = document.getElementById("progress-inner");' >> "$SHELL_FILE"
    printf '%s\n' '        if (!t) { s.textContent = ""; return; }' >> "$SHELL_FILE"
    printf '%s\n' '        var m = t.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);' >> "$SHELL_FILE"
    printf '%s\n' '        if (m) { p.style.width=(parseFloat(m[2])/parseFloat(m[4])*100)+"%"; s.textContent=m[1].trim(); }' >> "$SHELL_FILE"
    printf '%s\n' '        else { p.style.width=t?"100%":"0%"; s.textContent=t; }' >> "$SHELL_FILE"
    printf '%s\n' '      },' >> "$SHELL_FILE"
    printf '%s\n' '      onRuntimeInitialized: function() {' >> "$SHELL_FILE"
    printf '%s\n' '        document.getElementById("status").textContent = "";' >> "$SHELL_FILE"
    printf '%s\n' '        document.getElementById("progress-bar").style.display = "none";' >> "$SHELL_FILE"
    printf '%s\n' '      },' >> "$SHELL_FILE"
    printf '%s\n' '      print:    function(t) { console.log(t); },' >> "$SHELL_FILE"
    printf '%s\n' '      printErr: function(t) { console.error(t); },' >> "$SHELL_FILE"
    printf '%s\n' '    };' >> "$SHELL_FILE"
    printf '%s\n' '  </script>' >> "$SHELL_FILE"
    printf '%s\n' '  {{{ SCRIPT }}}' >> "$SHELL_FILE"
    printf '%s\n' '</body>' >> "$SHELL_FILE"
    printf '%s\n' '</html>' >> "$SHELL_FILE"

    echo "[OK] Đã tạo emscripten/shell.html."
}


prepare_build_dir() {
    dir="$1"
    if [ -d "$dir" ]; then
        printf "Thư mục '$dir' đã tồn tại. Xoá để build lại? [y/N]: "; read ans
        case "$ans" in
            [Yy]*)
                echo "Đang xoá $dir ..."
                rm -rf "$dir"
                ;;
        esac
    fi
    mkdir -p "$dir"
}

# ===========================================================
# MAIN
# ===========================================================
print_header

# --- Bước 1: Chọn loại build ---
echo "Bạn muốn build gì?"
echo "  1) Toàn bộ chương trình (ctetris)"
echo "  2) Riêng module gameStory"
echo "  3) Riêng module gameConsole"
echo "  4) Riêng module gameCore"
printf "Lựa chọn [1-4]: "; read build_choice

case "$build_choice" in
    1) MODULE="ALL";         EXE="ctetris"     ;;
    2) MODULE="gameStory";   EXE="gameStory"   ;;
    3) MODULE="gameConsole"; EXE="gameConsole" ;;
    4) MODULE="gameCore";    EXE="gameCore"    ;;
    *) echo "Lựa chọn không hợp lệ."; exit 1  ;;
esac

# --- Bước 2: Chọn nền tảng ---
echo ""
echo "Build cho nền tảng nào?"
echo "  1) macOS"
echo "  2) Ubuntu / Linux"
echo "  3) WebAssembly (WASM) — chạy trên trình duyệt"
printf "Lựa chọn [1-3]: "; read platform_choice

case "$platform_choice" in
    1) TARGET_OS="macos"; IS_WASM=0 ;;
    2) TARGET_OS="linux"; IS_WASM=0 ;;
    3) TARGET_OS="wasm";  IS_WASM=1 ;;
    *) echo "Lựa chọn không hợp lệ."; exit 1 ;;
esac

# Cảnh báo nếu OS thực tế khác với lựa chọn (chỉ cho Desktop)
if [ "$IS_WASM" = "0" ] && \
   [ "$CURRENT_OS" != "unknown" ] && \
   [ "$CURRENT_OS" != "$TARGET_OS" ]; then
    echo "[WARN] OS hiện tại ($CURRENT_OS) khác nền tảng đích ($TARGET_OS)."
    printf "Tiếp tục? [y/N]: "; read ans
    case "$ans" in [Yy]*) ;; *) exit 1 ;; esac
fi

# --- Bước 3: Kiểm tra và cài công cụ ---
echo ""
echo "[CHECK] Kiểm tra công cụ build..."

check_cmake
check_ninja

if [ "$IS_WASM" = "1" ]; then
    # WASM cần native compiler để FetchContent build host tools của VRSFML
    if [ "$CURRENT_OS" = "linux" ]; then install_linux_deps; fi
    if [ "$CURRENT_OS" = "macos" ]; then install_macos_deps; fi
    check_emsdk
else
    check_giflib
    if [ "$TARGET_OS" = "linux" ] && [ "$CURRENT_OS" = "linux" ]; then
        install_linux_deps
    fi
    if [ "$TARGET_OS" = "macos" ] && [ "$CURRENT_OS" = "macos" ]; then
        install_macos_deps
    fi
fi

# --- Bước 4: Chọn generator ---
if command_exists ninja; then
    GENERATOR="-GNinja"
    echo "[INFO] Dùng Ninja generator."
else
    GENERATOR=""
    echo "[INFO] Dùng Make generator (mặc định)."
fi

# --- Bước 5: Tạo shell.html nếu build WASM ---
if [ "$IS_WASM" = "1" ]; then
    ensure_shell_html
fi

# --- Bước 6: Chuẩn bị thư mục build ---
BUILD_DIR="build/${TARGET_OS}/${MODULE}"
prepare_build_dir "$BUILD_DIR"

# --- Bước 6: CMake configure ---
echo ""
echo "[BUILD] Đang chạy CMake configure cho $MODULE / $TARGET_OS ..."

if [ "$IS_WASM" = "1" ]; then
    # Tắt clang-scan-deps — macOS 11 và emsdk không có tool này
    # CMAKE_CXX_SCAN_FOR_MODULES=OFF ngăn CMake 4.x tìm clang-scan-deps
    emcmake cmake -S . -B "$BUILD_DIR" $GENERATOR \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_MODULE="$MODULE" \
        -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
        -DCMAKE_CXX_COMPILER_CLANG_SCAN_DEPS=OFF
else
    cmake -S . -B "$BUILD_DIR" $GENERATOR \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_MODULE="$MODULE"
fi

# --- Bước 7: Build ---
echo "[BUILD] Đang biên dịch..."
cmake --build "$BUILD_DIR" --parallel

# --- Kết quả ---
echo ""
echo "================================================="
echo "  Build thành công!"
echo ""
if [ "$IS_WASM" = "1" ]; then
    echo "  Output WASM:"
    echo "    $BUILD_DIR/$EXE.html"
    echo "    $BUILD_DIR/$EXE.wasm"
    echo "    $BUILD_DIR/$EXE.js"
    echo ""
    echo "  Chạy thử (cần web server, không mở file.html trực tiếp):"
    echo "    cd $BUILD_DIR && python3 -m http.server 8080"
    echo "    Truy cập: http://localhost:8080/$EXE.html"
elif [ "$TARGET_OS" = "macos" ] && [ "$MODULE" = "ALL" ]; then
    echo "  macOS .app bundle: $BUILD_DIR/ctetris.app"
    echo "  Chạy thử: open $BUILD_DIR/ctetris.app"
else
    echo "  Executable: $BUILD_DIR/$EXE"
    echo "  Chạy thử:  cd $BUILD_DIR && ./$EXE"
fi
echo "================================================="