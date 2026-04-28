# =============================================================================
# CMakeLists.txt -- Cau hinh build cTetris voi SDL3
# =============================================================================
# THAY DOI v1:
#   - Drop hoan toan phan embed icon binary cho native build:
#     * KHONG con tao file .rc + RC_ICON cho Windows .exe
#     * KHONG con copy .icns vao Resources cua macOS .app
#     * KHONG con cai .desktop voi Icon=PNG cho Linux
#     => Desktop apps dung icon mac dinh cua OS, gon nhe va khong phu thuoc
#        vao buoc rasterize PNG/ICO/ICNS.
#   - WASM build chi can copy mot file favicon.svg duy nhat. Khong copy
#     manifest.webmanifest va khong copy sw.js -- yeu cau v1 la disable
#     PWA install prompt va dung SVG-driven favicon thay vi PNG.
#
# THAY DOI v3 (SDL3 self-built cho WASM):
#   - DROP "-sUSE_SDL=3" (Emscripten port khong hoat dong on dinh tren
#     moi version emsdk -- 3.1.72 thieu / loi port SDL3).
#   - Thay vao do, cTetris find_package(SDL3 CONFIG) cho CA HAI branch:
#     * Native: SDL3 cai tu Homebrew/distro/source vao /usr/local hoac
#       /opt/homebrew (find_package tu nhan).
#     * WASM:   build.sh tu build SDL3 tinh tai $APP_DIR/.sdl3-wasm/ va
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
# nanosvg headers: build.sh truyen path qua -DNANOSVG_INCLUDE_DIR.
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
# SDL3 dependency: tim qua CMake config package.
# Cho native build, SDL3 nam o /usr/local/lib/cmake/SDL3 hoac
# /opt/homebrew/lib/cmake/SDL3 (Homebrew silicon). Cho WASM build, build.sh
# install SDL3 tinh vao $APP_DIR/.sdl3-wasm/ va truyen
# -DCMAKE_PREFIX_PATH=... khi goi emcmake.
# -----------------------------------------------------------------------------
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
    # Dung "SHELL:..." prefix de CMake KHONG deduplicate / mangle args.
    target_link_options(cTetris PRIVATE
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sASYNCIFY=1"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
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
    # macOS bundle: chi cau hinh ten bundle, KHONG copy .icns nua.
    # OS se dung icon mac dinh cua macOS Finder.
    # ---------------------------------------------------------------------
    if(APPLE)
        set_target_properties(cTetris PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "cTetris"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.uit.ctetris"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        )
    endif()

    # Khong tao .rc cho Windows va khong cai .desktop cho Linux nua.
    # Window title da bao gom "cTetris (c)" -- du nhan dien duoc.
endif()

# -----------------------------------------------------------------------------
# Output: in tom tat cau hinh sau khi configure xong
# -----------------------------------------------------------------------------
message(STATUS "[cTetris] CXX standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "[cTetris] Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "[cTetris] Output target: cTetris")