// integration/v1
// gameCore: ban co 240x480 + sidebar 30x480 voi 12 component dong nhat 30x40
#include "gameCore_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <ctime>

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
// Sidebar 12 component dong nhat 30x40, khong vien trang quanh component
// =========================================================
static SDL_FRect rectAt(int slotIndex) {
    return { (float)SIDEBAR_X, (float)(slotIndex * COMP_H), 30.0f, (float)COMP_H };
}

static const SDL_FRect RECT_QUIT      = rectAt(0);
static const SDL_FRect RECT_PAUSE     = rectAt(1);
static const SDL_FRect RECT_SCORE     = rectAt(2);
static const SDL_FRect RECT_SPEED_IND = rectAt(3);   // Speed indicator (read-only)
static const SDL_FRect RECT_NEXT1     = rectAt(4);
static const SDL_FRect RECT_NEXT2     = rectAt(5);
static const SDL_FRect RECT_NEXT3     = rectAt(6);
static const SDL_FRect RECT_ARR_UP    = rectAt(7);
static const SDL_FRect RECT_ARR_DOWN  = rectAt(8);
static const SDL_FRect RECT_ARR_LEFT  = rectAt(9);
static const SDL_FRect RECT_ARR_RIGHT = rectAt(10);
static const SDL_FRect RECT_SPEED_BTN = rectAt(11);  // Speed booster (giu = SPACE)

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

static bool hitTest(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// ---------- Icon helpers ----------

static void drawPowerIcon(SDL_Renderer* renderer, const SDL_FRect& host) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    SDL_FRect bottom = { cx - 9, cy + 6,  18, 2 };
    SDL_FRect leftV  = { cx - 9, cy - 4,   2, 12 };
    SDL_FRect rightV = { cx + 7, cy - 4,   2, 12 };
    SDL_FRect topL   = { cx - 9, cy - 4,   5, 2 };
    SDL_FRect topR   = { cx + 4, cy - 4,   5, 2 };
    SDL_FRect bar    = { cx - 1, cy - 12,  2, 12 };
    SDL_RenderFillRect(renderer, &bottom);
    SDL_RenderFillRect(renderer, &leftV);
    SDL_RenderFillRect(renderer, &rightV);
    SDL_RenderFillRect(renderer, &topL);
    SDL_RenderFillRect(renderer, &topR);
    SDL_RenderFillRect(renderer, &bar);
}

static void drawPauseIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool isPaused) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    if (!isPaused) {
        SDL_FRect sq = { cx - 6, cy - 6, 12, 12 };
        SDL_RenderFillRect(renderer, &sq);
    } else {
        for (int i = 0; i < 12; i++) {
            float w = (i < 6) ? (i * 1.6f + 2) : ((11 - i) * 1.6f + 2);
            SDL_FRect ln = { cx - 5, cy - 6 + i, w, 1.0f };
            SDL_RenderFillRect(renderer, &ln);
        }
    }
}

// Mui ten don gian: "tam giac" gia lap bang chuoi line
// dir: 0 = up, 1 = down, 2 = left, 3 = right
static void drawArrowIcon(SDL_Renderer* renderer, const SDL_FRect& host, int dir) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    const int N = 10; // do dai mui ten
    for (int i = 0; i < N; i++) {
        float w = (float)(i + 1);
        SDL_FRect ln;
        if (dir == 0) {           // up: dinh phia tren
            ln = { cx - w/2, cy - 5 + i, w, 1.0f };
        } else if (dir == 1) {    // down: dinh phia duoi
            ln = { cx - (N - i)/2.0f, cy - 5 + i, (float)(N - i), 1.0f };
        } else if (dir == 2) {    // left: dinh ben trai
            ln = { cx - 5 + i, cy - w/2, 1.0f, w };
        } else {                  // right: dinh ben phai
            ln = { cx - 5 + i, cy - (N - i)/2.0f, 1.0f, (float)(N - i) };
        }
        SDL_RenderFillRect(renderer, &ln);
    }
}

// 2 tam giac dac noi tiep (>>) cho speed booster
static void drawSpeedBoosterIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool active) {
    if (active) SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    else        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2;
    // Tam giac 1 (ben trai)
    for (int i = 0; i < 8; i++) {
        float h = (i < 4) ? (i + 1) : (8 - i);
        SDL_FRect ln = { cx - 9 + i, cy - h, 1.0f, 2 * h };
        SDL_RenderFillRect(renderer, &ln);
    }
    // Tam giac 2 (ben phai, dich offset)
    for (int i = 0; i < 8; i++) {
        float h = (i < 4) ? (i + 1) : (8 - i);
        SDL_FRect ln = { cx + 1 + i, cy - h, 1.0f, 2 * h };
        SDL_RenderFillRect(renderer, &ln);
    }
}

