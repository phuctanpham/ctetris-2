#pragma once
// gamecore-tao-giao-dien-169-00
#include <SDL3/SDL.h>  // Lay kieu Uint32 cho cac truong timer ben duoi

const int CORE_SCREEN_WIDTH  = 270;
const int CORE_SCREEN_HEIGHT = 480;

// Cau hinh ban co Tetris: sat mep trai
const int BOARD_COLS  = 10;
const int BOARD_ROWS  = 20;
const int BLOCK_SIZE  = 24;
const int BLOCK_PAD   = 1;
const int BOARD_W     = BOARD_COLS * BLOCK_SIZE;       // 240
const int BOARD_H     = BOARD_ROWS * BLOCK_SIZE;       // 480

// Cot dieu khien sat mep phai
const int SIDEBAR_X   = BOARD_W;                       // 240
const int SIDEBAR_W   = CORE_SCREEN_WIDTH - BOARD_W;   // 30

// 12 component dong nhat 30x40 (12 * 40 = 480)
const int COMP_H = 40;

// Cac struct dung chung cho gameCore
struct Point { int x, y; };

struct Tetromino {
    Point blocks[4];
    int colorID;
    int x, y;
};

struct GameState {
    int board[BOARD_ROWS][BOARD_COLS] = {0};
    Tetromino currentBlock;
    Tetromino nextBlock;
    Tetromino nextBlock2;
    Tetromino nextBlock3;
    int  score = 0;
    bool isGameOver    = false;
    bool isPaused      = false;
    bool showQuitPopup = false;
    bool softDrop      = false;
    bool speedHeld     = false;
    int  exitCode      = 0;
    int  retryCount    = 0;

    bool pendingClear  = false;
    int  clearRowCount = 0;
    int  clearRows[4]  = {0, 0, 0, 0};
    int  flashTick     = 0;
    bool flashOn       = false;
    Uint32 flashTimer  = 0;

    Uint32 gameStartTime  = 0;
    Uint32 pauseStartTime = 0;
    Uint32 totalPausedMs  = 0;
    bool   wasRunning     = false;

    bool mouseHeldQuit     = false;
    bool mouseHeldPause    = false;
    bool mouseHeldArrUp    = false;
    bool mouseHeldArrDown  = false;
    bool mouseHeldArrLeft  = false;
    bool mouseHeldArrRight = false;

    bool wasmShutdown = false;
    bool reloadHover  = false;

    // [V3] gamecore-game-over-screen-24
    // Set by onGameOver() when session score exceeds local sync_Records max.
    // Shown as gold "* NEW RECORD" banner in quit popup; cleared on resetGame().
    bool isNewRecord  = false;
};
