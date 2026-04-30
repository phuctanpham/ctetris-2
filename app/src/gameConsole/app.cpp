// integration/v1
#include "gameConsole_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <cmath>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Mau text "nhe" -- 220 thay vi 255
static const SDL_Color SOFT_WHITE  = {220, 220, 220, 255};
static const SDL_Color HIGHLIGHT_Y = {255, 215,   0, 255};

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
        SDL_SetRenderDrawColor(renderer, HIGHLIGHT_Y.r, HIGHLIGHT_Y.g, HIGHLIGHT_Y.b, 255);
        for (int k = 0; k < 2; k++) {
            SDL_FRect r = { b.rect.x - k, b.rect.y - k,
                            b.rect.w + 2 * k, b.rect.h + 2 * k };
            SDL_RenderRect(renderer, &r);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
        SDL_RenderRect(renderer, &b.rect);
    }
    int len = (int)SDL_strlen(b.label);
    float tx = b.rect.x + (b.rect.w - len * 8.0f) / 2.0f;
    float ty = b.rect.y + (b.rect.h - 8.0f) / 2.0f;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer, tx, ty, b.label);
}

// =========================================================
// Scrollbar tuong tac
// =========================================================
struct SBLayout {
    SDL_FRect upBtn;
    SDL_FRect track;
    SDL_FRect downBtn;
    SDL_FRect thumb;
};

struct SBInteraction {
    bool   upHeld         = false;
    bool   downHeld       = false;
    bool   dragging       = false;
    float  dragOffsetY    = 0.0f;
    Uint32 lastAutoScroll = 0;
    Uint32 nextAutoStart  = 0;
};

static const float SB_W     = 8.0f;
static const float SB_BTN_H = 12.0f;

static SBLayout layoutSB(float x, float y, float h, int total, int visible, int pos) {
    SBLayout sb;
    sb.upBtn   = { x, y,                 SB_W, SB_BTN_H };
    sb.downBtn = { x, y + h - SB_BTN_H,  SB_W, SB_BTN_H };
    sb.track   = { x, y + SB_BTN_H,      SB_W, h - 2 * SB_BTN_H };
    sb.thumb   = { 0, 0, 0, 0 };
    if (total > visible && sb.track.h > 0) {
        float thumbH = sb.track.h * (float)visible / (float)total;
        if (thumbH < 16.0f)        thumbH = 16.0f;
        if (thumbH > sb.track.h)   thumbH = sb.track.h;
        float maxScroll = (float)(total - visible);
        float thumbY = sb.track.y + (sb.track.h - thumbH) * (float)pos / maxScroll;
        sb.thumb = { x, thumbY, SB_W, thumbH };
    }
    return sb;
}

static int clampScroll(int pos, int total, int visible) {
    int maxS = total - visible;
    if (maxS < 0) maxS = 0;
    if (pos < 0) pos = 0;
    if (pos > maxS) pos = maxS;
    return pos;
}

static void drawSB(SDL_Renderer* r, const SBLayout& sb, int total, int visible,
                   bool upHeld, bool downHeld, bool dragging) {
    SDL_Color upC = upHeld   ? HIGHLIGHT_Y : SDL_Color{120, 120, 130, 255};
    SDL_Color dnC = downHeld ? HIGHLIGHT_Y : SDL_Color{120, 120, 130, 255};

    SDL_SetRenderDrawColor(r, upC.r, upC.g, upC.b, 255);
    SDL_RenderFillRect(r, &sb.upBtn);
    SDL_SetRenderDrawColor(r, dnC.r, dnC.g, dnC.b, 255);
    SDL_RenderFillRect(r, &sb.downBtn);

    SDL_SetRenderDrawColor(r, 30, 30, 40, 255);
    float ucx = sb.upBtn.x   + sb.upBtn.w   / 2;
    float ucy = sb.upBtn.y   + sb.upBtn.h   / 2;
    float dcx = sb.downBtn.x + sb.downBtn.w / 2;
    float dcy = sb.downBtn.y + sb.downBtn.h / 2;
    for (int i = 0; i < 3; i++) {
        SDL_FRect lnUp = { ucx - i, ucy - 1 + i, 1.0f + 2.0f * i, 1.0f };
        SDL_FRect lnDn = { dcx - i, dcy + 1 - i, 1.0f + 2.0f * i, 1.0f };
        SDL_RenderFillRect(r, &lnUp);
        SDL_RenderFillRect(r, &lnDn);
    }

    if (total > visible) {
        SDL_SetRenderDrawColor(r, 40, 40, 50, 255);
        SDL_RenderFillRect(r, &sb.track);
        SDL_Color tC = dragging ? HIGHLIGHT_Y : SDL_Color{200, 200, 200, 255};
        SDL_SetRenderDrawColor(r, tC.r, tC.g, tC.b, 255);
        SDL_RenderFillRect(r, &sb.thumb);
    }
}

static bool sbOnMouseDown(SBInteraction& sbi, const SBLayout& sb, float mx, float my,
                          int& scrollPos, int total, int visible, Uint32 nowMs) {
    if (hitTest(sb.upBtn, mx, my)) {
        sbi.upHeld = true;
        scrollPos = clampScroll(scrollPos - 1, total, visible);
        sbi.lastAutoScroll = nowMs;
        sbi.nextAutoStart  = nowMs + 300;
        return true;
    }
    if (hitTest(sb.downBtn, mx, my)) {
        sbi.downHeld = true;
        scrollPos = clampScroll(scrollPos + 1, total, visible);
        sbi.lastAutoScroll = nowMs;
        sbi.nextAutoStart  = nowMs + 300;
        return true;
    }
    if (total > visible) {
        if (hitTest(sb.thumb, mx, my)) {
            sbi.dragging    = true;
            sbi.dragOffsetY = my - sb.thumb.y;
            return true;
        }
        if (hitTest(sb.track, mx, my)) {
            float ratio = (my - sb.track.y) / sb.track.h;
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
            scrollPos = clampScroll((int)(ratio * (total - visible) + 0.5f), total, visible);
            return true;
        }
    }
    return false;
}

static void sbOnMouseMotion(SBInteraction& sbi, const SBLayout& sb, float my,
                            int& scrollPos, int total, int visible) {
    if (!sbi.dragging || total <= visible) return;
    float trackUsable = sb.track.h - sb.thumb.h;
    if (trackUsable <= 0) return;
    float thumbY = my - sbi.dragOffsetY;
    float ratio = (thumbY - sb.track.y) / trackUsable;
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    scrollPos = clampScroll((int)(ratio * (total - visible) + 0.5f), total, visible);
}

static void sbResetInteraction(SBInteraction& sbi) {
    sbi.upHeld = false;
    sbi.downHeld = false;
    sbi.dragging = false;
}

static void sbAutoRepeat(SBInteraction& sbi, int& scrollPos, int total, int visible, Uint32 nowMs) {
    const Uint32 REPEAT_INTERVAL = 60;
    if (nowMs < sbi.nextAutoStart) return;
    if (nowMs - sbi.lastAutoScroll < REPEAT_INTERVAL) return;
    if (sbi.upHeld) {
        scrollPos = clampScroll(scrollPos - 1, total, visible);
        sbi.lastAutoScroll = nowMs;
    }
    if (sbi.downHeld) {
        scrollPos = clampScroll(scrollPos + 1, total, visible);
        sbi.lastAutoScroll = nowMs;
    }
}

// =========================================================
// WASM Shutdown screen
// =========================================================
// Nut RELOAD giua man hinh -- kich thuoc dong nhat voi gameCore
static const SDL_FRect CONSOLE_RELOAD_BTN = {
    (CONSOLE_SCREEN_WIDTH  - 160.0f) / 2.0f,
    (CONSOLE_SCREEN_HEIGHT - 50.0f)  / 2.0f,
    160.0f, 50.0f
};

static void drawConsoleWasmShutdown(SDL_Renderer* renderer, bool reloadHover) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color btnBg = reloadHover ? SDL_Color{ 90, 130, 200, 255}
                                  : SDL_Color{ 60, 100, 170, 255};
    SDL_SetRenderDrawColor(renderer, btnBg.r, btnBg.g, btnBg.b, 255);
    SDL_RenderFillRect(renderer, &CONSOLE_RELOAD_BTN);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &CONSOLE_RELOAD_BTN);

    const char* label = "RELOAD";
    int ll = (int)SDL_strlen(label);
    SDL_RenderDebugText(renderer,
                        CONSOLE_RELOAD_BTN.x + (CONSOLE_RELOAD_BTN.w - ll * 8.0f) / 2.0f,
                        CONSOLE_RELOAD_BTN.y + (CONSOLE_RELOAD_BTN.h - 8.0f) / 2.0f,
                        label);

    // Hint 2 dong ngan -- dam bao <= CONSOLE_SCREEN_WIDTH (270px)
    // Dong 1: "Press F5 or click RELOAD"   25 chars x 8px = 200px
    // Dong 2: "to start a new game"        19 chars x 8px = 152px
    float hintY = CONSOLE_RELOAD_BTN.y + CONSOLE_RELOAD_BTN.h + 16.0f;
    const char* hint1 = "Press F5 or click RELOAD";
    const char* hint2 = "to start a new game";
    int h1l = (int)SDL_strlen(hint1);
    int h2l = (int)SDL_strlen(hint2);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer,
                        (CONSOLE_SCREEN_WIDTH - h1l * 8.0f) / 2.0f,
                        hintY, hint1);
    SDL_RenderDebugText(renderer,
                        (CONSOLE_SCREEN_WIDTH - h2l * 8.0f) / 2.0f,
                        hintY + 14.0f, hint2);
}

// =========================================================
// AppState
// =========================================================
struct AppState {
    bool showGuide    = false;
    bool showBoard    = false;
    bool isRunning    = true;
    int  nextScene    = 0;
    int  boardScroll  = 0;
    int  guideScroll  = 0;
    int  focusIndex   = 0;
    SBInteraction sb;

    // WASM-only: khi QUIT duoc chon, hien shutdown screen thay vi
    // tra ve 0 lam canvas trang (khong co SDL_DestroyWindow phu hop)
    bool wasmShutdown = false;
    bool reloadHover  = false;
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
static const int BOARD_TOTAL   = (int)(sizeof(BOARD_DATA)/sizeof(BOARD_DATA[0]));
static const int BOARD_VISIBLE = 9;

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

static const SDL_FRect GUIDE_POPUP = { 5.0f, 20.0f, 260.0f, 440.0f };
static const SDL_FRect GUIDE_CLOSE = { GUIDE_POPUP.x + GUIDE_POPUP.w - 22.0f,
                                       GUIDE_POPUP.y + 4.0f, 18.0f, 18.0f };
static const SDL_FRect BOARD_POPUP = { 5.0f, 30.0f, 260.0f, 420.0f };
static const SDL_FRect BOARD_CLOSE = { BOARD_POPUP.x + BOARD_POPUP.w - 22.0f,
                                       BOARD_POPUP.y + 4.0f, 18.0f, 18.0f };

static const float GUIDE_SB_X = GUIDE_POPUP.x + GUIDE_POPUP.w - 12.0f;
static const float GUIDE_SB_Y = GUIDE_POPUP.y + 30.0f;
static const float GUIDE_SB_H = GUIDE_POPUP.h - 40.0f;
static const float BOARD_ROW_H = 36.0f;
static const float BOARD_SB_X  = BOARD_POPUP.x + BOARD_POPUP.w - 12.0f;
static const float BOARD_SB_Y  = BOARD_POPUP.y + 60.0f;
static const float BOARD_SB_H  = BOARD_VISIBLE * BOARD_ROW_H;

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

static const char* GUIDE_LINES[] = {
    "HOW TO PLAY",
    "",
    "Movement keys:",
    " - LEFT / A : move left",
    " - RIGHT / D : move right",
    " - UP / W : rotate CCW",
    " - DOWN / S : rotate CW",
    " - SPACE : speed boost x5",
    " - ENTER : pause / resume",
    " - ESC : open quit menu",
    "",
    "Sidebar (12 components):",
    "  1 QUIT  : open quit menu",
    "  2 PAUSE : stop / play",
    "  3 SCORE : current points",
    "  4 TIMER : total play time",
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
    "to make piece drop 5x faster.",
    "",
    "Press X or ESC to close.",
};
static const int GUIDE_LINE_COUNT = (int)(sizeof(GUIDE_LINES)/sizeof(GUIDE_LINES[0]));
static const int GUIDE_VISIBLE_LINES = 24;

static void drawBackground(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    const char* title = "C T E T R I S";
    int titleLen = (int)SDL_strlen(title);
    float titleX = (CONSOLE_SCREEN_WIDTH - titleLen * 8.0f) / 2.0f;
    SDL_RenderDebugText(renderer, titleX, 60.0f, title);
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
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &r);
    SDL_RenderDebugText(renderer, r.x + r.w/2 - 4, r.y + r.h/2 - 4, "X");
}

