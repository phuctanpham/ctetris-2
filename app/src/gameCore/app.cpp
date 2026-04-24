// =========================
// gamecore-tao-giao-dien-169-00
// Tạo cửa sổ tỷ lệ 9:16, đảm bảo chạy đúng trên mọi nền tảng
// =========================
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <random>
#include <ctime>
#include <string>
#include "include/layout.h"

constexpr int M = 20; // Height
constexpr int N = 10; // Width
constexpr int BLOCK_SIZE = 48;

int field[M][N] = {0};
struct Point { int x, y; } a[4], b[4];

// =========================
// gamecore-tao-cac-khoi-xep-hinh-LITZO-01
// Định nghĩa các khối L, I, T, Z, O
// =========================
int figures[7][4] = {
    {1,3,5,7}, // I
    {2,4,5,7}, // Z
    {3,5,4,6}, // S
    {3,5,4,7}, // T
    {2,3,5,7}, // L
    {3,5,7,6}, // J
    {2,3,4,5}  // O
};

// =========================
// gamecore-do-mau-02
// Đổ màu ngẫu nhiên cho các khối
// =========================
sf::Color colors[8] = {
    sf::Color::Black,
    sf::Color::Cyan,
    sf::Color::Red,
    sf::Color::Green,
    sf::Color::Magenta,
    sf::Color(255, 165, 0), // Orange
    sf::Color::Blue,
    sf::Color::Yellow
};

bool check() {
    for (int i = 0; i < 4; i++) {
        if (a[i].x < 0 || a[i].x >= N || a[i].y >= M) return false;
        else if (a[i].y >= 0 && field[a[i].y][a[i].x]) return false;
    }
    return true;
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    // =========================
    // gamecore-tao-giao-dien-169-00
    // =========================
    int winHeight = 960;
    auto window = layout::create916Window(winHeight);
    float offsetX = (window.getSize().x - N * BLOCK_SIZE) / 2.0f;
    float offsetY = (window.getSize().y - M * BLOCK_SIZE) / 2.0f;

    sf::RectangleShape gridCell({(float)BLOCK_SIZE, (float)BLOCK_SIZE});
    gridCell.setFillColor(sf::Color::Transparent);
    gridCell.setOutlineThickness(-1.f);
    gridCell.setOutlineColor(sf::Color(255,255,255,40));

    sf::RectangleShape blockCell({(float)BLOCK_SIZE, (float)BLOCK_SIZE});
    blockCell.setOutlineThickness(-1.f);
    blockCell.setOutlineColor(sf::Color(255,255,255,80));

    int score = 0;
    int colorNum = 1;
    int nextBlockType = rand() % 7;
    int nextColorNum = nextBlockType + 1;
    int dx = 0;
    bool rotate = false;
    float timer = 0, delay = 0.3f;
    bool isGameOver = false;
    sf::Clock clock;

    // =========================
    // gamecore-xu-ly-roi-03
    // Khối rơi tự do ở vị trí bất kỳ (on top)
    // =========================
    auto spawnNewBlock = [&]() {
        int n = nextBlockType;
        colorNum = nextColorNum;
        for (int i = 0; i < 4; i++) {
            a[i].x = figures[n][i] % 2 + 4;
            a[i].y = figures[n][i] / 2 - 1;
        }
        if (!check()) isGameOver = true;
        nextBlockType = rand() % 7;
        nextColorNum = nextBlockType + 1;
    };
    spawnNewBlock();

    // =========================
    // gamecore-xu-ly-phim-04
    // Điều khiển khối bằng mũi tên và WASD
    // =========================
    while (window.isOpen()) {
        float time = clock.restart().asSeconds();
        timer += time;
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (isGameOver && event.key.code == sf::KeyboardKey::R) {
                    for (int i = 0; i < M; i++) for (int j = 0; j < N; j++) field[i][j] = 0;
                    score = 0; isGameOver = false; spawnNewBlock();
                }
                if (!isGameOver) {
                    if (event.key.code == sf::KeyboardKey::Up || event.key.code == sf::KeyboardKey::W) rotate = true;
                    else if (event.key.code == sf::KeyboardKey::Left || event.key.code == sf::KeyboardKey::A) dx = -1;
                    else if (event.key.code == sf::KeyboardKey::Right || event.key.code == sf::KeyboardKey::D) dx = 1;
                }
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::KeyboardKey::Down) || sf::Keyboard::isKeyPressed(sf::KeyboardKey::S)) delay = 0.05f;
        else delay = 0.3f;

        if (!isGameOver) {
            // Di chuyển ngang
            if (dx != 0) {
                for (int i = 0; i < 4; i++) { b[i] = a[i]; a[i].x += dx; }
                if (!check()) for (int i = 0; i < 4; i++) a[i] = b[i];
                dx = 0;
            }
            // Xử lý xoay
            if (rotate) {
                Point p = a[1];
                for (int i = 0; i < 4; i++) {
                    b[i] = a[i];
                    int deltaX = a[i].y - p.y;
                    int deltaY = a[i].x - p.x;
                    a[i].x = p.x - deltaX;
                    a[i].y = p.y + deltaY;
                }
                if (!check()) for (int i = 0; i < 4; i++) a[i] = b[i];
                rotate = false;
            }
            // =========================
            // gamecore-xu-ly-cham-05
            // Hiệu ứng chạm và dừng rơi khối
            // =========================
            if (timer > delay) {
                for (int i = 0; i < 4; i++) { b[i] = a[i]; a[i].y += 1; }
                if (!check()) {
                    for (int i = 0; i < 4; i++) {
                        if (b[i].y >= 0) field[b[i].y][b[i].x] = colorNum;
                        else isGameOver = true;
                    }
                    if (!isGameOver) spawnNewBlock();
                }
                timer = 0;
            }
            // =========================
            // gamecore-xu-ly-xoa-dong-06
            // Hiệu ứng xóa dòng
            // =========================
            int linesCleared = 0;
            int k = M - 1;
            for (int i = M - 1; i > 0; i--) {
                int count = 0;
                for (int j = 0; j < N; j++) {
                    if (field[i][j]) count++;
                    field[k][j] = field[i][j];
                }
                if (count < N) k--;
                else linesCleared++;
            }
            // =========================
            // gamecore-tinh-diem-07
            // Tính điểm
            // =========================
            if (linesCleared >= 1) score += linesCleared * 100;
        }

        // =========================
        // Vẽ giao diện
        // =========================
        window.clear(sf::Color(40,40,40));
        // Vẽ lưới
        for (int i = 0; i < M; i++) for (int j = 0; j < N; j++) {
            gridCell.setPosition(offsetX + j * BLOCK_SIZE, offsetY + i * BLOCK_SIZE);
            window.draw(gridCell);
        }
        // Vẽ các khối đã chốt
        for (int i = 0; i < M; i++) for (int j = 0; j < N; j++) {
            if (field[i][j] == 0) continue;
            blockCell.setFillColor(colors[field[i][j]]);
            blockCell.setPosition(offsetX + j * BLOCK_SIZE, offsetY + i * BLOCK_SIZE);
            window.draw(blockCell);
        }
        // Vẽ khối đang rơi
        for (int i = 0; i < 4; i++) {
            blockCell.setFillColor(colors[colorNum]);
            blockCell.setPosition(offsetX + a[i].x * BLOCK_SIZE, offsetY + a[i].y * BLOCK_SIZE);
            window.draw(blockCell);
        }
        // Vẽ chữ GAME OVER
        if (isGameOver) {
            sf::Font font;
            if (font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
                sf::Text gameOverText;
                gameOverText.setFont(font);
                gameOverText.setString("GAME OVER\nPress R to Restart");
                gameOverText.setCharacterSize(48);
                gameOverText.setFillColor(sf::Color::Red);
                gameOverText.setPosition(offsetX + 20, offsetY + 200);
                window.draw(gameOverText);
            }
        }
        window.display();
    }
    return 0;
}
         {'S','S',' ',' '},
         {' ',' ',' ',' '}},
        {{' ',' ',' ',' '},
         {'Z','Z',' ',' '},
         {' ','Z','Z',' '},
         {' ',' ',' ',' '}},
        {{' ',' ',' ',' '},
         {'J',' ',' ',' '},
         {'J','J','J',' '},
         {' ',' ',' ',' '}},
        {{' ',' ',' ',' '},
         {' ',' ','L',' '},
         {'L','L','L',' '},
         {' ',' ',' ',' '}}
};
bool canMove(int dx, int dy){
    for (int i = 0; i < 4; i++ )
        for (int j = 0; j < 4; j++ )
            if (blocks[b][i][j] != ' ') {
                int xt = x + j + dx;
                int yt = y + i + dy;
                if (xt < 1 || xt >= W-1 || yt >= H-1 ) return false;
                if (board[yt][xt] != ' ') return false;
            }
    return true;
}
void block2Board(){
    for (int i = 0; i < 4; i++ )
        for (int j = 0; j < 4; j++ )
            if (blocks[b][i][j] != ' ')
                board[y+i][x+j] = blocks[b][i][j];
}
void boardDelBlock(){
    for (int i = 0; i < 4; i++ )
        for (int j = 0; j < 4; j++ )
            if (blocks[b][i][j] != ' ')
                board[y+i][x+j] = ' ';
}
void initBoard(){
    for (int i = 0 ; i < H ; i++)
        for (int j = 0 ; j < W ; j++)
            if (i == 0 || i == H-1 || j ==0 || j == W-1) board[i][j] = '#';
            else board[i][j] = ' ';
}
void draw(){
    system("cls");

    for (int i = 0 ; i < H ; i++, cout<<endl)
        for (int j = 0 ; j < W ; j++) cout<<board[i][j];
}
void removeLine(){
    int i,j;
    for (i = H-2 ; i > 0 ; i-- ){
        for (j = 0 ; j < W ; j++)
            if (board[i][j] == ' ') break;
        if (j == W){
            for (int ii = i ; ii > 0 ; ii--)
                for (int jj = 0; jj < W; jj++)
                    board[ii][jj] = board[ii-1][jj];
            i++;
            draw();
            _sleep(200);
        }
    }
}

int main()
{
    srand(time(0));
    x = 5; y = 0; b = rand() % 7;
    initBoard();

    while (1) {
        boardDelBlock();

        if (kbhit()) {
            int c = getch();
            
            // Xử lý phím mũi tên (thường bắt đầu bằng 0 hoặc 224)
            if (c == 0 || c == 224) {
                char arrow = getch(); // Lấy mã thứ hai
                if (arrow == 75 && canMove(-1, 0)) x--;      // Mũi tên TRÁI
                else if (arrow == 77 && canMove(1, 0)) x++; // Mũi tên PHẢI
                else if (arrow == 80 && canMove(0, 1)) y++; // Mũi tên XUỐNG
            } 
            else {
                // Xử lý các phím thường
                if (c == 'a' && canMove(-1, 0)) x--;
                if (c == 'd' && canMove(1, 0)) x++;
                if (c == 's' && canMove(0, 1)) y++; // Thêm 's' cho giống game hiện đại
                if (c == 'x' && canMove(0, 1)) y++;
                if (c == 'q') break;
            }
        }

        // Khối gạch tự động rơi xuống
        if (canMove(0, 1)) {
            y++;
        }
        else {
            block2Board();
            removeLine();
            x = 5; y = 0; b = rand() % 7;
            // Kiểm tra nếu khối mới sinh ra đã bị kẹt thì Game Over
            if (!canMove(0, 0)) {
                draw();
                cout << "GAME OVER!" << endl;
                break;
            }
        }

        block2Board();
        draw();
        _sleep(300); // Giảm xuống 300ms để game mượt hơn một chút
    }
    return 0;
}
