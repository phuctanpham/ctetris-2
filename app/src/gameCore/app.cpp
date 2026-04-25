// integration/v1
// File chinh cua gameCore: vong lap chinh cua tro choi Tetris
#include "gameCore_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <ctime>

// Bang mau theo colorID (1..6); index 0 la o trong
const SDL_Color COLORS[] = {
    {  0,   0,   0, 255}, // 0: trong
    {  0,   0, 255, 255}, // 1: blue
    {255,   0,   0, 255}, // 2: red
    {  0, 255,   0, 255}, // 3: green
    {255, 255,   0, 255}, // 4: yellow
    {255, 165,   0, 255}, // 5: orange
    {128,   0, 128, 255}  // 6: purple
};

// gamecore-tao-cac-khoi-xep-hinh-LITZO-01
// Dinh nghia 6 shape co ban cua Tetris (L, I, T, S, Z, O)
// Toa do trong he local cua piece, cac offset duoc cong vao (t.x, t.y)
const Point SHAPE_L[4] = {{0,0}, {0,1}, {0,2}, {1,2}}; // L
const Point SHAPE_I[4] = {{0,0}, {1,0}, {2,0}, {3,0}}; // I
const Point SHAPE_T[4] = {{0,0}, {1,0}, {2,0}, {1,1}}; // T
const Point SHAPE_S[4] = {{1,0}, {2,0}, {0,1}, {1,1}}; // S (Z trai)
const Point SHAPE_Z[4] = {{0,0}, {1,0}, {1,1}, {2,1}}; // Z (Z phai)
const Point SHAPE_O[4] = {{0,0}, {1,0}, {0,1}, {1,1}}; // O

// Pool de random pick 1 shape moi khi spawn
const Point* const SHAPES[] = { SHAPE_L, SHAPE_I, SHAPE_T, SHAPE_S, SHAPE_Z, SHAPE_O };
const int NUM_SHAPES = (int)(sizeof(SHAPES) / sizeof(SHAPES[0]));

// gamecore-do-mau-02
// Sinh khoi moi: random shape (1/6) + random mau (1..6)
Tetromino spawnBlock() {
    Tetromino t;
    int idx = std::rand() % NUM_SHAPES;
    for (int i = 0; i < 4; i++) t.blocks[i] = SHAPES[idx][i];
    t.colorID = std::rand() % 6 + 1;
    t.x = BOARD_COLS / 2 - 1;
    t.y = 0;
    return t;
}

// Kiem tra va cham voi tuong / san / khoi da co
// Cho phep ny < 0 (piece dang spawn phia tren ban) - khong tinh la va cham
bool checkCollision(const GameState& state, const Tetromino& t) {
    for (int i = 0; i < 4; i++) {
        int nx = t.x + t.blocks[i].x;
        int ny = t.y + t.blocks[i].y;
        if (nx < 0 || nx >= BOARD_COLS || ny >= BOARD_ROWS) return true;
        if (ny >= 0 && state.board[ny][nx] != 0) return true;
    }
    return false;
}

// Helper xoay piece quanh pivot (1,1) - hop voi shape 3x3 va O
// clockwise = true  -> xoay theo chieu kim dong ho 90 do
// clockwise = false -> xoay nguoc chieu kim dong ho 90 do
// Cong thuc trong screen coords (y huong xuong):
//   CW : (x, y) -> (-y,  x)
//   CCW: (x, y) -> ( y, -x)
Tetromino rotated(const Tetromino& t, bool clockwise) {
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
// Mapping theo yeu cau user:
//   LEFT  / A : sang trai
//   RIGHT / D : sang phai
//   UP    / W : xoay nguoc chieu kim dong ho (CCW)
//   DOWN  / S : xoay theo chieu kim dong ho  (CW)
void handleInput(GameState& state, SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        Tetromino nextT = state.currentBlock;
        switch (event.key.key) {
            case SDLK_LEFT:
            case SDLK_A: nextT.x -= 1; break;
            case SDLK_RIGHT:
            case SDLK_D: nextT.x += 1; break;
            case SDLK_UP:
            case SDLK_W: nextT = rotated(state.currentBlock, false); break; // CCW
            case SDLK_DOWN:
            case SDLK_S: nextT = rotated(state.currentBlock, true);  break; // CW
            default: return;
        }
        // Chi commit neu khong va cham; neu va cham thi bo qua (khong wall-kick o v1)
        if (!checkCollision(state, nextT)) state.currentBlock = nextT;
    }
}

// gamecore-xu-ly-xoa-dong-06
// gamecore-tinh-diem-07
// Xoa cac dong da day, dich phia tren xuong, cong diem
void clearLines(GameState& state) {
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
            r++; // duyet lai dong vua dich xuong
        }
    }
    state.score += linesCleared * 100;
}

// gamecore-xu-ly-cham-05
// Khoa khoi vao ban + sinh khoi moi; neu khoi moi va cham ngay -> game over
void lockBlock(GameState& state) {
    for (int i = 0; i < 4; i++) {
        int nx = state.currentBlock.x + state.currentBlock.blocks[i].x;
        int ny = state.currentBlock.y + state.currentBlock.blocks[i].y;
        if (ny >= 0 && ny < BOARD_ROWS && nx >= 0 && nx < BOARD_COLS) {
            state.board[ny][nx] = state.currentBlock.colorID;
        }
    }
    clearLines(state);
    state.currentBlock = spawnBlock();
    if (checkCollision(state, state.currentBlock)) state.isGameOver = true;
}

// Render ban co + khoi hien tai
void renderGame(SDL_Renderer* renderer, const GameState& state) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);
    int offsetX = (CORE_SCREEN_WIDTH  - BOARD_COLS * BLOCK_SIZE) / 2;
    int offsetY = (CORE_SCREEN_HEIGHT - BOARD_ROWS * BLOCK_SIZE) / 2;

    // Ve cac o da co tren ban
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            if (state.board[r][c] != 0) {
                SDL_Color col = COLORS[state.board[r][c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_FRect rect = { (float)(offsetX + c * BLOCK_SIZE),
                                   (float)(offsetY + r * BLOCK_SIZE),
                                   (float)BLOCK_SIZE, (float)BLOCK_SIZE };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    // Ve khoi dang roi
    SDL_Color col = COLORS[state.currentBlock.colorID];
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
    for (int i = 0; i < 4; i++) {
        int nx = state.currentBlock.x + state.currentBlock.blocks[i].x;
        int ny = state.currentBlock.y + state.currentBlock.blocks[i].y;
        SDL_FRect rect = { (float)(offsetX + nx * BLOCK_SIZE),
                           (float)(offsetY + ny * BLOCK_SIZE),
                           (float)BLOCK_SIZE, (float)BLOCK_SIZE };
        SDL_RenderFillRect(renderer, &rect);
    }
}

// gamecore-xu-ly-roi-03
// Vong lap chinh: tu dong roi moi 500ms
int runGameCore(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    // Seed RNG mot lan duy nhat de moi run ra dau khac nhau
    std::srand((unsigned)std::time(nullptr));

    GameState state;
    state.currentBlock = spawnBlock();
    SDL_Event event;
    Uint32 lastFallTime = SDL_GetTicks();

    while (!state.isGameOver) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) std::exit(0);
            handleInput(state, event);
        }

        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastFallTime > 500) {
            Tetromino nextT = state.currentBlock;
            nextT.y += 1;
            if (checkCollision(state, nextT)) {
                lockBlock(state);
            } else {
                state.currentBlock = nextT;
            }
            lastFallTime = currentTime;
        }

        renderGame(renderer, state);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return 0;
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