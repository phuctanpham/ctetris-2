// integration/v1
#include <SDL3/SDL.h>
#include <iostream>

// Khai báo giao diện tích hợp 1 chiều
extern int runGameStory(SDL_Window* window, SDL_Renderer* renderer);
extern int runGameConsole(SDL_Window* window, SDL_Renderer* renderer);
extern int runGameCore(SDL_Window* window, SDL_Renderer* renderer);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Khong the khoi tao SDL3: %s", SDL_GetError());
        return -1;
    }

    // Tỷ lệ chuẩn 6:16 
    SDL_Window* window = SDL_CreateWindow("cTetris - Integrated", 360, 960, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!window || !renderer) {
        SDL_Log("Loi tao Window/Renderer: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    runGameStory(window, renderer);
    runGameConsole(window, renderer);
    runGameCore(window, renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}