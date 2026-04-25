cmake_minimum_required(VERSION 3.16)
project(ctetris VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Cau hinh SDL3: dung port -sUSE_SDL=3 cho Emscripten, find_package cho native ---
if(NOT EMSCRIPTEN)
    find_package(SDL3 REQUIRED CONFIG)
endif()

# Ham tien ich gan SDL3 vao tung target (native vs WASM)
function(link_sdl3 tgt)
    if(EMSCRIPTEN)
        # Emscripten 3.1.55+ ho tro SDL3 qua port
        target_compile_options(${tgt} PRIVATE "-sUSE_SDL=3")
        target_link_options(${tgt} PRIVATE
            "-sUSE_SDL=3"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sASYNCIFY")
        # Xuat .html de chay tren browser
        set_target_properties(${tgt} PROPERTIES SUFFIX ".html")
    else()
        target_link_libraries(${tgt} PRIVATE SDL3::SDL3)
    endif()
endfunction()

# --- Targets doc lap (dieu kien necessary) ---
add_executable(gameStory MACOSX_BUNDLE src/gameStory/app.cpp)
target_compile_definitions(gameStory PRIVATE BUILD_STANDALONE)
target_include_directories(gameStory PRIVATE src/gameStory/include)
link_sdl3(gameStory)

add_executable(gameConsole MACOSX_BUNDLE src/gameConsole/app.cpp)
target_compile_definitions(gameConsole PRIVATE BUILD_STANDALONE)
target_include_directories(gameConsole PRIVATE src/gameConsole/include)
link_sdl3(gameConsole)

add_executable(gameCore MACOSX_BUNDLE src/gameCore/app.cpp)
target_compile_definitions(gameCore PRIVATE BUILD_STANDALONE)
target_include_directories(gameCore PRIVATE src/gameCore/include)
link_sdl3(gameCore)

# --- Target tich hop toan bo (dieu kien sufficient) ---
add_executable(ctetris MACOSX_BUNDLE
    main.cpp
    src/gameStory/app.cpp
    src/gameConsole/app.cpp
    src/gameCore/app.cpp
)
target_include_directories(ctetris PRIVATE
    src/gameStory/include
    src/gameConsole/include
    src/gameCore/include
)
link_sdl3(ctetris)