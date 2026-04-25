// integration/v1
// File chinh cua gameStory: chay man hinh intro va loading bar
#include "gameStory_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>

// Thoi gian hieu ung intro (ms)
const int INTRO_DURATION = 3000;

// gamestory-logo-intro-01
// Ve logo dang khoi vuong fade-in theo thoi gian
void drawLogo(SDL_Renderer* renderer, Uint32 elapsedTime) {
    SDL_FRect logoRect = { (STORY_SCREEN_WIDTH - 150) / 2.0f, 300.0f, 150.0f, 150.0f };
    Uint8 alpha = (Uint8)((elapsedTime * 255) / INTRO_DURATION);
    SDL_SetRenderDrawColor(renderer, 0, 122, 204, alpha);
    SDL_RenderFillRect(renderer, &logoRect);
}

// gamestory-loading-bar-02
// Ve thanh tien trinh tuyen tinh dong bo voi thoi luong intro
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

// Entry duy nhat khi tich hop voi main.cpp
int runGameStory(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    bool running = true;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            // Khi nguoi dung dong cua so -> thoat toan bo chuong trinh
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
// Entry rieng khi chay file gameStory.exe / gameStory.app doc lap
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