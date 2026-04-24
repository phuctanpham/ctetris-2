// phucpt: app/src/gameStory/app.cpp

// =========================
// integration/v1
// Điểm tích hợp: hàm runGameStory() dùng khi tích hợp với main.cpp
// Khi build standalone (STANDALONE_GAMESTORY), hàm main() thay thế
// Ported sang VRSFML: thay SFML 3 thuần bằng vittorioromeo/VRSFML
// - sf::GraphicsContext::create() thay thế global OpenGL state
// - SFML_GAME_LOOP(window) thay thế while(window.isOpen()) — tương thích Emscripten
// - sf::base::Optional thay std::optional cho pollEvent
// - sf::Font::openFromFile(...).value() thay loadFromFile
// =========================

// =========================
// Hỗ trợ đa nền tảng (Environment)
// =========================
#include <SFML/Graphics.hpp>

#include <cstring>
#include <iostream>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#endif

// Tìm gif_lib.h theo thứ tự đường dẫn khác nhau tuỳ hệ thống
#ifdef __has_include
#  if __has_include(<gif_lib.h>)
#    include <gif_lib.h>
#  elif __has_include(<giflib/gif_lib.h>)
#    include <giflib/gif_lib.h>
#  else
#    error "gif_lib.h not found — cần cài giflib (apt: libgif-dev / brew: giflib)"
#  endif
#else
#  include <gif_lib.h>
#endif

#include "include/layout.h"

// =========================
// Cấu hình (Configuration)
// =========================
static constexpr int   STORY_WINDOW_HEIGHT  = 720;
static constexpr float FADE_IN_DURATION     = 0.4f;
static constexpr float FADE_OUT_DURATION    = 0.3f;

// =========================
// Kết quả trả về (Model)
// =========================
enum class StoryResult { Finished, Quit };

// =========================
// Khối tiện ích GIF (Environment / Model)
// Đọc tất cả frame của file GIF vào vector sf::Image
// =========================
static bool loadGifFrames(const std::string&      path,
                           std::vector<sf::Image>& frames,
                           int&                    frameDurationMs) {
    int          err = 0;
    GifFileType* gif = DGifOpenFileName(path.c_str(), &err);
    if (!gif) {
        std::cerr << "[gameStory] giflib: không mở được " << path << "\n";
        return false;
    }
    if (DGifSlurp(gif) != GIF_OK) {
        std::cerr << "[gameStory] giflib: DGifSlurp thất bại\n";
#if GIFLIB_MAJOR >= 5
        DGifCloseFile(gif, &err);
#else
        DGifCloseFile(gif);
#endif
        return false;
    }

    const int w = gif->SWidth;
    const int h = gif->SHeight;
    frameDurationMs = 100;

    std::vector<std::uint8_t> canvas(static_cast<std::size_t>(w * h * 4), 0);

    for (int f = 0; f < gif->ImageCount; ++f) {
        SavedImage&     img  = gif->SavedImages[f];
        ColorMapObject* cmap = img.ImageDesc.ColorMap
                                   ? img.ImageDesc.ColorMap
                                   : gif->SColorMap;

        // Đọc delay từ Graphic Control Extension
        for (int e = 0; e < img.ExtensionBlockCount; ++e) {
            ExtensionBlock& eb = img.ExtensionBlocks[e];
            if (eb.Function == GRAPHICS_EXT_FUNC_CODE && eb.ByteCount >= 4) {
                int d = (eb.Bytes[2] << 8 | eb.Bytes[1]) * 10;
                if (d > 0) frameDurationMs = d;
            }
        }

        const int fx = img.ImageDesc.Left, fy = img.ImageDesc.Top;
        const int fw = img.ImageDesc.Width, fh = img.ImageDesc.Height;
        int       transIdx = -1;
        for (int e = 0; e < img.ExtensionBlockCount; ++e) {
            ExtensionBlock& eb = img.ExtensionBlocks[e];
            if (eb.Function == GRAPHICS_EXT_FUNC_CODE && eb.ByteCount >= 4)
                if (eb.Bytes[0] & 0x01)
                    transIdx = static_cast<unsigned char>(eb.Bytes[3]);
        }

        for (int y = 0; y < fh; ++y)
            for (int x = 0; x < fw; ++x) {
                int ci = static_cast<unsigned char>(img.RasterBits[y * fw + x]);
                if (ci == transIdx) continue;
                int px2 = fx + x, py2 = fy + y;
                if (px2 >= w || py2 >= h) continue;
                GifColorType&  c   = cmap->Colors[ci];
                std::size_t    idx = static_cast<std::size_t>((py2 * w + px2) * 4);
                canvas[idx + 0] = c.Red;
                canvas[idx + 1] = c.Green;
                canvas[idx + 2] = c.Blue;
                canvas[idx + 3] = 255;
            }

        frames.emplace_back(sf::Vector2u(static_cast<unsigned>(w),
                                          static_cast<unsigned>(h)),
                             canvas.data());
    }

#if GIFLIB_MAJOR >= 5
    DGifCloseFile(gif, &err);
#else
    DGifCloseFile(gif);
#endif
    std::cout << "[gameStory] Đã tải " << frames.size()
              << " frame, " << frameDurationMs << "ms/frame\n";
    return !frames.empty();
}

// =========================
// Hàm chạy gameStory (Logic chính / Integration Entry Point)
// VRSFML yêu cầu sf::GraphicsContext được tạo trước bất kỳ thao tác đồ hoạ nào.
// Ở đây context được nhận từ main() để tránh tạo lại nhiều lần.
// =========================
StoryResult runGameStory(sf::GraphicsContext& gCtx) {
    (void)gCtx; // Context được giữ sống từ main(); không cần dùng trực tiếp ở đây

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // =========================
    // gameconsole-tao-giao-dien-169-00
    // Tạo cửa sổ tỷ lệ 9:16 — VRSFML dùng designated initializer
    // =========================
    auto window = layout::create916Window(STORY_WINDOW_HEIGHT, "ctetris — Story");
    window.setFramerateLimit(60);

    const sf::Vector2u wSize = window.getSize();

    // =========================
    // gameconsole-logo-intro-01
    // Hiển thị logo giới thiệu dạng GIF với hiệu ứng fade
    // =========================
    std::vector<sf::Image> frames;
    int frameDurationMs = 100;
    const bool hasGif = loadGifFrames("gameStory_intro.gif", frames, frameDurationMs);
    if (!hasGif) {
        std::cerr << "[gameStory] Không tải được GIF — bỏ qua intro.\n";
        return StoryResult::Finished;
    }

    const int totalFrames = static_cast<int>(frames.size());

    // VRSFML: sf::Texture tạo qua constructor có sẵn, update() vẫn tương tự
    sf::Texture tex(frames[0].getSize());
    tex.update(frames[0]);
    sf::Sprite  sprite(tex);

    const float scaleX = static_cast<float>(wSize.x) / static_cast<float>(frames[0].getSize().x);
    const float scaleY = static_cast<float>(wSize.y) / static_cast<float>(frames[0].getSize().y);
    const float scale  = std::min(scaleX, scaleY);
    const float offX   = (static_cast<float>(wSize.x) - frames[0].getSize().x * scale) / 2.f;
    const float offY   = (static_cast<float>(wSize.y) - frames[0].getSize().y * scale) / 2.f;
    sprite.setScale(sf::Vector2f(scale, scale));
    sprite.setPosition(sf::Vector2f(offX, offY));

    // =========================
    // gamestory-loading-bar-02
    // Thanh loading bar chạy theo tiến trình frame GIF
    // =========================
    const float barH   = 16.f, barW  = static_cast<float>(wSize.x);
    const float barX   = 0.f,  barY  = static_cast<float>(wSize.y) - barH - 7.f;
    const float radius = barH / 2.f;

    sf::RectangleShape barBgRect(sf::Vector2f(barW - barH, barH));
    barBgRect.setFillColor(sf::Color(60, 60, 60, 200));
    barBgRect.setPosition(sf::Vector2f(barX + radius, barY));

    sf::CircleShape barBgCapL(radius), barBgCapR(radius);
    barBgCapL.setFillColor(sf::Color(60, 60, 60, 200));
    barBgCapL.setPosition(sf::Vector2f(barX, barY));
    barBgCapR.setFillColor(sf::Color(60, 60, 60, 200));
    barBgCapR.setPosition(sf::Vector2f(barX + barW - barH, barY));

    sf::RectangleShape barFgRect(sf::Vector2f(0.f, barH));
    barFgRect.setFillColor(sf::Color(100, 220, 100));
    barFgRect.setPosition(sf::Vector2f(barX + radius, barY));

    sf::CircleShape barFgCapL(radius), barFgCapR(radius);
    barFgCapL.setFillColor(sf::Color(100, 220, 100));
    barFgCapL.setPosition(sf::Vector2f(barX, barY));
    barFgCapR.setFillColor(sf::Color(100, 220, 100));
    barFgCapR.setPosition(sf::Vector2f(barX, barY));

    sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(wSize.x),
                                             static_cast<float>(wSize.y)));
    overlay.setPosition(sf::Vector2f(0.f, 0.f));

    sf::Clock fadeInClock, fadeOutClock, animClock;
    bool      closing  = false;
    StoryResult result = StoryResult::Finished;

    // =========================
    // Vòng lặp chính (Logic + View)
    // VRSFML: SFML_GAME_LOOP(window) thay thế while(window.isOpen())
    // Trên Emscripten macro này gọi emscripten_set_main_loop nội bộ
    // Trên Desktop macro này là while(window.isOpen()) thông thường
    // =========================
    SFML_GAME_LOOP(window) {
        // Xử lý sự kiện — VRSFML: pollEvent trả sf::base::Optional
        while (const sf::base::Optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                if (!closing) { closing = true; fadeOutClock.restart(); }
            }
            if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Space ||
                    kp->code == sf::Keyboard::Key::Enter) {
                    result = StoryResult::Finished;
                    window.close();
                    return result; // Skip intro
                }
            }
        }

        if (closing) {
            if (fadeOutClock.getElapsedTime().asSeconds() >= FADE_OUT_DURATION) {
                result = StoryResult::Quit;
                window.close();
                return result;
            }
        }

        const int elapsed  = static_cast<int>(animClock.getElapsedTime().asMilliseconds());
        int curFrame       = elapsed / frameDurationMs;
        const bool done    = curFrame >= totalFrames;
        if (done) {
            result = StoryResult::Finished;
            window.close();
            return result; // GIF xong → tự chuyển tiếp
        }

        const float progress = static_cast<float>(curFrame) /
                               static_cast<float>(totalFrames - 1);
        tex.update(frames[curFrame]);

        const float fgW    = barW * progress;
        const float innerW = std::max(0.f, fgW - barH);
        barFgRect.setSize(sf::Vector2f(innerW, barH));
        barFgCapR.setPosition(sf::Vector2f(barX + std::max(0.f, fgW - barH), barY));

        // Vẽ (View)
        window.clear(sf::Color::Black);
        window.draw(sprite);
        window.draw(barBgRect);
        window.draw(barBgCapL);
        window.draw(barBgCapR);
        if (progress > 0.f) {
            if (innerW > 0.f) window.draw(barFgRect);
            window.draw(barFgCapL);
            if (fgW >= barH) window.draw(barFgCapR);
        }

        float alpha = 0.f;
        const float fi = fadeInClock.getElapsedTime().asSeconds();
        if (fi < FADE_IN_DURATION) {
            alpha = (1.f - fi / FADE_IN_DURATION) * 255.f;
        } else if (closing) {
            const float t = fadeOutClock.getElapsedTime().asSeconds();
            alpha = std::min(1.f, t / FADE_OUT_DURATION) * 255.f;
        }
        if (alpha > 0.f) {
            overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha)));
            window.draw(overlay);
        }
        window.display();
    }

    return result;
}

// =========================
// Điểm vào standalone — chỉ compile khi build riêng lẻ
// =========================
#ifdef STANDALONE_GAMESTORY
int main() {
    // VRSFML: bắt buộc tạo GraphicsContext trước bất kỳ thao tác đồ hoạ nào
    auto gCtx = sf::GraphicsContext::create().value();
    runGameStory(gCtx);
    return 0;
}
#endif