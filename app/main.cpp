// integration/v1
// Cua so duy nhat 270x480; Core co the tra ve 2 -> quay lai Console
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

    SDL_Window*   window   = SDL_CreateWindow("cTetris - Integrated", 270, 480, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!window || !renderer) { SDL_Quit(); return -1; }

    runGameStory(window, renderer);

    while (true) {
        int next = runGameConsole(window, renderer);
        if (next != 1) break;                  // QUIT tu console
        int back = runGameCore(window, renderer);
        if (back != 2) break;                  // 0 = thoat, 2 = ve console
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}