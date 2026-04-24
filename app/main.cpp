// phucpt: app/main.cpp

// =========================
// integration/v1
// Điều phối luồng: gameStory → gameConsole → gameCore
// VRSFML: GraphicsContext + AudioContext phải được tạo một lần duy nhất
// tại main() và truyền xuống toàn bộ các module con.
// Context giữ sống trong suốt vòng đời chương trình.
// =========================

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>

// Khai báo forward — mỗi module tự định nghĩa trong app.cpp của nó

// --- gameStory ---
enum class StoryResult   { Finished, Quit };
StoryResult   runGameStory(sf::GraphicsContext& gCtx);

// --- gameConsole ---
enum class ConsoleResult { StartGame, Quit };
ConsoleResult runGameConsole(sf::GraphicsContext& gCtx, sf::AudioContext& aCtx);

// --- gameCore ---
enum class CoreResult    { BackToConsole, Quit };
CoreResult    runGameCore(sf::GraphicsContext& gCtx);

// =========================
// main() — điều phối luồng màn hình
// VRSFML yêu cầu GraphicsContext & AudioContext tồn tại trước bất kỳ
// thao tác SFML nào và được giữ sống cho đến khi chương trình kết thúc.
// =========================
int main() {
    // Khởi tạo Graphics context (OpenGL ES 3.0+ / WebGL2 trên WASM)
    auto gCtxOpt = sf::GraphicsContext::create();
    if (!gCtxOpt.hasValue()) {
        std::cerr << "[main] Không tạo được GraphicsContext!\n";
        return 1;
    }
    sf::GraphicsContext& gCtx = gCtxOpt.value();

    // Khởi tạo Audio context (OpenAL / Web Audio API trên WASM)
    auto aCtxOpt = sf::AudioContext::create();
    if (!aCtxOpt.hasValue()) {
        std::cerr << "[main] Không tạo được AudioContext!\n";
        return 1;
    }
    sf::AudioContext& aCtx = aCtxOpt.value();

    // -------------------------
    // Bước 1: Màn hình giới thiệu (gameStory)
    // -------------------------
    StoryResult storyRes = runGameStory(gCtx);
    if (storyRes == StoryResult::Quit) return 0;

    // -------------------------
    // Bước 2 & 3: Vòng lặp Console ↔ Core
    // -------------------------
    bool keepRunning = true;
    while (keepRunning) {
        ConsoleResult consoleRes = runGameConsole(gCtx, aCtx);
        if (consoleRes == ConsoleResult::Quit) break;

        CoreResult coreRes = runGameCore(gCtx);
        keepRunning = (coreRes == CoreResult::BackToConsole);
    }

    return 0;
}