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
project(cTetris LANGUAGES C CXX)

# Read MANIFEST_GIST_URL from .env (fallback if not passed via -D)
if(NOT DEFINED MANIFEST_GIST_URL OR MANIFEST_GIST_URL STREQUAL "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
        file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" _env_lines)
        foreach(_line ${_env_lines})
            if(_line MATCHES "^MANIFEST_GIST_URL=(.+)$")
                set(MANIFEST_GIST_URL "${CMAKE_MATCH_1}")
            endif()
        endforeach()
    endif()
endif()
if(MANIFEST_GIST_URL)
    message(STATUS "MANIFEST_GIST_URL: ${MANIFEST_GIST_URL}")
else()
    message(WARNING "MANIFEST_GIST_URL not set — gameStory sync will be offline-only")
endif()

# Read CTETRIS_API_URL from .env (fallback if not passed via -D)
if(NOT DEFINED CTETRIS_API_URL OR CTETRIS_API_URL STREQUAL "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
        file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" _env_lines_api)
        foreach(_line ${_env_lines_api})
            if(_line MATCHES "^CTETRIS_API_URL=(.+)$")
                set(CTETRIS_API_URL "${CMAKE_MATCH_1}")
            endif()
        endforeach()
    endif()
endif()

# ─── Read DEBUG flag from app/.env ────────────────────────────────────────────
# When DEBUG=true, enables -DCTETRIS_DEBUG=1 for HTTP request/response logging
set(CTETRIS_DEBUG_FLAG "false")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" _env_lines_debug)
    foreach(_line ${_env_lines_debug})
        if(_line MATCHES "^DEBUG=(.+)$")
            set(CTETRIS_DEBUG_FLAG "${CMAKE_MATCH_1}")
        endif()
    endforeach()
endif()

string(TOLOWER "${CTETRIS_DEBUG_FLAG}" _debug_lc)
if(_debug_lc STREQUAL "true" OR _debug_lc STREQUAL "1")
    message(STATUS "[cTetris] DEBUG=true  ->  -DCTETRIS_DEBUG=1 enabled (HTTP debug log)")
    # Note: cTetris target is created later; add definition then via target_compile_definitions
    set(CTETRIS_DEBUG_ENABLED ON)
else()
    message(STATUS "[cTetris] DEBUG=${CTETRIS_DEBUG_FLAG}  ->  release build (no HTTP debug log)")
    set(CTETRIS_DEBUG_ENABLED OFF)
endif()

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
    src/shared/nanosvg_impl.cpp
)

# Include directories: moi module co thu muc include rieng + src/ cho logger.h
set(GAME_INCLUDE_DIRS
    src
    src/gameStory/include
    src/gameConsole/include
    src/gameCore/include
)

# -----------------------------------------------------------------------------
# Tao executable (ten dau ra: cTetris)
# WIN32: an console window tren Windows, chi hien GUI (co hieu luc tren Windows only)
# -----------------------------------------------------------------------------
add_executable(cTetris WIN32 ${GAME_SOURCES})

if(CTETRIS_API_URL AND
   NOT CTETRIS_API_URL STREQUAL "https://ctetris-api.OWNER.workers.dev")
    target_compile_definitions(cTetris PRIVATE
        "CTETRIS_API_URL=\"${CTETRIS_API_URL}\""
    )
    message(STATUS "CTETRIS_API_URL: ${CTETRIS_API_URL}")
else()
    message(WARNING "CTETRIS_API_URL not set — leaderboard API sync disabled")
endif()
target_include_directories(cTetris PRIVATE ${GAME_INCLUDE_DIRS})

# Apply DEBUG compile definition if enabled
if(CTETRIS_DEBUG_ENABLED)
    target_compile_definitions(cTetris PRIVATE CTETRIS_DEBUG=1)
endif()

# Inject MANIFEST_GIST_URL define
if(MANIFEST_GIST_URL AND
   NOT MANIFEST_GIST_URL STREQUAL "https://gist.githubusercontent.com/OWNER/GIST_ID/raw/manifest.json")
    target_compile_definitions(cTetris PRIVATE
        "MANIFEST_GIST_URL=\"${MANIFEST_GIST_URL}\""
    )
endif()

# nlohmann already in GAME_INCLUDE_DIRS; guard in case it was added conditionally
if(DEFINED NLOHMANN_INCLUDE_DIR AND NOT NLOHMANN_INCLUDE_DIR STREQUAL "")
    target_include_directories(cTetris PRIVATE ${NLOHMANN_INCLUDE_DIR})
endif()

# libcurl: consolidated find_package lives at the BOTTOM of this file
# (after CMAKE_PREFIX_PATH from build.ps1's Initialize-Curl is in scope).
# Do not re-add a duplicate find_package(CURL) here.

# WASM: Asyncify needed for emscripten_fetch synchronous mode in gameStory
if(BUILD_WASM)
    target_link_options(cTetris PRIVATE -sASYNCIFY=1 -sFETCH=1)
endif()

# Add Windows SDK paths for linking on Windows
if(WIN32)
    target_link_directories(cTetris PRIVATE
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64"
    )
    # Use main() as entry point even with Windows subsystem (hides console)
    target_link_options(cTetris PRIVATE 
        /SUBSYSTEM:WINDOWS 
        /ENTRY:mainCRTStartup
    )
endif()

# -----------------------------------------------------------------------------
# nanosvg headers: build.sh hoặc build.ps1 truyen path qua -DNANOSVG_INCLUDE_DIR.
# Neu khong co (vi du user mo project trong IDE truc tiep), thu tim trong
# libs/<OS>/downloads/nanosvg/.
# -----------------------------------------------------------------------------
set(_nanosvg_include_dir "")
if(NANOSVG_INCLUDE_DIR AND EXISTS "${NANOSVG_INCLUDE_DIR}/nanosvg.h")
    set(_nanosvg_include_dir "${NANOSVG_INCLUDE_DIR}")
else()
    foreach(_cand IN ITEMS
        "${CMAKE_SOURCE_DIR}/libs/windows/downloads/nanosvg"
        "${CMAKE_SOURCE_DIR}/libs/macos/downloads/nanosvg"
        "${CMAKE_SOURCE_DIR}/libs/linux/downloads/nanosvg"
        "${CMAKE_SOURCE_DIR}/libs/ubuntu/downloads/nanosvg"
        "${CMAKE_SOURCE_DIR}/libs/fedora/downloads/nanosvg"
        "${CMAKE_SOURCE_DIR}/libs/arch/downloads/nanosvg"
        "${CMAKE_SOURCE_DIR}/libs/alpine/downloads/nanosvg"
        "${CMAKE_SOURCE_DIR}/libs/opensuse/downloads/nanosvg"
    )
        if(EXISTS "${_cand}/nanosvg.h")
            set(_nanosvg_include_dir "${_cand}")
            break()
        endif()
    endforeach()
endif()

if(_nanosvg_include_dir)
    target_include_directories(cTetris PRIVATE "${_nanosvg_include_dir}")
    message(STATUS "[cTetris] nanosvg: ${_nanosvg_include_dir}")
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
# SQLite amalgamation: build.sh/build.ps1 truyen path qua -DSQLITE_DIR.
# Compile sqlite3.c truc tiep voi target cTetris (giam phu thuoc shared lib).
# Tat warnings cho file nay vi day la third-party amalgamation co nhieu
# warning vo hai (unused-function, missing-prototypes, sign-compare).
# -----------------------------------------------------------------------------
if(SQLITE_DIR)
    target_sources(cTetris PRIVATE "${SQLITE_DIR}/sqlite3.c")
    target_include_directories(cTetris PRIVATE "${SQLITE_DIR}")

    # Common SQLite compile-time flags (giam binary size, omit features khong dung)
    target_compile_definitions(cTetris PRIVATE
        SQLITE_OMIT_LOAD_EXTENSION
        SQLITE_DEFAULT_FOREIGN_KEYS=1
        SQLITE_DQS=0
    )

    # WASM: bat buoc tat threadsafe vi single-thread runtime
    if(BUILD_WASM)
        target_compile_definitions(cTetris PRIVATE SQLITE_THREADSAFE=0)
    else()
        target_compile_definitions(cTetris PRIVATE SQLITE_THREADSAFE=1)
    endif()

    # Tat warnings cho sqlite3.c (third-party, khong sua duoc)
    if(MSVC)
        set_source_files_properties("${SQLITE_DIR}/sqlite3.c" PROPERTIES
            COMPILE_FLAGS "/W0 /wd4244 /wd4267 /wd4996")
    else()
        set_source_files_properties("${SQLITE_DIR}/sqlite3.c" PROPERTIES
            COMPILE_FLAGS "-w")
    endif()

    # Linux: sqlite3 can libdl + libpthread (neu THREADSAFE=1)
    if(UNIX AND NOT APPLE AND NOT BUILD_WASM)
        target_link_libraries(cTetris PRIVATE dl pthread)
    endif()

    message(STATUS "[cTetris] SQLite: ${SQLITE_DIR}")
else()
    message(WARNING "[cTetris] SQLite amalgamation khong tim thay -- Stories popup se compile fail")
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
    # No --preload-file: leaderboard is fetched from Cloudflare D1 via
    # CTETRIS_API_URL, chapter content + media is fetched per chapter from
    # MANIFEST_GIST_URL + mediaBaseUrl at runtime. Both endpoints are
    # validated by build.sh/build.ps1 before linking.

    # [DEBUG] Reuse the .env DEBUG flag for the WASM GUI. shell.html ships with
    # `var _debugEnabled = false;`; when DEBUG=true we flip it to true in a
    # generated copy and feed THAT to --shell-file (source file stays clean).
    # This turns on the in-browser debug toolbar + SDL_Log accumulation so the
    # WASM build is debuggable the same way DEBUG=1 helps the native build.
    set(_shell_src "${CMAKE_SOURCE_DIR}/web/shell.html")
    set(_shell_out "${CMAKE_BINARY_DIR}/shell.html")
    file(READ "${_shell_src}" _shell_content)
    if(CTETRIS_DEBUG_ENABLED)
        string(REPLACE "var _debugEnabled = false;"
                       "var _debugEnabled = true;"
                       _shell_content "${_shell_content}")
        message(STATUS "[cTetris] WASM shell debug toolbar: ENABLED (DEBUG=true)")
    else()
        message(STATUS "[cTetris] WASM shell debug toolbar: disabled")
    endif()
    file(WRITE "${_shell_out}" "${_shell_content}")

    target_link_options(cTetris PRIVATE
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sASYNCIFY=1"
        "SHELL:--shell-file ${_shell_out}"
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

    # -------------------------------------------------------------------------
    # Windows post-build: copy SDL3.dll to executable directory so runtime can find it
    # -------------------------------------------------------------------------
    if(WIN32)
        get_target_property(_SDL3_LOCATION SDL3::SDL3 IMPORTED_LOCATION_RELEASE)
        if(NOT _SDL3_LOCATION)
            get_target_property(_SDL3_LOCATION SDL3::SDL3 IMPORTED_LOCATION)
        endif()
        if(_SDL3_LOCATION)
            add_custom_command(TARGET cTetris POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_SDL3_LOCATION}"
                    "$<TARGET_FILE_DIR:cTetris>/SDL3.dll"
                COMMENT "[cTetris] Copying SDL3.dll to output directory"
            )
            message(STATUS "[cTetris] SDL3.dll post-build copy: ${_SDL3_LOCATION}")
        endif()
    endif()

    # No local copy of gameConsole_board.json or chapters/ — both are now
    # remote-sourced at runtime:
    #   - Leaderboard  → Cloudflare D1  (CTETRIS_API_URL/leaderboard)
    #   - Chapters     → Gist manifest  (MANIFEST_GIST_URL)
    #   - Media (PNG/MP3) → raw GitHub  (per-chapter mediaBaseUrl)
    # Endpoint reachability is verified by build.sh/build.ps1 pre-link.
endif()

# -----------------------------------------------------------------------------
# Output: in tom tat cau hinh sau khi configure xong
# -----------------------------------------------------------------------------
message(STATUS "[cTetris] CXX standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "[cTetris] Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "[cTetris] Output target: cTetris")

# ── Read all keys from app/.env ───────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.env" _ENV_LINES
         REGEX "^[A-Z_]+=.+")
else()
    message(WARNING "[cTetris] app/.env not found — API URL and debug flag not set")
endif()

# ── MANIFEST_GIST_URL ─────────────────────────────────────────────────────────
set(MANIFEST_GIST_URL "")
foreach(_line ${_ENV_LINES})
    if(_line MATCHES "^MANIFEST_GIST_URL=(.+)$")
        set(MANIFEST_GIST_URL "${CMAKE_MATCH_1}")
    endif()
endforeach()
if(MANIFEST_GIST_URL AND NOT MANIFEST_GIST_URL MATCHES "GIST_ID")
    target_compile_definitions(cTetris PRIVATE
        "MANIFEST_GIST_URL=\"${MANIFEST_GIST_URL}\"")
    message(STATUS "[cTetris] MANIFEST_GIST_URL set")
else()
    message(WARNING "[cTetris] MANIFEST_GIST_URL is placeholder — Gist sync disabled")
endif()

# ── CTETRIS_API_URL ───────────────────────────────────────────────────────────
set(CTETRIS_API_URL "")
foreach(_line ${_ENV_LINES})
    if(_line MATCHES "^CTETRIS_API_URL=(.+)$")
        set(CTETRIS_API_URL "${CMAKE_MATCH_1}")
    endif()
endforeach()
if(CTETRIS_API_URL AND NOT CTETRIS_API_URL MATCHES "OWNER")
    target_compile_definitions(cTetris PRIVATE
        "CTETRIS_API_URL=\"${CTETRIS_API_URL}\"")
    message(STATUS "[cTetris] CTETRIS_API_URL: ${CTETRIS_API_URL}")
else()
    message(WARNING "[cTetris] CTETRIS_API_URL not set — leaderboard API sync disabled")
endif()

# ── DEBUG flag ────────────────────────────────────────────────────────────────
set(CTETRIS_DEBUG_FLAG "false")
foreach(_line ${_ENV_LINES})
    if(_line MATCHES "^DEBUG=(.+)$")
        set(CTETRIS_DEBUG_FLAG "${CMAKE_MATCH_1}")
    endif()
endforeach()
string(TOLOWER "${CTETRIS_DEBUG_FLAG}" _DEBUG_LC)
if(_DEBUG_LC STREQUAL "true" OR _DEBUG_LC STREQUAL "1")
    message(STATUS "[cTetris] DEBUG=true → -DCTETRIS_DEBUG=1")
    target_compile_definitions(cTetris PRIVATE CTETRIS_DEBUG=1)
else()
    message(STATUS "[cTetris] DEBUG=${CTETRIS_DEBUG_FLAG} → release build")
endif()

# ── libcurl (native only; WASM uses emscripten_fetch) ────────────────────────
# Windows: build.ps1 → Initialize-Curl installs libcurl (Schannel backend)
# to libs/windows/downloads/curl-native/ and passes it via CMAKE_PREFIX_PATH.
# macOS/Linux: build.sh → ensure_curl uses brew/apt/dnf/pacman to install
# the system dev package; find_package picks it up from /usr/local or /opt.
if(NOT EMSCRIPTEN)
    find_package(CURL QUIET CONFIG)
    if(NOT CURL_FOUND)
        find_package(CURL QUIET)   # fall back to FindCURL.cmake module mode
    endif()
    if(CURL_FOUND)
        message(STATUS "[cTetris] libcurl ${CURL_VERSION_STRING} → HAVE_LIBCURL=1")
        target_link_libraries(cTetris PRIVATE CURL::libcurl)
        target_compile_definitions(cTetris PRIVATE HAVE_LIBCURL=1)

        # Windows: copy libcurl-x64.dll (or libcurl.dll) next to cTetris.exe
        # so the runtime loader finds it. Mirrors the SDL3.dll POST_BUILD copy.
        if(WIN32)
            get_target_property(_CURL_LOC CURL::libcurl IMPORTED_LOCATION_RELEASE)
            if(NOT _CURL_LOC)
                get_target_property(_CURL_LOC CURL::libcurl IMPORTED_LOCATION)
            endif()
            if(_CURL_LOC AND EXISTS "${_CURL_LOC}")
                add_custom_command(TARGET cTetris POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_CURL_LOC}"
                        "$<TARGET_FILE_DIR:cTetris>/"
                    COMMENT "[cTetris] Copying libcurl runtime DLL")
                message(STATUS "[cTetris] libcurl DLL POST_BUILD copy: ${_CURL_LOC}")
            endif()
        endif()
    else()
        message(WARNING
            "[cTetris] libcurl NOT found — HTTP sync disabled (offline fallback).\n"
            "  Windows: re-run app\\build.ps1 native  (auto-installs libcurl)\n"
            "  macOS  : brew install curl\n"
            "  Ubuntu : sudo apt-get install libcurl4-openssl-dev\n"
            "  Fedora : sudo dnf install libcurl-devel\n"
            "  Arch   : sudo pacman -S curl")
    endif()
endif()
# WASM: emscripten_fetch is always available — HAVE_LIBCURL not needed.
