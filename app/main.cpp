// integration/v1
// File tich hop: gameStory -> gameConsole -> (PLAY) -> gameCore
#include <SDL3/SDL.h>

extern int runGameStory(SDL_Window* window, SDL_Renderer* renderer);
extern int runGameConsole(SDL_Window* window, SDL_Renderer* renderer);
extern int runGameCore(SDL_Window* window, SDL_Renderer* renderer);

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Khong the khoi tao SDL3: %s", SDL_GetError());
        return -1;
    }

    // Ty le 9:16 chuan
    SDL_Window*   window   = SDL_CreateWindow("cTetris - Integrated", 360, 960, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!window || !renderer) {
        SDL_Log("Loi tao Window/Renderer: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // 1) Story chay xong tu dong chuyen
    runGameStory(window, renderer);

    // 2) Console: chi sang Core khi user bam PLAY (return = 1)
    int next = runGameConsole(window, renderer);

    // 3) Core: chi chay neu duoc PLAY
    if (next == 1) runGameCore(window, renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}