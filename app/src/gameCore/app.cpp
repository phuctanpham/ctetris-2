// phucpt: app/src/gameCore/app.cpp

// =========================
// integration/v1
// Điểm tích hợp: hàm runGameCore() dùng khi tích hợp với main.cpp
// Ported sang VRSFML:
// - sf::GraphicsContext nhận từ main()
// - SFML_GAME_LOOP(window) thay while(window.isOpen())
// - sf::base::Optional cho pollEvent
// - sf::Text(font, str, size) constructor tường minh
// =========================

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include <array>
#include <ctime>
#include <random>
#include <string>

#include "include/layout.h"

// =========================
// Cấu hình (Configuration)
// =========================
static constexpr int   BOARD_ROWS         = 20;
static constexpr int   BOARD_COLS         = 10;
static constexpr int   CELL_PX            = 48;
static constexpr int   CORE_WINDOW_HEIGHT = 960;

// =========================
// Mô hình dữ liệu (Model)
// =========================

// Toạ độ một ô trên lưới
struct Point { int x, y; };

// Kết quả trả về
enum class CoreResult { BackToConsole, Quit };

// Định nghĩa 7 loại khối Tetris (L,I,T,Z,S,J,O)
static constexpr std::array<std::array<int,4>, 7> FIGURES = {{
    {1,3,5,7}, // I
    {2,4,5,7}, // Z
    {3,5,4,6}, // S
    {3,5,4,7}, // T
    {2,3,5,7}, // L
    {3,5,7,6}, // J
    {2,3,4,5}, // O
}};

// Bảng màu cho từng loại khối
static constexpr std::array<sf::Color, 8> BLOCK_COLORS = {{
    sf::Color::Black,
    sf::Color::Cyan,
    sf::Color::Red,
    sf::Color::Green,
    sf::Color::Magenta,
    sf::Color(255,165,0),
    sf::Color::Blue,
    sf::Color::Yellow,
}};

// =========================
// Khối logic xếp hình (Logic)
// =========================

// Kiểm tra hợp lệ (không ra ngoài / không chồng lên ô đã chốt)
static bool checkValid(const std::array<Point,4>& pts,
                       const int board[BOARD_ROWS][BOARD_COLS]) {
    for (const auto& p : pts) {
        if (p.x < 0 || p.x >= BOARD_COLS || p.y >= BOARD_ROWS) return false;
        if (p.y >= 0 && board[p.y][p.x] != 0)                   return false;
    }
    return true;
}

// Sinh khối mới tại đỉnh lưới
static void spawnBlock(int type, std::array<Point,4>& pts) {
    for (int i = 0; i < 4; ++i) {
        pts[i].x = FIGURES[type][i] % 2 + 4;
        pts[i].y = FIGURES[type][i] / 2 - 1;
    }
}

// Xoá các hàng đầy; trả về số hàng đã xoá
static int clearLines(int board[BOARD_ROWS][BOARD_COLS]) {
    int cleared = 0, k = BOARD_ROWS - 1;
    for (int i = BOARD_ROWS - 1; i > 0; --i) {
        int cnt = 0;
        for (int j = 0; j < BOARD_COLS; ++j) { board[k][j]=board[i][j]; if(board[i][j])++cnt; }
        if (cnt < BOARD_COLS) --k; else ++cleared;
    }
    return cleared;
}

// =========================
// Khối tiện ích view (View Helpers)
// =========================
static bool loadSystemFont(sf::Font& font) {
    const std::array<const char*, 5> paths = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Verdana.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    };
    for (const char* p : paths) {
        // VRSFML: openFromFile trả base::Optional
        auto opt = sf::Font::openFromFile(p);
        if (opt.hasValue()) { font = std::move(opt.value()); return true; }
    }
    return false;
}