static void drawGuideLightbox(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 70, 80, 110, 255);
    SDL_RenderFillRect(renderer, &GUIDE_POPUP);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &GUIDE_POPUP);

    const float CONTENT_X = GUIDE_POPUP.x + 8;
    const float CONTENT_Y0 = GUIDE_POPUP.y + 30;
    const float ROW_H = 14.0f;
    const int   MAX_CHARS = 28;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    int rowDrawn = 0;
    for (int i = 0; i < GUIDE_VISIBLE_LINES && rowDrawn < GUIDE_VISIBLE_LINES; i++) {
        int idx = state.guideScroll + i;
        if (idx >= GUIDE_LINE_COUNT) break;
        int used = drawWrappedText(renderer, GUIDE_LINES[idx],
                                   CONTENT_X, CONTENT_Y0 + rowDrawn * ROW_H,
                                   MAX_CHARS, 2);
        rowDrawn += (used > 0) ? used : 1;
    }

    SBLayout sb = layoutSB(GUIDE_SB_X, GUIDE_SB_Y, GUIDE_SB_H,
                           GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES, state.guideScroll);
    drawSB(renderer, sb, GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES,
           state.sb.upHeld, state.sb.downHeld, state.sb.dragging);

    drawCloseButton(renderer, GUIDE_CLOSE);
}

static void drawBoardLightbox(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 50, 100, 70, 255);
    SDL_RenderFillRect(renderer, &BOARD_POPUP);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &BOARD_POPUP);

    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 80, BOARD_POPUP.y + 14, "LEADERBOARD");
    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8, BOARD_POPUP.y + 38,
                        "#  USER         SCORE TIME");

    const float ROW_Y0 = BOARD_SB_Y;
    char line[64];
    for (int i = 0; i < BOARD_VISIBLE; i++) {
        int idx = state.boardScroll + i;
        if (idx >= BOARD_TOTAL) break;
        const BoardEntry& e = BOARD_DATA[idx];
        float y = ROW_Y0 + i * BOARD_ROW_H;
        const char* tShort = e.time;
        int tLen = (int)SDL_strlen(e.time);
        if (tLen >= 5) tShort = e.time + tLen - 5;
        SDL_snprintf(line, sizeof(line), "%2d %-12.12s %5d %s",
                     idx + 1, e.user, e.score, tShort);
        SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8, y, line);
    }

    SBLayout sb = layoutSB(BOARD_SB_X, BOARD_SB_Y, BOARD_SB_H,
                           BOARD_TOTAL, BOARD_VISIBLE, state.boardScroll);
    drawSB(renderer, sb, BOARD_TOTAL, BOARD_VISIBLE,
           state.sb.upHeld, state.sb.downHeld, state.sb.dragging);

    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8,
                        BOARD_POPUP.y + BOARD_POPUP.h - 14,
                        "W/S to scroll, ESC close");

    drawCloseButton(renderer, BOARD_CLOSE);
}

static void playBackgroundMusic() { SDL_Log("Phat nhac nen console"); }

static void activateButton(AppState& state, int index) {
    switch (index) {
        case 0: state.showGuide = true; state.guideScroll = 0;
                sbResetInteraction(state.sb); break;
        case 1: state.showBoard = true; state.boardScroll = 0;
                sbResetInteraction(state.sb); break;
        case 2: state.isRunning = false; state.nextScene = 1; break;
        case 3:
            // QUIT: tren WASM hien shutdown screen thay vi thoat lam canvas trang.
            // Tren native tra ve 0 binh thuong.
#ifdef __EMSCRIPTEN__
            state.wasmShutdown = true;
#else
            state.isRunning = false; state.nextScene = 0;
#endif
            break;
        default: break;
    }
}

int runGameConsole(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    AppState state;
    SDL_Event event;
    playBackgroundMusic();

    while (state.isRunning || state.wasmShutdown) {
        Uint32 nowMs = SDL_GetTicks();

        // ============= WASM shutdown screen =============
        if (state.wasmShutdown) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    state.wasmShutdown = false;
                    state.isRunning = false;
                    state.nextScene = 0;
                    break;
                }
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    state.reloadHover = hitTest(CONSOLE_RELOAD_BTN,
                                                event.motion.x, event.motion.y);
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    if (hitTest(CONSOLE_RELOAD_BTN, event.button.x, event.button.y)) {
#ifdef __EMSCRIPTEN__
                        emscripten_run_script("window.location.reload();");
#endif
                    }
                }
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    SDL_Keycode k = event.key.key;
                    if (k == SDLK_F5 || k == SDLK_RETURN ||
                        k == SDLK_KP_ENTER || k == SDLK_SPACE) {
#ifdef __EMSCRIPTEN__
                        emscripten_run_script("window.location.reload();");
#endif
                    }
                }
            }
            drawConsoleWasmShutdown(renderer, state.reloadHover);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
            continue;
        }

        // ============= Game thuong =============
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.isRunning = false; state.nextScene = 0; break;
            }

            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                if (state.showBoard) {
                    state.boardScroll = clampScroll(state.boardScroll - (int)event.wheel.y,
                                                    BOARD_TOTAL, BOARD_VISIBLE);
                } else if (state.showGuide) {
                    state.guideScroll = clampScroll(state.guideScroll - (int)event.wheel.y,
                                                    GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES);
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
                    if (state.showGuide)      { state.showGuide = false; sbResetInteraction(state.sb); }
                    else if (state.showBoard) { state.showBoard = false; sbResetInteraction(state.sb); }
                    else                      state.focusIndex = 3;
                }
                else if (state.showBoard && (isUp || isDown)) {
                    int delta = isUp ? -1 : 1;
                    state.boardScroll = clampScroll(state.boardScroll + delta,
                                                    BOARD_TOTAL, BOARD_VISIBLE);
                }
                else if (state.showGuide && (isUp || isDown)) {
                    int delta = isUp ? -1 : 1;
                    state.guideScroll = clampScroll(state.guideScroll + delta,
                                                    GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES);
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
                    if (hitTest(GUIDE_CLOSE, mx, my)) {
                        state.showGuide = false;
                        sbResetInteraction(state.sb);
                    } else {
                        SBLayout sb = layoutSB(GUIDE_SB_X, GUIDE_SB_Y, GUIDE_SB_H,
                                               GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES,
                                               state.guideScroll);
                        sbOnMouseDown(state.sb, sb, mx, my, state.guideScroll,
                                      GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES, nowMs);
                    }
                } else if (state.showBoard) {
                    if (hitTest(BOARD_CLOSE, mx, my)) {
                        state.showBoard = false;
                        sbResetInteraction(state.sb);
                    } else {
                        SBLayout sb = layoutSB(BOARD_SB_X, BOARD_SB_Y, BOARD_SB_H,
                                               BOARD_TOTAL, BOARD_VISIBLE,
                                               state.boardScroll);
                        sbOnMouseDown(state.sb, sb, mx, my, state.boardScroll,
                                      BOARD_TOTAL, BOARD_VISIBLE, nowMs);
                    }
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

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (state.sb.dragging) {
                    if (state.showGuide) {
                        SBLayout sb = layoutSB(GUIDE_SB_X, GUIDE_SB_Y, GUIDE_SB_H,
                                               GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES,
                                               state.guideScroll);
                        sbOnMouseMotion(state.sb, sb, event.motion.y,
                                        state.guideScroll, GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES);
                    } else if (state.showBoard) {
                        SBLayout sb = layoutSB(BOARD_SB_X, BOARD_SB_Y, BOARD_SB_H,
                                               BOARD_TOTAL, BOARD_VISIBLE,
                                               state.boardScroll);
                        sbOnMouseMotion(state.sb, sb, event.motion.y,
                                        state.boardScroll, BOARD_TOTAL, BOARD_VISIBLE);
                    }
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                event.button.button == SDL_BUTTON_LEFT) {
                sbResetInteraction(state.sb);
            }
        }

        if (state.showGuide && (state.sb.upHeld || state.sb.downHeld)) {
            sbAutoRepeat(state.sb, state.guideScroll,
                         GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES, nowMs);
        }
        if (state.showBoard && (state.sb.upHeld || state.sb.downHeld)) {
            sbAutoRepeat(state.sb, state.boardScroll,
                         BOARD_TOTAL, BOARD_VISIBLE, nowMs);
        }

        drawBackground(renderer);
        if (!state.showGuide && !state.showBoard) {
            for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
                drawButton(renderer, MAIN_BUTTONS[i], i == state.focusIndex);
        }
        if (state.showGuide) drawGuideLightbox(renderer, state);
        if (state.showBoard) drawBoardLightbox(renderer, state);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return state.nextScene;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Console \xC2\xA9 - Standalone",
                                          CONSOLE_SCREEN_WIDTH, CONSOLE_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameConsole(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif