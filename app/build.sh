#!/usr/bin/env bash
# =============================================================================
# build.sh -- Bash build script cho cTetris (macOS / Linux)
# =============================================================================
# THAY DOI v1:
#   - Drop hoan toan pipeline rasterize SVG -> PNG -> ICO/ICNS cho desktop.
#   - WASM build chi can mot file favicon.svg duy nhat (SVG-driven).
#
# THAY DOI v2 (smart-install):
#   - KHONG con chay "apt-get update + install" toan bo list nhu cu.
#   - Quy trinh moi: KIEM TRA -> DIFF -> INSTALL CHI THIEU
#       1. Kiem tra tung tool (cmake/git/curl/python3) co ton tai chua,
#          va version >= min yeu cau hay khong.
#       2. Kiem tra tung dev library qua dpkg/rpm/pacman... -- liet ke ra
#          danh sach THIEU thuc su.
#       3. CHI khi danh sach thieu khac rong: chay "apt-get update" (1 lan)
#          roi "apt-get install" voi danh sach loc.
#       4. Neu danh sach thieu rong -> bo qua hoan toan, log "OK".
#   - Tuong tu cho brew (macOS): chi install formula thieu.
#   - emsdk: kiem tra version da activate, chi re-activate khi khac.
#   - nanosvg: chi download neu file thieu (logic da co tu v1).
# =============================================================================

set -e

# -----------------------------------------------------------------------------
# Cau hinh duong dan + version yeu cau toi thieu
# -----------------------------------------------------------------------------
APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$APP_DIR/build"
THIRDPARTY_DIR="$APP_DIR/thirdparty"
EMSDK_DIR="$APP_DIR/.emsdk"
BRANDKIT_DIR="$APP_DIR/brandkit"
BRAND_LOGO_SVG="$BRANDKIT_DIR/logo.svg"

# Version pinning -- doi o day se tu dong invalidate cache CI
CMAKE_MIN_VERSION="3.16"
EMSDK_VERSION="3.1.72"
SDL3_VERSION="3.2.18"

BUILD_MODE="${1:-native}"

# -----------------------------------------------------------------------------
# Helper log co mau
# -----------------------------------------------------------------------------
log_info()  { echo -e "\033[0;36m[INFO]\033[0m  $*"; }
log_warn()  { echo -e "\033[0;33m[WARN]\033[0m  $*"; }
log_error() { echo -e "\033[0;31m[ERROR]\033[0m $*"; }
log_ok()    { echo -e "\033[0;32m[OK]\033[0m    $*"; }

# -----------------------------------------------------------------------------
# Helper version: tra ve 0 (true) neu $1 >= $2 theo so sanh semver
# Dung "sort -V" (version sort) cua coreutils -- co san o moi distro.
# -----------------------------------------------------------------------------
version_ge() {
    [ -z "$2" ] && return 0
    [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | head -n1)" = "$2" ]
}

# -----------------------------------------------------------------------------
# Helper: lay version cua mot command qua "<cmd> --version"
# Tra ve chuoi "X.Y.Z" dau tien tim duoc trong output.
# -----------------------------------------------------------------------------
get_cmd_version() {
    "$1" --version 2>&1 | head -n1 \
        | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1
}

# -----------------------------------------------------------------------------
# Helper: kiem tra command co ton tai voi version >= min hay khong
# Return: 0 = OK, 1 = thieu hoac qua cu
# -----------------------------------------------------------------------------
check_cmd_version() {
    local cmd="$1" min="$2"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        return 1
    fi
    local current
    current=$(get_cmd_version "$cmd")
    if [ -z "$current" ]; then
        return 0
    fi
    if version_ge "$current" "$min"; then
        return 0
    fi
    log_warn "$cmd version $current < $min (yeu cau)"
    return 1
}

# -----------------------------------------------------------------------------
# Helper sudo
# -----------------------------------------------------------------------------
get_sudo() {
    if [ "$EUID" -eq 0 ]; then
        echo ""
    elif command -v sudo &>/dev/null; then
        echo "sudo"
    else
        echo ""
    fi
}

# =============================================================================
# APT (Debian / Ubuntu): kiem tra package roi install chi cai thieu
# =============================================================================
is_apt_pkg_installed() {
    dpkg-query -W -f='${Status}' "$1" 2>/dev/null \
        | grep -q "install ok installed"
}