// =========================
// Hàm chạy gameCore (Logic chính / Integration Entry Point)
// =========================
CoreResult runGameCore(sf::GraphicsContext& gCtx) {
    (void)gCtx;

    // =========================
    // gamecore-tao-giao-dien-169-00
    // Tạo cửa sổ tỷ lệ 9:16 — VRSFML designated initializer
    // =========================
    auto window = layout::create916Window(CORE_WINDOW_HEIGHT, "ctetris — Core");
    window.setFramerateLimit(60);

    const sf::Vector2u wSize   = window.getSize();
    const float        offX    = (static_cast<float>(wSize.x) - BOARD_COLS*CELL_PX) / 2.f;
    const float        offY    = (static_cast<float>(wSize.y) - BOARD_ROWS*CELL_PX) / 2.f;
    const float        uiScale = static_cast<float>(wSize.y) / 960.f;

    sf::Font font;
    const bool hasFont = loadSystemFont(font);

    // =========================
    // gamecore-tao-cac-khoi-xep-hinh-LITZO-01
    // Khởi tạo lưới và trạng thái
    // =========================
    int board[BOARD_ROWS][BOARD_COLS] = {};

    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    auto rType  = [&]{ return static_cast<int>(rng() % 7); };
    auto rColor = [](int t){ return t + 1; };

    // =========================
    // gamecore-do-mau-02
    // Xác định loại và màu khối hiện tại / tiếp theo
    // =========================
    int nextType = rType(), nextColor = rColor(nextType);
    int curType  = rType(), curColor  = rColor(curType);
    std::array<Point,4> cur{};
    spawnBlock(curType, cur);

    // =========================
    // gamecore-xu-ly-roi-03
    // Biến trạng thái rơi
    // =========================
    float     timer = 0.f, delay = 0.3f;
    int       score = 0;
    bool      isGameOver = false;
    CoreResult result    = CoreResult::Quit;
    sf::Clock clock;

    sf::RectangleShape gridCell({static_cast<float>(CELL_PX), static_cast<float>(CELL_PX)});
    gridCell.setFillColor(sf::Color::Transparent);
    gridCell.setOutlineThickness(-1.f);
    gridCell.setOutlineColor(sf::Color(255,255,255,30));

    sf::RectangleShape blockCell({static_cast<float>(CELL_PX), static_cast<float>(CELL_PX)});
    blockCell.setOutlineThickness(-2.f);
    blockCell.setOutlineColor(sf::Color(255,255,255,80));

    // =========================
    // Vòng lặp chính — VRSFML SFML_GAME_LOOP
    // =========================
    SFML_GAME_LOOP(window) {
        const float dt = clock.restart().asSeconds();
        timer += dt;

        int  dx = 0;
        bool rotate = false;

        // =========================
        // gamecore-xu-ly-phim-04
        // Xử lý bàn phím — VRSFML: sf::base::Optional từ pollEvent
        // =========================
        while (const sf::base::Optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) { window.close(); return CoreResult::Quit; }

            if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Escape) {
                    result = CoreResult::BackToConsole;
                    window.close();
                    return result;
                }
                if (isGameOver && kp->code == sf::Keyboard::Key::R) {
                    for (int i=0;i<BOARD_ROWS;++i) for(int j=0;j<BOARD_COLS;++j) board[i][j]=0;
                    score=0; isGameOver=false;
                    curType=rType(); curColor=rColor(curType); spawnBlock(curType,cur);
                    nextType=rType(); nextColor=rColor(nextType);
                }
                if (!isGameOver) {
                    if (kp->code==sf::Keyboard::Key::Up  ||kp->code==sf::Keyboard::Key::W) rotate=true;
                    if (kp->code==sf::Keyboard::Key::Left||kp->code==sf::Keyboard::Key::A) dx=-1;
                    if (kp->code==sf::Keyboard::Key::Right||kp->code==sf::Keyboard::Key::D) dx=1;
                }
            }
        }

        // Phím giữ để tăng tốc rơi
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            delay = 0.05f;
        else delay = 0.3f;

        if (!isGameOver) {
            // Di chuyển ngang
            if (dx) { auto t=cur; for(auto&p:t)p.x+=dx; if(checkValid(t,board))cur=t; }

            // Xoay khối theo chiều kim đồng hồ
            if (rotate) {
                auto t=cur; Point piv=cur[1];
                for(auto&p:t){int ddx=p.y-piv.y,ddy=p.x-piv.x;p.x=piv.x-ddx;p.y=piv.y+ddy;}
                if(checkValid(t,board))cur=t;
            }

            // =========================
            // gamecore-xu-ly-cham-05
            // Chạm đất — gắn khối vào lưới
            // =========================
            if (timer > delay) {
                auto t=cur; for(auto&p:t)p.y+=1;
                if (checkValid(t,board)) { cur=t; }
                else {
                    for(const auto&p:cur) { if(p.y<0){isGameOver=true;break;} board[p.y][p.x]=curColor; }
                    if (!isGameOver) {
                        // =========================
                        // gamecore-xu-ly-xoa-dong-06
                        // Xoá hàng đầy
                        // =========================
                        int lines = clearLines(board);
                        // =========================
                        // gamecore-tinh-diem-07
                        // Tính điểm
                        // =========================
                        if (lines > 0) score += lines * 100;
                        curType=nextType; curColor=nextColor; spawnBlock(curType,cur);
                        if (!checkValid(cur,board)) isGameOver=true;
                        nextType=rType(); nextColor=rColor(nextType);
                    }
                }
                timer=0.f;
            }
        }

        // =========================
        // Vẽ giao diện (View)
        // =========================
        window.clear(sf::Color(28,28,36));

        // Lưới nền
        for(int i=0;i<BOARD_ROWS;++i) for(int j=0;j<BOARD_COLS;++j) {
            gridCell.setPosition({offX+j*CELL_PX, offY+i*CELL_PX});
            window.draw(gridCell);
        }
        // Ô đã chốt
        for(int i=0;i<BOARD_ROWS;++i) for(int j=0;j<BOARD_COLS;++j) {
            if(!board[i][j]) continue;
            blockCell.setFillColor(BLOCK_COLORS[board[i][j]]);
            blockCell.setPosition({offX+j*CELL_PX, offY+i*CELL_PX});
            window.draw(blockCell);
        }
        // Khối đang rơi
        for(const auto&p:cur) {
            if(p.y<0) continue;
            blockCell.setFillColor(BLOCK_COLORS[curColor]);
            blockCell.setPosition({offX+p.x*CELL_PX, offY+p.y*CELL_PX});
            window.draw(blockCell);
        }

        if (hasFont) {
            // VRSFML: sf::Text(font, string, charSize) — constructor tường minh
            sf::Text scoreT(font, "Score: "+std::to_string(score),
                            static_cast<unsigned>(22.f*uiScale));
            scoreT.setFillColor(sf::Color(220,240,255));
            scoreT.setPosition({offX, offY-32.f*uiScale});
            window.draw(scoreT);

            sf::Text escT(font, "ESC: Ve Console",
                          static_cast<unsigned>(16.f*uiScale));
            escT.setFillColor(sf::Color(180,190,210));
            escT.setPosition({offX, offY+BOARD_ROWS*CELL_PX+6.f});
            window.draw(escT);
        }

        // Màn hình Game Over
        if (isGameOver && hasFont) {
            sf::RectangleShape dark({static_cast<float>(wSize.x),static_cast<float>(wSize.y)});
            dark.setFillColor(sf::Color(0,0,0,160));
            window.draw(dark);
            sf::Text goT(font,
                "GAME OVER\nDiem: "+std::to_string(score)+"\n\n[R] Choi lai   [ESC] Ve Console",
                static_cast<unsigned>(36.f*uiScale));
            goT.setFillColor(sf::Color(255,80,80));
            const sf::FloatRect gb = goT.getLocalBounds();
            goT.setOrigin({gb.position.x+gb.size.x/2.f, gb.position.y+gb.size.y/2.f});
            goT.setPosition({static_cast<float>(wSize.x)/2.f,
                             static_cast<float>(wSize.y)/2.f});
            window.draw(goT);
        }
        window.display();
    }
    return result;
}

// =========================
// Điểm vào standalone
// =========================
#ifdef STANDALONE_GAMECORE
int main() {
    auto gCtx = sf::GraphicsContext::create().value();
    runGameCore(gCtx);
    return 0;
}
#endif