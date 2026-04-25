// integration/v1
#include "include/layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>

const int INTRO_DURATION = 3000;

// gamestory-logo-intro-01
void drawLogo(SDL_Renderer* renderer, Uint32 elapsedTime) {
    SDL_FRect logoRect = { (STORY_SCREEN_WIDTH - 150) / 2.0f, 300.0f, 150.0f, 150.0f };
    Uint8 alpha = (elapsedTime * 255) / INTRO_DURATION;
    SDL_SetRenderDrawColor(renderer, 0, 122, 204, alpha);
    SDL_RenderFillRect(renderer, &logoRect);
}

// gamestory-loading-bar-02
void drawLoadingBar(SDL_Renderer* renderer, Uint32 elapsedTime) {
    float progress = (float)elapsedTime / INTRO_DURATION;
    if (progress > 1.0f) progress = 1.0f;
    SDL_FRect bgBar = { 60.0f, 600.0f, 240.0f, 20.0f };
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &bgBar);
    SDL_FRect fgBar = { 60.0f, 600.0f, 240.0f * progress, 20.0f };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &fgBar);
}

int runGameStory(SDL_Window* window, SDL_Renderer* renderer) {
    bool running = true;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) exit(0);
        }
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime > INTRO_DURATION) running = false;

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
int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Story - Standalone", STORY_SCREEN_WIDTH, STORY_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameStory(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif