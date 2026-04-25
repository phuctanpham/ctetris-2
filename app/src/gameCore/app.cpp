// integration/v1
// File chinh cua gameCore: vong lap chinh + sidebar 30px voi 7 component
#include "gameCore_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <ctime>

// Bang mau theo colorID (1..6); index 0 la o trong
const SDL_Color COLORS[] = {
    {  0,   0,   0, 255},
    {  0,   0, 255, 255},
    {255,   0,   0, 255},
    {  0, 255,   0, 255},
    {255, 255,   0, 255},
    {255, 165,   0, 255},
    {128,   0, 128, 255}
};

// gamecore-tao-cac-khoi-xep-hinh-LITZO-01
const Point SHAPE_L[4] = {{0,0}, {0,1}, {0,2}, {1,2}};
const Point SHAPE_I[4] = {{0,0}, {1,0}, {2,0}, {3,0}};
const Point SHAPE_T[4] = {{0,0}, {1,0}, {2,0}, {1,1}};
const Point SHAPE_S[4] = {{1,0}, {2,0}, {0,1}, {1,1}};
const Point SHAPE_Z[4] = {{0,0}, {1,0}, {1,1}, {2,1}};
const Point SHAPE_O[4] = {{0,0}, {1,0}, {0,1}, {1,1}};

const Point* const SHAPES[] = { SHAPE_L, SHAPE_I, SHAPE_T, SHAPE_S, SHAPE_Z, SHAPE_O };
const int NUM_SHAPES = (int)(sizeof(SHAPES) / sizeof(SHAPES[0]));

// gamecore-do-mau-02
static Tetromino spawnBlock() {
    Tetromino t;
    int idx = std::rand() % NUM_SHAPES;
    for (int i = 0; i < 4; i++) t.blocks[i] = SHAPES[idx][i];
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
    state.score += linesCleared * 100;
}

// Reset toan bo trang thai bat dau choi lai voi diem 0
static void resetGame(GameState& state) {
    for (int r = 0; r < BOARD_ROWS; r++)
        for (int c = 0; c < BOARD_COLS; c++)
            state.board[r][c] = 0;
    state.score = 0;
    state.isGameOver = false;
    state.isPaused = false;
    state.showQuitPopup = false;
    state.softDrop = false;
    state.currentBlock = spawnBlock();
    state.nextBlock    = spawnBlock();
}

// gamecore-xu-ly-cham-05
// Khoa khoi vao ban; lay nextBlock thanh currentBlock va sinh next moi
// Neu khoi moi va cham ngay -> game over: ep mo popup quit + pause
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
// Sidebar 30px x 480: 7 component xep doc, KHONG ve vien trang quanh component
// =========================================================
static const SDL_FRect RECT_QUIT   = { (float)SIDEBAR_X, (float)COMP_QUIT_Y,   30.0f, 60.0f };
static const SDL_FRect RECT_PAUSE  = { (float)SIDEBAR_X, (float)COMP_PAUSE_Y,  30.0f, 60.0f };
static const SDL_FRect RECT_SCORE  = { (float)SIDEBAR_X, (float)COMP_SCORE_Y,  30.0f, 60.0f };
static const SDL_FRect RECT_NEXT1  = { (float)SIDEBAR_X, (float)COMP_NEXT1_Y,  30.0f, 75.0f };
static const SDL_FRect RECT_NEXT2  = { (float)SIDEBAR_X, (float)COMP_NEXT2_Y,  30.0f, 75.0f };
static const SDL_FRect RECT_NEXT3  = { (float)SIDEBAR_X, (float)COMP_NEXT3_Y,  30.0f, 75.0f };
static const SDL_FRect RECT_KEYPAD = { (float)SIDEBAR_X, (float)COMP_KEYPAD_Y, 30.0f, 75.0f };

// Keypad 30x75 chia: up/down 30x18, left/right/center 10x39 o giua chieu cao
// De don gian, dung hang ngang: top 22 = up, mid 31 = LRC, bot 22 = down
static const float KP_TOP_H = 22.0f;
static const float KP_MID_H = 31.0f;
static const float KP_BOT_H = 22.0f;
static const SDL_FRect KP_UP     = { RECT_KEYPAD.x,         RECT_KEYPAD.y,                                 30.0f, KP_TOP_H };
static const SDL_FRect KP_LEFT   = { RECT_KEYPAD.x,         RECT_KEYPAD.y + KP_TOP_H,                      10.0f, KP_MID_H };
static const SDL_FRect KP_CENTER = { RECT_KEYPAD.x + 10.0f, RECT_KEYPAD.y + KP_TOP_H,                      10.0f, KP_MID_H };
static const SDL_FRect KP_RIGHT  = { RECT_KEYPAD.x + 20.0f, RECT_KEYPAD.y + KP_TOP_H,                      10.0f, KP_MID_H };
static const SDL_FRect KP_DOWN   = { RECT_KEYPAD.x,         RECT_KEYPAD.y + KP_TOP_H + KP_MID_H,           30.0f, KP_BOT_H };

// Popup quit: nen mo + khung 230x300 can giua, tieu de 1 phan + cau hoi 1 phan + 4 nut xep doc
static const SDL_FRect POPUP_BG     = { 20.0f,  70.0f, 230.0f, 340.0f };
static const SDL_FRect POPUP_CLOSE  = { POPUP_BG.x + POPUP_BG.w - 24.0f, POPUP_BG.y + 6.0f, 18.0f, 18.0f };
// 4 nut, moi nut 180x40, gap 10, bat dau tu y = POPUP_BG.y + 130 de chua che cau hoi
static const float BTN_W = 180.0f, BTN_H = 36.0f, BTN_GAP = 10.0f;
static const float BTN_X = POPUP_BG.x + (POPUP_BG.w - BTN_W) / 2.0f;
static const float BTN_Y0 = POPUP_BG.y + 130.0f;
static const SDL_FRect POPUP_RESTART = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 0, BTN_W, BTN_H };
static const SDL_FRect POPUP_CONSOLE = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 1, BTN_W, BTN_H };
static const SDL_FRect POPUP_QUIT    = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 2, BTN_W, BTN_H };
static const SDL_FRect POPUP_CANCEL  = { BTN_X, BTN_Y0 + (BTN_H + BTN_GAP) * 3, BTN_W, BTN_H };

static bool hitTest(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// Ve icon "power" ⏻: vong cung gia lap bang khung chu nhat ho phia tren + thanh dung
// Vi tri tam bang tam cua RECT_QUIT
static void drawPowerIcon(SDL_Renderer* renderer, const SDL_FRect& host) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    // Vong (8 canh) tao boi 4 thanh chu nhat - chua nut tren cung
    SDL_FRect bottom = { cx - 9, cy + 6,  18, 2 };
    SDL_FRect leftV  = { cx - 9, cy - 4,   2, 12 };
    SDL_FRect rightV = { cx + 7, cy - 4,   2, 12 };
    SDL_FRect topL   = { cx - 9, cy - 4,   5, 2 };
    SDL_FRect topR   = { cx + 4, cy - 4,   5, 2 };
    SDL_RenderFillRect(renderer, &bottom);
    SDL_RenderFillRect(renderer, &leftV);
    SDL_RenderFillRect(renderer, &rightV);
    SDL_RenderFillRect(renderer, &topL);
    SDL_RenderFillRect(renderer, &topR);
    // Thanh dung tu tam tren xuong giua
    SDL_FRect bar = { cx - 1, cy - 12, 2, 12 };
    SDL_RenderFillRect(renderer, &bar);
}

// Ve icon "stop" (vuong dac) hoac "play" (tam giac dac don gian)
static void drawPauseIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool isPaused) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    if (!isPaused) {
        // STOP: vuong 14x14
        SDL_FRect sq = { cx - 7, cy - 7, 14, 14 };
        SDL_RenderFillRect(renderer, &sq);
    } else {
        // PLAY: tam giac dac (gia lap bang nhieu hang ngang)
        for (int i = 0; i < 14; i++) {
            float w = (i < 7) ? (i * 1.6f + 2) : ((13 - i) * 1.6f + 2);
            SDL_FRect ln = { cx - 6, cy - 7 + i, w, 1.0f };
            SDL_RenderFillRect(renderer, &ln);
        }
    }
}

// Ve thu nho 1 piece trong vung "next" (cell 4x4)
static void drawNextPreview(SDL_Renderer* renderer, const SDL_FRect& slot, const Tetromino& t) {
    SDL_Color c = COLORS[t.colorID];
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
    const float CELL = 4.0f;
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
                           CELL - 0.5f, CELL - 0.5f };
        SDL_RenderFillRect(renderer, &cell);
    }
}

// Keypad hien thi 5 vung mau xam dam, khong vien trang
static void drawKeypad(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &KP_UP);
    SDL_RenderFillRect(renderer, &KP_DOWN);
    SDL_RenderFillRect(renderer, &KP_LEFT);
    SDL_RenderFillRect(renderer, &KP_RIGHT);
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &KP_CENTER);
    // Vach phan cach mong giua cac phan
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect divH1 = { RECT_KEYPAD.x, RECT_KEYPAD.y + KP_TOP_H, 30.0f, 1.0f };
    SDL_FRect divH2 = { RECT_KEYPAD.x, RECT_KEYPAD.y + KP_TOP_H + KP_MID_H, 30.0f, 1.0f };
    SDL_FRect divV1 = { RECT_KEYPAD.x + 10.0f, RECT_KEYPAD.y + KP_TOP_H, 1.0f, KP_MID_H };
    SDL_FRect divV2 = { RECT_KEYPAD.x + 20.0f, RECT_KEYPAD.y + KP_TOP_H, 1.0f, KP_MID_H };
    SDL_RenderFillRect(renderer, &divH1);
    SDL_RenderFillRect(renderer, &divH2);
    SDL_RenderFillRect(renderer, &divV1);
    SDL_RenderFillRect(renderer, &divV2);
}

// Ve toan bo sidebar
static void drawSidebar(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect bg = { (float)SIDEBAR_X, 0, (float)SIDEBAR_W, (float)CORE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bg);

    drawPowerIcon(renderer, RECT_QUIT);
    drawPauseIcon(renderer, RECT_PAUSE, state.isPaused);

    // SCORE: chi text, khong vien
    char buf[16];
    SDL_snprintf(buf, sizeof(buf), "%d", state.score);
    int len = (int)SDL_strlen(buf);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer,
                        RECT_SCORE.x + (RECT_SCORE.w - len * 8.0f) / 2.0f,
                        RECT_SCORE.y + (RECT_SCORE.h - 8.0f) / 2.0f,
                        buf);

    // NEXT-1 hoat dong; NEXT-2/3 de trong (placeholder v2/v3)
    drawNextPreview(renderer, RECT_NEXT1, state.nextBlock);

    drawKeypad(renderer);
}

// Helper ve nut popup
static void drawPopupButton(SDL_Renderer* renderer, const SDL_FRect& r,
                            const char* label, SDL_Color bg) {
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &r);
    int ll = (int)SDL_strlen(label);
    SDL_RenderDebugText(renderer,
                        r.x + (r.w - ll * 8.0f) / 2.0f,
                        r.y + (r.h - 8.0f) / 2.0f, label);
}

// gamecore-popup-quit-08
static void drawQuitPopup(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_FRect screen = { 0, 0, (float)CORE_SCREEN_WIDTH, (float)CORE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
    SDL_RenderFillRect(renderer, &POPUP_BG);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &POPUP_BG);

    // Nut close [X] - chi tat popup, KHONG resume neu pause da bat
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &POPUP_CLOSE);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &POPUP_CLOSE);
    SDL_RenderDebugText(renderer, POPUP_CLOSE.x + 5, POPUP_CLOSE.y + 5, "X");

    // Tieu de
    SDL_RenderDebugText(renderer, POPUP_BG.x + 20, POPUP_BG.y + 30,
                        state.isGameOver ? "GAME OVER" : "PAUSED");
    // Cau hoi: 2 dong, dat o khoang trong tu y+60 toi y+120 (truoc nut tu y+130)
    SDL_RenderDebugText(renderer, POPUP_BG.x + 20, POPUP_BG.y + 70,
                        "What do you want to do?");
    char scoreLine[40];
    SDL_snprintf(scoreLine, sizeof(scoreLine), "Current score: %d", state.score);
    SDL_RenderDebugText(renderer, POPUP_BG.x + 20, POPUP_BG.y + 95, scoreLine);

    // 4 nut xep doc (Restart / Console / Quit / Cancel)
    drawPopupButton(renderer, POPUP_RESTART, "Restart (new game)",        { 70, 130,  90, 255});
    drawPopupButton(renderer, POPUP_CONSOLE, "Console (back to menu)",    { 70, 100, 160, 255});
    drawPopupButton(renderer, POPUP_QUIT,    "Quit (exit app)",           {180,  60,  60, 255});
    drawPopupButton(renderer, POPUP_CANCEL,  "Cancel (close popup)",      {100, 100, 100, 255});
}

// Render ban co: moi o vien den 1px thut vao trong (ve nen den 24x24 + lop mau 22x22)
static void renderBoard(SDL_Renderer* renderer, const GameState& state) {
    // Nen toan bo vung ban co la den (de "vien" cua cac o trong cung den)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect boardBg = { 0, 0, (float)BOARD_W, (float)BOARD_H };
    SDL_RenderFillRect(renderer, &boardBg);

    auto drawCell = [&](int gx, int gy, SDL_Color c) {
        // Lop mau thut vao 1px trong moi chieu -> tao vien den
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

    // Khoi dang roi
    for (int i = 0; i < 4; i++) {
        int nx = state.currentBlock.x + state.currentBlock.blocks[i].x;
        int ny = state.currentBlock.y + state.currentBlock.blocks[i].y;
        drawCell(nx, ny, COLORS[state.currentBlock.colorID]);
    }
}

// Mo popup quit (dong thoi pause). Dung chung cho ESC, click QUIT, va game-over auto
static void openQuitPopup(GameState& state) {
    state.showQuitPopup = true;
    state.isPaused      = true;
}

// Pause toggle: neu game over thi luc nao cung phai mo popup quit (khong cho play tiep)
static void togglePause(GameState& state) {
    if (state.isGameOver) {
        openQuitPopup(state);
        return;
    }
    state.isPaused = !state.isPaused;
}

// Tap trung cac action di chuyen / xoay / soft-drop
// gamecore-soft-drop-09
// gamecore-pause-quit-buttons-10
static void onAction(GameState& state, SDL_Keycode keyEquiv) {
    if (state.showQuitPopup || state.isPaused || state.isGameOver) return;
    if (keyEquiv == SDLK_SPACE) { state.softDrop = true; return; }
    applyMoveOrRotate(state, keyEquiv);
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
    const Uint32 FALL_INTERVAL_FAST   = 500 / 3;

    while (true) {
        state.softDrop = false;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.exitCode = 0;
                goto END;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                if (key == SDLK_ESCAPE) {
                    if (!state.showQuitPopup) openQuitPopup(state);
                    // Khi popup dang mo, ESC khong tat (chi click X tat)
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
                        // Dong popup nhung KHONG resume - giu nguyen pause
                        state.showQuitPopup = false;
                        // state.isPaused giu nguyen = true
                    } else if (hitTest(POPUP_RESTART, mx, my)) {
                        resetGame(state);
                        lastFallTime = SDL_GetTicks();
                    } else if (hitTest(POPUP_CONSOLE, mx, my)) {
                        state.exitCode = 2;
                        goto END;
                    } else if (hitTest(POPUP_QUIT, mx, my)) {
                        state.exitCode = 0;
                        goto END;
                    }
                } else {
                    // Sidebar
                    if (hitTest(RECT_QUIT, mx, my))       openQuitPopup(state);
                    else if (hitTest(RECT_PAUSE, mx, my)) togglePause(state);
                    // Keypad
                    else if (hitTest(KP_UP, mx, my))      onAction(state, SDLK_UP);
                    else if (hitTest(KP_DOWN, mx, my))    onAction(state, SDLK_DOWN);
                    else if (hitTest(KP_LEFT, mx, my))    onAction(state, SDLK_LEFT);
                    else if (hitTest(KP_RIGHT, mx, my))   onAction(state, SDLK_RIGHT);
                    else if (hitTest(KP_CENTER, mx, my))  onAction(state, SDLK_SPACE);
                }
            }
        }

        // SPACE giu -> soft-drop lien tuc
        const bool* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_SPACE] && !state.isPaused && !state.showQuitPopup && !state.isGameOver) {
            state.softDrop = true;
        }

        // Logic roi
        if (!state.isPaused && !state.showQuitPopup && !state.isGameOver) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 interval = state.softDrop ? FALL_INTERVAL_FAST : FALL_INTERVAL_NORMAL;
            if (currentTime - lastFallTime > interval) {
                Tetromino nextT = state.currentBlock;
                nextT.y += 1;
                if (checkCollision(state, nextT)) {
                    lockBlock(state); // co the set isGameOver + showQuitPopup
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
        drawSidebar(renderer, state);
        if (state.showQuitPopup) drawQuitPopup(renderer, state);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
END:
    return state.exitCode;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Core - Standalone",
                                          CORE_SCREEN_WIDTH, CORE_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameCore(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif