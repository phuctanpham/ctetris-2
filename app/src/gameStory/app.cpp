// integration/v1
#include "gameStory_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>

const int INTRO_DURATION = 3000;

// gamestory-logo-intro-01
static void drawLogo(SDL_Renderer* renderer, Uint32 elapsedTime) {
    const float logoW = 90.0f, logoH = 90.0f;
    SDL_FRect logoRect = { (STORY_SCREEN_WIDTH  - logoW) / 2.0f,
                           (STORY_SCREEN_HEIGHT - logoH) / 2.0f - 40.0f,
                           logoW, logoH };
    Uint8 alpha = (Uint8)((elapsedTime * 255) / INTRO_DURATION);
    SDL_SetRenderDrawColor(renderer, 0, 122, 204, alpha);
    SDL_RenderFillRect(renderer, &logoRect);
}

// gamestory-loading-bar-02
static void drawLoadingBar(SDL_Renderer* renderer, Uint32 elapsedTime) {
    float progress = (float)elapsedTime / INTRO_DURATION;
    if (progress > 1.0f) progress = 1.0f;
    const float barW = 180.0f, barH = 12.0f;
    const float barX = (STORY_SCREEN_WIDTH - barW) / 2.0f;
    const float barY = STORY_SCREEN_HEIGHT - 100.0f;

    SDL_FRect bgBar = { barX, barY, barW, barH };
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &bgBar);

    SDL_FRect fgBar = { barX, barY, barW * progress, barH };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &fgBar);
}

int runGameStory(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    bool running = true;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) std::exit(0);
        }
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime > (Uint32)INTRO_DURATION) running = false;

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        drawLogo(renderer, elapsedTime);
        drawLoadingBar(renderer, elapsedTime);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return 0;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Story - Standalone",
                                          STORY_SCREEN_WIDTH, STORY_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameStory(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif