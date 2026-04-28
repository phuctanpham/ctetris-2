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
# Branching theo platform: WASM (Emscripten) vs Native (macOS/Linux/Windows)
# -----------------------------------------------------------------------------
if(BUILD_WASM)
    # -------------------------------------------------------------------------
    # WASM build (Emscripten)
    # -------------------------------------------------------------------------
    message(STATUS "[cTetris] Build mode: WASM (Emscripten)")

    # USE_SDL=3 lay tu emsdk port -- tu link SDL3 san co
    set(EMS_FLAGS "-sUSE_SDL=3 -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY=1")
    set_target_properties(cTetris PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "${EMS_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
    )
    target_compile_options(cTetris PRIVATE -sUSE_SDL=3)

    # Copy favicon.svg ra cung cap voi cTetris.html. SVG-driven, khong PNG.
    # File favicon.svg do build.sh / build.ps1 copy tu brandkit/logo.svg
    # vao thu muc build SAU khi cmake --build hoan tat -- nen day chi can
    # khai bao quy uoc, khong can custom_command.
    message(STATUS "[cTetris] Favicon: SVG-driven (logo.svg). PWA install prompt: DISABLED")

else()
    # -------------------------------------------------------------------------
    # Native build (macOS / Linux / Windows)
    # -------------------------------------------------------------------------
    message(STATUS "[cTetris] Build mode: NATIVE")

    # Tim SDL3 thong qua sdl3-config / cmake config package
    find_package(SDL3 REQUIRED CONFIG)
    target_link_libraries(cTetris PRIVATE SDL3::SDL3)

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
