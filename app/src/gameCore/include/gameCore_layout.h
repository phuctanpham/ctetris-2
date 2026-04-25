#pragma once
// gamecore-tao-giao-dien-169-00
// Khung hinh chuan ty le 9:16 cho gameCore
const int CORE_SCREEN_WIDTH  = 360;
const int CORE_SCREEN_HEIGHT = 960;

// Cau hinh ban co Tetris
const int BOARD_COLS = 10;
const int BOARD_ROWS = 20;
const int BLOCK_SIZE = 36;

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
    int score = 0;
    bool isGameOver = false;
};