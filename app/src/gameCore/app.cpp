// integration/v1
// gameCore: ban co 240x480 + sidebar 30x480 voi 12 component dong nhat 30x40
#include "gameCore_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Tren WASM build, can goi window.location.reload() khi user click "Reload"
// o man hinh shutdown. Su dung emscripten_run_script de chen JS.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

const SDL_Color COLORS[] = {
    {  0,   0,   0, 255},
    {  0,   0, 255, 255},
    {255,   0,   0, 255},
    {  0, 255,   0, 255},
    {255, 255,   0, 255},
    {255, 165,   0, 255},
    {128,   0, 128, 255}
};

// Mau thong nhat cho hieu ung "active" (chuot giu / phim giu)
static const SDL_Color HIGHLIGHT_YELLOW = {255, 215, 0, 255};
// Mau text "nhe" -- dung 220 thay vi 255 de font xuat hien mong/it dam hon.
// (SDL_RenderDebugText la bitmap font co dinh, khong the doi font weight,
//  nen dung mau xam nhat de tao hieu ung "less thickness" thi giac.)
static const SDL_Color SOFT_WHITE = {220, 220, 220, 255};

// gamecore-tao-cac-khoi-xep-hinh-LITZO-01
// 5 hinh khoi co ban: L, I, Z, O, T (loai bo S so voi v0).
// Khi spawn, tung hinh se duoc ngau nhien lat guong (mirror) theo truc Y
// -> mo rong bien the hinh khoi ma khong can them shape data.
const Point SHAPE_L[4] = {{0,0}, {0,1}, {0,2}, {1,2}};
const Point SHAPE_I[4] = {{0,0}, {1,0}, {2,0}, {3,0}};
const Point SHAPE_Z[4] = {{0,0}, {1,0}, {1,1}, {2,1}};
const Point SHAPE_O[4] = {{0,0}, {1,0}, {0,1}, {1,1}};
const Point SHAPE_T[4] = {{0,0}, {1,0}, {2,0}, {1,1}};
const Point* const SHAPES[] = { SHAPE_L, SHAPE_I, SHAPE_Z, SHAPE_O, SHAPE_T };
const int NUM_SHAPES = (int)(sizeof(SHAPES) / sizeof(SHAPES[0]));   // = 5

// gamecore-do-mau-02
// Spawn 1 khoi: chon ngau nhien 1 trong 5 shape, lat guong ngau nhien (50%),
// roi do mau ngau nhien.
static Tetromino spawnBlock() {
    Tetromino t;
    int idx = std::rand() % NUM_SHAPES;
    for (int i = 0; i < 4; i++) t.blocks[i] = SHAPES[idx][i];

    // Lat guong (mirror flip) ngau nhien voi xac suat 50%.
    // Mirror = phan chieu theo truc doc: x_moi = (max_x - x_cu).
    // Nhu vay khoi van nam tron trong vung [0..max_x] sau khi flip.
    if ((std::rand() & 1) == 0) {
        int maxX = 0;
        for (int i = 0; i < 4; i++) {
            if (t.blocks[i].x > maxX) maxX = t.blocks[i].x;
        }
        for (int i = 0; i < 4; i++) {
            t.blocks[i].x = maxX - t.blocks[i].x;
        }
    }

    t.colorID = std::rand() % 6 + 1;
    t.x = BOARD_COLS / 2 - 1;
    t.y = 0;
    return t;
}

static bool checkCollision(const GameState& state, const Tetromino& t) {
    for (int i = 0; i < 4; i++) {
        int nx = t.x + t.blocks[i].x;
        int ny = t.y + t.blocks[i].y;
        if (nx < 0 || nx >= BOARD_COLS || ny >= BOARD_ROWS) return true;
        if (ny >= 0 && state.board[ny][nx] != 0) return true;
    }
    return false;
}

static Tetromino rotated(const Tetromino& t, bool clockwise) {
    Tetromino r = t;
    const int px = 1, py = 1;
    for (int i = 0; i < 4; i++) {
        int relX = t.blocks[i].x - px;
        int relY = t.blocks[i].y - py;
        if (clockwise) {
            r.blocks[i].x = -relY + px;
            r.blocks[i].y =  relX + py;
        } else {
            r.blocks[i].x =  relY + px;
            r.blocks[i].y = -relX + py;
        }
    }
    return r;
}

// gamecore-xu-ly-phim-04
static void applyMoveOrRotate(GameState& state, SDL_Keycode key) {
    Tetromino nextT = state.currentBlock;
    switch (key) {
        case SDLK_LEFT:
        case SDLK_A: nextT.x -= 1; break;
        case SDLK_RIGHT:
        case SDLK_D: nextT.x += 1; break;
        case SDLK_UP:
        case SDLK_W: nextT = rotated(state.currentBlock, false); break;
        case SDLK_DOWN:
        case SDLK_S: nextT = rotated(state.currentBlock, true);  break;
        default: return;
    }
    if (!checkCollision(state, nextT)) state.currentBlock = nextT;
}

// gamecore-xu-ly-xoa-dong-06
// gamecore-tinh-diem-07
static void clearLines(GameState& state) {
    int linesCleared = 0;
    for (int r = BOARD_ROWS - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < BOARD_COLS; c++) {
            if (state.board[r][c] == 0) { full = false; break; }
        }
        if (full) {
            linesCleared++;
            for (int y = r; y > 0; y--) {
                for (int x = 0; x < BOARD_COLS; x++)
                    state.board[y][x] = state.board[y - 1][x];
            }
            r++;
        }
    }
    // Quy tac: moi dong xoa = 1 diem. Score gioi han 5 chu so (max = 99999)
    // -- giam tu 6 chu so de fit slot 30x40 thoai mai hon.
    state.score += linesCleared;
    if (state.score > 99999) state.score = 99999;
}

static void resetGame(GameState& state) {
    for (int r = 0; r < BOARD_ROWS; r++)
        for (int c = 0; c < BOARD_COLS; c++)
            state.board[r][c] = 0;
    state.score = 0;
    state.isGameOver = false;
    state.isPaused = false;
    state.showQuitPopup = false;
    state.softDrop = false;
    state.speedHeld = false;
    state.currentBlock = spawnBlock();
    state.nextBlock    = spawnBlock();

    // Reset cac field timer va trang thai chuot giu khi restart
    state.gameStartTime  = SDL_GetTicks();
    state.pauseStartTime = 0;
    state.totalPausedMs  = 0;
    state.wasRunning     = true;
    state.mouseHeldQuit     = false;
    state.mouseHeldPause    = false;
    state.mouseHeldArrUp    = false;
    state.mouseHeldArrDown  = false;
    state.mouseHeldArrLeft  = false;
    state.mouseHeldArrRight = false;
    // Khong reset wasmShutdown -- neu da o man hinh shutdown thi giu nguyen
}

// gamecore-xu-ly-cham-05
static void lockBlock(GameState& state) {
    for (int i = 0; i < 4; i++) {
        int nx = state.currentBlock.x + state.currentBlock.blocks[i].x;
        int ny = state.currentBlock.y + state.currentBlock.blocks[i].y;
        if (ny >= 0 && ny < BOARD_ROWS && nx >= 0 && nx < BOARD_COLS) {
            state.board[ny][nx] = state.currentBlock.colorID;
        }
    }
    clearLines(state);
    state.currentBlock = state.nextBlock;
    state.currentBlock.x = BOARD_COLS / 2 - 1;
    state.currentBlock.y = 0;
    state.nextBlock = spawnBlock();
    if (checkCollision(state, state.currentBlock)) {
        state.isGameOver    = true;
        state.isPaused      = true;
        state.showQuitPopup = true;
    }
}

// =========================================================
// Sidebar 12 component dong nhat 30x40
// =========================================================
static SDL_FRect rectAt(int slotIndex) {
    return { (float)SIDEBAR_X, (float)(slotIndex * COMP_H), 30.0f, (float)COMP_H };
}

static const SDL_FRect RECT_QUIT      = rectAt(0);
static const SDL_FRect RECT_PAUSE     = rectAt(1);
static const SDL_FRect RECT_SCORE     = rectAt(2);
// Slot 3: TIMER hien thi tong thoi gian choi HH:MM (khong dem khi pause)
static const SDL_FRect RECT_TIMER     = rectAt(3);
static const SDL_FRect RECT_NEXT1     = rectAt(4);
static const SDL_FRect RECT_NEXT2     = rectAt(5);
static const SDL_FRect RECT_NEXT3     = rectAt(6);
static const SDL_FRect RECT_ARR_UP    = rectAt(7);
static const SDL_FRect RECT_ARR_DOWN  = rectAt(8);
static const SDL_FRect RECT_ARR_LEFT  = rectAt(9);
static const SDL_FRect RECT_ARR_RIGHT = rectAt(10);
static const SDL_FRect RECT_SPEED_BTN = rectAt(11);

// Popup quit
static const SDL_FRect POPUP_BG     = { 20.0f,  70.0f, 230.0f, 340.0f };
static const SDL_FRect POPUP_CLOSE  = { POPUP_BG.x + POPUP_BG.w - 24.0f, POPUP_BG.y + 6.0f, 18.0f, 18.0f };
static const float BTN_W = 180.0f, BTN_H = 36.0f, BTN_GAP = 10.0f;
static const float BTN_X = POPUP_BG.x + (POPUP_BG.w - BTN_W) / 2.0f;
static const float BTN_Y0 = POPUP_BG.y + 130.0f;
static const SDL_FRect POPUP_RESTART = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 0, BTN_W, BTN_H };
static const SDL_FRect POPUP_CONSOLE = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 1, BTN_W, BTN_H };
static const SDL_FRect POPUP_QUIT    = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 2, BTN_W, BTN_H };
static const SDL_FRect POPUP_CANCEL  = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 3, BTN_W, BTN_H };

// Reload button cho man hinh WASM shutdown: nut to giua man hinh
static const SDL_FRect RELOAD_BTN = {
    (CORE_SCREEN_WIDTH  - 160.0f) / 2.0f,
    (CORE_SCREEN_HEIGHT - 50.0f)  / 2.0f,
    160.0f, 50.0f
};

static bool hitTest(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// ---------- Helper: ve text co thu nho theo scale (de fit chu vao slot 30x40) ----------
// Dung SDL_SetRenderScale tam thoi de scale font 8x8 mac dinh xuong nho hon.
// Quy uoc: x, y la toa do MAN HINH (sau khi scale ra). Khi scale != 1.0, ta phai chia.
static void drawSmallText(SDL_Renderer* renderer, float x, float y, float scale, const char* text) {
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_RenderDebugText(renderer, x / scale, y / scale, text);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

// ---------- Icon helpers ----------

// Power icon: thu nho ve kich thuoc tuong duong pause icon (~12px)
// thay vi 18px nhu truoc. Cung chu vong tron (cung ho ~300 do) + thanh dung.
static void drawPowerIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool active) {
    SDL_Color c = active ? HIGHLIGHT_YELLOW : SOFT_WHITE;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2 + 1.0f;
    const float radius = 5.5f;  // nho hon ~30%, gan bang pause sq 12x12

    // Cung tron tu -60 do (top-right) den 240 do (top-left) clockwise,
    // chua khe ho ~60 do o dinh cho thanh dung
    for (int deg = -60; deg <= 240; deg += 4) {
        float rad = (float)deg * 3.14159265f / 180.0f;
        float px = cx + radius * std::cos(rad);
        float py = cy + radius * std::sin(rad);
        SDL_FRect dot = { px - 0.75f, py - 0.75f, 1.5f, 1.5f };
        SDL_RenderFillRect(renderer, &dot);
    }
    // Thanh dung: ngan hon, di tu top dinh xuong tam vong tron
    SDL_FRect bar = { cx - 0.75f, cy - 8.5f, 1.5f, 6.0f };
    SDL_RenderFillRect(renderer, &bar);
}

static void drawPauseIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool isPaused, bool active) {
    SDL_Color c = active ? HIGHLIGHT_YELLOW : SOFT_WHITE;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    if (!isPaused) {
        // Khi dang chay -> hien icon STOP (hinh vuong)
        SDL_FRect sq = { cx - 6, cy - 6, 12, 12 };
        SDL_RenderFillRect(renderer, &sq);
    } else {
        // Khi dang pause -> hien icon PLAY (tam giac trai sang phai)
        for (int i = 0; i < 12; i++) {
            float w = (i < 6) ? (i * 1.6f + 2) : ((11 - i) * 1.6f + 2);
            SDL_FRect ln = { cx - 5, cy - 6 + i, w, 1.0f };
            SDL_RenderFillRect(renderer, &ln);
        }
    }
}

// Mui ten dang net thang co 2 vet cheo o dau.
// dir: 0 = up, 1 = down, 2 = left, 3 = right
static void drawArrowIcon(SDL_Renderer* renderer, const SDL_FRect& host, int dir, bool active) {
    SDL_Color c = active ? HIGHLIGHT_YELLOW : SOFT_WHITE;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    const float L = 14.0f;
    const float HEAD = 5.0f;

    if (dir == 0 || dir == 1) {
        SDL_FRect shaft = { cx - 1, cy - L/2, 2, L };
        SDL_RenderFillRect(renderer, &shaft);
        for (int i = 0; i < (int)HEAD; i++) {
            float dy = (dir == 0) ? (cy - L/2 + i) : (cy + L/2 - i - 1);
            SDL_FRect pL = { cx - 1 - i, dy, 2, 1 };
            SDL_FRect pR = { cx + i,     dy, 2, 1 };
            SDL_RenderFillRect(renderer, &pL);
            SDL_RenderFillRect(renderer, &pR);
        }
    } else {
        SDL_FRect shaft = { cx - L/2, cy - 1, L, 2 };
        SDL_RenderFillRect(renderer, &shaft);
        for (int i = 0; i < (int)HEAD; i++) {
            float dx = (dir == 2) ? (cx - L/2 + i) : (cx + L/2 - i - 1);
            SDL_FRect pT = { dx, cy - 1 - i, 1, 2 };
            SDL_FRect pB = { dx, cy + i,     1, 2 };
            SDL_RenderFillRect(renderer, &pT);
            SDL_RenderFillRect(renderer, &pB);
        }
    }
}

static void drawSpeedBoosterIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool active) {
    SDL_Color c = active ? HIGHLIGHT_YELLOW : SOFT_WHITE;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;

    auto drawChevron = [&](float tipX) {
        for (int i = 0; i < 6; i++) {
            SDL_FRect pTop = { tipX - 6 + i, cy - 6 + i, 2, 2 };
            SDL_FRect pBot = { tipX - 6 + i, cy + 5 - i, 2, 2 };
            SDL_RenderFillRect(renderer, &pTop);
            SDL_RenderFillRect(renderer, &pBot);
        }
    };
    drawChevron(cx - 1);
    drawChevron(cx + 7);
}

// Preview piece thu nho trong slot voi STROKE RO RANG giua cac o.
// V1 dung CELL=4 va gap=0.5 -> stroke qua mong, hinh I co the thay nhung
// hinh O/T/Z/L thi cac o sat nhau. V2 tang CELL=5 va gap=1.5 de stroke
// xuat hien ro o moi shape (gap >=1px duoc rasterize chac chan).
static void drawNextPreview(SDL_Renderer* renderer, const SDL_FRect& slot, const Tetromino& t) {
    SDL_Color c = COLORS[t.colorID];
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);

    const float CELL = 5.0f;       // tang tu 4 -> 5 px
    const float CELL_GAP = 1.5f;   // tang khoang ho de stroke ro o moi shape
    const float CELL_INNER = CELL - CELL_GAP;  // 3.5 px ve thuc te

    int minx = 99, miny = 99, maxx = -1, maxy = -1;
    for (int i = 0; i < 4; i++) {
        if (t.blocks[i].x < minx) minx = t.blocks[i].x;
        if (t.blocks[i].y < miny) miny = t.blocks[i].y;
        if (t.blocks[i].x > maxx) maxx = t.blocks[i].x;
        if (t.blocks[i].y > maxy) maxy = t.blocks[i].y;
    }
    float pieceW = (maxx - minx + 1) * CELL;
    float pieceH = (maxy - miny + 1) * CELL;
    float ox = slot.x + (slot.w - pieceW) / 2 - minx * CELL;
    float oy = slot.y + (slot.h - pieceH) / 2 - miny * CELL;
    for (int i = 0; i < 4; i++) {
        SDL_FRect cell = { ox + t.blocks[i].x * CELL,
                           oy + t.blocks[i].y * CELL,
                           CELL_INNER, CELL_INNER };
        SDL_RenderFillRect(renderer, &cell);
    }
}

// Score: format "00000" 5 chu so. Font scale = 0.65 (TUONG TU TIMER) de
// 2 chi so co kich thuoc dong nhat -- truoc kia score dung 0.6 (~24px)
// nhin nho hon timer 0.65 (~26px), gay cam giac chu kho doc.
static void drawScoreInSlot(SDL_Renderer* renderer, const SDL_FRect& host, int score) {
    char buf[12];
    SDL_snprintf(buf, sizeof(buf), "%05d", score);
    const float SCALE = 0.65f;          // <- match TIMER scale
    float textW = 5 * 8.0f * SCALE;     // ~26 px
    float textH = 8.0f * SCALE;          // ~5.2 px
    float x = host.x + (host.w - textW) / 2.0f;
    float y = host.y + (host.h - textH) / 2.0f;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    drawSmallText(renderer, x, y, SCALE, buf);
}

// Timer: format "HH:MM" 1 dong (bo SS de gon hon trong slot 30x40)
static void drawTimerInSlot(SDL_Renderer* renderer, const SDL_FRect& host, Uint32 elapsedMs) {
    Uint32 totalMin = elapsedMs / 60000;
    int hours = (int)(totalMin / 60);
    int mins  = (int)(totalMin % 60);
    if (hours > 99) hours = 99;
    char buf[8];
    SDL_snprintf(buf, sizeof(buf), "%02d:%02d", hours, mins);
    // 5 ky tu * 8 px * SCALE = textW. Chon 0.65 -> ~26 px (fit 30)
    const float SCALE = 0.65f;
    float textW = 5 * 8.0f * SCALE;
    float textH = 8.0f * SCALE;
    float x = host.x + (host.w - textW) / 2.0f;
    float y = host.y + (host.h - textH) / 2.0f;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    drawSmallText(renderer, x, y, SCALE, buf);
}

// drawSidebar nhan them keys + elapsed de tinh hover/active va render timer
static void drawSidebar(SDL_Renderer* renderer, const GameState& state,
                        const bool* keys, Uint32 elapsedMs) {
    // Nen sidebar
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect bg = { (float)SIDEBAR_X, 0, (float)SIDEBAR_W, (float)CORE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bg);

    // Trang thai active = chuot giu HOAC phim tuong ung dang giu
    bool aQuit  = state.mouseHeldQuit  || (keys && keys[SDL_SCANCODE_ESCAPE]);
    bool aPause = state.mouseHeldPause || (keys && (keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_KP_ENTER]));
    bool aUp    = state.mouseHeldArrUp    || (keys && (keys[SDL_SCANCODE_UP]    || keys[SDL_SCANCODE_W]));
    bool aDown  = state.mouseHeldArrDown  || (keys && (keys[SDL_SCANCODE_DOWN]  || keys[SDL_SCANCODE_S]));
    bool aLeft  = state.mouseHeldArrLeft  || (keys && (keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A]));
    bool aRight = state.mouseHeldArrRight || (keys && (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]));
    bool aSpeed = state.speedHeld         || (keys && keys[SDL_SCANCODE_SPACE]);

    drawPowerIcon(renderer, RECT_QUIT, aQuit);
    drawPauseIcon(renderer, RECT_PAUSE, state.isPaused, aPause);

    // Score (5 digits) & Timer hien thi voi font thu nho
    drawScoreInSlot(renderer, RECT_SCORE, state.score);
    drawTimerInSlot(renderer, RECT_TIMER, elapsedMs);

    // Next 1/2/3 - v1 chi co next1
    drawNextPreview(renderer, RECT_NEXT1, state.nextBlock);

    // 4 mui ten + speed booster: hieu ung mau vang khi giu
    drawArrowIcon(renderer, RECT_ARR_UP,    0, aUp);
    drawArrowIcon(renderer, RECT_ARR_DOWN,  1, aDown);
    drawArrowIcon(renderer, RECT_ARR_LEFT,  2, aLeft);
    drawArrowIcon(renderer, RECT_ARR_RIGHT, 3, aRight);
    drawSpeedBoosterIcon(renderer, RECT_SPEED_BTN, aSpeed);
}

static void drawPopupButton(SDL_Renderer* renderer, const SDL_FRect& r,
                            const char* label, SDL_Color bg) {
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &r);
    int ll = (int)SDL_strlen(label);
    SDL_RenderDebugText(renderer,
                        r.x + (r.w - ll * 8.0f) / 2.0f,
                        r.y + (r.h - 8.0f) / 2.0f, label);
}

// Helper: ngat text dai theo so ky tu / dong (font 8px/ky tu)
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

// gamecore-popup-quit-08
// THAY DOI: Bo sung tham so elapsedMs de hien thi "Total time: HH:MM:SS"
// trong popup. Truoc kia popup chi co score, nay them dong total time.
static void drawQuitPopup(SDL_Renderer* renderer, const GameState& state, Uint32 elapsedMs) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_FRect screen = { 0, 0, (float)CORE_SCREEN_WIDTH, (float)CORE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
    SDL_RenderFillRect(renderer, &POPUP_BG);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &POPUP_BG);

    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &POPUP_CLOSE);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &POPUP_CLOSE);
    SDL_RenderDebugText(renderer, POPUP_CLOSE.x + 5, POPUP_CLOSE.y + 5, "X");

    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer, POPUP_BG.x + 15, POPUP_BG.y + 30,
                        state.isGameOver ? "GAME OVER" : "PAUSED");
    drawWrappedText(renderer, "What do you want to do?",
                    POPUP_BG.x + 15, POPUP_BG.y + 60, 25, 2);

    // Dong "Current score: N" -- giu nguyen
    char scoreLine[40];
    SDL_snprintf(scoreLine, sizeof(scoreLine), "Current score: %d", state.score);
    drawWrappedText(renderer, scoreLine,
                    POPUP_BG.x + 15, POPUP_BG.y + 95, 25, 2);

    // BO SUNG: dong "Total time: HH:MM:SS" o duoi score, format chuan
    // chia chinh xac giay/phut/gio tu elapsedMs (loai tru thoi gian pause)
    Uint32 totalSec = elapsedMs / 1000;
    int hours = (int)(totalSec / 3600);
    int mins  = (int)((totalSec % 3600) / 60);
    int secs  = (int)(totalSec % 60);
    if (hours > 99) hours = 99;
    char timeLine[40];
    SDL_snprintf(timeLine, sizeof(timeLine), "Total time: %02d:%02d:%02d",
                 hours, mins, secs);
    drawWrappedText(renderer, timeLine,
                    POPUP_BG.x + 15, POPUP_BG.y + 110, 25, 2);

    drawPopupButton(renderer, POPUP_RESTART, "Restart (new game)",     { 70, 130,  90, 255});
    drawPopupButton(renderer, POPUP_CONSOLE, "Console (back to menu)", { 70, 100, 160, 255});
    drawPopupButton(renderer, POPUP_QUIT,    "Quit (exit app)",        {180,  60,  60, 255});
    drawPopupButton(renderer, POPUP_CANCEL,  "Cancel (close popup)",   {100, 100, 100, 255});
}

// Man hinh shutdown cho WASM: thay vi exit() (khong work tot trong browser),
// hien 1 nut RELOAD duy nhat tai trung tam man hinh. Click nut nay tuong
// duong voi F5 / refresh button cua browser.
static void drawWasmShutdownScreen(SDL_Renderer* renderer, const GameState& state) {
    // Background den toan man hinh -- "tat man hinh" theo nghia game
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Nut Reload: hover -> mau sang hon, default -> mau xanh duong dam
    SDL_Color btnBg = state.reloadHover ? SDL_Color{ 90, 130, 200, 255}
                                        : SDL_Color{ 60, 100, 170, 255};
    SDL_SetRenderDrawColor(renderer, btnBg.r, btnBg.g, btnBg.b, 255);
    SDL_RenderFillRect(renderer, &RELOAD_BTN);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &RELOAD_BTN);

    // Label "RELOAD" can giua nut
    const char* label = "RELOAD";
    int ll = (int)SDL_strlen(label);
    SDL_RenderDebugText(renderer,
                        RELOAD_BTN.x + (RELOAD_BTN.w - ll * 8.0f) / 2.0f,
                        RELOAD_BTN.y + (RELOAD_BTN.h - 8.0f) / 2.0f,
                        label);

    // Hint duoi nut: bo ky tu copyright vi SDL_RenderDebugText la bitmap
    // ASCII font (0x20-0x7F), khong render duoc U+00A9 va "(C)" trong
    // ngoac don nhin xau. Thay bang text don gian, ro nghia.
    const char* hint = "cTetris -- press F5 or click RELOAD";
    int hl = (int)SDL_strlen(hint);
    SDL_RenderDebugText(renderer,
                        (CORE_SCREEN_WIDTH - hl * 8.0f) / 2.0f,
                        RELOAD_BTN.y + RELOAD_BTN.h + 16.0f,
                        hint);
}

static void renderBoard(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect boardBg = { 0, 0, (float)BOARD_W, (float)BOARD_H };
    SDL_RenderFillRect(renderer, &boardBg);

    auto drawCell = [&](int gx, int gy, SDL_Color c) {
        SDL_FRect inner = { (float)(gx * BLOCK_SIZE + BLOCK_PAD),
                            (float)(gy * BLOCK_SIZE + BLOCK_PAD),
                            (float)(BLOCK_SIZE - 2 * BLOCK_PAD),
                            (float)(BLOCK_SIZE - 2 * BLOCK_PAD) };
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderFillRect(renderer, &inner);
    };

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            if (state.board[r][c] != 0) drawCell(c, r, COLORS[state.board[r][c]]);
        }
    }
    for (int i = 0; i < 4; i++) {
        int nx = state.currentBlock.x + state.currentBlock.blocks[i].x;
        int ny = state.currentBlock.y + state.currentBlock.blocks[i].y;
        drawCell(nx, ny, COLORS[state.currentBlock.colorID]);
    }
}

static void openQuitPopup(GameState& state) {
    state.showQuitPopup = true;
    state.isPaused      = true;
}

static void togglePause(GameState& state) {
    if (state.isGameOver) {
        openQuitPopup(state);
        return;
    }
    state.isPaused = !state.isPaused;
}

// gamecore-soft-drop-09
// gamecore-pause-quit-buttons-10
static void onAction(GameState& state, SDL_Keycode keyEquiv) {
    if (state.showQuitPopup || state.isPaused || state.isGameOver) return;
    if (keyEquiv == SDLK_SPACE) { state.softDrop = true; return; }
    applyMoveOrRotate(state, keyEquiv);
}

// Helper: handle Quit -> tren WASM = vao shutdown screen, native = exit code 0
static void handleQuitAction(GameState& state) {
#ifdef __EMSCRIPTEN__
    // WASM: vao man hinh shutdown thay vi tat tien trinh
    state.wasmShutdown = true;
    state.showQuitPopup = false;
    state.isPaused = true;
#else
    // Native: tra exit code 0 (main loop se goto END)
    state.exitCode = 0;
    state.wasmShutdown = false;  // dummy field set, exitCode quan trong hon
#endif
}

// gamecore-xu-ly-roi-03
int runGameCore(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    std::srand((unsigned)std::time(nullptr));

    GameState state;
    state.currentBlock = spawnBlock();
    state.nextBlock    = spawnBlock();

    SDL_Event event;
    Uint32 lastFallTime = SDL_GetTicks();
    const Uint32 FALL_INTERVAL_NORMAL = 500;
    // Speed booster: x5 (truoc x3). 500 / 5 = 100 ms/step.
    const Uint32 FALL_INTERVAL_FAST   = 500 / 5;

    bool quitRequested = false;   // co flag ket thuc loop voi exitCode

    while (true) {
        Uint32 nowMs = SDL_GetTicks();

        // Khoi tao timer o frame dau tien
        if (state.gameStartTime == 0) {
            state.gameStartTime = nowMs;
            state.wasRunning    = true;
        }

        state.softDrop = false;

        while (SDL_PollEvent(&event)) {
            // ============= WASM shutdown screen: chi xu ly su kien lien quan =============
            if (state.wasmShutdown) {
                if (event.type == SDL_EVENT_QUIT) {
                    // Browser dong tab -> ket thuc
                    state.exitCode = 0;
                    quitRequested = true;
                    break;
                }
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    state.reloadHover = hitTest(RELOAD_BTN, event.motion.x, event.motion.y);
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    if (hitTest(RELOAD_BTN, event.button.x, event.button.y)) {
#ifdef __EMSCRIPTEN__
                        // Goi window.location.reload() qua emscripten JS bridge
                        emscripten_run_script("window.location.reload();");
#endif
                    }
                }
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    // Cho phep F5 / Enter / Space cung trigger reload
                    SDL_Keycode k = event.key.key;
                    if (k == SDLK_F5 || k == SDLK_RETURN ||
                        k == SDLK_KP_ENTER || k == SDLK_SPACE) {
#ifdef __EMSCRIPTEN__
                        emscripten_run_script("window.location.reload();");
#endif
                    }
                }
                continue;  // bo qua moi su kien khac khi shutdown
            }

            // ============= Game thuong =============
            if (event.type == SDL_EVENT_QUIT) {
                state.exitCode = 0;
                quitRequested = true;
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                if (key == SDLK_ESCAPE) {
                    if (!state.showQuitPopup) openQuitPopup(state);
                } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    if (!state.showQuitPopup) togglePause(state);
                } else {
                    onAction(state, key);
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT) {
                float mx = event.button.x;
                float my = event.button.y;

                if (state.showQuitPopup) {
                    if (hitTest(POPUP_CLOSE, mx, my) || hitTest(POPUP_CANCEL, mx, my)) {
                        state.showQuitPopup = false;
                        // giu pause
                    } else if (hitTest(POPUP_RESTART, mx, my)) {
                        resetGame(state);
                        lastFallTime = SDL_GetTicks();
                    } else if (hitTest(POPUP_CONSOLE, mx, my)) {
                        state.exitCode = 2;
                        quitRequested = true;
                        break;
                    } else if (hitTest(POPUP_QUIT, mx, my)) {
                        // Quit: WASM -> shutdown screen, Native -> exit
                        handleQuitAction(state);
                        if (!state.wasmShutdown) {
                            quitRequested = true;
                            break;
                        }
                    }
                } else {
                    // Ghi nhan trang thai chuot dang giu de bat hieu ung mau vang
                    if (hitTest(RECT_QUIT, mx, my)) {
                        state.mouseHeldQuit = true;
                        openQuitPopup(state);
                    } else if (hitTest(RECT_PAUSE, mx, my)) {
                        state.mouseHeldPause = true;
                        togglePause(state);
                    } else if (hitTest(RECT_ARR_UP, mx, my)) {
                        state.mouseHeldArrUp = true;
                        onAction(state, SDLK_UP);
                    } else if (hitTest(RECT_ARR_DOWN, mx, my)) {
                        state.mouseHeldArrDown = true;
                        onAction(state, SDLK_DOWN);
                    } else if (hitTest(RECT_ARR_LEFT, mx, my)) {
                        state.mouseHeldArrLeft = true;
                        onAction(state, SDLK_LEFT);
                    } else if (hitTest(RECT_ARR_RIGHT, mx, my)) {
                        state.mouseHeldArrRight = true;
                        onAction(state, SDLK_RIGHT);
                    } else if (hitTest(RECT_SPEED_BTN, mx, my)) {
                        state.speedHeld = true;
                    }
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                event.button.button == SDL_BUTTON_LEFT) {
                // Tha chuot -> reset toan bo trang thai chuot dang giu
                state.speedHeld         = false;
                state.mouseHeldQuit     = false;
                state.mouseHeldPause    = false;
                state.mouseHeldArrUp    = false;
                state.mouseHeldArrDown  = false;
                state.mouseHeldArrLeft  = false;
                state.mouseHeldArrRight = false;
            }
        }

        if (quitRequested) break;

        // ============= WASM shutdown render branch =============
        if (state.wasmShutdown) {
            drawWasmShutdownScreen(renderer, state);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
            continue;  // bo qua hoan toan game logic
        }

        // SPACE giu HOAC giu chuot tren nut booster -> soft drop
        const bool* keys = SDL_GetKeyboardState(NULL);
        bool active = !state.isPaused && !state.showQuitPopup && !state.isGameOver;
        if (active && (keys[SDL_SCANCODE_SPACE] || state.speedHeld)) {
            state.softDrop = true;
        }

        // Phat hien transition pause/resume de tinh totalPausedMs cho timer.
        // Yeu cau: TIMER KHONG DEM khi pause/quit-popup/game-over.
        // Logic: moc transition running->paused luu vao pauseStartTime,
        //        moc transition paused->running cong delta vao totalPausedMs.
        bool nowRunning = active;
        if (state.wasRunning && !nowRunning) {
            state.pauseStartTime = nowMs;
        }
        if (!state.wasRunning && nowRunning) {
            state.totalPausedMs += nowMs - state.pauseStartTime;
        }
        state.wasRunning = nowRunning;

        // Tinh thoi gian da choi (loai tru thoi gian pause)
        Uint32 elapsedMs;
        if (nowRunning) {
            // Dang chay: lay thoi gian thuc - moc start - tong thoi gian pause
            elapsedMs = nowMs - state.gameStartTime - state.totalPausedMs;
        } else {
            // Dang pause: dong bang tai pauseStartTime -> timer khong tang
            elapsedMs = state.pauseStartTime - state.gameStartTime - state.totalPausedMs;
        }

        // Logic roi
        if (active) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 interval = state.softDrop ? FALL_INTERVAL_FAST : FALL_INTERVAL_NORMAL;
            if (currentTime - lastFallTime > interval) {
                Tetromino nextT = state.currentBlock;
                nextT.y += 1;
                if (checkCollision(state, nextT)) {
                    lockBlock(state);
                } else {
                    state.currentBlock = nextT;
                }
                lastFallTime = currentTime;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        renderBoard(renderer, state);
        drawSidebar(renderer, state, keys, elapsedMs);
        if (state.showQuitPopup) drawQuitPopup(renderer, state, elapsedMs);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return state.exitCode;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    // Standalone window title: them suffix copyright (UTF-8: \xC2\xA9 = ©)
    SDL_Window* window = SDL_CreateWindow("Game Core \xC2\xA9 - Standalone",
                                          CORE_SCREEN_WIDTH, CORE_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameCore(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif