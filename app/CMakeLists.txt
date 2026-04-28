cmake_minimum_required(VERSION 3.16)
project(ctetris VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path constants
set(WEB_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/web")
set(SOURCE_SVG "${CMAKE_CURRENT_SOURCE_DIR}/src/gameStory/gameStory_logo.svg")

# ICON_DIR truyen tu build script (per-platform path).
# Fallback: dat trong build dir hien tai neu khong duoc cung cap.
if(NOT DEFINED ICON_DIR)
    set(ICON_DIR "${CMAKE_CURRENT_BINARY_DIR}/icons")
endif()
# Convert sang absolute path de moi configure_file/file() reference deu dung
get_filename_component(ICON_DIR "${ICON_DIR}" ABSOLUTE
                       BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "ICON_DIR (absolute) = ${ICON_DIR}")

# --- SDL3 setup ---
# Native: tim qua find_package; WASM: FetchContent build tu source vi
# co -sUSE_SDL=3 cua emsdk 3.1.72 khong bom header path on dinh.
if(EMSCRIPTEN OR DEFINED ENV{FORCE_FETCH_SDL})
    include(FetchContent)
    set(SDL_SHARED          OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC          ON  CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY    OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS           OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL         OFF CACHE BOOL "" FORCE)
    set(SDL_DISABLE_INSTALL ON  CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.2.18
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(SDL3)
else()
    find_package(SDL3 REQUIRED CONFIG)
endif()

function(link_sdl3 tgt)
    target_link_libraries(${tgt} PRIVATE SDL3::SDL3)
    if(EMSCRIPTEN)
        target_link_options(${tgt} PRIVATE
            "-sALLOW_MEMORY_GROWTH=1"
            "-sASYNCIFY")
        set_target_properties(${tgt} PROPERTIES SUFFIX ".html")
    endif()
endfunction()

# Apply custom HTML shell (clean white bg, mobile-first, PWA-ready)
function(use_clean_shell tgt)
    if(NOT EMSCRIPTEN)
        return()
    endif()
    target_link_options(${tgt} PRIVATE "--shell-file=${WEB_DIR}/shell.html")
    set_property(TARGET ${tgt} APPEND PROPERTY LINK_DEPENDS "${WEB_DIR}/shell.html")
endfunction()

# Copy PWA assets (manifest, sw, icons) sang build dir cung HTML
function(setup_pwa tgt)
    if(NOT EMSCRIPTEN)
        return()
    endif()
    foreach(asset manifest.webmanifest sw.js)
        if(EXISTS "${WEB_DIR}/${asset}")
            configure_file("${WEB_DIR}/${asset}"
                           "${CMAKE_CURRENT_BINARY_DIR}/${asset}" COPYONLY)
        endif()
    endforeach()
    foreach(icon icon-192.png icon-512.png icon-maskable-192.png
                 icon-maskable-512.png apple-touch-icon.png favicon.svg)
        if(EXISTS "${ICON_DIR}/${icon}")
            configure_file("${ICON_DIR}/${icon}"
                           "${CMAKE_CURRENT_BINARY_DIR}/${icon}" COPYONLY)
        endif()
    endforeach()
endfunction()

# --- Standalone targets (necessary condition) ---
# Moi command CMake phai o 1 dong rieng; ';' KHONG phai delimiter.
add_executable(gameStory MACOSX_BUNDLE src/gameStory/app.cpp)
target_compile_definitions(gameStory PRIVATE BUILD_STANDALONE)
target_include_directories(gameStory PRIVATE src/gameStory/include)
link_sdl3(gameStory)
use_clean_shell(gameStory)

add_executable(gameConsole MACOSX_BUNDLE src/gameConsole/app.cpp)
target_compile_definitions(gameConsole PRIVATE BUILD_STANDALONE)
target_include_directories(gameConsole PRIVATE src/gameConsole/include)
link_sdl3(gameConsole)
use_clean_shell(gameConsole)

add_executable(gameCore MACOSX_BUNDLE src/gameCore/app.cpp)
target_compile_definitions(gameCore PRIVATE BUILD_STANDALONE)
target_include_directories(gameCore PRIVATE src/gameCore/include)
link_sdl3(gameCore)
use_clean_shell(gameCore)

# --- Integrated target (sufficient condition) ---
add_executable(ctetris MACOSX_BUNDLE
    main.cpp
    src/gameStory/app.cpp
    src/gameConsole/app.cpp
    src/gameCore/app.cpp)
target_include_directories(ctetris PRIVATE
    src/gameStory/include
    src/gameConsole/include
    src/gameCore/include)
link_sdl3(ctetris)
use_clean_shell(ctetris)
setup_pwa(ctetris)

# --- Native app icons embed ---
if(APPLE AND NOT EMSCRIPTEN)
    if(EXISTS "${ICON_DIR}/icon.icns")
        set_target_properties(ctetris PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "icon.icns"
            MACOSX_BUNDLE_BUNDLE_NAME "cTetris"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.uit.ctetris"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
        target_sources(ctetris PRIVATE "${ICON_DIR}/icon.icns")
        set_source_files_properties("${ICON_DIR}/icon.icns"
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    endif()
elseif(WIN32)
    if(EXISTS "${ICON_DIR}/icon.ico")
        # Sinh .rc inline tro toi icon.ico trong ICON_DIR (forward slashes
        # cho duong dan Windows resource file tranh escape backslash)
        set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
        file(TO_CMAKE_PATH "${ICON_DIR}/icon.ico" ICON_PATH_FORWARD)
        file(WRITE "${RC_FILE}" "1 ICON \"${ICON_PATH_FORWARD}\"\n")
        target_sources(ctetris PRIVATE "${RC_FILE}")
    endif()
elseif(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    if(EXISTS "${ICON_DIR}/icon-512.png")
        configure_file("${ICON_DIR}/icon-512.png"
                       "${CMAKE_CURRENT_BINARY_DIR}/ctetris.png" COPYONLY)
    endif()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ctetris.desktop"
"[Desktop Entry]
Type=Application
Name=cTetris
Comment=Classic Tetris game built with SDL3
Exec=ctetris
Icon=ctetris
Categories=Game;
Terminal=false
")
endif()