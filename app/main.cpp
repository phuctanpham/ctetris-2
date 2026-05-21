// =============================================================================
// [D.6] PATCH for app/main.cpp
// =============================================================================
// Replace the existing forward declarations and main() body with the version
// below. Diff:
//   - Include gameConsole_layout.h (for SettingsConfig)
//   - runGameConsole signature: + SettingsConfig&
//   - runGameCore signature:    + const SettingsConfig&
//   - main() declares one SettingsConfig that persists across the loop --
//     volume + color choices made in Console survive Core round-trips.
// =============================================================================

// integration/v1
// Cua so duy nhat 270x480; Core co the tra ve 2 -> quay lai Console
// Tieu de cua so su dung "cTetris ©" -- ky tu copyright (U+00A9) duoc OS render
// truc tiep qua window manager nen hien thi tot tren title bar (khac voi text
// trong SDL canvas vi SDL_RenderDebugText chi ho tro ASCII).
#include <SDL3/SDL.h>
#include "gameConsole_layout.h"   // [D.6] SettingsConfig contract
#include "logger.h"               // File logging system
#include "ctetris_debug.h"        // HTTP debug logging

extern int runGameStory  (SDL_Window* window, SDL_Renderer* renderer,
                          int storyId = 0, int chapterId = 0);
extern int runGameConsole(SDL_Window* window, SDL_Renderer* renderer,
                          SettingsConfig& cfgInOut);
extern int runGameCore   (SDL_Window* window, SDL_Renderer* renderer,
                          const SettingsConfig& cfg);

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    CTDBG_INIT();   // Initialize HTTP debug log (ctetris.log on native, no-op on WASM)
    
    Logger& logger = Logger::getInstance();
    logger.log("=== cTetris Application Started ===");
    
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        logger.logError("Cannot initialize SDL3: %s", SDL_GetError());
        return -1;
    }
    
    logger.log("SDL3 initialized successfully (VIDEO | AUDIO)");

    // Tieu de cua so chinh: "cTetris ©" thay vi "cTetris - Integrated"
    SDL_Window*   window   = SDL_CreateWindow("cTetris \xC2\xA9", 270, 480, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!window || !renderer) { 
        logger.logError("Failed to create window or renderer");
        SDL_Quit(); 
        return -1; 
    }
    
    logger.log("Window created: 270x480");

    logger.logEvent("STORY", "Starting game story sequence");
    runGameStory(window, renderer);

    // [D.6] Settings persist across Console <-> Core round-trips.
    // Defaults from SettingsConfig (gameConsole_layout.h): volume=0.5,
    // all 7 colors enabled, story 0 / chapter 0.
    SettingsConfig cfg;
    
    logger.logEvent("CONSOLE", "Entering main game loop");

    while (true) {
        int next = runGameConsole(window, renderer, cfg);

        if (next == 0) {
            logger.log("User quit from console menu");
            break;
        }

        // nextScene=2: user clicked Play in Stories popup -> preview dialogue
        // then loop back to Console without going to gameCore.
        if (next == 2) {
            if (cfg.storyId > 0) {
                logger.logEvent("STORY", "Preview dialogue story=%d chapter=%d",
                                cfg.storyId, cfg.chapterId);
                runGameStory(window, renderer, cfg.storyId, cfg.chapterId);
            }
            continue;   // back to Console
        }

        if (next == 3) {
            // DB not found in Console -> go to gameStory to init + sync
            logger.logEvent("STORY", "DB missing -> gameStory init");
            runGameStory(window, renderer, 0, 0);
            continue;
        }

        // nextScene=1: PLAY button -> show story dialogue then enter gameCore
        if (cfg.storyId > 0) {
            logger.logEvent("STORY", "Playing dialogue story=%d chapter=%d",
                            cfg.storyId, cfg.chapterId);
            runGameStory(window, renderer, cfg.storyId, cfg.chapterId);
        }
        
        int back = runGameCore(window, renderer, cfg);
        if (back != 2) {
            logger.log("User quit from game core");
            break;
        }
        
        logger.logEvent("CONSOLE", "Returning to console");
    }

    logger.log("=== cTetris Application Shutdown ===");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    logger.close();
    
    return 0;
}
