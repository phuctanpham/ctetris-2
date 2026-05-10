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

extern int runGameStory  (SDL_Window* window, SDL_Renderer* renderer);
extern int runGameConsole(SDL_Window* window, SDL_Renderer* renderer,
                          SettingsConfig& cfgInOut);
extern int runGameCore   (SDL_Window* window, SDL_Renderer* renderer,
                          const SettingsConfig& cfg);

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Khong the khoi tao SDL3: %s", SDL_GetError());
        return -1;
    }

    // Tieu de cua so chinh: "cTetris ©" thay vi "cTetris - Integrated"
    SDL_Window*   window   = SDL_CreateWindow("cTetris \xC2\xA9", 270, 480, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!window || !renderer) { SDL_Quit(); return -1; }

    runGameStory(window, renderer);

    // [D.6] Settings persist across Console <-> Core round-trips.
    // Defaults from SettingsConfig (gameConsole_layout.h): volume=0.5,
    // all 7 colors enabled, story 0 / chapter 0.
    SettingsConfig cfg;

    while (true) {
        int next = runGameConsole(window, renderer, cfg);
        if (next != 1) break;                  // QUIT tu console
        int back = runGameCore(window, renderer, cfg);
        if (back != 2) break;                  // 0 = thoat, 2 = ve console
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
