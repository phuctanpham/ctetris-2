// v23



// =========================
// support cross platform
// =========================
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cstring>
#ifdef _WIN32
#  include <windows.h>
#endif
#ifdef __has_include
#  if __has_include(<gif_lib.h>)
#    include <gif_lib.h>
#  elif __has_include(<giflib/gif_lib.h>)
#    include <giflib/gif_lib.h>
#  else
#    error "gif_lib.h not found"
#  endif
#else
#  include <gif_lib.h>
#endif
#include "include/layout.h"

// =========================
// gameconsole-tao-giao-dien-169-00
// Tạo cửa sổ tỷ lệ 9:16, đảm bảo chạy đúng trên mọi nền tảng
// =========================

bool loadGifFrames(const std::string& path,
                   std::vector<sf::Image>& frames,
                   int& frameDuration)
{
    int err = 0;
    GifFileType* gif = DGifOpenFileName(path.c_str(), &err);
    if (!gif) { std::cerr << "giflib: cannot open " << path << "\n"; return false; }
    if (DGifSlurp(gif) != GIF_OK) {
        std::cerr << "giflib: DGifSlurp failed\n";
#if GIFLIB_MAJOR >= 5
        DGifCloseFile(gif, &err);
#else
        DGifCloseFile(gif);
#endif
        return false;
    }

    int w = gif->SWidth, h = gif->SHeight;
    frameDuration = 100;
    std::vector<std::uint8_t> canvas(w * h * 4, 0);

    for (int f = 0; f < gif->ImageCount; f++) {
        SavedImage& img = gif->SavedImages[f];
        ColorMapObject* cmap = img.ImageDesc.ColorMap ? img.ImageDesc.ColorMap : gif->SColorMap;

        for (int e = 0; e < img.ExtensionBlockCount; e++) {
            ExtensionBlock& eb = img.ExtensionBlocks[e];
            if (eb.Function == GRAPHICS_EXT_FUNC_CODE && eb.ByteCount >= 4) {
                int delay = (eb.Bytes[2] << 8 | eb.Bytes[1]) * 10;
                if (delay > 0) frameDuration = delay;
            }
        }

        int fx = img.ImageDesc.Left, fy = img.ImageDesc.Top;
        int fw = img.ImageDesc.Width, fh = img.ImageDesc.Height;
        int transIdx = -1;
        for (int e = 0; e < img.ExtensionBlockCount; e++) {
            ExtensionBlock& eb = img.ExtensionBlocks[e];
            if (eb.Function == GRAPHICS_EXT_FUNC_CODE && eb.ByteCount >= 4)
                if (eb.Bytes[0] & 0x01) transIdx = (unsigned char)eb.Bytes[3];
        }

        for (int y = 0; y < fh; y++) {
            for (int x = 0; x < fw; x++) {
                int ci = (unsigned char)img.RasterBits[y * fw + x];
                if (ci == transIdx) continue;
                int px = fx + x, py = fy + y;
                if (px >= w || py >= h) continue;
                GifColorType& c = cmap->Colors[ci];
                int idx = (py * w + px) * 4;
                canvas[idx+0] = c.Red;
                canvas[idx+1] = c.Green;
                canvas[idx+2] = c.Blue;
                canvas[idx+3] = 255;
            }
        }

        sf::Image sfImg(sf::Vector2u(w, h), canvas.data());
        frames.push_back(std::move(sfImg));
    }

#if GIFLIB_MAJOR >= 5
    DGifCloseFile(gif, &err);
#else
    DGifCloseFile(gif);
#endif
    std::cout << "Loaded " << frames.size() << " frames, " << frameDuration << "ms/frame\n";
    return !frames.empty();
}

int main() {
#ifdef _WIN32
    // Bật UTF-8 cho console trên Windows
    SetConsoleOutputCP(CP_UTF8);
#endif

    // =========================
    // gameconsole-tao-giao-dien-169-00
    // Tạo cửa sổ tỷ lệ 9:16, đảm bảo chạy đúng trên mọi nền tảng
    // =========================
    int windowHeight = 720;
    auto window = layout::create916Window(windowHeight);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    // =========================
    // gameconsole-logo-intro-01
    // Hiển thị logo UIT với hiệu ứng (có thể thêm âm thanh ở đây)
    // =========================


    std::vector<sf::Image> frames;
    int frameDurationMs = 100;
    if (!loadGifFrames("gameStory_intro.gif", frames, frameDurationMs)) {
        std::cerr << "Không load được GIF\n";
        return 1;
    }

    int totalFrames = (int)frames.size();

    // v8 cách tạo texture — đã hoạt động
    sf::Texture tex(frames[0].getSize());
    tex.update(frames[0]);
    sf::Sprite sprite(tex);

    auto winSize = window.getSize();
    float scaleX = winSize.x / (float)frames[0].getSize().x;
    float scaleY = winSize.y / (float)frames[0].getSize().y;
    float scale  = std::min(scaleX, scaleY);
    float offX   = (winSize.x - frames[0].getSize().x * scale) / 2.f;
    float offY   = (winSize.y - frames[0].getSize().y * scale) / 2.f;
    sprite.setScale(sf::Vector2f(scale, scale));
    sprite.setPosition(sf::Vector2f(offX, offY));

    // =========================
    // gamestory-loading-bar-02
    // Thanh loading bar chạy theo hiệu ứng logo
    // =========================


    // Thanh loading bar — full width, bo tròn, cách mép dưới 7pt
    float barH  = 16.f;
    float barW  = (float)winSize.x;
    float barX  = 0.f;
    float barY  = winSize.y - barH - 7.f;
    float radius = barH / 2.f;

    // Nền loading bar
    sf::RectangleShape barBgRect(sf::Vector2f(barW - barH, barH));
    barBgRect.setFillColor(sf::Color(60, 60, 60, 200));
    barBgRect.setPosition(sf::Vector2f(barX + radius, barY));

    sf::CircleShape barBgCapL(radius);
    barBgCapL.setFillColor(sf::Color(60, 60, 60, 200));
    barBgCapL.setPosition(sf::Vector2f(barX, barY));

    sf::CircleShape barBgCapR(radius);
    barBgCapR.setFillColor(sf::Color(60, 60, 60, 200));
    barBgCapR.setPosition(sf::Vector2f(barX + barW - barH, barY));

    // Foreground loading bar (tiến trình)
    sf::RectangleShape barFgRect(sf::Vector2f(0, barH));
    barFgRect.setFillColor(sf::Color(100, 220, 100));
    barFgRect.setPosition(sf::Vector2f(barX + radius, barY));

    sf::CircleShape barFgCapL(radius);
    barFgCapL.setFillColor(sf::Color(100, 220, 100));
    barFgCapL.setPosition(sf::Vector2f(barX, barY));

    sf::CircleShape barFgCapR(radius);
    barFgCapR.setFillColor(sf::Color(100, 220, 100));
    barFgCapR.setPosition(sf::Vector2f(barX, barY));


    // =========================
    // (Các phần hiệu ứng khác giữ nguyên)
    // =========================

    // Fade overlay
    sf::RectangleShape overlay(sf::Vector2f((float)winSize.x, (float)winSize.y));
    overlay.setPosition(sf::Vector2f(0, 0));
    const float fadeInDuration  = 0.4f;
    const float fadeOutDuration = 0.3f;
    sf::Clock fadeInClock;
    sf::Clock fadeOutClock;
    bool closing = false;

    sf::Clock clock;
    int curFrame = 0;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                if (!closing) { closing = true; fadeOutClock.restart(); }
            }
        }

        if (closing) {
            float t = fadeOutClock.getElapsedTime().asSeconds();
            if (t >= fadeOutDuration) { window.close(); break; }
        }

        // Chuyển frame
        int elapsed = (int)clock.getElapsedTime().asMilliseconds();
        curFrame = elapsed / frameDurationMs;
        bool finished = curFrame >= totalFrames;
        if (finished) curFrame = totalFrames - 1;
        float progress = finished ? 1.f : (float)curFrame / (float)(totalFrames - 1);

        // Cập nhật texture
        tex.update(frames[curFrame]);

        // Cập nhật thanh loading bar
        float fgW    = barW * progress;
        float innerW = std::max(0.f, fgW - barH);
        barFgRect.setSize(sf::Vector2f(innerW, barH));
        barFgCapR.setPosition(sf::Vector2f(barX + std::max(0.f, fgW - barH), barY));

        // Vẽ giao diện
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

        // Hiệu ứng fade in/out
        float alpha = 0.f;
        float fi = fadeInClock.getElapsedTime().asSeconds();
        if (fi < fadeInDuration) {
            alpha = (1.f - fi / fadeInDuration) * 255.f;
        } else if (closing) {
            float t = fadeOutClock.getElapsedTime().asSeconds();
            alpha = std::min(1.f, t / fadeOutDuration) * 255.f;
        }
        if (alpha > 0.f) {
            overlay.setFillColor(sf::Color(0, 0, 0, (std::uint8_t)alpha));
            window.draw(overlay);
        }

        window.display();
    }

    return 0;
}