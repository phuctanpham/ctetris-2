// integration/v1
// gameCore: ban co 240x480 + sidebar 30x480 voi 12 component dong nhat 30x40
#include "gameCore_layout.h"
#include "gameConsole_layout.h"   // [D.6] SettingsConfig contract
#include <SDL3/SDL.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>

#include "gameConsole_db.h"
#include "sqlite3.h"

// Tren WASM build, can goi window.location.reload() khi user click "Reload"
// o man hinh shutdown. Su dung emscripten_run_script de chen JS.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static const SettingsConfig* s_cfg = nullptr;

const SDL_Color COLORS[] = {
    {  0,   0,   0, 255},
    {  0,   0, 255, 255},
    {255,   0,   0, 255},
    {  0, 255,   0, 255},
    {255, 255,   0, 255},
    {255, 165,   0, 255},
    {128,   0, 128, 255},
    {255, 105, 180, 255}
};

// Mau thong nhat cho hieu ung "active" (chuot giu / phim giu)
static const SDL_Color HIGHLIGHT_YELLOW = {255, 215, 0, 255};
// Mau text "nhe" -- dung 220 thay vi 255 de font xuat hien mong/it dam hon.
static const SDL_Color SOFT_WHITE = {220, 220, 220, 255};

// gamecore-tao-cac-khoi-xep-hinh-LITZO-01
const Point SHAPE_L[4] = {{0,0}, {0,1}, {0,2}, {1,2}};
const Point SHAPE_I[4] = {{0,0}, {1,0}, {2,0}, {3,0}};
const Point SHAPE_Z[4] = {{0,0}, {1,0}, {1,1}, {2,1}};
const Point SHAPE_O[4] = {{0,0}, {1,0}, {0,1}, {1,1}};
const Point SHAPE_T[4] = {{0,0}, {1,0}, {2,0}, {1,1}};
const Point* const SHAPES[] = { SHAPE_L, SHAPE_I, SHAPE_Z, SHAPE_O, SHAPE_T };
const int NUM_SHAPES = (int)(sizeof(SHAPES) / sizeof(SHAPES[0]));

// gamecore-do-mau-02
static Tetromino spawnBlock() {
    Tetromino t;
    int idx = std::rand() % NUM_SHAPES;
    for (int i = 0; i < 4; i++) t.blocks[i] = SHAPES[idx][i];

    if ((std::rand() & 1) == 0) {
        int maxX = 0;
        for (int i = 0; i < 4; i++) {
            if (t.blocks[i].x > maxX) maxX = t.blocks[i].x;
        }
        for (int i = 0; i < 4; i++) {
            t.blocks[i].x = maxX - t.blocks[i].x;
        }
    }

    static const int PALETTE_TO_COLOR[7] = {2, 5, 7, 4, 3, 1, 6};
    int enabledIds[7];
    int n = 0;
    if (s_cfg) {
        for (int i = 0; i < 7; i++) {
            if (s_cfg->colorEnabled[i]) enabledIds[n++] = PALETTE_TO_COLOR[i];
        }
    }
    t.colorID = (n > 0) ? enabledIds[std::rand() % n] : (std::rand() % 6 + 1);
    t.x = BOARD_COLS / 2 - 1;
    t.y = 0;
    return t;
}

// gamecore-tang-do-kho-16
static Uint32 getFallInterval(int score) {
    if (score >= 50) return 100;
    if (score >= 30) return 200;
    if (score >= 15) return 300;
    if (score >=  5) return 400;
    return 500;
}

// gamecore-chen-nhac-nen-15
static SDL_AudioStream* g_coreBgmStream = nullptr;

static void coreOpenBgm(float volume) {
    if (g_coreBgmStream) return;
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 44100 };
    g_coreBgmStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                 &spec, NULL, NULL);
    if (!g_coreBgmStream) {
        SDL_Log("[gameCore] BGM open fail: %s", SDL_GetError());
        return;
    }
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    SDL_SetAudioStreamGain(g_coreBgmStream, volume);
    SDL_ResumeAudioStreamDevice(g_coreBgmStream);
    SDL_Log("[gameCore] BGM ready gain=%.2f", volume);
}

static void coreCloseBgm() {
    if (g_coreBgmStream) {
        SDL_DestroyAudioStream(g_coreBgmStream);
        g_coreBgmStream = nullptr;
    }
}

static bool checkCollision(const GameState& state, const Tetromino& t);

// gamecore-table-matrix-21
// Format: semicolon-separated rows (bottom-aligned), comma-separated colorID values.
// e.g. "1,0,2,0,3,0,4,0,5,0;6,1,2,3,4,5,6,1,2,3" = 2 pre-filled bottom rows.
static void applyTableMatrix(GameState& state, const std::string& tm) {
    if (tm.empty()) return;
    int rowCount = 1;
    for (char c : tm) if (c == ';') rowCount++;
    int startRow = BOARD_ROWS - rowCount;
    if (startRow < 0) startRow = 0;

    int curRow = startRow, curCol = 0, val = 0;
    bool hasVal = false;
    for (size_t i = 0; i <= tm.size(); i++) {
        char c = (i < tm.size()) ? tm[i] : ';';
        if (c == ',' || c == ';') {
            if (hasVal && curRow < BOARD_ROWS && curCol < BOARD_COLS)
                state.board[curRow][curCol] = val;
            curCol++; val = 0; hasVal = false;
            if (c == ';') { curRow++; curCol = 0; }
        } else if (c >= '0' && c <= '9') {
            val = val * 10 + (c - '0');
            hasVal = true;
        }
    }
    SDL_Log("[gameCore] tableMatrix applied: %d rows", rowCount);
}

// gamecore-save-record-22 / gamecore-update-story-progress-23
static void onGameOver(const GameState& state, Uint32 elapsedMs) {
    if (!s_cfg) return;
    const bool coreOpenedDb = !dbIsOpen();
    if (!dbOpen("default")) {
        SDL_Log("[gameCore] onGameOver: dbOpen fail");
        return;
    }

    GameRecord rec;
    rec.idUser       = "default";
    rec.startTS      = (int64_t)state.gameStartTime;
    rec.endTS        = (int64_t)SDL_GetTicks();
    rec.idStory      = s_cfg->storyId;
    rec.idChapter    = s_cfg->chapterId;
    rec.totalScore   = state.score;
    rec.totalSeconds = (int)(elapsedMs / 1000);
    rec.avgSpeed     = rec.totalSeconds > 0
        ? (float)rec.totalScore / (float)rec.totalSeconds : 0.0f;
    rec.retryNo      = state.retryCount;
    dbInsertRecord(rec);

    if (s_cfg->storyId > 0) {
        dbUpsertStoryProgress("default", s_cfg->storyId, s_cfg->chapterId, true, true);
        dbCheckAndUnlockStories("default");
    }
    if (coreOpenedDb) dbClose();
    SDL_Log("[gameCore] onGameOver saved: score=%d story=%d retries=%d",
            state.score, s_cfg->storyId, state.retryCount);
}

// gamecore-hieu-ung-khi-xoa-dong-18 / gamecore-diem-thuong-khi-xoa-nhieu-dong-19
static void finishLineClear(GameState& state) {
    state.score += state.clearRowCount * state.clearRowCount;
    if (state.score > 99999) state.score = 99999;

    for (int r = BOARD_ROWS - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < BOARD_COLS; c++) {
            if (state.board[r][c] == 0) { full = false; break; }
        }
        if (full) {
            for (int y = r; y > 0; y--) {
                for (int x = 0; x < BOARD_COLS; x++)
                    state.board[y][x] = state.board[y - 1][x];
            }
            for (int x = 0; x < BOARD_COLS; x++) state.board[0][x] = 0;
            r++;
        }
    }

    state.currentBlock   = state.nextBlock;
    state.currentBlock.x = BOARD_COLS / 2 - 1;
    state.currentBlock.y = 0;
    state.nextBlock  = state.nextBlock2;
    state.nextBlock2 = state.nextBlock3;
    state.nextBlock3 = spawnBlock();

    if (checkCollision(state, state.currentBlock)) {
        state.isGameOver    = true;
        state.isPaused      = true;
        state.showQuitPopup = true;
    }
    state.pendingClear  = false;
    state.clearRowCount = 0;
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
    state.pendingClear  = false;
    state.clearRowCount = 0;
    state.flashTick     = 0;
    state.flashOn       = false;
    state.currentBlock = spawnBlock();
    state.nextBlock    = spawnBlock();
    state.nextBlock2   = spawnBlock();
    state.nextBlock3   = spawnBlock();
    if (s_cfg) applyTableMatrix(state, s_cfg->tableMatrix);

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

    state.clearRowCount = 0;
    for (int r = BOARD_ROWS - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < BOARD_COLS; c++) {
            if (state.board[r][c] == 0) { full = false; break; }
        }
        if (full && state.clearRowCount < 4)
            state.clearRows[state.clearRowCount++] = r;
    }

    if (state.clearRowCount > 0) {
        state.pendingClear = true;
        state.flashTick    = 0;
        state.flashOn      = true;
        state.flashTimer   = SDL_GetTicks();
    } else {
        state.currentBlock   = state.nextBlock;
        state.currentBlock.x = BOARD_COLS / 2 - 1;
        state.currentBlock.y = 0;
        state.nextBlock  = state.nextBlock2;
        state.nextBlock2 = state.nextBlock3;
        state.nextBlock3 = spawnBlock();
        if (checkCollision(state, state.currentBlock)) {
            state.isGameOver    = true;
            state.isPaused      = true;
            state.showQuitPopup = true;
        }
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

// Reload button cho man hinh WASM shutdown
static const SDL_FRect RELOAD_BTN = {
    (CORE_SCREEN_WIDTH  - 160.0f) / 2.0f,
    (CORE_SCREEN_HEIGHT - 50.0f)  / 2.0f,
    160.0f, 50.0f
};

static bool hitTest(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// Touch swipe: returns true if screen point (mx,my) lands on any cell of the falling block
static bool isTouchOnFallingBlock(const GameState& state, float mx, float my) {
    for (int i = 0; i < 4; i++) {
        int gx = state.currentBlock.x + state.currentBlock.blocks[i].x;
        int gy = state.currentBlock.y + state.currentBlock.blocks[i].y;
        float cellX = (float)(gx * BLOCK_SIZE);
        float cellY = (float)(gy * BLOCK_SIZE);
        if (mx >= cellX && mx < cellX + BLOCK_SIZE &&
            my >= cellY && my < cellY + BLOCK_SIZE)
            return true;
    }
    return false;
}

static void drawSmallText(SDL_Renderer* renderer, float x, float y, float scale, const char* text) {
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_RenderDebugText(renderer, x / scale, y / scale, text);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

static void drawPowerIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool active) {
    SDL_Color c = active ? HIGHLIGHT_YELLOW : SOFT_WHITE;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

    float cx = host.x + host.w / 2;
    float cy = host.y + host.h / 2 + 1.0f;
    const float radius = 5.5f;

    for (int deg = -60; deg <= 240; deg += 4) {
        float rad = (float)deg * 3.14159265f / 180.0f;
        float px = cx + radius * std::cos(rad);
        float py = cy + radius * std::sin(rad);
        SDL_FRect dot = { px - 0.75f, py - 0.75f, 1.5f, 1.5f };
        SDL_RenderFillRect(renderer, &dot);
    }
    SDL_FRect bar = { cx - 0.75f, cy - 8.5f, 1.5f, 6.0f };
    SDL_RenderFillRect(renderer, &bar);
}

static void drawPauseIcon(SDL_Renderer* renderer, const SDL_FRect& host, bool isPaused, bool active) {
    SDL_Color c = active ? HIGHLIGHT_YELLOW : SOFT_WHITE;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
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

static void drawNextPreview(SDL_Renderer* renderer, const SDL_FRect& slot, const Tetromino& t) {
    SDL_Color c = COLORS[t.colorID];
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);

    const float CELL = 5.0f;
    const float CELL_GAP = 1.5f;
    const float CELL_INNER = CELL - CELL_GAP;

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

static void drawScoreInSlot(SDL_Renderer* renderer, const SDL_FRect& host, int score) {
    char buf[12];
    SDL_snprintf(buf, sizeof(buf), "%05d", score);
    const float SCALE = 0.65f;
    float textW = 5 * 8.0f * SCALE;
    float textH = 8.0f * SCALE;
    float x = host.x + (host.w - textW) / 2.0f;
    float y = host.y + (host.h - textH) / 2.0f;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    drawSmallText(renderer, x, y, SCALE, buf);
}

static void drawTimerInSlot(SDL_Renderer* renderer, const SDL_FRect& host, Uint32 elapsedMs) {
    Uint32 totalMin = elapsedMs / 60000;
    int hours = (int)(totalMin / 60);
    int mins  = (int)(totalMin % 60);
    if (hours > 99) hours = 99;
    char buf[8];
    SDL_snprintf(buf, sizeof(buf), "%02d:%02d", hours, mins);
    const float SCALE = 0.65f;
    float textW = 5 * 8.0f * SCALE;
    float textH = 8.0f * SCALE;
    float x = host.x + (host.w - textW) / 2.0f;
    float y = host.y + (host.h - textH) / 2.0f;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    drawSmallText(renderer, x, y, SCALE, buf);
}

static void drawSidebar(SDL_Renderer* renderer, const GameState& state,
                        const bool* keys, Uint32 elapsedMs) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect bg = { (float)SIDEBAR_X, 0, (float)SIDEBAR_W, (float)CORE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bg);

    bool aQuit  = state.mouseHeldQuit  || (keys && keys[SDL_SCANCODE_ESCAPE]);
    bool aPause = state.mouseHeldPause || (keys && (keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_KP_ENTER]));
    bool aUp    = state.mouseHeldArrUp    || (keys && (keys[SDL_SCANCODE_UP]    || keys[SDL_SCANCODE_W]));
    bool aDown  = state.mouseHeldArrDown  || (keys && (keys[SDL_SCANCODE_DOWN]  || keys[SDL_SCANCODE_S]));
    bool aLeft  = state.mouseHeldArrLeft  || (keys && (keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A]));
    bool aRight = state.mouseHeldArrRight || (keys && (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]));
    bool aSpeed = state.speedHeld         || (keys && keys[SDL_SCANCODE_SPACE]);

    drawPowerIcon(renderer, RECT_QUIT, aQuit);
    drawPauseIcon(renderer, RECT_PAUSE, state.isPaused, aPause);
    drawScoreInSlot(renderer, RECT_SCORE, state.score);
    drawTimerInSlot(renderer, RECT_TIMER, elapsedMs);
    drawNextPreview(renderer, RECT_NEXT1, state.nextBlock);
    // Issue 2.7: show story label "S{storyId}-C{chapterId}" in RECT_SCORE slot
    // (the score slot already draws score via drawScoreInSlot above;
    //  use a small label above it, inside RECT_TIMER which is the 4th component).
    // Sidebar slot 1 (RECT_SCORE, y=80) already shows score.
    // Sidebar slot 0 (RECT_QUIT)  is power icon.
    // We render the story label as a tiny overlay inside RECT_NEXT1 header area.
    if (s_cfg && s_cfg->storyId > 0) {
        char storyLbl[12];
        SDL_snprintf(storyLbl, sizeof(storyLbl), "S%d-C%d",
                     s_cfg->storyId, s_cfg->chapterId);
        int sll = (int)SDL_strlen(storyLbl);
        SDL_SetRenderDrawColor(renderer, 160, 160, 100, 255);
        drawSmallText(renderer,
                      RECT_NEXT1.x + (RECT_NEXT1.w - sll * 8.0f * 0.55f) / 2.0f,
                      RECT_NEXT1.y + 1.0f,
                      0.55f, storyLbl);
    }

    // TODO(V3): replace nextBlockScore*2 fallback with cfg.nextBlockScore3
    // when SettingsConfig and shared_data extend schema. (Issue 2.8)
    bool showNext2 = s_cfg && s_cfg->nextBlockScore > 0
                     && state.score >= s_cfg->nextBlockScore;
    bool showNext3 = showNext2 && state.score >= s_cfg->nextBlockScore * 2;
    if (showNext2) drawNextPreview(renderer, RECT_NEXT2, state.nextBlock2);
    if (showNext3) drawNextPreview(renderer, RECT_NEXT3, state.nextBlock3);
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

    char scoreLine[40];
    SDL_snprintf(scoreLine, sizeof(scoreLine), "Current score: %d", state.score);
    drawWrappedText(renderer, scoreLine,
                    POPUP_BG.x + 15, POPUP_BG.y + 95, 25, 2);

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

// gamecore-wasm-shutdown
// Man hinh shutdown cho WASM: hien nut RELOAD giua man hinh.
// FIX: hint text duoc chia 2 dong ngan <= 270px thay vi 1 dong dai tran vien.
//   Dong 1: "Press F5 or click RELOAD"   -- 25 chars x 8px = 200px (OK)
//   Dong 2: "to start a new game"        -- 19 chars x 8px = 152px (OK)
// Khoang cach dong: 14px (dong nhat voi cac text khac trong game).
static void drawWasmShutdownScreen(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color btnBg = state.reloadHover ? SDL_Color{ 90, 130, 200, 255}
                                        : SDL_Color{ 60, 100, 170, 255};
    SDL_SetRenderDrawColor(renderer, btnBg.r, btnBg.g, btnBg.b, 255);
    SDL_RenderFillRect(renderer, &RELOAD_BTN);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &RELOAD_BTN);

    const char* label = "RELOAD";
    int ll = (int)SDL_strlen(label);
    SDL_RenderDebugText(renderer,
                        RELOAD_BTN.x + (RELOAD_BTN.w - ll * 8.0f) / 2.0f,
                        RELOAD_BTN.y + (RELOAD_BTN.h - 8.0f) / 2.0f,
                        label);

    // Hint 2 dong -- moi dong <= 270px, khong bi tran vien
    float hintY = RELOAD_BTN.y + RELOAD_BTN.h + 16.0f;
    const char* hint1 = "Press F5 or click RELOAD";   // 25 chars = 200px
    const char* hint2 = "to start a new game";         // 19 chars = 152px
    int h1l = (int)SDL_strlen(hint1);
    int h2l = (int)SDL_strlen(hint2);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer,
                        (CORE_SCREEN_WIDTH - h1l * 8.0f) / 2.0f,
                        hintY, hint1);
    SDL_RenderDebugText(renderer,
                        (CORE_SCREEN_WIDTH - h2l * 8.0f) / 2.0f,
                        hintY + 14.0f, hint2);
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

    if (state.pendingClear) {
        SDL_Color fc = state.flashOn ? SDL_Color{255, 255, 255, 255}
                                     : SDL_Color{20, 20, 20, 255};
        SDL_SetRenderDrawColor(renderer, fc.r, fc.g, fc.b, 255);
        for (int ri = 0; ri < state.clearRowCount; ri++) {
            for (int c = 0; c < BOARD_COLS; c++) {
                SDL_FRect cell = {
                    (float)(c * BLOCK_SIZE + BLOCK_PAD),
                    (float)(state.clearRows[ri] * BLOCK_SIZE + BLOCK_PAD),
                    (float)(BLOCK_SIZE - 2 * BLOCK_PAD),
                    (float)(BLOCK_SIZE - 2 * BLOCK_PAD)
                };
                SDL_RenderFillRect(renderer, &cell);
            }
        }
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
static void onAction(GameState& state, SDL_Keycode keyEquiv) {
    if (state.showQuitPopup || state.isPaused || state.isGameOver) return;
    if (keyEquiv == SDLK_SPACE) { state.softDrop = true; return; }
    applyMoveOrRotate(state, keyEquiv);
}

// Issue 2.5: record save before quit is handled at the call-site in the
// event loop (gameOverRecorded guard), so handleQuitAction itself only
// sets the shutdown/exit flags. This keeps the function state-mutation-only
// and avoids needing the elapsed-time calculation here.
static void handleQuitAction(GameState& state) {
#ifdef __EMSCRIPTEN__
    state.wasmShutdown = true;
    state.showQuitPopup = false;
    state.isPaused = true;
#else
    state.exitCode = 0;
    state.wasmShutdown = false;
#endif
}

// gamecore-xu-ly-roi-03
// [D.6] Signature accepts SettingsConfig& -- volume + color palette
// chosen in gameConsole. Currently unused; V2 will consume.
int runGameCore(SDL_Window* window, SDL_Renderer* renderer,
                const SettingsConfig& cfg) {
    (void)window;
    s_cfg = &cfg;                          // [C6/C7] module cfg pointer
    coreOpenBgm(cfg.volume);               // [C1]
    std::srand((unsigned)std::time(nullptr));

    GameState state;
    state.currentBlock = spawnBlock();
    state.nextBlock    = spawnBlock();
    state.nextBlock2   = spawnBlock();
    state.nextBlock3   = spawnBlock();
    applyTableMatrix(state, cfg.tableMatrix); // [C7]

    SDL_Event event;
    Uint32 lastFallTime = SDL_GetTicks();

    bool quitRequested    = false;
    bool gameOverRecorded = false;
    // Swipe gesture state (touch / mobile)
    bool  swipeActive    = false;
    float swipeStartX    = 0.0f;
    float swipeStartY    = 0.0f;
    bool  swipeOnBlock   = false;   // true = finger started on falling block
    const float SWIPE_THRESHOLD = 15.0f;

    while (true) {
        Uint32 nowMs = SDL_GetTicks();

        if (state.gameStartTime == 0) {
            state.gameStartTime = nowMs;
            state.wasRunning    = true;
        }

        state.softDrop = false;

        if (state.pendingClear && !state.isPaused && !state.wasmShutdown) {
            if (nowMs - state.flashTimer > 80) {
                state.flashTick++;
                state.flashOn    = (state.flashTick % 2 == 0);
                state.flashTimer = nowMs;
                if (state.flashTick >= 6) {
                    finishLineClear(state);
                    lastFallTime = SDL_GetTicks();
                }
            }
        }

        if (state.isGameOver && !gameOverRecorded) {
            Uint32 _elapsed = state.pauseStartTime > 0
                ? state.pauseStartTime - state.gameStartTime - state.totalPausedMs
                : nowMs - state.gameStartTime - state.totalPausedMs;
            onGameOver(state, _elapsed);
            gameOverRecorded = true;
        }

        while (SDL_PollEvent(&event)) {
            if (state.wasmShutdown) {
                if (event.type == SDL_EVENT_QUIT) {
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
                continue;
            }

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
                    } else if (hitTest(POPUP_RESTART, mx, my)) {
                        // Issue 2.5: save record for this session before resetting.
                        // Record is written even when score==0 (valid retry entry).
                        if (!gameOverRecorded) {
                            Uint32 _now = SDL_GetTicks();
                            Uint32 _elapsed = state.pauseStartTime > 0
                                ? state.pauseStartTime - state.gameStartTime - state.totalPausedMs
                                : _now - state.gameStartTime - state.totalPausedMs;
                            onGameOver(state, _elapsed);
                        }
                        state.retryCount++;           // [C8]
                        resetGame(state);
                        gameOverRecorded = false;     // allow re-recording next game-over
                        lastFallTime = SDL_GetTicks();
                    } else if (hitTest(POPUP_CONSOLE, mx, my)) {
                        // Issue 2.5: save record before leaving to Console.
                        if (!gameOverRecorded) {
                            Uint32 _now = SDL_GetTicks();
                            Uint32 _elapsed = state.pauseStartTime > 0
                                ? state.pauseStartTime - state.gameStartTime - state.totalPausedMs
                                : _now - state.gameStartTime - state.totalPausedMs;
                            onGameOver(state, _elapsed);
                            gameOverRecorded = true;
                        }
                        state.exitCode = 2;
                        quitRequested = true;
                        break;
                    } else if (hitTest(POPUP_QUIT, mx, my)) {
                        // Issue 2.5: save record before quit.
                        if (!gameOverRecorded) {
                            Uint32 _now = SDL_GetTicks();
                            Uint32 _elapsed = state.pauseStartTime > 0
                                ? state.pauseStartTime - state.gameStartTime - state.totalPausedMs
                                : _now - state.gameStartTime - state.totalPausedMs;
                            onGameOver(state, _elapsed);
                            gameOverRecorded = true;
                        }
                        handleQuitAction(state);
                        if (!state.wasmShutdown) {
                            quitRequested = true;
                            break;
                        }
                    }
                } else {
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
                    // Board-zone swipe start: record only when game is live and touch is left of sidebar
                    if (mx < (float)BOARD_W && !state.showQuitPopup &&
                        !state.isPaused && !state.isGameOver) {
                        swipeActive  = true;
                        swipeStartX  = mx;
                        swipeStartY  = my;
                        swipeOnBlock = isTouchOnFallingBlock(state, mx, my);
                    }
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                event.button.button == SDL_BUTTON_LEFT) {
                // Swipe gesture recognition — fires on finger lift
                if (swipeActive) {
                    float dx  = event.button.x - swipeStartX;
                    float dy  = event.button.y - swipeStartY;
                    float adx = (dx < 0) ? -dx : dx;
                    float ady = (dy < 0) ? -dy : dy;
                    if (swipeOnBlock) {
                        // Finger started on falling block -> horizontal swipe = move
                        if (adx > SWIPE_THRESHOLD && adx > ady)
                            onAction(state, dx < 0 ? SDLK_LEFT : SDLK_RIGHT);
                    } else {
                        // Finger started on board background -> vertical swipe = rotate
                        if (ady > SWIPE_THRESHOLD && ady > adx)
                            onAction(state, dy < 0 ? SDLK_UP : SDLK_DOWN);
                    }
                    swipeActive = false;
                }
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

        if (state.wasmShutdown) {
            drawWasmShutdownScreen(renderer, state);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
            continue;
        }

        const bool* keys = SDL_GetKeyboardState(NULL);
        bool active = !state.isPaused && !state.showQuitPopup && !state.isGameOver;
        if (active && (keys[SDL_SCANCODE_SPACE] || state.speedHeld)) {
            state.softDrop = true;
        }

        bool nowRunning = active;
        if (state.wasRunning && !nowRunning) {
            state.pauseStartTime = nowMs;
        }
        if (!state.wasRunning && nowRunning) {
            state.totalPausedMs += nowMs - state.pauseStartTime;
        }
        state.wasRunning = nowRunning;

        Uint32 elapsedMs;
        if (nowRunning) {
            elapsedMs = nowMs - state.gameStartTime - state.totalPausedMs;
        } else {
            elapsedMs = state.pauseStartTime - state.gameStartTime - state.totalPausedMs;
        }

        if (active && !state.pendingClear) {   // [C4] no fall while flashing
            Uint32 currentTime = SDL_GetTicks();
            Uint32 fi       = getFallInterval(state.score); // [C2]
            Uint32 interval = state.softDrop ? fi / 5 : fi;
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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        renderBoard(renderer, state);
        drawSidebar(renderer, state, keys, elapsedMs);
        if (state.showQuitPopup) drawQuitPopup(renderer, state, elapsedMs);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    coreCloseBgm();   // [C1]
    return state.exitCode;
}

// integration/v2
// V2 additions verified (see taskCore.md Task 2.10):
//   [C1] BGM stub — SDL_AudioStream silent stream; cfg.volume gain applied on entry
//   [C2] Dynamic fall speed — getFallInterval(score) 5-step 500→100ms
//   [C3] 3-block queue (nextBlock/2/3); NEXT-2/3 gated by cfg.nextBlockScore
//   [C4] Flash-clear — 6 ticks×80ms white/dark animation; fall blocked during flash
//   [C5] n² combo scoring; cap 99999
//   [C6] Color palette from cfg.colorEnabled[7] via PALETTE_TO_COLOR[7]
//   [C7] tableMatrix pre-population via applyTableMatrix() inside resetGame()
//   [C8] onGameOver(): dbInsertRecord → dbUpsertStoryProgress → cascade unlock
//   [C9] DB ownership guard — coreOpenedDb = !dbIsOpen() (B3)
//        Only calls dbClose() when Core opened the connection
//   Story label "S{id}-C{id}" overlay on sidebar NEXT-1 slot (slot 4)
//   DB lifecycle: Core opens/closes per onGameOver() transaction only;
//                 Console owns the persistent connection between sessions
#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Core \xC2\xA9 - Standalone",
                                          CORE_SCREEN_WIDTH, CORE_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SettingsConfig cfg; // [D.6] standalone owns its own defaults
    runGameCore(window, renderer, cfg);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif