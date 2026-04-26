// integration/v1
#include "gameConsole_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>

struct Button {
    SDL_FRect rect;
    const char* label;
    SDL_Color bg;
};

static bool hitTest(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

static void drawButton(SDL_Renderer* renderer, const Button& b, bool focused) {
    SDL_SetRenderDrawColor(renderer, b.bg.r, b.bg.g, b.bg.b, b.bg.a);
    SDL_RenderFillRect(renderer, &b.rect);
    if (focused) {
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int k = 0; k < 2; k++) {
            SDL_FRect r = { b.rect.x - k, b.rect.y - k,
                            b.rect.w + 2 * k, b.rect.h + 2 * k };
            SDL_RenderRect(renderer, &r);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &b.rect);
    }
    int len = (int)SDL_strlen(b.label);
    float tx = b.rect.x + (b.rect.w - len * 8.0f) / 2.0f;
    float ty = b.rect.y + (b.rect.h - 8.0f) / 2.0f;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, tx, ty, b.label);
}

struct AppState {
    bool showGuide  = false;
    bool showBoard  = false;
    bool isRunning  = true;
    int  nextScene  = 0;
    int  boardScroll = 0;
    int  guideScroll = 0;
    int  focusIndex  = 0;
};

struct BoardEntry { const char* user; int score; const char* time; };
static const BoardEntry BOARD_DATA[] = {
    {"ShadowWalker", 8540, "04-12 08:15"}, {"PixelKnight",  7210, "04-12 09:22"},
    {"CyberGhost",   6900, "04-12 10:05"}, {"NeonViper",    6550, "04-12 07:30"},
    {"IronDragon",   6120, "04-11 23:45"}, {"StormBreaker", 5890, "04-12 05:12"},
    {"AlphaCoder",   5700, "04-12 11:20"}, {"MysticSage",   5430, "04-11 18:55"},
    {"FrostBite",    5210, "04-12 02:10"}, {"NovaStar",     4980, "04-12 04:30"},
    {"GlitchMaster", 4750, "04-12 12:00"}, {"ZenithHero",   4620, "04-11 20:15"},
    {"RogueZero",    4300, "04-12 01:45"}, {"TitanPulse",   4150, "04-12 06:10"},
    {"LunarSeeker",  3900, "04-11 15:30"}, {"BlazeFury",    3750, "04-12 08:50"},
    {"VoidRunner",   3520, "04-12 03:25"}, {"StarLord99",   3400, "04-11 22:12"},
    {"EchoWhisper",  3280, "04-12 10:40"}, {"DriftKing",    3150, "04-12 05:55"},
    {"AstroBoy",     2900, "04-11 19:05"}, {"SilentBlade",  2750, "04-12 07:15"},
    {"MagmaCore",    2600, "04-12 09:30"}, {"AquaMarine",   2450, "04-12 11:05"},
    {"ThunderBolt",  2200, "04-11 17:45"}, {"WindWalker",   1950, "04-12 02:50"},
    {"NightOwl",     1800, "04-12 04:20"}, {"SilverFang",   1650, "04-11 21:10"},
    {"GoldenEye",    1400, "04-12 06:45"}, {"ProGamerVN",   1250, "04-12 08:05"}
};
static const int BOARD_TOTAL = (int)(sizeof(BOARD_DATA)/sizeof(BOARD_DATA[0]));
static const int BOARD_VISIBLE = 9;   // tang tu 7 -> 9

static const float BTN_W = 140.0f;
static const float BTN_H = 30.0f;
static const float BTN_X = (CONSOLE_SCREEN_WIDTH - BTN_W) / 2.0f;
static const Button MAIN_BUTTONS[] = {
    { { BTN_X, 160.0f, BTN_W, BTN_H }, "GUIDE", { 70,  90, 160, 255} },
    { { BTN_X, 200.0f, BTN_W, BTN_H }, "BOARD", { 70, 130,  90, 255} },
    { { BTN_X, 240.0f, BTN_W, BTN_H }, "PLAY",  {180,  70,  70, 255} },
    { { BTN_X, 280.0f, BTN_W, BTN_H }, "QUIT",  {110, 110, 110, 255} }
};
static const int NUM_MAIN_BUTTONS = (int)(sizeof(MAIN_BUTTONS)/sizeof(MAIN_BUTTONS[0]));

// Popup guide chiem gan toan man hinh de chua noi dung dai
static const SDL_FRect GUIDE_POPUP = { 5.0f, 20.0f, 260.0f, 440.0f };
static const SDL_FRect GUIDE_CLOSE = { GUIDE_POPUP.x + GUIDE_POPUP.w - 22.0f,
                                       GUIDE_POPUP.y + 4.0f, 18.0f, 18.0f };
// Popup board: rong 260 de fit 3 cot
static const SDL_FRect BOARD_POPUP = { 5.0f, 30.0f, 260.0f, 420.0f };
static const SDL_FRect BOARD_CLOSE = { BOARD_POPUP.x + BOARD_POPUP.w - 22.0f,
                                       BOARD_POPUP.y + 4.0f, 18.0f, 18.0f };

// Helper ngat text theo so ky tu max moi dong (font 8px/char)
static int drawWrappedText(SDL_Renderer* renderer, const char* text,
                           float x, float y, int maxCharsPerLine, int maxLines) {
    int len = (int)SDL_strlen(text);
    int pos = 0;
    int lineCount = 0;
    char buf[128];
    while (pos < len && lineCount < maxLines) {
        int remaining = len - pos;
        int take = (remaining > maxCharsPerLine) ? maxCharsPerLine : remaining;
        if (take == maxCharsPerLine && pos + take < len) {
            int back = take;
            while (back > 0 && text[pos + back - 1] != ' ') back--;
            if (back > 0) take = back;
        }
        if (take >= (int)sizeof(buf)) take = (int)sizeof(buf) - 1;
        SDL_memcpy(buf, text + pos, take);
        buf[take] = '\0';
        SDL_RenderDebugText(renderer, x, y + lineCount * 14.0f, buf);
        pos += take;
        while (pos < len && text[pos] == ' ') pos++;
        lineCount++;
    }
    return lineCount;
}

// Helper scrollbar dung
static void drawVerticalScrollbar(SDL_Renderer* renderer,
                                  float x, float y, float h,
                                  int totalLines, int visibleLines, int scrollPos) {
    if (totalLines <= visibleLines) return;
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
    SDL_FRect track = { x, y, 4.0f, h };
    SDL_RenderFillRect(renderer, &track);
    float thumbH = h * (float)visibleLines / (float)totalLines;
    if (thumbH < 8.0f) thumbH = 8.0f;
    float thumbY = y + (h - thumbH) * (float)scrollPos
                   / (float)(totalLines - visibleLines);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_FRect thumb = { x, thumbY, 4.0f, thumbH };
    SDL_RenderFillRect(renderer, &thumb);
}

// Noi dung guide tieng Anh - mo ta 12 component cua sidebar gameCore
static const char* GUIDE_LINES[] = {
    "HOW TO PLAY",
    "",
    "Movement keys:",
    " - LEFT / A : move left",
    " - RIGHT / D : move right",
    " - UP / W : rotate CCW",
    " - DOWN / S : rotate CW",
    " - SPACE : speed boost x3",
    " - ENTER : pause / resume",
    " - ESC : open quit menu",
    "",
    "Sidebar (12 components):",
    "  1 QUIT  : open quit menu",
    "  2 PAUSE : stop / play",
    "  3 SCORE : current points",
    "  4 SPEED : drop speed level",
    "  5 NEXT-1: upcoming piece",
    "  6 NEXT-2: reserved (v2)",
    "  7 NEXT-3: reserved (v3)",
    "  8 ARR UP    = key UP/W",
    "  9 ARR DOWN  = key DOWN/S",
    " 10 ARR LEFT  = key LEFT/A",
    " 11 ARR RIGHT = key RIGHT/D",
    " 12 SPEED BOOST: hold=SPACE",
    "",
    "Quit popup options:",
    " - Restart: new game, score 0",
    " - Console: back to setting",
    " - Quit   : close app",
    " - Cancel : keep paused",
    "",
    "Tip: hold SPEED BOOST",
    "to make piece drop 3x faster.",
    "",
    "Press X or ESC to close.",
};
static const int GUIDE_LINE_COUNT = (int)(sizeof(GUIDE_LINES)/sizeof(GUIDE_LINES[0]));
static const int GUIDE_VISIBLE_LINES = 24;

static void drawBackground(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    float titleX = (CONSOLE_SCREEN_WIDTH - 13 * 8.0f) / 2.0f;
    SDL_RenderDebugText(renderer, titleX, 60.0f, "C T E T R I S");
    float subX = (CONSOLE_SCREEN_WIDTH - 18 * 8.0f) / 2.0f;
    SDL_RenderDebugText(renderer, subX, 90.0f, "-- GAME CONSOLE --");

    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderDebugText(renderer, 10.0f, 380.0f, "TAB/DOWN/LEFT(S/A): next");
    SDL_RenderDebugText(renderer, 10.0f, 395.0f, "SHIFT+TAB/UP/RIGHT(W/D):prev");
    SDL_RenderDebugText(renderer, 10.0f, 410.0f, "ENTER/SPACE: select");
    SDL_RenderDebugText(renderer, 10.0f, 425.0f, "ESC: close / cancel");
}

static void drawCloseButton(SDL_Renderer* renderer, const SDL_FRect& r) {
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &r);
    SDL_RenderDebugText(renderer, r.x + r.w/2 - 4, r.y + r.h/2 - 4, "X");
}

static void drawGuideLightbox(SDL_Renderer* renderer, int scroll) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 70, 80, 110, 255);
    SDL_RenderFillRect(renderer, &GUIDE_POPUP);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &GUIDE_POPUP);

    // Vung text: chua 4px le trai, 12px le phai (de cho scrollbar)
    const float CONTENT_X = GUIDE_POPUP.x + 8;
    const float CONTENT_Y0 = GUIDE_POPUP.y + 30;
    const float ROW_H = 14.0f;
    const int   MAX_CHARS = 28; // 28 * 8 = 224 px <= 240px content width
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    int rowDrawn = 0;
    for (int i = 0; i < GUIDE_VISIBLE_LINES && rowDrawn < GUIDE_VISIBLE_LINES; i++) {
        int idx = scroll + i;
        if (idx >= GUIDE_LINE_COUNT) break;
        // wrap: neu line dai > MAX_CHARS, dung helper de ngat (toi da 2 dong)
        int used = drawWrappedText(renderer, GUIDE_LINES[idx],
                                   CONTENT_X, CONTENT_Y0 + rowDrawn * ROW_H,
                                   MAX_CHARS, 2);
        rowDrawn += (used > 0) ? used : 1;
    }

    // Scrollbar
    drawVerticalScrollbar(renderer,
                          GUIDE_POPUP.x + GUIDE_POPUP.w - 8,
                          GUIDE_POPUP.y + 30, GUIDE_POPUP.h - 40,
                          GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES, scroll);

    drawCloseButton(renderer, GUIDE_CLOSE);
}

static void drawBoardLightbox(SDL_Renderer* renderer, int scroll) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 50, 100, 70, 255);
    SDL_RenderFillRect(renderer, &BOARD_POPUP);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &BOARD_POPUP);

    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 80, BOARD_POPUP.y + 14, "LEADERBOARD");

    // Header 3 cot: # USER         SCORE TIME
    // Layout 1 dong: "## UUUUUUUUUUUU SSSSS MM-DD HH:MM"
    //                 2 + 1 + 12      + 1 + 5 + 1 + 11 = 33 ky tu
    // Voi font 8px/char => 33*8 = 264px < 252px content => can rut gon them
    // Dieu chinh: bo MM-DD chi giu HH:MM (5 ky tu), tong = 27 ky tu = 216px (fit)
    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8, BOARD_POPUP.y + 38,
                        "#  USER         SCORE TIME");

    const float ROW_H = 36.0f; // 9 hang * 36 = 324px, vua trong 420 - header - footer
    const float ROW_Y0 = BOARD_POPUP.y + 60.0f;
    char line[64];
    for (int i = 0; i < BOARD_VISIBLE; i++) {
        int idx = scroll + i;
        if (idx >= BOARD_TOTAL) break;
        const BoardEntry& e = BOARD_DATA[idx];
        float y = ROW_Y0 + i * ROW_H;
        // Chi lay 5 ky tu cuoi cua time (HH:MM) de fit 1 dong
        const char* tShort = e.time;
        int tLen = (int)SDL_strlen(e.time);
        if (tLen >= 5) tShort = e.time + tLen - 5;
        SDL_snprintf(line, sizeof(line), "%2d %-12.12s %5d %s",
                     idx + 1, e.user, e.score, tShort);
        SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8, y, line);
    }

    // Scrollbar
    drawVerticalScrollbar(renderer,
                          BOARD_POPUP.x + BOARD_POPUP.w - 8,
                          ROW_Y0, BOARD_VISIBLE * ROW_H,
                          BOARD_TOTAL, BOARD_VISIBLE, scroll);

    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8,
                        BOARD_POPUP.y + BOARD_POPUP.h - 14,
                        "W/S to scroll, ESC close");

    drawCloseButton(renderer, BOARD_CLOSE);
}

static void playBackgroundMusic() { SDL_Log("Phat nhac nen console"); }

static void activateButton(AppState& state, int index) {
    switch (index) {
        case 0: state.showGuide = true; state.guideScroll = 0; break;
        case 1: state.showBoard = true; state.boardScroll = 0; break;
        case 2: state.isRunning = false; state.nextScene = 1; break;
        case 3: state.isRunning = false; state.nextScene = 0; break;
        default: break;
    }
}

int runGameConsole(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    AppState state;
    SDL_Event event;
    playBackgroundMusic();

    while (state.isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.isRunning = false; state.nextScene = 0; break;
            }

            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                if (state.showBoard) {
                    state.boardScroll -= (int)event.wheel.y;
                    if (state.boardScroll < 0) state.boardScroll = 0;
                    int maxScroll = BOARD_TOTAL - BOARD_VISIBLE;
                    if (state.boardScroll > maxScroll) state.boardScroll = maxScroll;
                } else if (state.showGuide) {
                    state.guideScroll -= (int)event.wheel.y;
                    if (state.guideScroll < 0) state.guideScroll = 0;
                    int maxScroll = GUIDE_LINE_COUNT - GUIDE_VISIBLE_LINES;
                    if (maxScroll < 0) maxScroll = 0;
                    if (state.guideScroll > maxScroll) state.guideScroll = maxScroll;
                }
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                bool shiftHeld = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                bool isUp    = (key == SDLK_UP    || key == SDLK_W);
                bool isDown  = (key == SDLK_DOWN  || key == SDLK_S);
                bool isLeft  = (key == SDLK_LEFT  || key == SDLK_A);
                bool isRight = (key == SDLK_RIGHT || key == SDLK_D);

                if (key == SDLK_ESCAPE) {
                    if (state.showGuide)      state.showGuide = false;
                    else if (state.showBoard) state.showBoard = false;
                    else                      state.focusIndex = 3;
                }
                else if (state.showBoard && (isUp || isDown)) {
                    int delta = isUp ? -1 : 1;
                    state.boardScroll += delta;
                    if (state.boardScroll < 0) state.boardScroll = 0;
                    int maxScroll = BOARD_TOTAL - BOARD_VISIBLE;
                    if (state.boardScroll > maxScroll) state.boardScroll = maxScroll;
                }
                else if (state.showGuide && (isUp || isDown)) {
                    int delta = isUp ? -1 : 1;
                    state.guideScroll += delta;
                    if (state.guideScroll < 0) state.guideScroll = 0;
                    int maxScroll = GUIDE_LINE_COUNT - GUIDE_VISIBLE_LINES;
                    if (maxScroll < 0) maxScroll = 0;
                    if (state.guideScroll > maxScroll) state.guideScroll = maxScroll;
                }
                else if (!state.showGuide && !state.showBoard) {
                    if ((key == SDLK_TAB && !shiftHeld) || isLeft || isDown)
                        state.focusIndex = (state.focusIndex + 1) % NUM_MAIN_BUTTONS;
                    else if ((key == SDLK_TAB && shiftHeld) || isRight || isUp)
                        state.focusIndex = (state.focusIndex - 1 + NUM_MAIN_BUTTONS) % NUM_MAIN_BUTTONS;
                    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
                        activateButton(state, state.focusIndex);
                }
            }

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

        drawBackground(renderer);
        if (!state.showGuide && !state.showBoard) {
            for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
                drawButton(renderer, MAIN_BUTTONS[i], i == state.focusIndex);
        }
        if (state.showGuide) drawGuideLightbox(renderer, state.guideScroll);
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