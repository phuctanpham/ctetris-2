#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "include/layout.h"

namespace fs = std::filesystem;

struct BoardEntry {
    std::string user;
    int score = 0;
    std::string time;
};

struct Button {
    sf::FloatRect bounds;
    std::string label;
};

enum class PopupMode {
    None,
    Guide,
    Board,
};

bool loadFirstAvailableFont(sf::Font& font, fs::path& loadedPath) {
    const std::array<fs::path, 6> candidates = {
        fs::path{"/System/Library/Fonts/Supplemental/Arial Unicode.ttf"},
        fs::path{"/System/Library/Fonts/Supplemental/Arial.ttf"},
        fs::path{"/System/Library/Fonts/Supplemental/Verdana.ttf"},
        fs::path{"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"},
        fs::path{"/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"},
        fs::path{"/usr/share/fonts/truetype/freefont/FreeSans.ttf"},
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate) && font.openFromFile(candidate)) {
            loadedPath = candidate;
            return true;
        }
    }
    return false;
}

sf::Texture buildBackgroundTexture() {
    constexpr unsigned int width = 540;
    constexpr unsigned int height = 960;
    std::vector<std::uint8_t> pixels(width * height * 4, 255);

    for (unsigned int y = 0; y < height; ++y) {
        float fy = static_cast<float>(y) / static_cast<float>(height - 1);
        for (unsigned int x = 0; x < width; ++x) {
            float fx = static_cast<float>(x) / static_cast<float>(width - 1);
            std::size_t index = (static_cast<std::size_t>(y) * width + x) * 4;

            float wave = 0.5f + 0.5f * std::sin(fx * 9.0f + fy * 5.0f);
            float glow = 0.5f + 0.5f * std::cos((fx - 0.3f) * 8.0f + (fy - 0.45f) * 6.0f);

            std::uint8_t red = static_cast<std::uint8_t>(18 + 20 * fy + 65 * wave + 25 * glow);
            std::uint8_t green = static_cast<std::uint8_t>(30 + 28 * fy + 45 * wave + 18 * glow);
            std::uint8_t blue = static_cast<std::uint8_t>(55 + 42 * fy + 95 * wave + 75 * glow);

            if (((x * 17 + y * 13) % 211) == 0 || ((x * 11 + y * 19) % 317) == 0) {
                red = 250;
                green = 250;
                blue = 255;
            }

            pixels[index + 0] = red;
            pixels[index + 1] = green;
            pixels[index + 2] = blue;
            pixels[index + 3] = 255;
        }
    }

    sf::Image image(sf::Vector2u(width, height), pixels.data());
    sf::Texture texture(sf::Vector2u(width, height));
    texture.update(image);
    return texture;
}

std::vector<BoardEntry> loadBoardEntries(const fs::path& path) {
    std::vector<BoardEntry> entries;
    std::ifstream input(path);
    if (!input) {
        std::cerr << "Cannot open board data: " << path << '\n';
        return entries;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string content = buffer.str();

    const std::regex entryPattern(
        "\\{\\s*\"user\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"score\"\\s*:\\s*(\\d+)\\s*,\\s*\"time\"\\s*:\\s*\"([^\"]+)\"\\s*\\}");

    for (std::sregex_iterator it(content.begin(), content.end(), entryPattern), end; it != end; ++it) {
        BoardEntry entry;
        entry.user = (*it)[1].str();
        entry.score = std::stoi((*it)[2].str());
        entry.time = (*it)[3].str();
        entries.push_back(std::move(entry));
    }

    return entries;
}

bool buildAmbientBuffer(sf::SoundBuffer& buffer) {
    constexpr unsigned sampleRate = 44100;
    constexpr float durationSeconds = 4.0f;
    constexpr std::size_t channelCount = 2;
    constexpr std::size_t sampleCount = static_cast<std::size_t>(sampleRate * durationSeconds * channelCount);

    std::vector<std::int16_t> samples(sampleCount, 0);
    const float pi = 3.14159265358979323846f;

    for (std::size_t frame = 0; frame < sampleCount / channelCount; ++frame) {
        float t = static_cast<float>(frame) / static_cast<float>(sampleRate);
        float pulse = 0.42f + 0.18f * std::sin(t * 0.85f * 2.0f * pi);
        float tone =
            0.55f * std::sin(t * 110.0f * 2.0f * pi) +
            0.28f * std::sin(t * 146.83f * 2.0f * pi) +
            0.18f * std::sin(t * 220.0f * 2.0f * pi);
        float value = std::clamp(tone * pulse, -1.0f, 1.0f);
        std::int16_t sample = static_cast<std::int16_t>(value * 2400.0f);
        samples[frame * 2 + 0] = sample;
        samples[frame * 2 + 1] = sample;
    }

    const std::vector<sf::SoundChannel> channelMap = {sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight};
    return buffer.loadFromSamples(samples.data(), static_cast<std::uint64_t>(samples.size()), 2, sampleRate, channelMap);
}

void drawTextCentered(sf::RenderTarget& target, sf::Text text, const sf::Vector2f& position) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    text.setPosition(position);
    target.draw(text);
}

void drawButton(sf::RenderTarget& target, const sf::Font& font, const Button& button, bool hovered, float scale) {
    sf::RectangleShape shape(sf::Vector2f(button.bounds.size.x, button.bounds.size.y));
    shape.setPosition(button.bounds.position);
    shape.setFillColor(hovered ? sf::Color(50, 104, 180, 235) : sf::Color(26, 35, 58, 228));
    shape.setOutlineColor(hovered ? sf::Color(255, 255, 255, 220) : sf::Color(190, 214, 255, 145));
    shape.setOutlineThickness(2.f * scale);
    target.draw(shape);

    sf::Text label(font);
    label.setString(button.label);
    label.setCharacterSize(static_cast<unsigned int>(28.f * scale));
    label.setFillColor(sf::Color::White);
    drawTextCentered(target,
                     label,
                     sf::Vector2f(button.bounds.position.x + button.bounds.size.x / 2.f,
                                  button.bounds.position.y + button.bounds.size.y / 2.f - 2.f * scale));
}

void drawOverlayPanel(sf::RenderTarget& target, const sf::Vector2u& windowSize, float alpha) {
    sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha)));
    target.draw(overlay);
}

int clampBoardScroll(int desiredScroll, int itemCount, int visibleRows) {
    const int maxScroll = std::max(0, itemCount - visibleRows);
    return std::clamp(desiredScroll, 0, maxScroll);
}

void drawGuidePopup(sf::RenderTarget& target,
                    const sf::Font& font,
                    const sf::Vector2u& windowSize,
                    const Button& closeButton,
                    float scale,
                    bool closeHovered) {
    const float panelWidth = static_cast<float>(windowSize.x) * 0.74f;
    const float panelHeight = static_cast<float>(windowSize.y) * 0.70f;
    const float panelX = (static_cast<float>(windowSize.x) - panelWidth) / 2.f;
    const float panelY = (static_cast<float>(windowSize.y) - panelHeight) / 2.f;

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(sf::Vector2f(panelX, panelY));
    panel.setFillColor(sf::Color(18, 23, 44, 245));
    panel.setOutlineColor(sf::Color(110, 163, 255, 210));
    panel.setOutlineThickness(3.f * scale);
    target.draw(panel);

    sf::Text title(font);
    title.setString("GUIDE");
    title.setCharacterSize(static_cast<unsigned int>(36.f * scale));
    title.setFillColor(sf::Color(255, 243, 194));
    drawTextCentered(target, title, sf::Vector2f(panelX + panelWidth / 2.f, panelY + 48.f * scale));

    const std::array<std::string, 4> lines = {
        "Rotate clockwise   : Up arrow or W",
        "Move left          : Left arrow or A",
        "Move right         : Right arrow or D",
        "Rotate counterwise : Down arrow or S",
    };

    float lineY = panelY + 115.f * scale;
    for (const auto& line : lines) {
        sf::Text text(font);
        text.setString(line);
        text.setCharacterSize(static_cast<unsigned int>(24.f * scale));
        text.setFillColor(sf::Color::White);
        text.setPosition(sf::Vector2f(panelX + 40.f * scale, lineY));
        target.draw(text);
        lineY += 56.f * scale;
    }

    drawButton(target, font, closeButton, closeHovered, scale);
}

void drawBoardPopup(sf::RenderTarget& target,
                   const sf::Font& font,
                   const sf::Vector2u& windowSize,
                   const std::vector<BoardEntry>& entries,
                   int scrollIndex,
                   const Button& closeButton,
                   float scale,
                   bool closeHovered) {
    const float panelWidth = static_cast<float>(windowSize.x) * 0.84f;
    const float panelHeight = static_cast<float>(windowSize.y) * 0.80f;
    const float panelX = (static_cast<float>(windowSize.x) - panelWidth) / 2.f;
    const float panelY = (static_cast<float>(windowSize.y) - panelHeight) / 2.f;

    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(sf::Vector2f(panelX, panelY));
    panel.setFillColor(sf::Color(16, 20, 36, 245));
    panel.setOutlineColor(sf::Color(120, 180, 255, 220));
    panel.setOutlineThickness(3.f * scale);
    target.draw(panel);

    sf::Text title(font);
    title.setString("BOARD");
    title.setCharacterSize(static_cast<unsigned int>(36.f * scale));
    title.setFillColor(sf::Color(255, 243, 194));
    drawTextCentered(target, title, sf::Vector2f(panelX + panelWidth / 2.f, panelY + 44.f * scale));

    sf::Text hint(font);
    hint.setString("Scroll with mouse wheel or Up / Down to move one row at a time");
    hint.setCharacterSize(static_cast<unsigned int>(18.f * scale));
    hint.setFillColor(sf::Color(195, 210, 236));
    hint.setPosition(sf::Vector2f(panelX + 34.f * scale, panelY + 82.f * scale));
    target.draw(hint);

    const int visibleRows = 7;
    const float listTop = panelY + 126.f * scale;
    const float rowHeight = 58.f * scale;
    const float rowGap = 6.f * scale;
    const float leftPadding = panelX + 28.f * scale;
    const float rightPadding = panelX + panelWidth - 28.f * scale;

    for (int row = 0; row < visibleRows; ++row) {
        const int entryIndex = scrollIndex + row;
        if (entryIndex >= static_cast<int>(entries.size())) {
            break;
        }

        const float rowY = listTop + row * (rowHeight + rowGap);
        sf::RectangleShape rowShape(sf::Vector2f(panelWidth - 56.f * scale, rowHeight));
        rowShape.setPosition(sf::Vector2f(leftPadding, rowY));
        rowShape.setFillColor(row % 2 == 0 ? sf::Color(37, 47, 75, 240) : sf::Color(28, 37, 60, 240));
        rowShape.setOutlineColor(sf::Color(110, 155, 226, 150));
        rowShape.setOutlineThickness(1.5f * scale);
        target.draw(rowShape);

        const BoardEntry& entry = entries[entryIndex];

        sf::Text rankText(font);
        rankText.setString(std::to_string(entryIndex + 1));
        rankText.setCharacterSize(static_cast<unsigned int>(22.f * scale));
        rankText.setFillColor(sf::Color(255, 230, 160));
        rankText.setPosition(sf::Vector2f(leftPadding + 16.f * scale, rowY + 15.f * scale));
        target.draw(rankText);

        sf::Text userText(font);
        userText.setString(entry.user);
        userText.setCharacterSize(static_cast<unsigned int>(22.f * scale));
        userText.setFillColor(sf::Color::White);
        userText.setPosition(sf::Vector2f(leftPadding + 72.f * scale, rowY + 15.f * scale));
        target.draw(userText);

        sf::Text scoreText(font);
        scoreText.setString(std::to_string(entry.score));
        scoreText.setCharacterSize(static_cast<unsigned int>(22.f * scale));
        scoreText.setFillColor(sf::Color(120, 255, 180));
        const sf::FloatRect scoreBounds = scoreText.getLocalBounds();
        scoreText.setPosition(sf::Vector2f(rightPadding - scoreBounds.size.x - 8.f * scale, rowY + 15.f * scale));
        target.draw(scoreText);

        sf::Text timeText(font);
        timeText.setString(entry.time);
        timeText.setCharacterSize(static_cast<unsigned int>(18.f * scale));
        timeText.setFillColor(sf::Color(190, 205, 230));
        const sf::FloatRect timeBounds = timeText.getLocalBounds();
        timeText.setPosition(sf::Vector2f(rightPadding - timeBounds.size.x - 8.f * scale, rowY + 35.f * scale));
        target.draw(timeText);
    }

    sf::Text footer(font);
    footer.setString("Showing up to 7 rows; use row-by-row scroll");
    footer.setCharacterSize(static_cast<unsigned int>(18.f * scale));
    footer.setFillColor(sf::Color(190, 205, 230));
    footer.setPosition(sf::Vector2f(panelX + 28.f * scale, panelY + panelHeight - 62.f * scale));
    target.draw(footer);

    drawButton(target, font, closeButton, closeHovered, scale);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // =========================
    // gameconsole-tao-giao-dien-169-00
    // Tạo cửa sổ tỷ lệ 9:16 để dùng cho menu console
    // =========================
    const int windowHeight = 720;
    auto window = layout::create916Window(windowHeight);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    const sf::Vector2u windowSize = window.getSize();
    const float uiScale = static_cast<float>(windowSize.y) / 720.f;

    sf::Font font;
    fs::path fontPath;
    const bool hasFont = loadFirstAvailableFont(font, fontPath);
    if (!hasFont) {
        std::cerr << "No system font found; UI labels will be skipped.\n";
    }

    // =========================
    // gameconsole-chen-backgound-01
    // Tạo background có texture và full-fit theo tỷ lệ cửa sổ
    // =========================
    sf::Texture backgroundTexture = buildBackgroundTexture();
    sf::Sprite backgroundSprite(backgroundTexture);
    const sf::Vector2f backgroundSize = sf::Vector2f(backgroundTexture.getSize());
    const float backgroundScaleX = static_cast<float>(windowSize.x) / backgroundSize.x;
    const float backgroundScaleY = static_cast<float>(windowSize.y) / backgroundSize.y;
    const float backgroundScale = std::max(backgroundScaleX, backgroundScaleY);
    backgroundSprite.setScale(sf::Vector2f(backgroundScale, backgroundScale));
    backgroundSprite.setPosition(sf::Vector2f((windowSize.x - backgroundSize.x * backgroundScale) / 2.f,
                                              (windowSize.y - backgroundSize.y * backgroundScale) / 2.f));

    sf::RectangleShape vignette(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    vignette.setFillColor(sf::Color(0, 0, 0, 55));

    // =========================
    // gameconsole-chen-nhac-05
    // Tạo nhạc nền loop nhẹ bằng buffer để menu chạy được mà không cần file ngoài
    // =========================
    sf::SoundBuffer ambientBuffer;
    std::optional<sf::Sound> ambientSound;
    if (buildAmbientBuffer(ambientBuffer)) {
        ambientSound.emplace(ambientBuffer);
        ambientSound->setLooping(true);
        ambientSound->setVolume(18.f);
        ambientSound->play();
    }

    const std::vector<BoardEntry> boardEntries = loadBoardEntries("gameConsole_board.json");
    if (boardEntries.empty()) {
        std::cerr << "Board data is empty; popup will show no entries.\n";
    }

    Button guideButton{sf::FloatRect(sf::Vector2f(windowSize.x * 0.12f, windowSize.y * 0.70f),
                                     sf::Vector2f(windowSize.x * 0.32f, 64.f)),
                       "GUIDE"};
    Button boardButton{sf::FloatRect(sf::Vector2f(windowSize.x * 0.56f, windowSize.y * 0.70f),
                                     sf::Vector2f(windowSize.x * 0.32f, 64.f)),
                       "BOARD"};
    Button closeButton{sf::FloatRect(sf::Vector2f(windowSize.x * 0.5f - 86.f, windowSize.y * 0.82f),
                                     sf::Vector2f(172.f, 52.f)),
                       "CLOSE"};

    PopupMode popupMode = PopupMode::None;
    int boardScroll = 0;

    while (window.isOpen()) {
        bool openGuide = false;
        bool openBoard = false;
        bool closePopup = false;
        int scrollDelta = 0;
        bool clicked = false;

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (popupMode == PopupMode::Board) {
                    scrollDelta += (mouseWheel->delta > 0.f) ? -1 : 1;
                }
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    if (popupMode == PopupMode::None) {
                        window.close();
                    } else {
                        closePopup = true;
                    }
                }

                if (popupMode == PopupMode::Guide && (keyPressed->code == sf::Keyboard::Key::Enter ||
                                                      keyPressed->code == sf::Keyboard::Key::Space)) {
                    closePopup = true;
                }

                if (popupMode == PopupMode::Board) {
                    if (keyPressed->code == sf::Keyboard::Key::Up) {
                        scrollDelta -= 1;
                    } else if (keyPressed->code == sf::Keyboard::Key::Down) {
                        scrollDelta += 1;
                    } else if (keyPressed->code == sf::Keyboard::Key::Enter ||
                               keyPressed->code == sf::Keyboard::Key::Space) {
                        closePopup = true;
                    }
                }
            }

            if (const auto* mouseButton = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButton->button == sf::Mouse::Button::Left) {
                    clicked = true;
                }
            }
        }

        if (scrollDelta != 0 && popupMode == PopupMode::Board) {
            boardScroll = clampBoardScroll(boardScroll + scrollDelta, static_cast<int>(boardEntries.size()), 7);
        }

        if (closePopup) {
            popupMode = PopupMode::None;
        }

        const sf::Vector2f mousePosition = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (clicked && popupMode == PopupMode::None) {
            if (guideButton.bounds.contains(mousePosition)) {
                openGuide = true;
            } else if (boardButton.bounds.contains(mousePosition)) {
                openBoard = true;
            }
        } else if (clicked && popupMode != PopupMode::None && closeButton.bounds.contains(mousePosition)) {
            closePopup = true;
        }

        if (openGuide) {
            popupMode = PopupMode::Guide;
        }
        if (openBoard) {
            popupMode = PopupMode::Board;
        }
        if (closePopup) {
            popupMode = PopupMode::None;
        }

        const bool guideHovered = popupMode == PopupMode::None && guideButton.bounds.contains(mousePosition);
        const bool boardHovered = popupMode == PopupMode::None && boardButton.bounds.contains(mousePosition);
        const bool closeHovered = popupMode != PopupMode::None && closeButton.bounds.contains(mousePosition);

        window.clear(sf::Color::Black);
        window.draw(backgroundSprite);
        window.draw(vignette);

        sf::RectangleShape topGlow(sf::Vector2f(static_cast<float>(windowSize.x), 190.f * uiScale));
        topGlow.setFillColor(sf::Color(130, 180, 255, 35));
        window.draw(topGlow);

        if (hasFont) {
            sf::Text title(font);
            title.setString("ctetris");
            title.setCharacterSize(static_cast<unsigned int>(56.f * uiScale));
            title.setFillColor(sf::Color::White);
            drawTextCentered(window, title, sf::Vector2f(windowSize.x / 2.f, 120.f * uiScale));

            sf::Text subtitle(font);
            subtitle.setString("SFML 3 menu console");
            subtitle.setCharacterSize(static_cast<unsigned int>(20.f * uiScale));
            subtitle.setFillColor(sf::Color(210, 225, 255));
            drawTextCentered(window, subtitle, sf::Vector2f(windowSize.x / 2.f, 170.f * uiScale));

            sf::Text note(font);
            note.setString("Guide and board popups are available in task 1.1-1.5 only");
            note.setCharacterSize(static_cast<unsigned int>(16.f * uiScale));
            note.setFillColor(sf::Color(180, 196, 226));
            drawTextCentered(window, note, sf::Vector2f(windowSize.x / 2.f, 206.f * uiScale));

            sf::Text musicLabel(font);
            musicLabel.setString("Ambient menu music: on");
            musicLabel.setCharacterSize(static_cast<unsigned int>(16.f * uiScale));
            musicLabel.setFillColor(sf::Color(198, 235, 202));
            drawTextCentered(window, musicLabel, sf::Vector2f(windowSize.x / 2.f, 238.f * uiScale));

            if (popupMode == PopupMode::None) {
                drawButton(window, font, guideButton, guideHovered, uiScale);
                drawButton(window, font, boardButton, boardHovered, uiScale);
            } else if (popupMode == PopupMode::Guide) {
                drawOverlayPanel(window, windowSize, 145.f);
                drawGuidePopup(window, font, windowSize, closeButton, uiScale, closeHovered);
            } else if (popupMode == PopupMode::Board) {
                drawOverlayPanel(window, windowSize, 145.f);
                drawBoardPopup(window, font, windowSize, boardEntries, boardScroll, closeButton, uiScale, closeHovered);
            }
        }

        window.display();
    }

    return 0;
}