// integration/v1
// File chinh cua gameConsole: man hinh setting + lightbox guide/board + dieu huong ban phim
#include "gameConsole_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>

// --- Cau truc nut bam dung chung cho man hinh chinh ---
struct Button {
    SDL_FRect rect;
    const char* label;
    SDL_Color bg;
};

// Helper: kiem tra toa do chuot co nam trong rect khong
static bool hitTest(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// Helper: ve nut hinh chu nhat co vien + nhan chu giua nut
// focused = true -> ve them khung vang day de chi nut dang duoc chon
static void drawButton(SDL_Renderer* renderer, const Button& b, bool focused) {
    SDL_SetRenderDrawColor(renderer, b.bg.r, b.bg.g, b.bg.b, b.bg.a);
    SDL_RenderFillRect(renderer, &b.rect);

    if (focused) {
        // Vien vang day 3px de noi bat nut dang focus
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int k = 0; k < 3; k++) {
            SDL_FRect r = { b.rect.x - k, b.rect.y - k,
                            b.rect.w + 2 * k, b.rect.h + 2 * k };
            SDL_RenderRect(renderer, &r);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &b.rect);
    }

    // Nhan chu (font debug 8x8, can giua)
    int len = (int)SDL_strlen(b.label);
    float tx = b.rect.x + (b.rect.w - len * 8.0f) / 2.0f;
    float ty = b.rect.y + (b.rect.h - 8.0f) / 2.0f;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, tx, ty, b.label);
}

// --- Trang thai man hinh ---
struct AppState {
    bool showGuide  = false;
    bool showBoard  = false;
    bool isRunning  = true;
    int  nextScene  = 0;       // 0: thoat, 1: chuyen sang gameCore
    int  boardScroll = 0;      // chi so dong dau tien dang hien thi trong board
    int  focusIndex  = 0;      // nut dang duoc focus tren man hinh chinh
};

// --- Du lieu board hardcode (dong bo voi gameConsole_board.json) ---
struct BoardEntry { const char* user; int score; const char* time; };
static const BoardEntry BOARD_DATA[] = {
    {"ShadowWalker", 8540, "2026-04-12 08:15"},
    {"PixelKnight",  7210, "2026-04-12 09:22"},
    {"CyberGhost",   6900, "2026-04-12 10:05"},
    {"NeonViper",    6550, "2026-04-12 07:30"},
    {"IronDragon",   6120, "2026-04-11 23:45"},
    {"StormBreaker", 5890, "2026-04-12 05:12"},
    {"AlphaCoder",   5700, "2026-04-12 11:20"},
    {"MysticSage",   5430, "2026-04-11 18:55"},
    {"FrostBite",    5210, "2026-04-12 02:10"},
    {"NovaStar",     4980, "2026-04-12 04:30"},
    {"GlitchMaster", 4750, "2026-04-12 12:00"},
    {"ZenithHero",   4620, "2026-04-11 20:15"},
    {"RogueZero",    4300, "2026-04-12 01:45"},
    {"TitanPulse",   4150, "2026-04-12 06:10"},
    {"LunarSeeker",  3900, "2026-04-11 15:30"},
    {"BlazeFury",    3750, "2026-04-12 08:50"},
    {"VoidRunner",   3520, "2026-04-12 03:25"},
    {"StarLord99",   3400, "2026-04-11 22:12"},
    {"EchoWhisper",  3280, "2026-04-12 10:40"},
    {"DriftKing",    3150, "2026-04-12 05:55"},
    {"AstroBoy",     2900, "2026-04-11 19:05"},
    {"SilentBlade",  2750, "2026-04-12 07:15"},
    {"MagmaCore",    2600, "2026-04-12 09:30"},
    {"AquaMarine",   2450, "2026-04-12 11:05"},
    {"ThunderBolt",  2200, "2026-04-11 17:45"},
    {"WindWalker",   1950, "2026-04-12 02:50"},
    {"NightOwl",     1800, "2026-04-12 04:20"},
    {"SilverFang",   1650, "2026-04-11 21:10"},
    {"GoldenEye",    1400, "2026-04-12 06:45"},
    {"ProGamerVN",   1250, "2026-04-12 08:05"}
};
static const int BOARD_TOTAL  = (int)(sizeof(BOARD_DATA)/sizeof(BOARD_DATA[0]));
static const int BOARD_VISIBLE = 7;

// --- Layout 4 nut chinh: GUIDE, BOARD, PLAY, QUIT ---
static const Button MAIN_BUTTONS[] = {
    { {  60.0f, 360.0f, 240.0f, 60.0f }, "GUIDE", { 70,  90, 160, 255} },
    { {  60.0f, 450.0f, 240.0f, 60.0f }, "BOARD", { 70, 130,  90, 255} },
    { {  60.0f, 540.0f, 240.0f, 60.0f }, "PLAY",  {180,  70,  70, 255} },
    { {  60.0f, 630.0f, 240.0f, 60.0f }, "QUIT",  {110, 110, 110, 255} }
};
static const int NUM_MAIN_BUTTONS = (int)(sizeof(MAIN_BUTTONS)/sizeof(MAIN_BUTTONS[0]));

// Vung popup + nut close cua tung lightbox
static const SDL_FRect GUIDE_POPUP = {  30.0f, 200.0f, 300.0f, 480.0f };
static const SDL_FRect GUIDE_CLOSE = { 295.0f, 205.0f,  30.0f,  30.0f };
static const SDL_FRect BOARD_POPUP = {  20.0f, 140.0f, 320.0f, 680.0f };
static const SDL_FRect BOARD_CLOSE = { 305.0f, 145.0f,  30.0f,  30.0f };

// gameconsole-chen-backgound-01
// Ve nen don sac (v2 se thay bang anh full-fit)
static void drawBackground(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    SDL_RenderDebugText(renderer, 130.0f, 120.0f, "C T E T R I S");
    SDL_RenderDebugText(renderer, 110.0f, 150.0f, "-- GAME CONSOLE --");

    // Goi y dieu huong ban phim
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderDebugText(renderer,  20.0f, 770.0f, "TAB / DOWN / LEFT  : next");
    SDL_RenderDebugText(renderer,  20.0f, 790.0f, "SHIFT+TAB / UP / RIGHT: prev");
    SDL_RenderDebugText(renderer,  20.0f, 810.0f, "ENTER / SPACE      : chon");
    SDL_RenderDebugText(renderer,  20.0f, 830.0f, "ESC                : dong / huy");
}

// Helper ve nut close [X] dung chung cho cac lightbox
static void drawCloseButton(SDL_Renderer* renderer, const SDL_FRect& r) {
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &r);
    SDL_RenderDebugText(renderer, r.x + r.w/2 - 4, r.y + r.h/2 - 4, "X");
}

// gameconsole-nut-guide-02
// Ve lightbox huong dan dieu khien + nut close
static void drawGuideLightbox(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0.0f, 0.0f,
                         (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 70, 80, 110, 255);
    SDL_RenderFillRect(renderer, &GUIDE_POPUP);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &GUIDE_POPUP);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 110.0f, 250.0f, "HUONG DAN");
    SDL_RenderDebugText(renderer,  50.0f, 300.0f, "Lat ngc kim dong ho:  UP / W");
    SDL_RenderDebugText(renderer,  50.0f, 340.0f, "Lat thuan kim dong ho: DOWN/S");
    SDL_RenderDebugText(renderer,  50.0f, 380.0f, "Di chuyen trai:        LEFT/A");
    SDL_RenderDebugText(renderer,  50.0f, 420.0f, "Di chuyen phai:        RIGHT/D");
    SDL_RenderDebugText(renderer,  50.0f, 480.0f, "Bam X hoac ESC de dong");

    drawCloseButton(renderer, GUIDE_CLOSE);
}

// gameconsole-nut-board-03
// Ve lightbox bang xep hang + nut close, hien tu BOARD_DATA, scroll bang wheel
static void drawBoardLightbox(SDL_Renderer* renderer, int scroll) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0.0f, 0.0f,
                         (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 50, 100, 70, 255);
    SDL_RenderFillRect(renderer, &BOARD_POPUP);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &BOARD_POPUP);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 110.0f, 190.0f, "LEADERBOARD");
    SDL_RenderDebugText(renderer,  35.0f, 220.0f, "#  USER          SCORE   TIME");

    const float ROW_H = 60.0f;
    const float ROW_Y0 = 250.0f;
    for (int i = 0; i < BOARD_VISIBLE; i++) {
        int idx = scroll + i;
        if (idx >= BOARD_TOTAL) break;
        const BoardEntry& e = BOARD_DATA[idx];
        float y = ROW_Y0 + i * ROW_H;

        char line1[64];
        SDL_snprintf(line1, sizeof(line1), "%2d %-12.12s %5d", idx + 1, e.user, e.score);
        SDL_RenderDebugText(renderer, 35.0f, y,         line1);
        SDL_RenderDebugText(renderer, 35.0f, y + 14.0f, e.time);
    }

    SDL_RenderDebugText(renderer, 60.0f, 780.0f, "Wheel/UP-DOWN de scroll, ESC de dong");

    drawCloseButton(renderer, BOARD_CLOSE);
}

// gameconsole-chen-nhac-05
// Phat nhac nen (v1: log placeholder; v2 se dung SDL_audio)
static void playBackgroundMusic() {
    SDL_Log("Phat nhac nen console");
}

// gameconsole-dieu-huong-ban-phim-06
// Kich hoat nut tai vi tri index hien tai (tuong duong click chuot vao nut do)
static void activateButton(AppState& state, int index) {
    switch (index) {
        case 0: // GUIDE
            state.showGuide = true;
            break;
        case 1: // BOARD
            state.showBoard = true;
            state.boardScroll = 0;
            break;
        case 2: // PLAY
            state.isRunning = false;
            state.nextScene = 1;
            break;
        case 3: // QUIT
            state.isRunning = false;
            state.nextScene = 0;
            break;
        default: break;
    }
}

// Entry duy nhat khi tich hop voi main.cpp
// Tra ve: 0 = thoat, 1 = chuyen sang gameCore (PLAY)
int runGameConsole(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    AppState state;
    SDL_Event event;
    playBackgroundMusic();

    while (state.isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.isRunning = false;
                state.nextScene = 0;
                break;
            }

            // --- Wheel: chi tac dung trong lightbox board ---
            if (event.type == SDL_EVENT_MOUSE_WHEEL && state.showBoard) {
                state.boardScroll -= (int)event.wheel.y;
                if (state.boardScroll < 0) state.boardScroll = 0;
                int maxScroll = BOARD_TOTAL - BOARD_VISIBLE;
                if (state.boardScroll > maxScroll) state.boardScroll = maxScroll;
            }

            // --- Phim bam: dieu huong nut + dong lightbox + scroll ---
            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                bool shiftHeld = (event.key.mod & SDL_KMOD_SHIFT) != 0;

                // ESC: dong lightbox neu dang mo, neu o man hinh chinh thi focus QUIT
                if (key == SDLK_ESCAPE) {
                    if (state.showGuide)      state.showGuide = false;
                    else if (state.showBoard) state.showBoard = false;
                    else                      state.focusIndex = 3; // QUIT
                }
                // Trong lightbox board: UP/DOWN dung de scroll
                else if (state.showBoard && (key == SDLK_UP || key == SDLK_DOWN)) {
                    int delta = (key == SDLK_UP) ? -1 : 1;
                    state.boardScroll += delta;
                    if (state.boardScroll < 0) state.boardScroll = 0;
                    int maxScroll = BOARD_TOTAL - BOARD_VISIBLE;
                    if (state.boardScroll > maxScroll) state.boardScroll = maxScroll;
                }
                // Khong co lightbox dang mo: dieu huong nut tren man hinh chinh
                else if (!state.showGuide && !state.showBoard) {
                    // Tien (next): TAB (khong shift) / LEFT / DOWN
                    if ((key == SDLK_TAB && !shiftHeld) ||
                        key == SDLK_LEFT || key == SDLK_DOWN) {
                        state.focusIndex = (state.focusIndex + 1) % NUM_MAIN_BUTTONS;
                    }
                    // Lui (prev): SHIFT+TAB / RIGHT / UP
                    else if ((key == SDLK_TAB && shiftHeld) ||
                             key == SDLK_RIGHT || key == SDLK_UP) {
                        state.focusIndex = (state.focusIndex - 1 + NUM_MAIN_BUTTONS) % NUM_MAIN_BUTTONS;
                    }
                    // Kich hoat: ENTER / SPACE
                    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) {
                        activateButton(state, state.focusIndex);
                    }
                }
            }

            // --- Click chuot: van giu nguyen nhu cu ---
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT) {
                float mx = event.button.x;
                float my = event.button.y;

                if (state.showGuide) {
                    if (hitTest(GUIDE_CLOSE, mx, my)) state.showGuide = false;
                } else if (state.showBoard) {
                    if (hitTest(BOARD_CLOSE, mx, my)) state.showBoard = false;
                } else {
                    for (int i = 0; i < NUM_MAIN_BUTTONS; i++) {
                        if (hitTest(MAIN_BUTTONS[i].rect, mx, my)) {
                            state.focusIndex = i;
                            activateButton(state, i);
                            break;
                        }
                    }
                }
            }
        }

        // Render frame
        drawBackground(renderer);
        if (!state.showGuide && !state.showBoard) {
            for (int i = 0; i < NUM_MAIN_BUTTONS; i++) {
                drawButton(renderer, MAIN_BUTTONS[i], i == state.focusIndex);
            }
        }
        if (state.showGuide) drawGuideLightbox(renderer);
        if (state.showBoard) drawBoardLightbox(renderer, state.boardScroll);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return state.nextScene;
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