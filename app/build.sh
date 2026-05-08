#!/usr/bin/env bash
# =============================================================================
# build.sh -- Bash build script cho cTetris (macOS / Linux)
# =============================================================================
# THAY DOI v3 (validation + os-driven paths):
#   - Detect OS thuc (macos/ubuntu/fedora/arch/alpine/opensuse) va dung
#     OS_NAME lam tien to cho duong dan downloads va build artifacts:
#       app/libs/<OS>/downloads/    <- moi clone & download vao day
#       app/build/desktop/<OS>/     <- native binary
#       app/build/wasm/<OS>/        <- WASM artifact (theo host OS)
#   - Bo HOAN TOAN viec sinh file C tu noi dung fixed (vi du SVG -> .h):
#     cac file *_svg.h va *_layout.h phai duoc commit san trong repo.
#     Build script chi xac thuc, cai dat & build -- KHONG tao file co
#     content cung trong script.
#   - Validation truoc khi cai dat:
#       1. Detect xem tool/lib da co qua package manager (brew/apt/dnf...).
#       2. Detect xem tool/lib da co tu install thu cong (PATH, ~/emsdk,
#          /usr/local, /opt/homebrew, libs/<OS>/downloads/...).
#       3. So sanh version voi yeu cau toi thieu.
#       4. CHI khi thieu / qua cu moi cai. Uu tien package manager truoc;
#          neu khong duoc moi git clone / download.
#   - Heavy package (emsdk, SDL3 source) clone shallow (--depth 1) vao
#     libs/<OS>/downloads/, de CI cache toan thu muc nay -- tiet kiem
#     download lap di lap lai.
# =============================================================================

set -e

# -----------------------------------------------------------------------------
# Detect OS -- chuoi dung lam ten thu muc con cho libs/ va build/
# -----------------------------------------------------------------------------
detect_os() {
    case "$(uname -s)" in
        Darwin) echo "macos" ;;
        Linux)
            # Phan biet distro Linux qua /etc/os-release (ID field)
            if [ -r /etc/os-release ]; then
                # shellcheck source=/dev/null
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

# -----------------------------------------------------------------------------
# Cau hinh duong dan -- TAT CA path quan trong tap trung o day, OS-driven
# -----------------------------------------------------------------------------
APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Downloads + clones rieng cho moi OS de cache CI deterministic
LIBS_DIR="$APP_DIR/libs/$OS_NAME"
DOWNLOAD_DIR="$LIBS_DIR/downloads"

# Build artifacts: native -> build/desktop/<OS>, wasm -> build/wasm/<OS>
BUILD_NATIVE_DIR="$APP_DIR/build/desktop/$OS_NAME"
BUILD_WASM_DIR="$APP_DIR/build/wasm/$OS_NAME"

# Brandkit + favicon (SVG-driven, khong PNG)
BRANDKIT_DIR="$APP_DIR/brandkit"
BRAND_LOGO_SVG="$BRANDKIT_DIR/logo.svg"

# Web assets (PWA) -- da commit san trong repo, chi can copy
WEB_DIR="$APP_DIR/web"

# Version pinning -- doi o day se tu invalidate cache CI
# SDL3_VERSION_MIN: version toi thieu yeu cau (game can SDL3 >= 3.2.0)
# SDL3_VERSION:     version dung khi build SDL3 tu source (pin de deterministic).
# NEU detect duoc SDL3 cai san qua brew/apt (>= MIN), thi WASM build se tu
# match dung version DO de tranh dij ban native vs wasm. Xem ensure_sdl3_native
# va build_wasm.
CMAKE_MIN_VERSION="3.16"
EMSDK_VERSION="3.1.72"
SDL3_VERSION_MIN="3.2.0"
SDL3_VERSION="3.2.18"   # default pin neu native khong co SDL3
DETECTED_SDL3_VERSION="" # se duoc set boi ensure_sdl3_native neu phat hien

BUILD_MODE="${1:-native}"

# -----------------------------------------------------------------------------
# Logging
# -----------------------------------------------------------------------------
log_info()  { echo -e "\033[0;36m[INFO]\033[0m  $*"; }
log_warn()  { echo -e "\033[0;33m[WARN]\033[0m  $*"; }
log_error() { echo -e "\033[0;31m[ERROR]\033[0m $*"; }
log_ok()    { echo -e "\033[0;32m[OK]\033[0m    $*"; }

# In thong tin nen ngay tu dau de debug CI
log_info "OS detected: $OS_NAME"
log_info "Downloads dir: $DOWNLOAD_DIR"

# -----------------------------------------------------------------------------
# Helper version semver: "3.1.72" >= "3.0" -> true
# -----------------------------------------------------------------------------
version_ge() {
    [ -z "$2" ] && return 0
    [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | head -n1)" = "$2" ]
}

# Lay version "X.Y.Z" tu output cua --version
get_cmd_version() {
    "$1" --version 2>&1 | head -n1 \
        | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1
}

# Check command + version >= min
check_cmd_version() {
    local cmd="$1" min="$2" cur
    command -v "$cmd" >/dev/null 2>&1 || return 1
    cur=$(get_cmd_version "$cmd")
    [ -z "$cur" ] && return 0  # khong parse duoc -> coi nhu OK
    if version_ge "$cur" "$min"; then
        return 0
    fi
    log_warn "$cmd version $cur < $min (yeu cau)"
    return 1
}

get_sudo() {
    if [ "$EUID" -eq 0 ]; then echo ""
    elif command -v sudo &>/dev/null; then echo "sudo"
    else echo ""; fi
}


# (Removed all package manager wrappers and install logic)

# =============================================================================
# nanosvg -- tai vao $DOWNLOAD_DIR/nanosvg/, KHONG copy vao src/
# CMakeLists tu them path nay vao include dirs qua bien NANOSVG_INCLUDE_DIR.
# =============================================================================
ensure_nanosvg() {
    local nano_dir="$DOWNLOAD_DIR/nanosvg"
    local nano="$nano_dir/nanosvg.h"
    local nano_rast="$nano_dir/nanosvgrast.h"

    # Priority 1: da co trong source tree (vendored / committed) -> bo qua
    if [ -f "$APP_DIR/src/gameStory/include/nanosvg.h" ] && \
       [ -f "$APP_DIR/src/gameStory/include/nanosvgrast.h" ]; then
        log_ok "nanosvg da co trong source tree (vendored)"
        return 0
    fi

    # Priority 2: da co trong libs/<OS>/downloads/nanosvg/
    if [ -f "$nano" ] && [ -f "$nano_rast" ]; then
        log_ok "nanosvg da co tai $nano_dir"
        return 0
    fi

    # Priority 3: download
    log_info "Tai nanosvg headers vao $nano_dir..."
    mkdir -p "$nano_dir"
    local base="https://raw.githubusercontent.com/memononen/nanosvg/master/src"
    curl -fsSL "$base/nanosvg.h"     -o "$nano"
    curl -fsSL "$base/nanosvgrast.h" -o "$nano_rast"
    log_ok "nanosvg da san sang tai $nano_dir"
}

# =============================================================================
# emsdk -- multi-source detection, uu tien resource da co truoc khi clone
# Priority chain:
#   1. emcc/em++ da o tren PATH (vd: user da source emsdk_env.sh truoc khi run)
#   2. ~/emsdk (user's personal install)
#   3. $DOWNLOAD_DIR/emsdk (managed boi script nay)
#   4. Clone moi vao $DOWNLOAD_DIR/emsdk
# Sau khi tim duoc:
#   - Neu version >= EMSDK_VERSION: dung luon, KHONG install/activate lai
#   - Neu version < EMSDK_VERSION: install + activate target tai cung path
# =============================================================================
ensure_emsdk() {
    # Priority 1: em++ da co tren PATH
    if command -v em++ >/dev/null 2>&1; then
        local cur
        cur=$(em++ --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
        if [ -n "$cur" ] && version_ge "$cur" "$EMSDK_VERSION"; then
            log_ok "em++ da o PATH version $cur >= $EMSDK_VERSION (skip install)"
            return 0
        fi
        log_warn "em++ tren PATH version $cur < $EMSDK_VERSION, can update"
    fi

    # Priority 2: ~/emsdk (user's personal install)
    local user_emsdk="$HOME/emsdk"
    if [ -f "$user_emsdk/emsdk_env.sh" ]; then
        log_info "Phat hien $user_emsdk -- dung emsdk cua user"
        # shellcheck disable=SC1091
        source "$user_emsdk/emsdk_env.sh" >/dev/null 2>&1 || true
        if command -v em++ >/dev/null 2>&1; then
            local cur
            cur=$(em++ --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
            if [ -n "$cur" ] && version_ge "$cur" "$EMSDK_VERSION"; then
                log_ok "~/emsdk version $cur >= $EMSDK_VERSION (skip install)"
                return 0
            fi
            log_info "~/emsdk version $cur < $EMSDK_VERSION, install + activate target..."
            ( cd "$user_emsdk" && ./emsdk install "$EMSDK_VERSION" && ./emsdk activate "$EMSDK_VERSION" )
            # shellcheck disable=SC1091
            source "$user_emsdk/emsdk_env.sh"
            return 0
        fi
    fi

    # Priority 3: managed download dir
    local managed="$DOWNLOAD_DIR/emsdk"
    if [ -f "$managed/emsdk_env.sh" ]; then
        log_info "Phat hien managed emsdk tai $managed"
        # shellcheck disable=SC1091
        source "$managed/emsdk_env.sh" >/dev/null 2>&1 || true
        if command -v em++ >/dev/null 2>&1; then
            local cur
            cur=$(em++ --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
            if [ -n "$cur" ] && version_ge "$cur" "$EMSDK_VERSION"; then
                log_ok "managed emsdk version $cur >= $EMSDK_VERSION (skip install)"
                return 0
            fi
        fi
        log_info "Update managed emsdk len $EMSDK_VERSION..."
        ( cd "$managed" && ./emsdk install "$EMSDK_VERSION" && ./emsdk activate "$EMSDK_VERSION" )
        # shellcheck disable=SC1091
        source "$managed/emsdk_env.sh"
        return 0
    fi

    # Priority 4: clone moi (last resort)
    log_info "Khong tim thay emsdk, clone vao $managed..."
    mkdir -p "$DOWNLOAD_DIR"
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$managed"
    ( cd "$managed" && ./emsdk install "$EMSDK_VERSION" && ./emsdk activate "$EMSDK_VERSION" )
    # shellcheck disable=SC1091
    source "$managed/emsdk_env.sh"
    log_ok "emsdk active: $(em++ --version | head -n1)"
}

# =============================================================================
# SDL3 native -- always source-build to match Windows behavior.
# (Removed: pkg-config detection, sdl3-config detection, brew/apt/dnf/pacman
#  package manager paths. All OSes now use $DOWNLOAD_DIR/sdl3-native/.)
# =============================================================================
ensure_sdl3_native() {
    log_info "SDL3 native -> source-build $SDL3_VERSION vao $DOWNLOAD_DIR/sdl3-native/"
    DETECTED_SDL3_VERSION="$SDL3_VERSION"
    ensure_linux_dev_libs   # no-op tren macOS; cai X11/Wayland/GL dev libs tren Linux
    build_sdl3_from_source "$DOWNLOAD_DIR/sdl3-native" "native" "$SDL3_VERSION"
}

# =============================================================================
# SDL3 WASM -- KHONG dung -sUSE_SDL=3 (port emsdk khong on dinh).
# Tu build SDL3 tinh tai $DOWNLOAD_DIR/sdl3-wasm/, find_package(SDL3) trong
# CMakeLists tim duoc qua CMAKE_PREFIX_PATH.
# =============================================================================
ensure_sdl3_wasm() {
    # Quyet dinh version SDL3 cho WASM:
    #   - Neu native da detect duoc SDL3 (DETECTED_SDL3_VERSION khac rong)
    #     -> dung version DO de tranh dij ban native vs wasm.
    #   - Neu khong (truong hop CI hoac he thong khong co SDL3) -> dung
    #     SDL3_VERSION pin trong script.
    local target_version="${DETECTED_SDL3_VERSION:-$SDL3_VERSION}"
    log_info "WASM SDL3 target version: $target_version"

    # Cache directory bind theo version: doi version -> cache key khac ->
    # tu rebuild tinh tai vao thu muc rieng. Cho phep parallel cache nhieu version.
    local install_dir="$DOWNLOAD_DIR/sdl3-wasm-$target_version"

    log_info "Checking SDL3 WASM cache tai $install_dir..."

    # Priority 1: Cache tu lan build truoc (cung version)
    if [ -d "$install_dir/lib/cmake/SDL3" ] || [ -d "$install_dir/lib64/cmake/SDL3" ]; then
        if find "$install_dir" -name 'libSDL3*.a' -print -quit 2>/dev/null | grep -q .; then
            log_ok "SDL3 WASM cache HIT version $target_version -- skip rebuild (~1-2 phut tiet kiem)"
            return 0
        fi
        log_warn "Cache co cmake config nhung thieu .a -- rebuild"
    else
        log_info "Cache MISS version $target_version -- chua build SDL3 cho WASM lan nay"
    fi

    # Note quan trong: System SDL3 (brew/apt) la binary native arch
    # (x86_64/arm64), KHONG TUONG THICH voi WASM target (wasm32).
    # Phai build SDL3 RIENG voi Emscripten toolchain, MATCH dung version
    # cua native de tranh dij ban API.
    log_info "System SDL3 (neu co) la native arch -- KHONG dung duoc cho wasm32"
    log_info "Build SDL3 $target_version cho WASM target (lan dau, ~1-2 phut)..."

    build_sdl3_from_source "$install_dir" "wasm" "$target_version"
}

# Helper: build SDL3 tu source. Tham so:
#   $1 = install prefix
#   $2 = "native" hoac "wasm"
#   $3 = SDL3 version (vd "3.4.4" hoac "3.2.18")
build_sdl3_from_source() {
    local install_prefix="$1" target="$2" sdl_ver="$3"
    # Clone source vao thu muc rieng theo version -- cho phep cache nhieu
    # version song song neu can (vi du test version moi).
    local sdl_src="$DOWNLOAD_DIR/SDL-$sdl_ver"
    local sdl_build="$sdl_src/build-$target"

    mkdir -p "$DOWNLOAD_DIR"
    if [ ! -f "$sdl_src/CMakeLists.txt" ]; then
    if [ -d "$sdl_src" ]; then
        log_warn "SDL3 source dir ton tai nhung thieu CMakeLists.txt (cache bi hong) -- xoa va re-clone"
        rm -rf "$sdl_src"
    fi
    log_info "Clone SDL3 release-$sdl_ver vao $sdl_src..."
    git clone --depth 1 --branch "release-$sdl_ver" \
        https://github.com/libsdl-org/SDL "$sdl_src"
    else
        log_ok "SDL3 source $sdl_ver da co tai $sdl_src"
    fi
    # Cau hinh build:
    #   - native: shared lib, install vao prefix
    #   - wasm:   static lib (Emscripten khong support .so), tat tests
    local cmake_cmd="cmake"
    local extra_flags=()
    if [ "$target" = "wasm" ]; then
        cmake_cmd="emcmake cmake"
        extra_flags=(-DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF)
    else
        # FIX: Force disable optional features khong dung trong cTetris -- giam
        # mat ~12 dev packages khi build CI Ubuntu (dbus/udev/wayland/pulse/alsa/ibus).
        # Game chi can X11 + OpenGL + dummy audio (chua co audio system).
        # Neu sau nay can audio: doi SDL_DUMMYAUDIO=OFF + SDL_PULSEAUDIO=ON.
        extra_flags=(-DSDL_SHARED=ON
                     -DSDL_X11=ON -DSDL_OPENGL=ON -DSDL_OPENGLES=ON
                     -DSDL_PULSEAUDIO=ON -DSDL_ALSA=ON
                     -DSDL_WAYLAND=OFF -DSDL_KMSDRM=OFF
                     -DSDL_SNDIO=OFF -DSDL_IBUS=OFF -DSDL_HIDAPI_LIBUSB=OFF)
    fi

    eval "$cmake_cmd" -S "$sdl_src" -B "$sdl_build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$install_prefix" \
        "${extra_flags[@]}"
    cmake --build "$sdl_build" -j

    # Install vao local prefix (khong can sudo)
    cmake --install "$sdl_build"
    log_ok "SDL3 $sdl_ver ($target) da install vao $install_prefix"
}


# Icon: Only copy pre-created icon from brandkit to build output
copy_desktop_icon() {
    local out_dir="$1"
    mkdir -p "$out_dir"
    case "$OS_NAME" in
        macos)
            local icns_src="$BRANDKIT_DIR/logo.icns"
            local icns_dst="$out_dir/cTetris.icns"
            if [ -f "$icns_src" ]; then
                cp "$icns_src" "$icns_dst"
                log_ok "Copied icon: $icns_dst"
            else
                log_warn "No $icns_src found, using default OS icon."
            fi
            ;;
        windows)
            local ico_src="$BRANDKIT_DIR/logo.ico"
            local ico_dst="$out_dir/cTetris.ico"
            if [ -f "$ico_src" ]; then
                cp "$ico_src" "$ico_dst"
                log_ok "Copied icon: $ico_dst"
            else
                log_warn "No $ico_src found, using default OS icon."
            fi
            ;;
        ubuntu|fedora|arch|alpine|opensuse|linux)
            local svg_src="$BRANDKIT_DIR/logo.svg"
            local svg_dst="$out_dir/cTetris.svg"
            if [ -f "$svg_src" ]; then
                cp "$svg_src" "$svg_dst"
                log_ok "Copied icon: $svg_dst"
            else
                log_warn "No $svg_src found, using default OS icon."
            fi
            ;;
    esac
}
ensure_linux_dev_libs() {
    case "$OS_NAME" in
        ubuntu)
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
            # Removed install_apt_packages_if_missing (no package install in CI)
            ;;
        fedora)
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
            ;;
        arch)
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
            ;;
        alpine)
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
            ;;
        macos)
            : # macOS khong can dev libs -- SDL3 brew tu lo
            ;;
        *)
            log_warn "OS $OS_NAME khong duoc nhan dien, bo qua dev libs"
            ;;
    esac
}

# =============================================================================
# Tools co ban (cmake/git/python3) cho moi loai build
# =============================================================================
ensure_basic_tools() {
    case "$OS_NAME" in
        macos)
            command -v brew >/dev/null 2>&1 || {
                log_error "Homebrew chua cai. Cai tu https://brew.sh/"
                return 1
            }
            local pkgs=()
            check_cmd_version cmake "$CMAKE_MIN_VERSION" || pkgs+=(cmake)
            command -v git >/dev/null 2>&1     || pkgs+=(git)
            command -v python3 >/dev/null 2>&1 || pkgs+=(python3)
            [ ${#pkgs[@]} -gt 0 ] && install_brew_formulas_if_missing "${pkgs[@]}" \
                                  || log_ok "Basic tools da co"
            ;;
        ubuntu)
            # Removed install_apt_packages_if_missing (no package install in CI)
            ;;
        fedora)
            install_dnf_packages_if_missing gcc-c++ make cmake git curl python3 ca-certificates pkgconf-pkg-config
            ;;
        arch)
            install_pacman_packages_if_missing base-devel cmake git curl python ca-certificates pkgconf
            ;;
        alpine)
            install_apk_packages_if_missing build-base cmake git curl python3 ca-certificates pkgconfig
            ;;
        *)
            log_warn "OS $OS_NAME, basic tools check skipped"
            ;;
    esac
}

# =============================================================================
# Validate truoc khi build: cac source file & header bat buoc da co
# =============================================================================
validate_sources() {
    local missing=()
    local required=(
        "$APP_DIR/main.cpp"
        "$APP_DIR/src/gameStory/app.cpp"
        "$APP_DIR/src/gameConsole/app.cpp"
        "$APP_DIR/src/gameCore/app.cpp"
        "$APP_DIR/src/gameStory/include/gameStory_layout.h"
        "$APP_DIR/src/gameStory/include/gameStory_logo_svg.h"
        "$APP_DIR/src/gameStory/include/gameStory_corp_svg.h"
        "$APP_DIR/src/gameConsole/include/gameConsole_layout.h"
        "$APP_DIR/src/gameCore/include/gameCore_layout.h"
        "$APP_DIR/CMakeLists.txt"
    )
    for f in "${required[@]}"; do
        [ -f "$f" ] || missing+=("$f")
    done
    if [ ${#missing[@]} -gt 0 ]; then
        log_error "Thieu source file (phai duoc commit san trong repo):"
        for f in "${missing[@]}"; do log_error "  - $f"; done
        log_error "Build script KHONG sinh file co content cung -- vui long"
        log_error "commit cac file thieu vao repo truoc khi build."
        exit 1
    fi
    log_ok "Source files validated"
}

# =============================================================================
# Build entry points
# =============================================================================
build_native() {
    log_info "Build NATIVE mode -> $BUILD_NATIVE_DIR"
    validate_sources
    ensure_sdl3_native
    ensure_nanosvg

    # Copy icon from brandkit to build output
    copy_desktop_icon "$BUILD_NATIVE_DIR"

    # Tim path SDL3Config.cmake de truyen truc tiep qua SDL3_DIR.
    local sdl_dir_arg=()
    for candidate in \
        "$DOWNLOAD_DIR/sdl3-native/lib/cmake/SDL3" \
        "$DOWNLOAD_DIR/sdl3-native/lib64/cmake/SDL3"; do
        if [ -f "$candidate/SDL3Config.cmake" ]; then
            sdl_dir_arg=(-DSDL3_DIR="$candidate")
            log_info "SDL3_DIR = $candidate"
            break
        fi
    done

    # Truyen path icon (neu da copy) cho CMake de embed vao bundle/exe
    local icon_arg=()
    case "$OS_NAME" in
        macos)
            local icns="$BUILD_NATIVE_DIR/cTetris.icns"
            [ -f "$icns" ] && icon_arg=(-DCTETRIS_ICON_PATH="$icns")
            ;;
        windows)
            local ico="$BUILD_NATIVE_DIR/cTetris.ico"
            [ -f "$ico" ] && icon_arg=(-DCTETRIS_ICON_PATH="$ico")
            ;;
        ubuntu|fedora|arch|alpine|opensuse|linux)
            local svg="$BUILD_NATIVE_DIR/cTetris.svg"
            [ -f "$svg" ] && icon_arg=(-DCTETRIS_ICON_PATH="$svg")
            ;;
    esac

    mkdir -p "$BUILD_NATIVE_DIR"
    cmake -S "$APP_DIR" -B "$BUILD_NATIVE_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_WASM=OFF \
          -DCMAKE_PREFIX_PATH="$DOWNLOAD_DIR/sdl3-native" \
          -DNANOSVG_INCLUDE_DIR="$DOWNLOAD_DIR/nanosvg" \
          "${sdl_dir_arg[@]}" \
          "${icon_arg[@]}"
    cmake --build "$BUILD_NATIVE_DIR" -j
    log_ok "Native build hoan tat: $BUILD_NATIVE_DIR"
}

build_wasm() {
    log_info "Build WASM mode -> $BUILD_WASM_DIR"
    validate_sources

    # Native va WASM dung CHUNG version SDL3 -- ensure_sdl3_native chay
    # truoc de detect version brew/apt/dnf cai (set DETECTED_SDL3_VERSION),
    # roi ensure_sdl3_wasm dung chinh version do build tinh tai cho wasm32.
    # Neu khong tim duoc native (vd CI fresh), fallback ve SDL3_VERSION pin.
    log_info "Detect SDL3 native version de match cho WASM..."
    ensure_sdl3_native || true

    ensure_emsdk
    ensure_sdl3_wasm
    ensure_nanosvg

    # Derive sdl_install path tu version da quyet dinh trong ensure_sdl3_wasm
    local target_version="${DETECTED_SDL3_VERSION:-$SDL3_VERSION}"
    local sdl_install="$DOWNLOAD_DIR/sdl3-wasm-$target_version"

    # Tim chinh xac thu muc chua SDL3Config.cmake -- truyen qua SDL3_DIR
    # de bypass van de Emscripten root path mode.
    local sdl_dir=""
    for candidate in \
        "$sdl_install/lib/cmake/SDL3" \
        "$sdl_install/lib64/cmake/SDL3"; do
        if [ -f "$candidate/SDL3Config.cmake" ]; then
            sdl_dir="$candidate"
            break
        fi
    done
    if [ -z "$sdl_dir" ]; then
        log_error "Khong tim thay SDL3Config.cmake trong $sdl_install/lib*/cmake/SDL3/"
        log_error "Co the build SDL3 WASM bi loi -- xem log build_sdl3_from_source"
        exit 1
    fi
    log_info "SDL3_DIR = $sdl_dir"

    mkdir -p "$BUILD_WASM_DIR"
    emcmake cmake -S "$APP_DIR" -B "$BUILD_WASM_DIR" \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DBUILD_WASM=ON \
                  -DSDL3_DIR="$sdl_dir" \
                  -DCMAKE_PREFIX_PATH="$sdl_install" \
                  -DNANOSVG_INCLUDE_DIR="$DOWNLOAD_DIR/nanosvg"
    cmake --build "$BUILD_WASM_DIR" -j

    # Favicon SVG-driven -- copy tu brandkit
    if [ -f "$BRAND_LOGO_SVG" ]; then
        cp "$BRAND_LOGO_SVG" "$BUILD_WASM_DIR/favicon.svg"
        log_ok "Copy favicon.svg (SVG-driven)"
    else
        log_warn "Khong co $BRAND_LOGO_SVG, favicon se thieu"
    fi
    # Fallback favicon.ico cho browser khong ho tro SVG favicon
    local brand_ico="$BRANDKIT_DIR/logo.ico"
    if [ -f "$brand_ico" ]; then
        cp "$brand_ico" "$BUILD_WASM_DIR/favicon.ico"
        log_ok "Copy favicon.ico (ICO fallback)"
    else
        log_warn "Khong co $brand_ico, favicon.ico fallback se thieu"
    fi

    # PWA assets -- copy tu web/ (da commit san, khong sinh on-the-fly)
    # manifest.webmanifest + sw.js can thiet de browser hien nut Install
    # va Service Worker hoat dong (offline + full-height standalone mode).
    for asset in manifest.webmanifest sw.js; do
        local src="$WEB_DIR/$asset"
        if [ -f "$src" ]; then
            cp "$src" "$BUILD_WASM_DIR/$asset"
            log_ok "Copy PWA asset: $asset"
        else
            log_warn "Khong co $src -- PWA se thieu $asset"
        fi
    done

    log_ok "WASM build hoan tat: $BUILD_WASM_DIR"
}

case "$BUILD_MODE" in
    native) build_native ;;
    wasm)   build_wasm ;;
    all)    build_native; build_wasm ;;
    clean)
        log_info "Don dep build artifacts (giu lai libs/ cache)..."
        rm -rf "$APP_DIR/build"
        log_ok "Da xoa $APP_DIR/build (libs/$OS_NAME/downloads van con de cache)"
        ;;
    deepclean)
        log_info "Don dep TOAN BO (build/ + libs/)..."
        rm -rf "$APP_DIR/build" "$APP_DIR/libs"
        log_ok "Da xoa build/ va libs/"
        ;;
    *)
        echo "Usage: $0 [native|wasm|all|clean|deepclean]"
        exit 1
        ;;
esac