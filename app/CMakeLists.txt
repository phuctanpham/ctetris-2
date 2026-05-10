# =============================================================================
# CMakeLists.txt -- Cau hinh build cTetris voi SDL3
# =============================================================================
# THAY DOI v1:
#   - Drop hoan toan phan embed icon binary cho native build:
#     * KHONG con tao file .rc + RC_ICON cho Windows .exe
#     * KHONG con sinh/copy .icns vao Resources cua macOS .app (chi copy neu co)
#     * KHONG con cai .desktop voi Icon=PNG cho Linux
#     => Desktop apps dung icon mac dinh cua OS, gon nhe va khong phu thuoc
#        vao buoc rasterize PNG/ICO/ICNS. Neu muon icon, chi copy tu brandkit.
#   - WASM build chi can copy mot file favicon.svg duy nhat. Khong copy
#     manifest.webmanifest va khong copy sw.js -- yeu cau v1 la disable
#     PWA install prompt va dung SVG-driven favicon thay vi PNG.
#f
# THAY DOI v3 (SDL3 self-built cho WASM):
#   - DROP "-sUSE_SDL=3" (Emscripten port khong hoat dong on dinh tren
#     moi version emsdk -- 3.1.72 thieu / loi port SDL3).
#   - Thay vao do, cTetris find_package(SDL3 CONFIG) cho CA HAI branch:
#     * Native: SDL3 cai tu Homebrew/distro/source vao /usr/local hoac
#       /opt/homebrew (find_package tu nhan).
#     * WASM:   build.sh hoặc build.ps1 tu build SDL3 tinh tai $APP_DIR/.sdl3-wasm/ va
#       truyen vao -DCMAKE_PREFIX_PATH; find_package se tim duoc.
#   - Loi the: dung cung mot trinh tu link SDL3::SDL3 cho moi platform,
#     chi khac nhau o link options dac thu cua Emscripten.
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(cTetris LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Cho phep gan flag BUILD_WASM=ON tu command line de switch sang Emscripten
option(BUILD_WASM "Build for WebAssembly via Emscripten" OFF)

# -----------------------------------------------------------------------------
# Source files cua game (modules: gameStory, gameConsole, gameCore)
# -----------------------------------------------------------------------------
set(GAME_SOURCES
    main.cpp
    src/gameStory/app.cpp
    src/gameConsole/app.cpp
    src/gameCore/app.cpp
)

# Include directories: moi module co thu muc include rieng
set(GAME_INCLUDE_DIRS
    src/gameStory/include
    src/gameConsole/include
    src/gameCore/include
)

# -----------------------------------------------------------------------------
# Tao executable (ten dau ra: cTetris)
# -----------------------------------------------------------------------------
add_executable(cTetris ${GAME_SOURCES})
target_include_directories(cTetris PRIVATE ${GAME_INCLUDE_DIRS})

# -----------------------------------------------------------------------------
# nanosvg headers: build.sh hoặc build.ps1 truyen path qua -DNANOSVG_INCLUDE_DIR.
# Neu khong co (vi du user mo project trong IDE truc tiep), thu tim trong
# source tree (vendored) hoac libs/<OS>/downloads/nanosvg/.
# -----------------------------------------------------------------------------
if(NANOSVG_INCLUDE_DIR AND EXISTS "${NANOSVG_INCLUDE_DIR}/nanosvg.h")
    target_include_directories(cTetris PRIVATE "${NANOSVG_INCLUDE_DIR}")
    message(STATUS "[cTetris] nanosvg: ${NANOSVG_INCLUDE_DIR}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/src/gameStory/include/nanosvg.h")
    message(STATUS "[cTetris] nanosvg: source tree (vendored)")
else()
    message(WARNING "[cTetris] nanosvg headers khong tim thay -- compile co the fail")
endif()

# -----------------------------------------------------------------------------
# nlohmann/json headers: build.sh hoặc build.ps1 truyen path qua -DNLOHMANN_INCLUDE_DIR.
# -----------------------------------------------------------------------------
if(NLOHMANN_INCLUDE_DIR AND EXISTS "${NLOHMANN_INCLUDE_DIR}/nlohmann/json.hpp")
    target_include_directories(cTetris PRIVATE "${NLOHMANN_INCLUDE_DIR}")
    message(STATUS "[cTetris] nlohmann/json: ${NLOHMANN_INCLUDE_DIR}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/src/gameStory/include/nlohmann/json.hpp")
    target_include_directories(cTetris PRIVATE "${CMAKE_SOURCE_DIR}/src/gameStory/include")
    message(STATUS "[cTetris] nlohmann/json: source tree (vendored)")
else()
    message(WARNING "[cTetris] nlohmann/json header khong tim thay -- compile co the fail")
endif()

# -----------------------------------------------------------------------------
# SDL3 dependency: tim qua CMake config package.
# Cho native build, SDL3 nam o /usr/local/lib/cmake/SDL3 hoac
# /opt/homebrew/lib/cmake/SDL3 (Homebrew silicon).
# Cho WASM build, build.sh hoặc build.ps1
# install SDL3 tinh vao $APP_DIR/libs/<OS>/downloads/sdl3-wasm/ va truyen
# -DSDL3_DIR=... + -DCMAKE_PREFIX_PATH=... khi goi emcmake.
# -----------------------------------------------------------------------------
# CRITICAL: Emscripten toolchain (Emscripten.cmake) mac dinh set
#   CMAKE_FIND_ROOT_PATH_MODE_PACKAGE = ONLY
# nghia la find_package CHI search trong sysroot cua emsdk va BO QUA
# CMAKE_PREFIX_PATH cua user. Day la cu phap bao ve cross-compile, nhung
# trong project nay ta CO tinh truyen path den SDL3 da build cho wasm32 --
# nen phai mo rong sang BOTH (ca sysroot va prefix path) khi build WASM.
if(BUILD_WASM)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
endif()

find_package(SDL3 REQUIRED CONFIG)
target_link_libraries(cTetris PRIVATE SDL3::SDL3)
message(STATUS "[cTetris] SDL3 found: ${SDL3_DIR}")

# -----------------------------------------------------------------------------
# Branching theo platform: WASM vs Native (chi khac nhau o link options +
# bundle properties; SDL3 link da o tren chung cho ca 2)
# -----------------------------------------------------------------------------
if(BUILD_WASM)
    # -------------------------------------------------------------------------
    # WASM build (Emscripten)
    # -------------------------------------------------------------------------
    message(STATUS "[cTetris] Build mode: WASM (Emscripten)")

    # Link options dac thu cua Emscripten:
    #   - ALLOW_MEMORY_GROWTH=1: cho heap WASM tu cap phat them khi can,
    #     tranh OOM khi runtime alloc nhieu.
    #   - ASYNCIFY=1: cho phep dung vong lap dong bo + SDL_PollEvent
    #     thay vi phai chia tach main loop qua emscripten_set_main_loop.
    #   - shell-file: HTML wrapper tuy bien (favicon, title, no PWA).
    #   - preload-file: Dong goi file JSON vao virtual filesystem de doc tai runtime.
    # Dung "SHELL:..." prefix de CMake KHONG deduplicate / mangle args.
    target_link_options(cTetris PRIVATE
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sASYNCIFY=1"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/src/gameConsole/gameConsole_board.json@gameConsole_board.json"
    )

    # Sinh ra cTetris.html (kem .js + .wasm) thay vi binary mac dinh
    set_target_properties(cTetris PROPERTIES SUFFIX ".html")

    message(STATUS "[cTetris] Favicon: SVG-driven (logo.svg). PWA install prompt: DISABLED")

else()
    # -------------------------------------------------------------------------
    # Native build (macOS / Linux / Windows)
    # -------------------------------------------------------------------------
    message(STATUS "[cTetris] Build mode: NATIVE")

    # ---------------------------------------------------------------------
    # macOS bundle: embed icon .icns vao Resources cua .app neu duoc cung
    # cap qua -DCTETRIS_ICON_PATH (do build.sh hoặc build.ps1 sinh tu brandkit/logo.svg
    # bang iconutil + sips + rsvg-convert). Neu khong co, OS dung icon
    # mac dinh.
    # ---------------------------------------------------------------------
    if(APPLE)
        set_target_properties(cTetris PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "cTetris"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.uit.ctetris"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        )
        if(CTETRIS_ICON_PATH AND EXISTS "${CTETRIS_ICON_PATH}")
            # Embed .icns in Resources and set as bundle icon
            set_source_files_properties("${CTETRIS_ICON_PATH}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources")
            target_sources(cTetris PRIVATE "${CTETRIS_ICON_PATH}")
            get_filename_component(_icon_name "${CTETRIS_ICON_PATH}" NAME)
            set_target_properties(cTetris PROPERTIES
                MACOSX_BUNDLE_ICON_FILE "${_icon_name}")
            message(STATUS "[cTetris] macOS icon: ${CTETRIS_ICON_PATH}")
        else()
            message(STATUS "[cTetris] macOS icon: default (CTETRIS_ICON_PATH not set)")
        endif()
    endif()

    # ---------------------------------------------------------------------
    # Linux: SDL_SetWindowIcon co the load SVG runtime (qua nanosvg trong app),
    # KHONG embed file vao binary -- de SVG nam canh executable trong build/.
    # Khong tao .desktop, ten cua so se hien thi qua SDL_CreateWindow().
    # ---------------------------------------------------------------------
    if(UNIX AND NOT APPLE)
        if(CTETRIS_ICON_PATH AND EXISTS "${CTETRIS_ICON_PATH}")
            # Install/copy SVG icon next to binary for Linux desktop integration
            install(FILES "${CTETRIS_ICON_PATH}" DESTINATION "." RENAME "cTetris.svg")
            message(STATUS "[cTetris] Linux icon SVG installed: ${CTETRIS_ICON_PATH}")
        endif()
    endif()

    # ---------------------------------------------------------------------
    # Windows: embed icon .ico vao .exe qua resource compiler (rc.exe).
    # Sinh file .rc tam tro den .ico, them vao source list cua target.
    # CMake/MSVC se tu compile .rc -> .res va link vao executable.
    # ---------------------------------------------------------------------
    if(WIN32)
        if(CTETRIS_ICON_PATH AND EXISTS "${CTETRIS_ICON_PATH}")
            # FIX: rc.exe interprets \a \b \t \n... nhu C escape sequences trong
            # quoted strings. Raw Windows backslash path -> path mangled.
            # Convert sang forward slashes (rc.exe accept ca 2 dang).
            file(TO_CMAKE_PATH "${CTETRIS_ICON_PATH}" _icon_rc_path)
            set(_rc_file "${CMAKE_BINARY_DIR}/cTetris_icon.rc")
            file(WRITE "${_rc_file}" "IDI_ICON1 ICON DISCARDABLE \"${_icon_rc_path}\"\n")
            target_sources(cTetris PRIVATE "${_rc_file}")
            message(STATUS "[cTetris] Windows icon embedded: ${_icon_rc_path}")
        else()
            message(STATUS "[cTetris] Windows icon: default (CTETRIS_ICON_PATH not set)")
        endif()
    endif()

    # Khong tao .rc cho Windows tu day -- icon chi copy tu brandkit neu co.
endif()

# -----------------------------------------------------------------------------
# Output: in tom tat cau hinh sau khi configure xong
# -----------------------------------------------------------------------------
message(STATUS "[cTetris] CXX standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "[cTetris] Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "[cTetris] Output target: cTetris")