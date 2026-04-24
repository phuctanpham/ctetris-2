// phucpt: app/src/gameConsole/app.cpp

// =========================
// integration/v1
// Điểm tích hợp: hàm runGameConsole() dùng khi tích hợp với main.cpp
// Ported sang VRSFML:
// - sf::GraphicsContext + sf::AudioContext nhận từ main()
// - SFML_GAME_LOOP(window) thay while(window.isOpen())
// - sf::base::Optional cho pollEvent
// - sf::Font::openFromFile().value() thay loadFromFile
// - sf::SoundBuffer::loadFromSamples() API giữ tương tự SFML 3
// =========================
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "include/layout.h"

namespace fs = std::filesystem;

// =========================
// Mô hình dữ liệu (Model)
// =========================

// Một mục trong bảng điểm
struct BoardEntry {
    std::string user;
    int         score = 0;
    std::string time;
};

// Nút bấm UI
struct Button {
    sf::FloatRect bounds;
    std::string   label;
};

// Trạng thái popup
enum class PopupMode { None, Guide, Board };

// Kết quả trả về sau gameConsole
enum class ConsoleResult { StartGame, Quit };

// =========================
// Cấu hình (Configuration)
// =========================
static constexpr int CONSOLE_WINDOW_HEIGHT = 720;

// =========================
// Khối tiện ích môi trường (Environment Helpers)
// =========================

// Tải font hệ thống — VRSFML: sf::Font::openFromFile trả sf::base::Optional
static bool loadSystemFont(sf::Font& font) {
    const std::array<const char*, 6> paths = {
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Verdana.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    };
    for (const char* p : paths) {
        // VRSFML: openFromFile trả base::Optional<sf::Font>
        auto opt = sf::Font::openFromFile(p);
        if (opt.hasValue()) { font = std::move(opt.value()); return true; }
    }
    return false;
}

// Tải dữ liệu bảng điểm từ file JSON đơn giản
static std::vector<BoardEntry> loadBoardEntries(const fs::path& path) {
    std::vector<BoardEntry> entries;
    std::ifstream           in(path);
    if (!in) { std::cerr << "[gameConsole] Không mở được: " << path << '\n'; return entries; }
    std::ostringstream buf; buf << in.rdbuf();
    const std::string  content = buf.str();
    const std::regex   pat(
        "\\{\\s*\"user\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"score\"\\s*:\\s*(\\d+)"
        "\\s*,\\s*\"time\"\\s*:\\s*\"([^\"]+)\"\\s*\\}");
    for (std::sregex_iterator it(content.begin(), content.end(), pat), end; it != end; ++it) {
        BoardEntry e; e.user  = (*it)[1]; e.score = std::stoi((*it)[2]); e.time = (*it)[3];
        entries.push_back(std::move(e));
    }
    return entries;
}

// Xây dựng âm thanh nền loop từ mẫu tổng hợp — không cần file ngoài
static bool buildAmbientBuffer(sf::SoundBuffer& buf) {
    constexpr unsigned  sr  = 44100;
    constexpr float     dur = 4.f;
    constexpr std::size_t ch = 2;
    const std::size_t   n   = static_cast<std::size_t>(sr * dur * ch);
    std::vector<std::int16_t> s(n, 0);
    const float pi = 3.14159265358979323846f;
    for (std::size_t f = 0; f < n / ch; ++f) {
        float t  = static_cast<float>(f) / static_cast<float>(sr);
        float p  = 0.42f + 0.18f * std::sin(t * 0.85f * 2.f * pi);
        float v  = std::clamp(
            (0.55f * std::sin(t*110.f*2.f*pi) + 0.28f * std::sin(t*146.83f*2.f*pi)
             + 0.18f * std::sin(t*220.f*2.f*pi)) * p, -1.f, 1.f);
        std::int16_t sample = static_cast<std::int16_t>(v * 2400.f);
        s[f*2]=s[f*2+1]=sample;
    }
    const std::vector<sf::SoundChannel> chMap =
        {sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight};
    return buf.loadFromSamples(s.data(), static_cast<std::uint64_t>(n), 2, sr, chMap);
}

// =========================
// Khối view (View Helpers)
// =========================

// Texture nền sinh từ pixel — không cần file ảnh
static sf::Texture buildBackgroundTexture() {
    constexpr unsigned W = 540, H = 960;
    std::vector<std::uint8_t> px(W * H * 4, 255);
    for (unsigned y = 0; y < H; ++y) {
        float fy = static_cast<float>(y) / (H - 1);
        for (unsigned x = 0; x < W; ++x) {
            float fx  = static_cast<float>(x) / (W - 1);
            auto  idx = (static_cast<std::size_t>(y) * W + x) * 4;
            float wv  = 0.5f + 0.5f * std::sin(fx*9.f+fy*5.f);
            float gl  = 0.5f + 0.5f * std::cos((fx-.3f)*8.f+(fy-.45f)*6.f);
            px[idx]   = static_cast<std::uint8_t>(18+20*fy+65*wv+25*gl);
            px[idx+1] = static_cast<std::uint8_t>(30+28*fy+45*wv+18*gl);
            px[idx+2] = static_cast<std::uint8_t>(55+42*fy+95*wv+75*gl);
            if (((x*17+y*13)%211)==0||((x*11+y*19)%317)==0) px[idx]=px[idx+1]=px[idx+2]=250;
            px[idx+3]=255;
        }
    }
    sf::Image img(sf::Vector2u(W, H), px.data());
    sf::Texture tex(sf::Vector2u(W, H));
    tex.update(img);
    return tex;
}

// Vẽ văn bản căn giữa
static void drawTextCentered(sf::RenderTarget& rt, sf::Text t, const sf::Vector2f& pos) {
    const sf::FloatRect b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x/2.f, b.position.y + b.size.y/2.f});
    t.setPosition(pos);
    rt.draw(t);
}

// Vẽ nút bấm với hiệu ứng hover
static void drawButton(sf::RenderTarget& rt, const sf::Font& font,
                        const Button& btn, bool hovered, float scale) {
    sf::RectangleShape sh(btn.bounds.size);
    sh.setPosition(btn.bounds.position);
    sh.setFillColor(hovered ? sf::Color(50,104,180,235) : sf::Color(26,35,58,228));
    sh.setOutlineColor(hovered ? sf::Color(255,255,255,220) : sf::Color(190,214,255,145));
    sh.setOutlineThickness(2.f*scale);
    rt.draw(sh);
    sf::Text lbl(font, btn.label, static_cast<unsigned>(28.f*scale));
    lbl.setFillColor(sf::Color::White);
    drawTextCentered(rt, lbl,
        {btn.bounds.position.x+btn.bounds.size.x/2.f,
         btn.bounds.position.y+btn.bounds.size.y/2.f - 2.f*scale});
}

// Lớp phủ tối lightbox
static void drawOverlayPanel(sf::RenderTarget& rt, sf::Vector2u ws, float alpha) {
    sf::RectangleShape ov({static_cast<float>(ws.x), static_cast<float>(ws.y)});
    ov.setFillColor(sf::Color(0,0,0,static_cast<std::uint8_t>(alpha)));
    rt.draw(ov);
}

// =========================
// Khối logic scroll bảng điểm (Logic)
// =========================
static int clampBoardScroll(int d, int n, int vis) {
    return std::clamp(d, 0, std::max(0, n - vis));
}

// =========================
// Khối vẽ popup Guide (View)
// =========================
static void drawGuidePopup(sf::RenderTarget& rt, const sf::Font& font,
                            sf::Vector2u ws, const Button& closeBtn,
                            float scale, bool closeHov) {
    const float pw=ws.x*.74f, ph=ws.y*.70f, px=(ws.x-pw)/2.f, py=(ws.y-ph)/2.f;
    sf::RectangleShape panel({pw,ph});
    panel.setPosition({px,py});
    panel.setFillColor(sf::Color(18,23,44,245));
    panel.setOutlineColor(sf::Color(110,163,255,210));
    panel.setOutlineThickness(3.f*scale);
    rt.draw(panel);
    sf::Text title(font, "GUIDE", static_cast<unsigned>(36.f*scale));
    title.setFillColor(sf::Color(255,243,194));
    drawTextCentered(rt, title, {px+pw/2.f, py+48.f*scale});
    const std::array<const char*,4> lines = {
        "Rotate clockwise   : Up arrow or W",
        "Move left          : Left arrow or A",
        "Move right         : Right arrow or D",
        "Rotate counterwise : Down arrow or S",
    };
    float ly = py+115.f*scale;
    for (const char* ln : lines) {
        sf::Text t(font, ln, static_cast<unsigned>(24.f*scale));
        t.setFillColor(sf::Color::White);
        t.setPosition({px+40.f*scale, ly});
        rt.draw(t); ly += 56.f*scale;
    }
    drawButton(rt, font, closeBtn, closeHov, scale);
}

// =========================
// Khối vẽ popup Board (View)
// =========================
static void drawBoardPopup(sf::RenderTarget& rt, const sf::Font& font,
                            sf::Vector2u ws, const std::vector<BoardEntry>& entries,
                            int scroll, const Button& closeBtn,
                            float scale, bool closeHov) {
    const float pw=ws.x*.84f, ph=ws.y*.80f, px=(ws.x-pw)/2.f, py=(ws.y-ph)/2.f;
    sf::RectangleShape panel({pw,ph});
    panel.setPosition({px,py});
    panel.setFillColor(sf::Color(16,20,36,245));
    panel.setOutlineColor(sf::Color(120,180,255,220));
    panel.setOutlineThickness(3.f*scale);
    rt.draw(panel);
    sf::Text title(font, "BOARD", static_cast<unsigned>(36.f*scale));
    title.setFillColor(sf::Color(255,243,194));
    drawTextCentered(rt, title, {px+pw/2.f, py+44.f*scale});
    sf::Text hint(font, "Scroll: mouse wheel / Up Down keys",
                  static_cast<unsigned>(18.f*scale));
    hint.setFillColor(sf::Color(195,210,236));
    hint.setPosition({px+34.f*scale, py+82.f*scale});
    rt.draw(hint);
    constexpr int vis   = 7;
    const float listTop = py+126.f*scale, rowH=58.f*scale, rowGap=6.f*scale;
    const float lp=px+28.f*scale, rp=px+pw-28.f*scale;
    for (int row=0; row<vis; ++row) {
        const int idx = scroll+row;
        if (idx >= static_cast<int>(entries.size())) break;
        const float rowY = listTop + row*(rowH+rowGap);
        sf::RectangleShape rs({pw-56.f*scale, rowH});
        rs.setPosition({lp,rowY});
        rs.setFillColor(row%2==0?sf::Color(37,47,75,240):sf::Color(28,37,60,240));
        rs.setOutlineColor(sf::Color(110,155,226,150));
        rs.setOutlineThickness(1.5f*scale);
        rt.draw(rs);
        const BoardEntry& e = entries[idx];
        auto makeT = [&](const std::string& s, unsigned sz) {
            sf::Text t(font, s, sz); return t; };
        auto rankT  = makeT(std::to_string(idx+1), static_cast<unsigned>(22.f*scale));
        rankT.setFillColor(sf::Color(255,230,160));
        rankT.setPosition({lp+16.f*scale, rowY+15.f*scale});
        rt.draw(rankT);
        auto userT  = makeT(e.user, static_cast<unsigned>(22.f*scale));
        userT.setFillColor(sf::Color::White);
        userT.setPosition({lp+72.f*scale, rowY+15.f*scale});
        rt.draw(userT);
        auto scoreT = makeT(std::to_string(e.score), static_cast<unsigned>(22.f*scale));
        scoreT.setFillColor(sf::Color(120,255,180));
        const sf::FloatRect sb = scoreT.getLocalBounds();
        scoreT.setPosition({rp-sb.size.x-8.f*scale, rowY+15.f*scale});
        rt.draw(scoreT);
        auto timeT  = makeT(e.time, static_cast<unsigned>(18.f*scale));
        timeT.setFillColor(sf::Color(190,205,230));
        const sf::FloatRect tb = timeT.getLocalBounds();
        timeT.setPosition({rp-tb.size.x-8.f*scale, rowY+35.f*scale});
        rt.draw(timeT);
    }
    sf::Text footer(font, "Showing up to 7 rows; use row-by-row scroll",
                    static_cast<unsigned>(18.f*scale));
    footer.setFillColor(sf::Color(190,205,230));
    footer.setPosition({px+28.f*scale, py+ph-62.f*scale});
    rt.draw(footer);
    drawButton(rt, font, closeBtn, closeHov, scale);
}

// =========================
// Hàm chạy gameConsole (Logic chính / Integration Entry Point)
// =========================
ConsoleResult runGameConsole(sf::GraphicsContext& gCtx, sf::AudioContext& aCtx) {
    (void)gCtx; (void)aCtx;
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // =========================
    // gameconsole-tao-giao-dien-169-00
    // Tạo cửa sổ tỷ lệ 9:16
    // =========================
    auto window = layout::create916Window(CONSOLE_WINDOW_HEIGHT, "ctetris — Console");
    window.setFramerateLimit(60);

    const sf::Vector2u wSize   = window.getSize();
    const float        uiScale = static_cast<float>(wSize.y) / 720.f;

    // VRSFML: sf::Font dùng factory openFromFile trả base::Optional
    sf::Font font;
    const bool hasFont = loadSystemFont(font);
    if (!hasFont) std::cerr << "[gameConsole] Không tìm thấy font hệ thống.\n";

    // =========================
    // gameconsole-chen-backgound-01
    // Background texture full-fit
    // =========================
    sf::Texture bgTex = buildBackgroundTexture();
    sf::Sprite  bgSpr(bgTex);
    const sf::Vector2f bgSz = sf::Vector2f(bgTex.getSize());
    const float bgSc = std::max(static_cast<float>(wSize.x)/bgSz.x,
                                 static_cast<float>(wSize.y)/bgSz.y);
    bgSpr.setScale({bgSc,bgSc});
    bgSpr.setPosition({(wSize.x-bgSz.x*bgSc)/2.f,(wSize.y-bgSz.y*bgSc)/2.f});
    sf::RectangleShape vignette({static_cast<float>(wSize.x),static_cast<float>(wSize.y)});
    vignette.setFillColor(sf::Color(0,0,0,55));

    // =========================
    // gameconsole-chen-nhac-05
    // Nhạc nền loop — VRSFML: sf::Sound API giữ tương tự
    // =========================
    sf::SoundBuffer        ambBuf;
    std::optional<sf::Sound> ambSound;
    if (buildAmbientBuffer(ambBuf)) {
        ambSound.emplace(ambBuf);
        ambSound->setLooping(true);
        ambSound->setVolume(18.f);
        ambSound->play();
    }

    const std::vector<BoardEntry> board = loadBoardEntries("gameConsole_board.json");

    // =========================
    // gameconsole-nut-guide-02  gameconsole-nut-board-03
    // Định nghĩa các nút GUIDE, BOARD, START, CLOSE
    // =========================
    const float bw = wSize.x*0.24f, bh = 64.f;
    const float by = wSize.y*0.68f;
    Button guideBtn{{sf::Vector2f(wSize.x*0.06f, by), sf::Vector2f(bw,bh)}, "GUIDE"};
    Button boardBtn{{sf::Vector2f(wSize.x*0.38f, by), sf::Vector2f(bw,bh)}, "BOARD"};
    Button startBtn{{sf::Vector2f(wSize.x*0.70f, by), sf::Vector2f(bw,bh)}, "START"};
    Button closeBtn{{sf::Vector2f(wSize.x*.5f-86.f, wSize.y*.82f), sf::Vector2f(172.f,52.f)}, "CLOSE"};

    PopupMode popupMode   = PopupMode::None;
    int       boardScroll = 0;
    ConsoleResult result  = ConsoleResult::Quit;

    // =========================
    // Vòng lặp chính — VRSFML SFML_GAME_LOOP
    // =========================
    SFML_GAME_LOOP(window) {
        bool openGuide=false, openBoard=false, closePopup=false, startGame=false;
        int  scrollDelta=0;
        bool clicked=false;

        while (const sf::base::Optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();

            if (const auto* mw = ev->getIf<sf::Event::MouseWheelScrolled>())
                if (popupMode==PopupMode::Board)
                    scrollDelta += (mw->delta>0.f)?-1:1;

            if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
                if (kp->code==sf::Keyboard::Key::Escape) {
                    if (popupMode==PopupMode::None) window.close();
                    else closePopup=true;
                }
                if (popupMode==PopupMode::Guide &&
                    (kp->code==sf::Keyboard::Key::Enter||kp->code==sf::Keyboard::Key::Space))
                    closePopup=true;
                if (popupMode==PopupMode::Board) {
                    if (kp->code==sf::Keyboard::Key::Up)   scrollDelta-=1;
                    else if (kp->code==sf::Keyboard::Key::Down) scrollDelta+=1;
                    else if (kp->code==sf::Keyboard::Key::Enter||kp->code==sf::Keyboard::Key::Space)
                        closePopup=true;
                }
            }
            if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>())
                if (mb->button==sf::Mouse::Button::Left) clicked=true;
        }

        if (scrollDelta && popupMode==PopupMode::Board)
            boardScroll = clampBoardScroll(boardScroll+scrollDelta,
                                           static_cast<int>(board.size()), 7);
        if (closePopup) popupMode=PopupMode::None;

        const sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (clicked && popupMode==PopupMode::None) {
            if      (guideBtn.bounds.contains(mp)) openGuide=true;
            else if (boardBtn.bounds.contains(mp)) openBoard=true;
            else if (startBtn.bounds.contains(mp)) startGame=true;
        } else if (clicked && popupMode!=PopupMode::None && closeBtn.bounds.contains(mp)) {
            closePopup=true;
        }

        if (openGuide)  popupMode=PopupMode::Guide;
        if (openBoard)  popupMode=PopupMode::Board;
        if (closePopup) popupMode=PopupMode::None;
        if (startGame)  { result=ConsoleResult::StartGame; window.close(); return result; }

        const bool gHov = (popupMode==PopupMode::None)&&guideBtn.bounds.contains(mp);
        const bool bHov = (popupMode==PopupMode::None)&&boardBtn.bounds.contains(mp);
        const bool sHov = (popupMode==PopupMode::None)&&startBtn.bounds.contains(mp);
        const bool cHov = (popupMode!=PopupMode::None)&&closeBtn.bounds.contains(mp);

        // Vẽ (View)
        window.clear(sf::Color::Black);
        window.draw(bgSpr);
        window.draw(vignette);
        sf::RectangleShape topGlow({static_cast<float>(wSize.x),190.f*uiScale});
        topGlow.setFillColor(sf::Color(130,180,255,35));
        window.draw(topGlow);

        if (hasFont) {
            // VRSFML: sf::Text(font, string, size) — constructor tham số tường minh
            sf::Text title(font, "ctetris", static_cast<unsigned>(56.f*uiScale));
            title.setFillColor(sf::Color::White);
            drawTextCentered(window, title, {static_cast<float>(wSize.x)/2.f, 120.f*uiScale});

            sf::Text sub(font, "SFML via VRSFML — Guide  Board  Start",
                         static_cast<unsigned>(20.f*uiScale));
            sub.setFillColor(sf::Color(210,225,255));
            drawTextCentered(window, sub, {static_cast<float>(wSize.x)/2.f, 170.f*uiScale});

            if (popupMode==PopupMode::None) {
                drawButton(window, font, guideBtn, gHov, uiScale);
                drawButton(window, font, boardBtn, bHov, uiScale);
                drawButton(window, font, startBtn, sHov, uiScale);
            } else if (popupMode==PopupMode::Guide) {
                drawOverlayPanel(window, wSize, 145.f);
                drawGuidePopup(window, font, wSize, closeBtn, uiScale, cHov);
            } else if (popupMode==PopupMode::Board) {
                drawOverlayPanel(window, wSize, 145.f);
                drawBoardPopup(window, font, wSize, board,
                               boardScroll, closeBtn, uiScale, cHov);
            }
        }
        window.display();
    }
    return result;
}

// =========================
// Điểm vào standalone
// =========================
#ifdef STANDALONE_GAMECONSOLE
int main() {
    auto gCtx = sf::GraphicsContext::create().value();
    auto aCtx = sf::AudioContext::create().value();
    runGameConsole(gCtx, aCtx);
    return 0;
}
#endif