// Preview piece thu nho trong slot
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

// Ve text gon trong rect (can giua)
static void drawCenterText(SDL_Renderer* renderer, const SDL_FRect& r, const char* text) {
    int len = (int)SDL_strlen(text);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer,
                        r.x + (r.w - len * 8.0f) / 2.0f,
                        r.y + (r.h - 8.0f) / 2.0f, text);
}

static void drawSidebar(SDL_Renderer* renderer, const GameState& state) {
    // Nen sidebar
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect bg = { (float)SIDEBAR_X, 0, (float)SIDEBAR_W, (float)CORE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bg);

    drawPowerIcon(renderer, RECT_QUIT);
    drawPauseIcon(renderer, RECT_PAUSE, state.isPaused);

    // Score
    char buf[16];
    SDL_snprintf(buf, sizeof(buf), "%d", state.score);
    drawCenterText(renderer, RECT_SCORE, buf);

    // Speed indicator: hien thi cap (1 hoac 3 trong v1)
    int speedLevel = state.softDrop ? 3 : 1;
    char sbuf[8];
    SDL_snprintf(sbuf, sizeof(sbuf), "x%d", speedLevel);
    drawCenterText(renderer, RECT_SPEED_IND, sbuf);

    // Next 1/2/3
    drawNextPreview(renderer, RECT_NEXT1, state.nextBlock);
    // RECT_NEXT2 / RECT_NEXT3: trong (placeholder)

    // 4 mui ten
    drawArrowIcon(renderer, RECT_ARR_UP,    0);
    drawArrowIcon(renderer, RECT_ARR_DOWN,  1);
    drawArrowIcon(renderer, RECT_ARR_LEFT,  2);
    drawArrowIcon(renderer, RECT_ARR_RIGHT, 3);

    // Speed booster
    drawSpeedBoosterIcon(renderer, RECT_SPEED_BTN, state.softDrop);
}

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

    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &POPUP_CLOSE);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &POPUP_CLOSE);
    SDL_RenderDebugText(renderer, POPUP_CLOSE.x + 5, POPUP_CLOSE.y + 5, "X");

    SDL_RenderDebugText(renderer, POPUP_BG.x + 20, POPUP_BG.y + 30,
                        state.isGameOver ? "GAME OVER" : "PAUSED");
    SDL_RenderDebugText(renderer, POPUP_BG.x + 20, POPUP_BG.y + 70,
                        "What do you want to do?");
    char scoreLine[40];
    SDL_snprintf(scoreLine, sizeof(scoreLine), "Current score: %d", state.score);
    SDL_RenderDebugText(renderer, POPUP_BG.x + 20, POPUP_BG.y + 95, scoreLine);

    drawPopupButton(renderer, POPUP_RESTART, "Restart (new game)",     { 70, 130,  90, 255});
    drawPopupButton(renderer, POPUP_CONSOLE, "Console (back to menu)", { 70, 100, 160, 255});
    drawPopupButton(renderer, POPUP_QUIT,    "Quit (exit app)",        {180,  60,  60, 255});
    drawPopupButton(renderer, POPUP_CANCEL,  "Cancel (close popup)",   {100, 100, 100, 255});
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
        // softDrop tinh moi frame: bat khi SPACE giu hoac chuot giu nut booster
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
                        goto END;
                    } else if (hitTest(POPUP_QUIT, mx, my)) {
                        state.exitCode = 0;
                        goto END;
                    }
                } else {
                    // Sidebar
                    if (hitTest(RECT_QUIT, mx, my))            openQuitPopup(state);
                    else if (hitTest(RECT_PAUSE, mx, my))      togglePause(state);
                    // 4 mui ten = phim mui ten
                    else if (hitTest(RECT_ARR_UP, mx, my))     onAction(state, SDLK_UP);
                    else if (hitTest(RECT_ARR_DOWN, mx, my))   onAction(state, SDLK_DOWN);
                    else if (hitTest(RECT_ARR_LEFT, mx, my))   onAction(state, SDLK_LEFT);
                    else if (hitTest(RECT_ARR_RIGHT, mx, my))  onAction(state, SDLK_RIGHT);
                    // Speed booster: bat trang thai held; soft-drop chinh duoc tinh moi frame
                    else if (hitTest(RECT_SPEED_BTN, mx, my))  state.speedHeld = true;
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                event.button.button == SDL_BUTTON_LEFT) {
                state.speedHeld = false;
            }
        }

        // SPACE giu HOAC giu chuot tren nut booster -> soft drop
        const bool* keys = SDL_GetKeyboardState(NULL);
        bool active = !state.isPaused && !state.showQuitPopup && !state.isGameOver;
        if (active && (keys[SDL_SCANCODE_SPACE] || state.speedHeld)) {
            state.softDrop = true;
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