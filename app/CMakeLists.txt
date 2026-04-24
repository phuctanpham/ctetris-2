# phucpt: app/CMakeLists.txt
# CMakeLists hợp nhất — dùng VRSFML (vittorioromeo/VRSFML) qua FetchContent
# Hỗ trợ: macOS · Linux · Windows · WebAssembly/WASM (Emscripten)
#
# Cách dùng:
#   Build toàn bộ (mặc định):
#     cmake -S . -B build -DBUILD_MODULE=ALL
#   Build riêng lẻ:
#     cmake -S . -B build -DBUILD_MODULE=gameStory
#     cmake -S . -B build -DBUILD_MODULE=gameConsole
#     cmake -S . -B build -DBUILD_MODULE=gameCore
#   Build WASM (cần emsdk đã kích hoạt):
#     emcmake cmake -S . -B build/wasm -DBUILD_MODULE=ALL

cmake_minimum_required(VERSION 3.16)
project(ctetris VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Tham số build
# =========================
set(BUILD_MODULE "ALL" CACHE STRING
    "Module to build: ALL | gameStory | gameConsole | gameCore")
set_property(CACHE BUILD_MODULE PROPERTY STRINGS
    ALL gameStory gameConsole gameCore)

message(STATUS "BUILD_MODULE = ${BUILD_MODULE}")

# =========================
# Vô hiệu hoá clang-scan-deps dependency scanner
# macOS 11 / Xcode CLT cũ không có clang-scan-deps trong PATH
# Emscripten toolchain cũng không cần C++20 module scanning
# =========================
set(CMAKE_CXX_SCAN_FOR_MODULES OFF CACHE BOOL "" FORCE)
# Đặt thành giá trị trống để tránh lỗi NOTFOUND khi CMake tìm không thấy
set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "" CACHE FILEPATH "" FORCE)

# =========================
# Phát hiện Emscripten (WASM)
# =========================
if(EMSCRIPTEN)
    message(STATUS "Target: WebAssembly / Emscripten")
    set(IS_WASM TRUE)
else()
    set(IS_WASM FALSE)
    if(APPLE)
        message(STATUS "Target: macOS")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")
    elseif(UNIX)
        message(STATUS "Target: Linux")
    elseif(WIN32)
        message(STATUS "Target: Windows")
    endif()
endif()

# =========================
# VRSFML qua FetchContent
# Khoá tại commit ổn định trên nhánh master (Emscripten 3.1.x-compatible)
# =========================
include(FetchContent)

# =========================
# SDL — libs locally at the exact SHA VRSFML/CPM requires
# (7dc1c88...) instead of fetching from git at build time
# CPM_sdl_SOURCE must be set BEFORE FetchContent_MakeAvailable(VRSFML)
# so that VRSFML's internal CPM finds SDL already populated here.
# =========================
set(CPM_sdl_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/libs/sdl" CACHE STRING "" FORCE)

FetchContent_Declare(
    sdl
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/sdl"
)
FetchContent_MakeAvailable(sdl)

# =========================
# VRSFML — libs locally, no git needed at build time
# =========================
FetchContent_Declare(
    VRSFML
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/VRSFML"
)

set(SFML_BUILD_AUDIO      ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK    OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_IMGUI      OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(VRSFML)

# =========================
# giflib — chỉ cần cho gameStory (decode GIF frames)
# Không cần trên WASM nếu GIF được pre-load qua JS
# =========================
if(NOT IS_WASM AND (BUILD_MODULE STREQUAL "ALL" OR BUILD_MODULE STREQUAL "gameStory"))
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GIF QUIET giflib)
    endif()
    if(NOT GIF_FOUND)
        find_path(GIF_INCLUDE_DIR gif_lib.h
            PATHS /usr/include /usr/local/include
                  /opt/homebrew/include /opt/homebrew/include/giflib)
        find_library(GIF_LIBRARY NAMES gif
            PATHS /usr/lib /usr/local/lib /opt/homebrew/lib)
        if(GIF_INCLUDE_DIR AND GIF_LIBRARY)
            set(GIF_FOUND TRUE)
            set(GIF_INCLUDE_DIRS ${GIF_INCLUDE_DIR})
            set(GIF_LIBRARIES    ${GIF_LIBRARY})
        else()
            message(WARNING
                "giflib không tìm thấy — gameStory intro GIF sẽ bị bỏ qua")
        endif()
    endif()
endif()

# =========================
# Helper: include paths chung cho tất cả các module
# =========================
set(SHARED_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shared/include)

# =========================
# Helper: định nghĩa target standalone
# =========================
function(add_standalone_module TARGET SRC INC DEFINE)
    add_executable(${TARGET} ${SRC})
    target_include_directories(${TARGET} PRIVATE ${INC} ${SHARED_INCLUDE_DIR})
    target_compile_definitions(${TARGET} PRIVATE ${DEFINE})
    target_link_libraries(${TARGET} PRIVATE
        SFML::Graphics SFML::Window SFML::System SFML::Audio)
    # Link giflib nếu có và đây là gameStory
    if(GIF_FOUND AND "${DEFINE}" STREQUAL "STANDALONE_GAMESTORY")
        target_include_directories(${TARGET} PRIVATE ${GIF_INCLUDE_DIRS})
        target_link_libraries(${TARGET} PRIVATE ${GIF_LIBRARIES})
    endif()
    # Tuỳ chỉnh WASM output
    if(IS_WASM)
        set_target_properties(${TARGET} PROPERTIES SUFFIX ".html")
        target_link_options(${TARGET} PRIVATE
            -sUSE_WEBGL2=1
            -sALLOW_MEMORY_GROWTH=1
            -sASYNCIFY=1
            -sEXPORTED_RUNTIME_METHODS=['ccall']
            --preload-file ${CMAKE_SOURCE_DIR}/src/gameConsole/gameConsole_board.json@/gameConsole_board.json
        )
    endif()
    message(STATUS "Target standalone: ${TARGET}")
endfunction()

# =========================
# Build riêng lẻ từng module
# =========================
if(BUILD_MODULE STREQUAL "gameStory")
    add_standalone_module(
        gameStory
        src/gameStory/app.cpp
        src/gameStory/include
        STANDALONE_GAMESTORY)
    return()
endif()

if(BUILD_MODULE STREQUAL "gameConsole")
    add_standalone_module(
        gameConsole
        src/gameConsole/app.cpp
        src/gameConsole/include
        STANDALONE_GAMECONSOLE)
    return()
endif()

if(BUILD_MODULE STREQUAL "gameCore")
    add_standalone_module(
        gameCore
        src/gameCore/app.cpp
        src/gameCore/include
        STANDALONE_GAMECORE)
    return()
endif()

# =========================
# Build tích hợp toàn bộ (ALL)
# =========================
if(BUILD_MODULE STREQUAL "ALL")
    set(INTEGRATED_SOURCES
        main.cpp
        src/gameStory/app.cpp
        src/gameConsole/app.cpp
        src/gameCore/app.cpp)

    add_executable(ctetris ${INTEGRATED_SOURCES})

    target_include_directories(ctetris PRIVATE
        ${SHARED_INCLUDE_DIR}
        src/gameStory/include
        src/gameConsole/include
        src/gameCore/include)

    target_link_libraries(ctetris PRIVATE
        SFML::Graphics SFML::Window SFML::System SFML::Audio)

    if(GIF_FOUND)
        target_include_directories(ctetris PRIVATE ${GIF_INCLUDE_DIRS})
        target_link_libraries(ctetris PRIVATE ${GIF_LIBRARIES})
    endif()

    # --- macOS .app bundle ---
    if(APPLE)
        set_target_properties(ctetris PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "ctetris"
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    endif()

    # --- Windows: ẩn console đen ---
    if(WIN32)
        set_target_properties(ctetris PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    # --- WASM: cấu hình Emscripten link options ---
    if(IS_WASM)
        set_target_properties(ctetris PROPERTIES SUFFIX ".html")
        target_link_options(ctetris PRIVATE
            -sUSE_WEBGL2=1
            -sALLOW_MEMORY_GROWTH=1
            -sASYNCIFY=1                         # Cho phép blocking IO giả lập
            -sEXPORTED_RUNTIME_METHODS=['ccall']
            -sINITIAL_MEMORY=67108864            # 64 MB ban đầu
            # Đóng gói assets vào WASM bundle
            --preload-file ${CMAKE_SOURCE_DIR}/src/gameConsole/gameConsole_board.json@/gameConsole_board.json
            --preload-file ${CMAKE_SOURCE_DIR}/src/gameStory/gameStory_intro.gif@/gameStory_intro.gif
        )
    endif()

    # --- Copy assets vào build directory ---
    foreach(asset
        src/gameConsole/gameConsole_board.json
        src/gameStory/gameStory_intro.gif)
        if(EXISTS "${CMAKE_SOURCE_DIR}/${asset}")
            get_filename_component(asset_name ${asset} NAME)
            configure_file(${asset} ${CMAKE_BINARY_DIR}/${asset_name} COPYONLY)
        endif()
    endforeach()

    message(STATUS "Target tích hợp: ctetris")
    return()
endif()

message(FATAL_ERROR
    "BUILD_MODULE không hợp lệ: '${BUILD_MODULE}'. "
    "Dùng: ALL | gameStory | gameConsole | gameCore")