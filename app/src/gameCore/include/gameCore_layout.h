#pragma once
// gamecore-tao-giao-dien-169-00
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
    int  score = 0;
    bool isGameOver    = false;
    bool isPaused      = false;
    bool showQuitPopup = false;
    bool softDrop      = false;     // SPACE / giu nut speed booster
    bool speedHeld     = false;     // chuot dang giu nut speed booster
    int  exitCode      = 0;
};