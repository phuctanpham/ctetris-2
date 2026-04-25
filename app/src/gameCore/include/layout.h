#pragma once
// gamecore-tao-giao-dien-169-00
const int CORE_SCREEN_WIDTH = 360;
const int CORE_SCREEN_HEIGHT = 960;
const int BOARD_COLS = 10;
const int BOARD_ROWS = 20;
const int BLOCK_SIZE = 36; 

struct Point { int x, y; };

struct Tetromino {
    Point blocks[4];
    int colorID;
    int x, y;
};

struct GameState {
    int board[20][10] = {0};
    Tetromino currentBlock;
    int score = 0;
    bool isGameOver = false;
};