install_apt_packages_if_missing() {
    local pkgs=("$@") missing=()
    for p in "${pkgs[@]}"; do
        is_apt_pkg_installed "$p" || missing+=("$p")
    done
    if [ ${#missing[@]} -eq 0 ]; then
        log_ok "Tat ca ${#pkgs[@]} apt packages da co san, bo qua install"
        return 0
    fi
    log_info "Thieu ${#missing[@]}/${#pkgs[@]} apt packages: ${missing[*]}"
    local SUDO; SUDO=$(get_sudo)
    log_info "Chay apt-get update (chi 1 lan, vi co package can install)..."
    $SUDO apt-get update -qq || log_warn "apt-get update fail"
    $SUDO apt-get install -y --no-install-recommends "${missing[@]}"
}

# =============================================================================
# DNF / YUM (Fedora / RHEL) -- kiem tra qua "rpm -q"
# =============================================================================
is_rpm_pkg_installed() { rpm -q "$1" >/dev/null 2>&1; }

install_dnf_packages_if_missing() {
    local pkgs=("$@") missing=()
    for p in "${pkgs[@]}"; do is_rpm_pkg_installed "$p" || missing+=("$p"); done
    if [ ${#missing[@]} -eq 0 ]; then
        log_ok "Tat ca ${#pkgs[@]} dnf packages da co san"; return 0
    fi
    log_info "Thieu ${#missing[@]}/${#pkgs[@]} dnf packages: ${missing[*]}"
    local SUDO; SUDO=$(get_sudo)
    $SUDO dnf install -y "${missing[@]}"
}

install_yum_packages_if_missing() {
    local pkgs=("$@") missing=()
    for p in "${pkgs[@]}"; do is_rpm_pkg_installed "$p" || missing+=("$p"); done
    if [ ${#missing[@]} -eq 0 ]; then
        log_ok "Tat ca ${#pkgs[@]} yum packages da co san"; return 0
    fi
    log_info "Thieu ${#missing[@]}/${#pkgs[@]} yum packages: ${missing[*]}"
    local SUDO; SUDO=$(get_sudo)
    $SUDO yum install -y "${missing[@]}"
}

# =============================================================================
# PACMAN / ZYPPER / APK
# =============================================================================
is_pacman_pkg_installed() { pacman -Qi "$1" >/dev/null 2>&1; }

install_pacman_packages_if_missing() {
    local pkgs=("$@") missing=()
    for p in "${pkgs[@]}"; do is_pacman_pkg_installed "$p" || missing+=("$p"); done
    if [ ${#missing[@]} -eq 0 ]; then
        log_ok "Tat ca ${#pkgs[@]} pacman packages da co san"; return 0
    fi
    log_info "Thieu ${#missing[@]}/${#pkgs[@]} pacman packages: ${missing[*]}"
    local SUDO; SUDO=$(get_sudo)
    $SUDO pacman -Sy --needed --noconfirm "${missing[@]}"
}

install_zypper_packages_if_missing() {
    local pkgs=("$@") missing=()
    for p in "${pkgs[@]}"; do is_rpm_pkg_installed "$p" || missing+=("$p"); done
    if [ ${#missing[@]} -eq 0 ]; then
        log_ok "Tat ca ${#pkgs[@]} zypper packages da co san"; return 0
    fi
    log_info "Thieu ${#missing[@]}/${#pkgs[@]} zypper packages: ${missing[*]}"
    local SUDO; SUDO=$(get_sudo)
    $SUDO zypper install -y "${missing[@]}"
}

is_apk_pkg_installed() { apk info -e "$1" >/dev/null 2>&1; }

install_apk_packages_if_missing() {
    local pkgs=("$@") missing=()
    for p in "${pkgs[@]}"; do is_apk_pkg_installed "$p" || missing+=("$p"); done
    if [ ${#missing[@]} -eq 0 ]; then
        log_ok "Tat ca ${#pkgs[@]} apk packages da co san"; return 0
    fi
    log_info "Thieu ${#missing[@]}/${#pkgs[@]} apk packages: ${missing[*]}"
    local SUDO; SUDO=$(get_sudo)
    $SUDO apk add --no-cache "${missing[@]}"
}

# =============================================================================
# HOMEBREW (macOS): chi install formula thuc su thieu
# =============================================================================
is_brew_formula_installed() {
    brew list --formula "$1" >/dev/null 2>&1
}

install_brew_formulas_if_missing() {
    local pkgs=("$@") missing=()
    for p in "${pkgs[@]}"; do
        [ -z "$p" ] && continue
        is_brew_formula_installed "$p" || missing+=("$p")
    done
    if [ ${#missing[@]} -eq 0 ]; then
        log_ok "Tat ca brew formulas da co san"; return 0
    fi
    log_info "Thieu ${#missing[@]} brew formulas: ${missing[*]}"
    # brew install tu fetch metadata moi -- khong can chu dong "brew update"
    brew install "${missing[@]}"
}

# =============================================================================
# nanosvg + SVG header generation
# =============================================================================
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

generate_svg_header() {
    local svg_path="$1" header_path="$2" var_name="$3"
    [ -f "$svg_path" ] || { log_error "Khong tim thay $svg_path"; return 1; }
    mkdir -p "$(dirname "$header_path")"

    # Skip neu header da fresh hon SVG nguon -- tranh recompile khong can
    if [ -f "$header_path" ] && [ "$header_path" -nt "$svg_path" ]; then
        log_ok "Header $(basename "$header_path") da fresh, bo qua regenerate"
        return 0
    fi

    {
        echo "#pragma once"
        echo "// File nay duoc sinh tu dong tu $(basename "$svg_path") boi build.sh"
        echo "// KHONG sua tay -- moi thay doi se bi ghi de o lan build tiep theo."
        echo "static const char* ${var_name} = R\"SVG_RAW_DATA("
        cat "$svg_path"
        echo ")SVG_RAW_DATA\";"
    } > "$header_path"
    log_ok "Sinh header: $(basename "$header_path") (tu $(basename "$svg_path"))"
}

generate_logo_headers() {
    local story_dir="$APP_DIR/src/gameStory"
    local include_dir="$story_dir/include"
    [ -f "$story_dir/gameStory_logo.svg" ] && \
        generate_svg_header "$story_dir/gameStory_logo.svg" \
                            "$include_dir/gameStory_logo_svg.h" "LOGO_SVG_DATA"
    [ -f "$story_dir/gameStory_corp.svg" ] && \
        generate_svg_header "$story_dir/gameStory_corp.svg" \
                            "$include_dir/gameStory_corp_svg.h" "CORP_SVG_DATA"
}

# =============================================================================
# emsdk: chi install/activate neu version dang active != target
# =============================================================================
ensure_emsdk() {
    if [ ! -d "$EMSDK_DIR" ] || [ ! -f "$EMSDK_DIR/emsdk_env.sh" ]; then
        log_info "Cai emsdk lan dau tu GitHub..."
        git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
    else
        log_ok "emsdk thu muc da ton tai"
    fi

    # Kiem tra ban da install va active
    local installed_marker="$EMSDK_DIR/upstream/emscripten"
    local need_install=true

    if [ -d "$installed_marker" ]; then
        # Co em++ da active -- so version voi target
        # shellcheck disable=SC1091
        source "$EMSDK_DIR/emsdk_env.sh" >/dev/null 2>&1 || true
        if command -v em++ >/dev/null 2>&1; then
            local cur
            cur=$(em++ --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
            if [ "$cur" = "$EMSDK_VERSION" ]; then
                log_ok "emsdk $EMSDK_VERSION da active, bo qua install/activate"
                need_install=false
            else
                log_warn "emsdk active version $cur != target $EMSDK_VERSION, se re-activate"
            fi
        fi
    fi

    if $need_install; then
        log_info "Install + activate emsdk $EMSDK_VERSION..."
        ( cd "$EMSDK_DIR"
          ./emsdk install "$EMSDK_VERSION"
          ./emsdk activate "$EMSDK_VERSION" )
        # shellcheck disable=SC1091
        source "$EMSDK_DIR/emsdk_env.sh"
    fi
    log_ok "emsdk active: $(em++ --version | head -n1)"
}

# =============================================================================
# SDL3 self-built cho WASM
# -----------------------------------------------------------------------------
# Tai sao tu build:
#   - Emscripten port "-sUSE_SDL=3" yeu cau emsdk version moi (SDL3 chi
#     duoc them rat gan day, va co ban release-3.1.72 chua ho tro on dinh).
#   - Build SDL3 tinh tai dia phuong dam bao chinh xac version SDL3_VERSION
#     va khong phu thuoc vao bien dong cua emsdk port.
# Quy trinh:
#   1. Re-use $THIRDPARTY_DIR/SDL (clone tu native deps neu da co).
#   2. Build vao thu muc rieng $THIRDPARTY_DIR/SDL/build-wasm (khong dum
#      voi build native).
#   3. Install vao $APP_DIR/.sdl3-wasm/ -- nho hon /usr/local de cache CI.
#   4. CMakeLists.txt cua app dung find_package(SDL3 CONFIG); build.sh
#      truyen -DCMAKE_PREFIX_PATH=$APP_DIR/.sdl3-wasm de no tim duoc.
# Skip neu .a + cmake config files da ton tai -- tiet kiem ~1-2 phut moi build.
# =============================================================================
ensure_sdl3_wasm() {
    local sdl_install="$APP_DIR/.sdl3-wasm"

    # Kiem tra san pham da install: co .a + co cmake config
    if [ -d "$sdl_install/lib/cmake/SDL3" ] || [ -d "$sdl_install/lib64/cmake/SDL3" ]; then
        if find "$sdl_install" -name 'libSDL3*.a' -print -quit 2>/dev/null | grep -q .; then
            log_ok "SDL3 WASM static da co tai $sdl_install (skip rebuild)"
            return 0
        fi
    fi

    log_info "Build SDL3 $SDL3_VERSION cho WASM (lan dau, ~1-2 phut)..."
    mkdir -p "$THIRDPARTY_DIR"

    # Re-use thu muc SDL clone neu da co tu ensure_native_deps
    if [ ! -d "$THIRDPARTY_DIR/SDL" ]; then
        git clone --depth 1 --branch "release-$SDL3_VERSION" \
            https://github.com/libsdl-org/SDL "$THIRDPARTY_DIR/SDL"
    fi

    local sdl_build="$THIRDPARTY_DIR/SDL/build-wasm"

    # SDL3 tu nhan Emscripten platform khi chay qua emcmake -- no tu
    # disable X11/Wayland/ALSA va su dung backend html5/web audio.
    # Chi can them flag chinh:
    #   - SDL_SHARED=OFF + SDL_STATIC=ON: chi sinh .a (WASM khong support .so)
    #   - SDL_TESTS=OFF: skip cac demo/test cua SDL, tiet kiem 80% thoi gian
    #   - SDL_TEST_LIBRARY=OFF: khong sinh libSDL3_test.a (khong dung)
    emcmake cmake -S "$THIRDPARTY_DIR/SDL" -B "$sdl_build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSDL_SHARED=OFF \
        -DSDL_STATIC=ON \
        -DSDL_TESTS=OFF \
        -DSDL_TEST_LIBRARY=OFF \
        -DCMAKE_INSTALL_PREFIX="$sdl_install"
    cmake --build "$sdl_build" -j
    cmake --install "$sdl_build"

    log_ok "SDL3 WASM static da install vao $sdl_install"
}

# =============================================================================
# Linux dev libs cho SDL3 build tu source -- chi install package thieu
# =============================================================================
ensure_linux_dev_libs() {
    [ "$(uname)" = "Linux" ] || return 0

    if command -v apt-get &>/dev/null; then
        log_info "Detect apt-get -> kiem tra dev libs Debian/Ubuntu..."
        local pkgs=(
            build-essential pkg-config cmake git curl python3
            libx11-dev libxext-dev libxrandr-dev libxcursor-dev
            libxi-dev libxinerama-dev libxxf86vm-dev libxss-dev
            libwayland-dev libxkbcommon-dev wayland-protocols
            libdrm-dev libgbm-dev
            libgl1-mesa-dev libglu1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev
            libpulse-dev libasound2-dev libsndio-dev
            libudev-dev libdbus-1-dev libibus-1.0-dev
            libunwind-dev
        )
        install_apt_packages_if_missing "${pkgs[@]}"
        return 0
    fi

    if command -v dnf &>/dev/null; then
        log_info "Detect dnf -> kiem tra dev libs Fedora/RHEL..."
        local pkgs=(
            gcc-c++ make pkgconf-pkg-config cmake git curl python3
            libX11-devel libXext-devel libXrandr-devel libXcursor-devel
            libXi-devel libXinerama-devel libXxf86vm-devel libXScrnSaver-devel
            wayland-devel libxkbcommon-devel wayland-protocols-devel
            libdrm-devel mesa-libgbm-devel
            mesa-libGL-devel mesa-libGLU-devel mesa-libEGL-devel mesa-libGLES-devel
            pulseaudio-libs-devel alsa-lib-devel
            systemd-devel dbus-devel ibus-devel
            libunwind-devel
        )
        install_dnf_packages_if_missing "${pkgs[@]}"
        return 0
    fi

    if command -v yum &>/dev/null; then
        log_info "Detect yum -> kiem tra dev libs RHEL7/CentOS7..."
        local pkgs=(
            gcc-c++ make pkgconfig cmake git curl python3
            libX11-devel libXext-devel libXrandr-devel libXcursor-devel
            libXi-devel libXinerama-devel libXxf86vm-devel libXScrnSaver-devel
            wayland-devel libxkbcommon-devel
            libdrm-devel mesa-libgbm-devel
            mesa-libGL-devel mesa-libGLU-devel mesa-libEGL-devel
            pulseaudio-libs-devel alsa-lib-devel
            systemd-devel dbus-devel
            libunwind-devel
        )
        install_yum_packages_if_missing "${pkgs[@]}"
        return 0
    fi

    if command -v pacman &>/dev/null; then
        log_info "Detect pacman -> kiem tra dev libs Arch/Manjaro..."
        local pkgs=(
            base-devel pkgconf cmake git curl python
            libx11 libxext libxrandr libxcursor libxi libxinerama libxxf86vm libxss
            wayland libxkbcommon wayland-protocols
            libdrm mesa
            alsa-lib libpulse
            systemd dbus
            libunwind
        )
        install_pacman_packages_if_missing "${pkgs[@]}"
        return 0
    fi

    if command -v zypper &>/dev/null; then
        log_info "Detect zypper -> kiem tra dev libs openSUSE..."
        local pkgs=(
            cmake git curl pkg-config python3
            libX11-devel libXext-devel libXrandr-devel libXcursor-devel
            libXi-devel libXinerama-devel libXxf86vm-devel
            wayland-devel libxkbcommon-devel
            libdrm-devel Mesa-libGL-devel Mesa-libEGL-devel Mesa-libgbm-devel
            libpulse-devel alsa-devel
            systemd-devel dbus-1-devel
            libunwind-devel
        )
        install_zypper_packages_if_missing "${pkgs[@]}"
        return 0
    fi

    if command -v apk &>/dev/null; then
        log_info "Detect apk -> kiem tra dev libs Alpine..."
        local pkgs=(
            build-base cmake git curl pkgconfig python3
            libx11-dev libxext-dev libxrandr-dev libxcursor-dev
            libxi-dev libxinerama-dev libxxf86vm-dev libxscrnsaver-dev
            wayland-dev libxkbcommon-dev wayland-protocols
            libdrm-dev mesa-dev
            pulseaudio-dev alsa-lib-dev
            eudev-dev dbus-dev
            libunwind-dev
        )
        install_apk_packages_if_missing "${pkgs[@]}"
        return 0
    fi

    log_error "Khong nhan dien duoc package manager. Cai thu cong cac dev libs."
    return 1
}

# =============================================================================
# macOS deps: chi install brew formula thuc su thieu
# =============================================================================
ensure_macos_deps() {
    [ "$(uname)" = "Darwin" ] || return 0

    if ! command -v brew >/dev/null 2>&1; then
        log_warn "Khong co Homebrew, vui long cai tu https://brew.sh/"
        return 1
    fi

    # Build danh sach formula can co tu thieu (loai bo nhung gi da OK)
    local pkgs=()
    check_cmd_version cmake "$CMAKE_MIN_VERSION" || pkgs+=(cmake)
    command -v git >/dev/null 2>&1               || pkgs+=(git)
    command -v python3 >/dev/null 2>&1           || pkgs+=(python3)

    # SDL3 phat hien qua pkg-config -- chi them vao list neu chua co
    if ! ( command -v pkg-config >/dev/null 2>&1 && \
           pkg-config --exists sdl3 2>/dev/null ); then
        pkgs+=(sdl3)
    fi

    if [ ${#pkgs[@]} -eq 0 ]; then
        log_ok "macOS deps deu OK (cmake, git, python3, sdl3)"
        return 0
    fi
    log_info "macOS thieu: ${pkgs[*]}"
    install_brew_formulas_if_missing "${pkgs[@]}"
}

# =============================================================================
# Setup native dependencies (SDL3 san sang) cho Linux/macOS
# =============================================================================
ensure_native_deps() {
    # Fast path: SDL3 da co tren he thong
    if command -v sdl3-config &>/dev/null; then
        log_ok "SDL3 da co (sdl3-config)"
        return 0
    fi
    if command -v pkg-config >/dev/null 2>&1 && \
       pkg-config --exists sdl3 2>/dev/null; then
        log_ok "SDL3 da co qua pkg-config (version $(pkg-config --modversion sdl3))"
        return 0
    fi

    if [ "$(uname)" = "Darwin" ]; then
        ensure_macos_deps
        # Sau khi brew install, kiem tra lai
        if pkg-config --exists sdl3 2>/dev/null; then
            log_ok "SDL3 san sang sau khi brew install"
            return 0
        fi
        log_warn "SDL3 chua thay sau brew install -- thu build tu source"
    fi

    # Linux (hoac macOS fallback): cai dev libs roi build SDL3 tu source
    ensure_linux_dev_libs

    # Build SDL3 chi khi chua co source/build artifact
    if [ -d "$THIRDPARTY_DIR/SDL/build" ] && \
       find "$THIRDPARTY_DIR/SDL/build" -name 'libSDL3*' -print -quit | grep -q .; then
        log_ok "SDL3 build artifact da co tai $THIRDPARTY_DIR/SDL/build"
    else
        log_info "Build SDL3 $SDL3_VERSION tu source..."
        mkdir -p "$THIRDPARTY_DIR"
        if [ ! -d "$THIRDPARTY_DIR/SDL" ]; then
            git clone --depth 1 --branch "release-$SDL3_VERSION" \
                https://github.com/libsdl-org/SDL "$THIRDPARTY_DIR/SDL"
        fi
        cmake -S "$THIRDPARTY_DIR/SDL" -B "$THIRDPARTY_DIR/SDL/build" \
              -DCMAKE_BUILD_TYPE=Release \
              -DSDL_SHARED=ON \
              -DSDL_X11=ON \
              -DSDL_OPENGL=ON \
              -DSDL_OPENGLES=ON
        cmake --build "$THIRDPARTY_DIR/SDL/build" -j
    fi

    local SUDO; SUDO=$(get_sudo)
    $SUDO cmake --install "$THIRDPARTY_DIR/SDL/build" || true
}

# =============================================================================
# Build entry points
# =============================================================================
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

build_wasm() {
    log_info "Build WASM mode"

    # WASM khong can SDL3 system, nhung SDL3 source CMake build-time can
    # mot so headers cu ban (libc, etc.) -- thuong san co tren CI runner.
    # Chi cai basic tools + emsdk + python3.
    if [ "$(uname)" = "Linux" ]; then
        if command -v apt-get &>/dev/null; then
            install_apt_packages_if_missing \
                build-essential cmake git curl python3 ca-certificates
        elif command -v dnf &>/dev/null; then
            install_dnf_packages_if_missing \
                gcc-c++ make cmake git curl python3 ca-certificates
        elif command -v pacman &>/dev/null; then
            install_pacman_packages_if_missing \
                base-devel cmake git curl python ca-certificates
        fi
    elif [ "$(uname)" = "Darwin" ]; then
        if command -v brew >/dev/null 2>&1; then
            local pkgs=()
            check_cmd_version cmake "$CMAKE_MIN_VERSION" || pkgs+=(cmake)
            command -v git >/dev/null 2>&1 || pkgs+=(git)
            command -v python3 >/dev/null 2>&1 || pkgs+=(python3)
            [ ${#pkgs[@]} -gt 0 ] && install_brew_formulas_if_missing "${pkgs[@]}" \
                                  || log_ok "Basic tools cho WASM build da co"
        fi
    fi

    ensure_emsdk
    ensure_sdl3_wasm    # Build SDL3 tinh cho WASM neu chua co
    ensure_nanosvg
    generate_logo_headers

    # Truyen CMAKE_PREFIX_PATH den thu muc SDL3 da install de
    # find_package(SDL3 CONFIG) trong CMakeLists.txt cua app tim duoc.
    local sdl_prefix="$APP_DIR/.sdl3-wasm"

    mkdir -p "$BUILD_DIR/wasm"
    emcmake cmake -S "$APP_DIR" -B "$BUILD_DIR/wasm" \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DBUILD_WASM=ON \
                  -DCMAKE_PREFIX_PATH="$sdl_prefix"
    cmake --build "$BUILD_DIR/wasm" -j

    if [ -f "$BRAND_LOGO_SVG" ]; then
        cp "$BRAND_LOGO_SVG" "$BUILD_DIR/wasm/favicon.svg"
        log_ok "Copy favicon.svg (SVG-driven)"
    else
        log_warn "Khong co brandkit/logo.svg cho favicon"
    fi

    log_ok "WASM build hoan tat: $BUILD_DIR/wasm"
}

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