// phucpt: app/src/shared/include/layout.h
// File layout chung — VRSFML (fork của SFML 3 hỗ trợ Emscripten/WASM)
// Tất cả module (gameStory, gameConsole, gameCore) dùng file này
#pragma once

#include <SFML/Graphics.hpp>

namespace layout {

    // =========================
    // Cấu hình (Configuration)
    // Tạo cửa sổ tỷ lệ 9:16 — VRSFML dùng designated initializer thay vì
    // sf::VideoMode + sf::Style riêng lẻ như SFML 3 thuần
    // =========================
    inline sf::RenderWindow create916Window(int height, const char* title) {
        const unsigned w = static_cast<unsigned>(height * 9 / 16);
        const unsigned h = static_cast<unsigned>(height);

        // VRSFML: sf::RenderWindow nhận struct WindowSettings với designated initializers
        return sf::RenderWindow({.size{w, h}, .title = title, .vsync = true});
    }

} // namespace layout