// integration/v1
// File chinh cua gameConsole: man hinh setting + lightbox guide/board
#include "gameConsole_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>

// Trang thai hien tai cua man hinh console
struct AppState {
    bool showGuide  = false;
    bool showBoard  = false;
    bool isRunning  = true;
};

// gameconsole-chen-backgound-01
// Ve nen don sac (v1 dung mau dac, v2 se thay bang anh full-fit)
void drawBackground(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderClear(renderer);
}

// gameconsole-nut-guide-02
// Ve lightbox huong dan dieu khien + nut close
void drawGuideLightbox(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0.0f, 0.0f,
                         (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_FRect popup = { 30.0f, 300.0f, 300.0f, 300.0f };
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
    SDL_RenderFillRect(renderer, &popup);

    SDL_FRect closeBtn = { 300.0f, 310.0f, 20.0f, 20.0f };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &closeBtn);
}

// gameconsole-nut-board-03
// Ve lightbox bang xep hang + nut close (data tu gameConsole_board.json - se nap o v2)
void drawBoardLightbox(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0.0f, 0.0f,
                         (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_FRect popup = { 30.0f, 200.0f, 300.0f, 500.0f };
    SDL_SetRenderDrawColor(renderer, 60, 100, 60, 255);
    SDL_RenderFillRect(renderer, &popup);

    SDL_FRect closeBtn = { 300.0f, 210.0f, 20.0f, 20.0f };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &closeBtn);
}

// gameconsole-chen-nhac-05
// Phat nhac nen (v1 chi log placeholder, v2 se dung SDL_audio thuc su)
void playBackgroundMusic() {
    SDL_Log("Phat nhac nen console");
}

// Entry duy nhat khi tich hop voi main.cpp
int runGameConsole(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    AppState state;
    SDL_Event event;
    playBackgroundMusic();

    while (state.isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) std::exit(0);
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float mx = event.button.x;
                float my = event.button.y;
                if (state.showGuide && mx > 300 && mx < 320 && my > 310 && my < 330) {
                    state.showGuide = false;
                } else if (state.showBoard && mx > 300 && mx < 320 && my > 210 && my < 230) {
                    state.showBoard = false;
                } else if (!state.showGuide && !state.showBoard) {
                    if (event.button.button == SDL_BUTTON_LEFT)   state.showGuide = true;
                    if (event.button.button == SDL_BUTTON_RIGHT)  state.showBoard = true;
                    if (event.button.button == SDL_BUTTON_MIDDLE) state.isRunning = false;
                }
            }
        }
        drawBackground(renderer);
        if (state.showGuide) drawGuideLightbox(renderer);
        if (state.showBoard) drawBoardLightbox(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return 0;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Console - Standalone",
                                          CONSOLE_SCREEN_WIDTH, CONSOLE_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameConsole(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif