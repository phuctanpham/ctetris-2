#!/usr/bin/env bash
set -e
echo "[gameConsole] Build script starting..."

# Detect platform
case "$(uname -s)" in
    Darwin*)   PLATFORM="macos";;
    Linux*)    PLATFORM="linux";;
    CYGWIN*|MINGW*|MSYS*) PLATFORM="windows";;
    *)         echo "Unsupported platform: $(uname -s)"; exit 1;;
esac
echo "Detected platform: $PLATFORM"

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "CMake not found. Please install CMake."
    exit 1
fi

# Check for SFML
if ! pkg-config --exists sfml-graphics; then
    echo "SFML not found. Please install SFML 3.x."
    exit 1
fi

# Build
mkdir -p build
cd build
cmake ..
make
cd ..

# Copy assets if needed
if [ -f gameConsole_board.json ]; then
    cp gameConsole_board.json build/
fi

echo "[gameConsole] Build complete."
#!/bin/sh
#set -e

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

check_sfml_version() {
    version=""
    config_paths="/usr/local/lib/cmake/SFML/SFMLConfigVersion.cmake /usr/lib/cmake/SFML/SFMLConfigVersion.cmake /opt/homebrew/lib/cmake/SFML/SFMLConfigVersion.cmake"
    if command_exists pkg-config; then
        version=$(pkg-config --modversion sfml-all 2>/dev/null || true)
    fi
    for path in $config_paths; do
        if [ -f "$path" ]; then
            version_file=$(grep "set(SFML_VERSION" "$path" | grep -o '[0-9]\.[0-9]\.[0-9]')
            if [ -n "$version_file" ]; then
                version="$version_file"
                break
            fi
    case "$version" in
        3.*)
            echo "SFML version 3.x detected: $version"
            echo "SFML not found."
            printf "Do you want to install SFML 3? [y/N]: "
            read install_sfml
            echo "SFML version $version detected (not 3.x)."
            printf "Do you want to clean up the old SFML version and reinstall SFML 3? [y/N]: "
            read cleanup
                [Yy]*)
                    echo "Cleaning up old SFML..."
                    sudo rm -rf /usr/local/lib/cmake/SFML /usr/local/include/SFML /usr/local/lib/libsfml* /usr/lib/cmake/SFML /usr/lib/libsfml* /opt/homebrew/lib/cmake/SFML /opt/homebrew/include/SFML /opt/homebrew/lib/libsfml*
                    return 1
                    ;;
                *) echo "Aborting build."; exit 1 ;;
            ;;
    esac
}
install_brew_deps_if_needed() {
    BREW_DEPS="bison flac libogg pkgconf cmake freetype libpng sfml doxygen giflib libvorbis"
    MISSING_DEPS=""
    for dep in $BREW_DEPS; do
        if ! brew list --formula | grep -q "^$dep$"; then
            MISSING_DEPS="$MISSING_DEPS $dep"
    if [ -n "$MISSING_DEPS" ]; then
        echo "Các thư viện sau chưa được cài đặt qua brew: $MISSING_DEPS"
        printf "Bạn có muốn cài đặt các thư viện này không? [y/N]: "
        case "$install_brew_deps" in
            [Yy]*) brew install $MISSING_DEPS ;;
            *) echo "Aborting build."; exit 1 ;;
        esac
    fi
}
echo "Select target platform to build for:"
echo "1) Ubuntu (Linux)"
echo "2) macOS"
printf "Enter choice [1-2]: "
read platform

if [ "$platform" = "1" ]; then
    TARGET="Ubuntu"
    check_sfml_version || {
        echo "Updating apt..."
        sudo apt update
        if ! command_exists cmake; then
            echo "Installing cmake..."
            sudo apt install -y cmake
        fi
        sudo apt install -y g++ libx11-dev libxrandr-dev libudev-dev libopengl-dev libflac-dev libvorbis-dev libopenal-dev libfreetype-dev libxcursor-dev libxi-dev libgl1-mesa-dev libmbedtls-dev libogg-dev libssh2-1-dev libharfbuzz-dev
        if ! command_exists git; then
            echo "Installing git..."
            sudo apt install -y git
        TMP_SFML_DIR="$HOME/SFML"
        if [ ! -d "$TMP_SFML_DIR" ]; then
            git clone https://github.com/SFML/SFML.git "$TMP_SFML_DIR"
    cd "$TMP_SFML_DIR"
    git fetch --all
    git checkout master
    git pull
    mkdir -p build && cd build
    rm -rf *
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    sudo make install
        sudo ldconfig
        cd -
    }
elif [ "$platform" = "2" ]; then
    TARGET="macOS"
    check_sfml_version || {
        if ! command_exists brew; then
            echo "Homebrew not found. Installing..."
            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            eval "$($(brew --prefix)/bin/brew shellenv)"
        fi
        brew update
        install_brew_deps_if_needed
    }
else
    echo "Invalid choice. Exiting."
    exit 1
fi
if [ -d "build" ]; then
    printf "Thư mục build đã tồn tại. Bạn có muốn xoá thư mục build cũ không? [y/N]: "
    read remove_build
    case "$remove_build" in
        [Yy]*)
            echo "Đang xoá thư mục build cũ..."
            rm -rf build
            echo "Đã xoá thư mục build cũ."
            ;;
    esac
fi
if [ ! -d "build" ]; then
    echo "Tạo thư mục build..."
    mkdir build
fi

cd build

echo "Running CMake for $TARGET..."
cmake ..

echo "Building..."
make

echo "To run the executable, use:"
echo "  ./build/gameConsole"
#!/bin/sh
set -e

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

check_sfml_version() {
    version=""
    config_paths="/usr/local/lib/cmake/SFML/SFMLConfigVersion.cmake /usr/lib/cmake/SFML/SFMLConfigVersion.cmake /opt/homebrew/lib/cmake/SFML/SFMLConfigVersion.cmake"
    if command_exists pkg-config; then
        version=$(pkg-config --modversion sfml-all 2>/dev/null || true)
    fi
    for path in $config_paths; do
        if [ -f "$path" ]; then
            version_file=$(grep "set(SFML_VERSION" "$path" | grep -o '[0-9]\.[0-9]\.[0-9]')
            if [ -n "$version_file" ]; then
                version="$version_file"
                break
            fi
        fi
    done
    case "$version" in
        3.*)
            echo "SFML version 3.x detected: $version"
            return 0
            ;;
        "")
            echo "SFML not found."
            printf "Do you want to install SFML 3? [y/N]: "
            read install_sfml
            case "$install_sfml" in
                [Yy]*) return 1 ;;
                *) echo "Aborting build."; exit 1 ;;
            esac
            ;;
        *)
            echo "SFML version $version detected (not 3.x)."
            printf "Do you want to clean up the old SFML version and reinstall SFML 3? [y/N]: "
            read cleanup
            case "$cleanup" in
                [Yy]*)
                    echo "Cleaning up old SFML..."
                    sudo rm -rf /usr/local/lib/cmake/SFML /usr/local/include/SFML /usr/local/lib/libsfml* /usr/lib/cmake/SFML /usr/lib/libsfml* /opt/homebrew/lib/cmake/SFML /opt/homebrew/include/SFML /opt/homebrew/lib/libsfml*
                    return 1
                    ;;
                *) echo "Aborting build."; exit 1 ;;
            esac
            ;;
    esac
}

install_brew_deps_if_needed() {
    BREW_DEPS="bison flac libogg pkgconf cmake freetype libpng sfml doxygen libvorbis"
    MISSING_DEPS=""
    for dep in $BREW_DEPS; do
        if ! brew list --formula | grep -q "^$dep$"; then
            MISSING_DEPS="$MISSING_DEPS $dep"
        fi
    done
    if [ -n "$MISSING_DEPS" ]; then
        echo "Các thư viện sau chưa được cài đặt qua brew: $MISSING_DEPS"
        printf "Bạn có muốn cài đặt các thư viện này không? [y/N]: "
        read install_brew_deps
        case "$install_brew_deps" in
            [Yy]*) brew install $MISSING_DEPS ;;
            *) echo "Aborting build."; exit 1 ;;
        esac
    fi
}

echo "Select target platform to build for:"
echo "1) Ubuntu (Linux)"
echo "2) macOS"
printf "Enter choice [1-2]: "
read platform

if [ "$platform" = "1" ]; then
    TARGET="Ubuntu"
    check_sfml_version || {
        echo "Updating apt..."
        sudo apt update
        if ! command_exists cmake; then
            echo "Installing cmake..."
            sudo apt install -y cmake
        fi
        sudo apt install -y g++ libx11-dev libxrandr-dev libudev-dev libopengl-dev libflac-dev libvorbis-dev libopenal-dev libfreetype-dev libxcursor-dev libxi-dev libgl1-mesa-dev libmbedtls-dev libogg-dev libssh2-1-dev libharfbuzz-dev
        if ! command_exists git; then
            echo "Installing git..."
            sudo apt install -y git
        fi
        TMP_SFML_DIR="$HOME/SFML"
        if [ ! -d "$TMP_SFML_DIR" ]; then
            git clone https://github.com/SFML/SFML.git "$TMP_SFML_DIR"
        fi
        cd "$TMP_SFML_DIR"
        git fetch --all
        git checkout master
        git pull
        mkdir -p build && cd build
        rm -rf *
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
        sudo make install
        sudo ldconfig
        cd -
    }
elif [ "$platform" = "2" ]; then
    TARGET="macOS"
    check_sfml_version || {
        if ! command_exists brew; then
            echo "Homebrew not found. Installing..."
            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            eval "$($(brew --prefix)/bin/brew shellenv)"
        fi
        brew update
        install_brew_deps_if_needed
    }
else
    echo "Invalid choice. Exiting."
    exit 1
fi

if [ -d "build" ]; then
    printf "Thư mục build đã tồn tại. Bạn có muốn xoá thư mục build cũ không? [y/N]: "
    read remove_build
    case "$remove_build" in
        [Yy]*)
            echo "Đang xoá thư mục build cũ..."
            rm -rf build
            echo "Đã xoá thư mục build cũ."
            ;;
    esac
fi
if [ ! -d "build" ]; then
    echo "Tạo thư mục build..."
    mkdir build
fi

cd build

echo "Running CMake for $TARGET..."
cmake ..

echo "Building..."
make

echo "To run the executable, use:"
echo "  ./build/gameConsole"
