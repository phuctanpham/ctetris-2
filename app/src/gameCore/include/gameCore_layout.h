#pragma once
// gamecore-tao-giao-dien-169-00
// Khung hinh chuan ty le 9:16 cho gameCore (270 x 480)
const int CORE_SCREEN_WIDTH  = 270;
const int CORE_SCREEN_HEIGHT = 480;

// Cau hinh ban co Tetris: sat mep trai
const int BOARD_COLS  = 10;
const int BOARD_ROWS  = 20;
const int BLOCK_SIZE  = 24;     // 10*24=240, 20*24=480
const int BLOCK_PAD   = 1;      // vien den thut vao trong moi o
const int BOARD_W     = BOARD_COLS * BLOCK_SIZE; // 240
const int BOARD_H     = BOARD_ROWS * BLOCK_SIZE; // 480

// Cot dieu khien sat mep phai
const int SIDEBAR_X   = BOARD_W;                       // 240
const int SIDEBAR_W   = CORE_SCREEN_WIDTH - BOARD_W;   // 30

// Layout 7 component (3*60 + 4*75 = 480)
const int COMP_QUIT_Y   = 0;   const int COMP_QUIT_H   = 60;
const int COMP_PAUSE_Y  = 60;  const int COMP_PAUSE_H  = 60;
const int COMP_SCORE_Y  = 120; const int COMP_SCORE_H  = 60;
const int COMP_NEXT1_Y  = 180; const int COMP_NEXT_H   = 75;
const int COMP_NEXT2_Y  = 255;
const int COMP_NEXT3_Y  = 330;
const int COMP_KEYPAD_Y = 405; const int COMP_KEYPAD_H = 75;

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
    bool softDrop      = false;
    int  exitCode      = 0;     // 0 = thoat, 2 = ve console
};