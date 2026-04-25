cmake_minimum_required(VERSION 3.16)
project(ctetris VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tìm thư viện SDL3
find_package(SDL3 REQUIRED)

# --- Các targets độc lập (Đảm bảo điều kiện necessary) ---
add_executable(gameStory MACOSX_BUNDLE src/gameStory/app.cpp)
target_compile_definitions(gameStory PRIVATE BUILD_STANDALONE)
target_link_libraries(gameStory PRIVATE SDL3::SDL3)

add_executable(gameConsole MACOSX_BUNDLE src/gameConsole/app.cpp)
target_compile_definitions(gameConsole PRIVATE BUILD_STANDALONE)
target_link_libraries(gameConsole PRIVATE SDL3::SDL3)

add_executable(gameCore MACOSX_BUNDLE src/gameCore/app.cpp)
target_compile_definitions(gameCore PRIVATE BUILD_STANDALONE)
target_link_libraries(gameCore PRIVATE SDL3::SDL3)

# --- Target tích hợp toàn bộ (Đảm bảo điều kiện sufficient) ---
add_executable(ctetris MACOSX_BUNDLE
    main.cpp
    src/gameStory/app.cpp
    src/gameConsole/app.cpp
    src/gameCore/app.cpp
)
target_link_libraries(ctetris PRIVATE SDL3::SDL3)