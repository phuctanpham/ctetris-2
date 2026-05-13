// integration/v1
#include "gameConsole_layout.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <cmath>

// [C.4/C.5] STL for board data (vector<BoardEntry>) + ISO 8601 parsing
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <cstdio>
#include <ctime>

// [C.5] nlohmann/json for JSON-driven leaderboard
#include <nlohmann/json.hpp>

// [F.1] SQLite database for Stories / Records / Catalogue
#include "gameConsole_db.h"
#include "sqlite3.h"

// [G/H] Smart Sorting Engine v2.0 -- powers sort-by-time + sort-by-score
//        in the board popup. Router picks Insertion for n<=64 by default,
//        so our 30-row leaderboard runs near O(n).
#include "gameConsole_sort.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// [F.7] IDBFS persistence for sqlite file across browser tab reloads.
// FS.syncfs is async; Asyncify.handleSleep blocks the C caller until JS
// resolves. Requires -sASYNCIFY=1 (already set in CMakeLists.txt WASM link
// options). All three helpers degrade to no-op if FS isn't ready.
EM_JS(void, idbfs_mount_dir, (const char* path), {
    var p = UTF8ToString(path);
    try {
        FS.mkdirTree(p);
        var nodeMount = FS.lookupPath(p).node.mount;
        // Skip if this dir is already an IDBFS mount (Console re-entry).
        if (nodeMount && nodeMount.type && nodeMount.type.name === 'IDBFS') return;
        FS.mount(IDBFS, {}, p);
    } catch (e) {
        console.warn('[IDBFS] mount fail at', p, e);
    }
});
EM_JS(void, idbfs_load_from_idb, (), {
    Asyncify.handleSleep(function(wakeUp) {
        FS.syncfs(true, function(err) {
            if (err) console.warn('[IDBFS] load fail:', err);
            wakeUp();
        });
    });
});
EM_JS(void, idbfs_save_to_idb, (), {
    Asyncify.handleSleep(function(wakeUp) {
        FS.syncfs(false, function(err) {
            if (err) console.warn('[IDBFS] save fail:', err);
            wakeUp();
        });
    });
});
#endif

// =========================================================
// [B.2] nanosvg SVG rasterizer -- single-header library.
// Implementation duoc build mot lan tai src/shared/nanosvg_impl.cpp.
// =========================================================
#include "nanosvg.h"
#include "nanosvgrast.h"

// [B.1] Background SVG da embed san duoi dang raw string
#include "gameConsole_bg_svg.h"

// Mau text "nhe" -- 220 thay vi 255
static const SDL_Color SOFT_WHITE  = {220, 220, 220, 255};
static const SDL_Color HIGHLIGHT_Y = {255, 215,   0, 255};

// =========================================================
// [B.2] SvgTexture + loadSvgTextureFromMem helper
// Mirror cua gameStory createSvgTexture: parse SVG, rasterize tai
// chieu rong targetW (giu aspect), upload thanh SDL_Texture.
// Caller chiu trach nhiem destroy texture (xem [B.4] cleanup).
// =========================================================
struct SvgTexture {
    SDL_Texture* texture = nullptr;
    int          w       = 0;
    int          h       = 0;
};

static SvgTexture loadSvgTextureFromMem(SDL_Renderer* renderer,
                                        const char* svgData,
                                        int targetW) {
    SvgTexture result;

    // nsvgParse mutates buffer -- copy to mutable mem truoc
    size_t svgLen = SDL_strlen(svgData);
    char* svgCopy = (char*)SDL_malloc(svgLen + 1);
    if (!svgCopy) return result;
    SDL_memcpy(svgCopy, svgData, svgLen + 1);

    NSVGimage* image = nsvgParse(svgCopy, "px", 96.0f);
    SDL_free(svgCopy);
    if (!image || image->width <= 0 || image->height <= 0) {
        if (image) nsvgDelete(image);
        SDL_Log("[gameConsole] Khong parse duoc SVG");
        return result;
    }

    float scale = (float)targetW / image->width;
    int outW = targetW;
    int outH = (int)(image->height * scale + 0.5f);

    size_t pixelBytes = (size_t)outW * (size_t)outH * 4;
    unsigned char* pixels = (unsigned char*)SDL_malloc(pixelBytes);
    if (!pixels) { nsvgDelete(image); return result; }
    SDL_memset(pixels, 0, pixelBytes);

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) { SDL_free(pixels); nsvgDelete(image); return result; }
    nsvgRasterize(rast, image, 0.0f, 0.0f, scale,
                  pixels, outW, outH, outW * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    SDL_Surface* surface = SDL_CreateSurfaceFrom(outW, outH,
                                                 SDL_PIXELFORMAT_RGBA32,
                                                 pixels, outW * 4);
    if (!surface) { SDL_free(pixels); return result; }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    SDL_free(pixels);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        result.texture = texture;
        result.w       = outW;
        result.h       = outH;
    } else {
        SDL_Log("[gameConsole] Khong tao duoc SDL_Texture: %s", SDL_GetError());
    }
    return result;
}

// [B.3] Cache 1 lan, dung lai cho moi frame -- tranh re-rasterize 60fps
static SvgTexture g_consoleBg;

struct Button {
    SDL_FRect rect;
    const char* label;
    SDL_Color bg;
};

static bool hitTest(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

static void drawButton(SDL_Renderer* renderer, const Button& b, bool focused) {
    SDL_SetRenderDrawColor(renderer, b.bg.r, b.bg.g, b.bg.b, b.bg.a);
    SDL_RenderFillRect(renderer, &b.rect);
    if (focused) {
        SDL_SetRenderDrawColor(renderer, HIGHLIGHT_Y.r, HIGHLIGHT_Y.g, HIGHLIGHT_Y.b, 255);
        for (int k = 0; k < 2; k++) {
            SDL_FRect r = { b.rect.x - k, b.rect.y - k,
                            b.rect.w + 2 * k, b.rect.h + 2 * k };
            SDL_RenderRect(renderer, &r);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
        SDL_RenderRect(renderer, &b.rect);
    }
    int len = (int)SDL_strlen(b.label);
    float tx = b.rect.x + (b.rect.w - len * 8.0f) / 2.0f;
    float ty = b.rect.y + (b.rect.h - 8.0f) / 2.0f;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer, tx, ty, b.label);
}

// =========================================================
// Scrollbar tuong tac
// =========================================================
struct SBLayout {
    SDL_FRect upBtn;
    SDL_FRect track;
    SDL_FRect downBtn;
    SDL_FRect thumb;
};

struct SBInteraction {
    bool   upHeld         = false;
    bool   downHeld       = false;
    bool   dragging       = false;
    float  dragOffsetY    = 0.0f;
    Uint32 lastAutoScroll = 0;
    Uint32 nextAutoStart  = 0;
};

static const float SB_W     = 8.0f;
static const float SB_BTN_H = 12.0f;

static SBLayout layoutSB(float x, float y, float h, int total, int visible, int pos) {
    SBLayout sb;
    sb.upBtn   = { x, y,                 SB_W, SB_BTN_H };
    sb.downBtn = { x, y + h - SB_BTN_H,  SB_W, SB_BTN_H };
    sb.track   = { x, y + SB_BTN_H,      SB_W, h - 2 * SB_BTN_H };
    sb.thumb   = { 0, 0, 0, 0 };
    if (total > visible && sb.track.h > 0) {
        float thumbH = sb.track.h * (float)visible / (float)total;
        if (thumbH < 16.0f)        thumbH = 16.0f;
        if (thumbH > sb.track.h)   thumbH = sb.track.h;
        float maxScroll = (float)(total - visible);
        float thumbY = sb.track.y + (sb.track.h - thumbH) * (float)pos / maxScroll;
        sb.thumb = { x, thumbY, SB_W, thumbH };
    }
    return sb;
}

static int clampScroll(int pos, int total, int visible) {
    int maxS = total - visible;
    if (maxS < 0) maxS = 0;
    if (pos < 0) pos = 0;
    if (pos > maxS) pos = maxS;
    return pos;
}

static void drawSB(SDL_Renderer* r, const SBLayout& sb, int total, int visible,
                   bool upHeld, bool downHeld, bool dragging) {
    SDL_Color upC = upHeld   ? HIGHLIGHT_Y : SDL_Color{120, 120, 130, 255};
    SDL_Color dnC = downHeld ? HIGHLIGHT_Y : SDL_Color{120, 120, 130, 255};

    SDL_SetRenderDrawColor(r, upC.r, upC.g, upC.b, 255);
    SDL_RenderFillRect(r, &sb.upBtn);
    SDL_SetRenderDrawColor(r, dnC.r, dnC.g, dnC.b, 255);
    SDL_RenderFillRect(r, &sb.downBtn);

    SDL_SetRenderDrawColor(r, 30, 30, 40, 255);
    float ucx = sb.upBtn.x   + sb.upBtn.w   / 2;
    float ucy = sb.upBtn.y   + sb.upBtn.h   / 2;
    float dcx = sb.downBtn.x + sb.downBtn.w / 2;
    float dcy = sb.downBtn.y + sb.downBtn.h / 2;
    for (int i = 0; i < 3; i++) {
        SDL_FRect lnUp = { ucx - i, ucy - 1 + i, 1.0f + 2.0f * i, 1.0f };
        SDL_FRect lnDn = { dcx - i, dcy + 1 - i, 1.0f + 2.0f * i, 1.0f };
        SDL_RenderFillRect(r, &lnUp);
        SDL_RenderFillRect(r, &lnDn);
    }

    if (total > visible) {
        SDL_SetRenderDrawColor(r, 40, 40, 50, 255);
        SDL_RenderFillRect(r, &sb.track);
        SDL_Color tC = dragging ? HIGHLIGHT_Y : SDL_Color{200, 200, 200, 255};
        SDL_SetRenderDrawColor(r, tC.r, tC.g, tC.b, 255);
        SDL_RenderFillRect(r, &sb.thumb);
    }
}

static bool sbOnMouseDown(SBInteraction& sbi, const SBLayout& sb, float mx, float my,
                          int& scrollPos, int total, int visible, Uint32 nowMs) {
    if (hitTest(sb.upBtn, mx, my)) {
        sbi.upHeld = true;
        scrollPos = clampScroll(scrollPos - 1, total, visible);
        sbi.lastAutoScroll = nowMs;
        sbi.nextAutoStart  = nowMs + 300;
        return true;
    }
    if (hitTest(sb.downBtn, mx, my)) {
        sbi.downHeld = true;
        scrollPos = clampScroll(scrollPos + 1, total, visible);
        sbi.lastAutoScroll = nowMs;
        sbi.nextAutoStart  = nowMs + 300;
        return true;
    }
    if (total > visible) {
        if (hitTest(sb.thumb, mx, my)) {
            sbi.dragging    = true;
            sbi.dragOffsetY = my - sb.thumb.y;
            return true;
        }
        if (hitTest(sb.track, mx, my)) {
            float ratio = (my - sb.track.y) / sb.track.h;
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
            scrollPos = clampScroll((int)(ratio * (total - visible) + 0.5f), total, visible);
            return true;
        }
    }
    return false;
}

static void sbOnMouseMotion(SBInteraction& sbi, const SBLayout& sb, float my,
                            int& scrollPos, int total, int visible) {
    if (!sbi.dragging || total <= visible) return;
    float trackUsable = sb.track.h - sb.thumb.h;
    if (trackUsable <= 0) return;
    float thumbY = my - sbi.dragOffsetY;
    float ratio = (thumbY - sb.track.y) / trackUsable;
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    scrollPos = clampScroll((int)(ratio * (total - visible) + 0.5f), total, visible);
}

static void sbResetInteraction(SBInteraction& sbi) {
    sbi.upHeld = false;
    sbi.downHeld = false;
    sbi.dragging = false;
}

static void sbAutoRepeat(SBInteraction& sbi, int& scrollPos, int total, int visible, Uint32 nowMs) {
    const Uint32 REPEAT_INTERVAL = 60;
    if (nowMs < sbi.nextAutoStart) return;
    if (nowMs - sbi.lastAutoScroll < REPEAT_INTERVAL) return;
    if (sbi.upHeld) {
        scrollPos = clampScroll(scrollPos - 1, total, visible);
        sbi.lastAutoScroll = nowMs;
    }
    if (sbi.downHeld) {
        scrollPos = clampScroll(scrollPos + 1, total, visible);
        sbi.lastAutoScroll = nowMs;
    }
}

// =========================================================
// WASM Shutdown screen
// =========================================================
// Nut RELOAD giua man hinh -- kich thuoc dong nhat voi gameCore
static const SDL_FRect CONSOLE_RELOAD_BTN = {
    (CONSOLE_SCREEN_WIDTH  - 160.0f) / 2.0f,
    (CONSOLE_SCREEN_HEIGHT - 50.0f)  / 2.0f,
    160.0f, 50.0f
};

static void drawConsoleWasmShutdown(SDL_Renderer* renderer, bool reloadHover) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color btnBg = reloadHover ? SDL_Color{ 90, 130, 200, 255}
                                  : SDL_Color{ 60, 100, 170, 255};
    SDL_SetRenderDrawColor(renderer, btnBg.r, btnBg.g, btnBg.b, 255);
    SDL_RenderFillRect(renderer, &CONSOLE_RELOAD_BTN);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &CONSOLE_RELOAD_BTN);

    const char* label = "RELOAD";
    int ll = (int)SDL_strlen(label);
    SDL_RenderDebugText(renderer,
                        CONSOLE_RELOAD_BTN.x + (CONSOLE_RELOAD_BTN.w - ll * 8.0f) / 2.0f,
                        CONSOLE_RELOAD_BTN.y + (CONSOLE_RELOAD_BTN.h - 8.0f) / 2.0f,
                        label);

    float hintY = CONSOLE_RELOAD_BTN.y + CONSOLE_RELOAD_BTN.h + 16.0f;
    const char* hint1 = "Press F5 or click RELOAD";
    const char* hint2 = "to start a new game";
    int h1l = (int)SDL_strlen(hint1);
    int h2l = (int)SDL_strlen(hint2);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer,
                        (CONSOLE_SCREEN_WIDTH - h1l * 8.0f) / 2.0f,
                        hintY, hint1);
    SDL_RenderDebugText(renderer,
                        (CONSOLE_SCREEN_WIDTH - h2l * 8.0f) / 2.0f,
                        hintY + 14.0f, hint2);
}

// =========================================================
// AppState
// =========================================================
// [G/H] gameconsole-sort-by-time-in-board-14 / -by-score-in-board-15
enum BoardSortMode {
    BOARD_SORT_SCORE_DESC = 0,   // default: highest score first
    BOARD_SORT_SCORE_ASC,
    BOARD_SORT_TIME_DESC,        // newest first
    BOARD_SORT_TIME_ASC
};

struct AppState {
    bool showGuide    = false;
    bool showBoard    = false;
    bool showSettings = false;   // [A.3] Settings popup visibility
    bool showStories  = false;   // [F.2] Stories popup visibility
    int  currentStoryChapter = 1;   // [F.3] active chapter shown in stories popup
    std::vector<StoryRow> storiesCache;
    int  storiesCacheChapter = -1;
    int  storiesMaxChapter   = 1;
    bool isRunning    = true;
    int  nextScene    = 0;
    int  boardScroll  = 0;
    int  guideScroll  = 0;
    BoardSortMode boardSortMode = BOARD_SORT_SCORE_DESC;   // [G/H]
    int  focusIndex   = 0;
    SBInteraction sb;

    // [D.6] Pointer to caller's SettingsConfig (NOT owned). UI mutations
    // through state.cfg->* are immediately visible to runGameCore via the
    // shared reference -- no boundary copy needed. Initialized in
    // runGameConsole entry; stays non-null for the lifetime of the call.
    SettingsConfig* cfg = nullptr;

    // [D.3] Volume slider drag state
    bool draggingVolume = false;

    // WASM-only: khi QUIT duoc chon, hien shutdown screen thay vi
    // tra ve 0 lam canvas trang (khong co SDL_DestroyWindow phu hop)
    bool wasmShutdown = false;
    bool reloadHover  = false;
};

// =========================================================
// [C.4] BoardEntry: now uses std::string (was const char*) and adds
// timeEpoch (int64_t Unix seconds, UTC) for sort tasks G/H.
// =========================================================
struct BoardEntry {
    std::string user;
    int         score;
    std::string time;        // display "MM-DD HH:MM"
    int64_t     timeEpoch;   // Unix epoch UTC, 0 = unknown
};

// =========================================================
// [C.4] ISO 8601 helpers -- format "YYYY-MM-DDTHH:MM:SSZ" (Z = UTC).
// Manual sscanf approach because strptime() is missing on MSVC. timegm()
// is POSIX; Windows equivalent is _mkgmtime() in <time.h>.
// =========================================================
static int64_t parseIso8601(const char* iso) {
    int y, mo, d, h, mi, se;
    if (std::sscanf(iso, "%d-%d-%dT%d:%d:%dZ", &y, &mo, &d, &h, &mi, &se) != 6) {
        return 0;  // unparseable -> sentinel
    }
    std::tm t = {};
    t.tm_year = y - 1900;
    t.tm_mon  = mo - 1;
    t.tm_mday = d;
    t.tm_hour = h;
    t.tm_min  = mi;
    t.tm_sec  = se;
#if defined(_WIN32)
    return (int64_t)_mkgmtime(&t);
#else
    return (int64_t)timegm(&t);
#endif
}

static std::string formatIsoForDisplay(const char* iso) {
    int y, mo, d, h, mi;
    if (std::sscanf(iso, "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) != 5) {
        return std::string(iso);  // can't parse -> show raw
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d-%02d %02d:%02d", mo, d, h, mi);
    return std::string(buf);
}

static std::string formatEpochForDisplay(int64_t epoch) {
    std::tm tm{};
    std::time_t t = (std::time_t)epoch;
#if defined(_WIN32)
    if (gmtime_s(&tm, &t) != 0) {
        return std::string();
    }
#else
    if (std::tm* tmp = std::gmtime(&t)) {
        tm = *tmp;
    } else {
        return std::string();
    }
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d-%02d %02d:%02d",
                  tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
    return std::string(buf);
}

// =========================================================
// [C.5] Fallback hardcoded leaderboard (used if JSON load fails).
// Format matches gameConsole_board.json (ISO 8601 timestamps) so the
// SAME parseIso8601 + formatIsoForDisplay path runs in both branches --
// no special-case display logic needed.
// =========================================================
struct FallbackRow { const char* user; int score; const char* iso; };
static const FallbackRow FALLBACK_BOARD_ROWS[] = {
    {"ShadowWalker", 8540, "2026-04-12T08:15:30Z"}, {"PixelKnight",  7210, "2026-04-12T09:22:15Z"},
    {"CyberGhost",   6900, "2026-04-12T10:05:45Z"}, {"NeonViper",    6550, "2026-04-12T07:30:12Z"},
    {"IronDragon",   6120, "2026-04-11T23:45:00Z"}, {"StormBreaker", 5890, "2026-04-12T05:12:33Z"},
    {"AlphaCoder",   5700, "2026-04-12T11:20:10Z"}, {"MysticSage",   5430, "2026-04-11T18:55:20Z"},
    {"FrostBite",    5210, "2026-04-12T02:10:05Z"}, {"NovaStar",     4980, "2026-04-12T04:30:55Z"},
    {"GlitchMaster", 4750, "2026-04-12T12:00:01Z"}, {"ZenithHero",   4620, "2026-04-11T20:15:40Z"},
    {"RogueZero",    4300, "2026-04-12T01:45:22Z"}, {"TitanPulse",   4150, "2026-04-12T06:10:18Z"},
    {"LunarSeeker",  3900, "2026-04-11T15:30:00Z"}, {"BlazeFury",    3750, "2026-04-12T08:50:45Z"},
    {"VoidRunner",   3520, "2026-04-12T03:25:30Z"}, {"StarLord99",   3400, "2026-04-11T22:12:05Z"},
    {"EchoWhisper",  3280, "2026-04-12T10:40:15Z"}, {"DriftKing",    3150, "2026-04-12T05:55:50Z"},
    {"AstroBoy",     2900, "2026-04-11T19:05:10Z"}, {"SilentBlade",  2750, "2026-04-12T07:15:25Z"},
    {"MagmaCore",    2600, "2026-04-12T09:30:00Z"}, {"AquaMarine",   2450, "2026-04-12T11:05:40Z"},
    {"ThunderBolt",  2200, "2026-04-11T17:45:15Z"}, {"WindWalker",   1950, "2026-04-12T02:50:35Z"},
    {"NightOwl",     1800, "2026-04-12T04:20:20Z"}, {"SilverFang",   1650, "2026-04-11T21:10:55Z"},
    {"GoldenEye",    1400, "2026-04-12T06:45:12Z"}, {"ProGamerVN",   1250, "2026-04-12T08:05:30Z"}
};

// [C.5] Live board container -- populated at runtime by loadBoard().
// Replaces the old static const BOARD_DATA[]. g_boardTotal cached for
// the (very hot) hit-test loop in event handlers.
static std::vector<BoardEntry> g_board;
static int g_boardTotal  = 0;
static const int BOARD_VISIBLE = 9;

// [C.5] Load JSON from <basepath>/gameConsole_board.json. Returns false
// on any failure (no file, parse error, schema mismatch). Caller falls
// back to FALLBACK_BOARD_ROWS.
static bool loadBoardFromJson() {
    const char* basePath = SDL_GetBasePath();
    std::string path;
    if (basePath && basePath[0] != '\0') {
        path = std::string(basePath) + "gameConsole_board.json";
    } else {
        path = "gameConsole_board.json";  // CWD fallback
    }
    // SDL_GetBasePath in SDL3: returns const char*, do NOT free

    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
    if (!io) {
        SDL_Log("[gameConsole] khong mo duoc %s -- dung fallback", path.c_str());
        return false;
    }
    Sint64 sz = SDL_GetIOSize(io);
    if (sz <= 0) { SDL_CloseIO(io); return false; }
    std::string raw((size_t)sz, '\0');
    size_t got = SDL_ReadIO(io, &raw[0], (size_t)sz);
    SDL_CloseIO(io);
    if (got != (size_t)sz) {
        SDL_Log("[gameConsole] read sai size: got %zu need %lld",
                got, (long long)sz);
        return false;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(raw);
        const auto& arr = j.at("board");
        std::vector<BoardEntry> out;
        out.reserve(arr.size());
        for (const auto& e : arr) {
            BoardEntry be;
            be.user      = e.at("user").get<std::string>();
            be.score     = e.at("score").get<int>();
            std::string iso = e.at("time").get<std::string>();
            be.timeEpoch = parseIso8601(iso.c_str());
            be.time      = formatIsoForDisplay(iso.c_str());
            out.push_back(std::move(be));
        }
        if (out.empty()) return false;
        g_board = std::move(out);
        return true;
    } catch (const std::exception& ex) {
        SDL_Log("[gameConsole] JSON parse fail: %s", ex.what());
        return false;
    }
}

// [C.5] Load with fallback. Always populates g_board with at least 30
// fallback rows so leaderboard is never empty.
static void loadBoardWithFallback() {
    g_board.clear();
    std::vector<BoardRecord> rows = dbLoadRecords();
    g_board.reserve(rows.size());
    for (const auto& r : rows) {
        BoardEntry be;
        be.user = r.idUser;
        be.score = r.totalScore;
        be.timeEpoch = r.endTS;
        be.time = formatEpochForDisplay(r.endTS);
        g_board.push_back(std::move(be));
    }
    SDL_Log("[gameConsole] board loaded tu DB: %d entries", (int)g_board.size());
    g_boardTotal = (int)g_board.size();
}

// Apply current sort mode via the Smart Engine. Logs the algorithm the
// router picked so the engine is transparent during development.
// CTX_DEFAULT + n=30 -> Insertion Sort (near-O(n) on already-sorted data,
// which is the common case for repeat opens of the popup).
static void applyBoardSort(AppState& state) {
    using namespace SortEngine;
    SortAlgo picked = ALGO_AUTO;
    const size_t n = g_board.size();
    switch (state.boardSortMode) {
        case BOARD_SORT_SCORE_DESC:
            picked = sort(g_board.data(), n,
                [](const BoardEntry& a, const BoardEntry& b){ return a.score > b.score; },
                CTX_DEFAULT);
            break;
        case BOARD_SORT_SCORE_ASC:
            picked = sort(g_board.data(), n,
                [](const BoardEntry& a, const BoardEntry& b){ return a.score < b.score; },
                CTX_DEFAULT);
            break;
        case BOARD_SORT_TIME_DESC:
            picked = sort(g_board.data(), n,
                [](const BoardEntry& a, const BoardEntry& b){ return a.timeEpoch > b.timeEpoch; },
                CTX_DEFAULT);
            break;
        case BOARD_SORT_TIME_ASC:
            picked = sort(g_board.data(), n,
                [](const BoardEntry& a, const BoardEntry& b){ return a.timeEpoch < b.timeEpoch; },
                CTX_DEFAULT);
            break;
    }
    SDL_Log("[gameConsole] board sort mode=%d algo=%s n=%d",
            (int)state.boardSortMode, algoName(picked), (int)n);
    state.boardScroll = 0;
}

// =========================================================
// [F.1] SQLite database layer (Stories + Records + Catalogue)
// Path: SDL_GetPrefPath("uit", "cTetris") + "<idUser>.sqlite"
// Lifetime: opened on runGameConsole entry, closed on exit.
// WASM IDBFS persistence wired in Step 2.6.12.
// =========================================================
static sqlite3*    g_db = nullptr;
static std::string g_dbCurrentUser;
static std::string g_dbPath;

// Helper: returns "{idUser}_{suffix}" for Group-1 dynamic tables
static std::string userTable(const std::string& suffix) {
    return g_dbCurrentUser + "_" + suffix;
}

static bool dbExec(const char* sql, const char* what) {
    if (!g_db) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(g_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        SDL_Log("[gameConsole_db] %s fail: %s",
                what, errMsg ? errMsg : "(no msg)");
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// [F.7] Flush in-memory MEMFS contents to IndexedDB so the sqlite file
// survives a browser tab close + reload. No-op on native (DB file is
// already on real disk).
static void dbSyncToPersistent() {
#ifdef __EMSCRIPTEN__
    if (g_db) idbfs_save_to_idb();
#endif
}

bool dbOpen(const char* idUser) {
    if (!idUser || !*idUser) {
        SDL_Log("[gameConsole_db] dbOpen: idUser rong");
        return false;
    }
    if (g_db && g_dbCurrentUser == idUser) {
        return true;  // already open for same user
    }
    if (g_db) {
        sqlite3_close(g_db);
        g_db = nullptr;
    }

    char* pref = SDL_GetPrefPath("uit", "cTetris");
    if (!pref) {
        SDL_Log("[gameConsole_db] SDL_GetPrefPath fail: %s", SDL_GetError());
        return false;
    }
    std::string prefDir(pref);
    SDL_free(pref);   // SDL3: SDL_GetPrefPath returns SDL-allocated string

#ifdef __EMSCRIPTEN__
    // [F.7] Mount IDBFS on the pref dir + preload previously persisted
    // contents BEFORE sqlite3_open so the DB file is read off IDBFS,
    // not from a blank MEMFS.
    idbfs_mount_dir(prefDir.c_str());
    idbfs_load_from_idb();
#endif

    g_dbPath = prefDir + idUser + ".sqlite";

    int rc = sqlite3_open(g_dbPath.c_str(), &g_db);
    if (rc != SQLITE_OK) {
        SDL_Log("[gameConsole_db] sqlite3_open fail: %s",
                g_db ? sqlite3_errmsg(g_db) : "(null)");
        if (g_db) { sqlite3_close(g_db); g_db = nullptr; }
        g_dbPath.clear();
        return false;
    }

    g_dbCurrentUser = idUser;
    SDL_Log("[gameConsole_db] DB mo: %s", g_dbPath.c_str());
    return true;
}

void dbClose() {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = nullptr;
        g_dbCurrentUser.clear();
        SDL_Log("[gameConsole_db] DB dong");
    }
}

bool dbInitSchema() {
    if (!g_db) {
        SDL_Log("[gameConsole_db] dbInitSchema: DB chua mo");
        return false;
    }

    // ── Group 1: {idUser}_* — personal R/W ──────────────────────────────
    std::string rec  = userTable("Records");
    std::string stor = userTable("Stories");
    std::string sett = userTable("Settings");

    auto createSQL = [&](const std::string& sql, const char* what) -> bool {
        return dbExec(sql.c_str(), what);
    };

    if (!createSQL(
        "CREATE TABLE IF NOT EXISTS " + rec + " ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  idUser        TEXT NOT NULL,"
        "  startTS       INTEGER NOT NULL,"
        "  endTS         INTEGER NOT NULL,"
        "  idStory       INTEGER,"
        "  idChapter     INTEGER,"
        "  totalScore    INTEGER,"
        "  totalSeconds  INTEGER,"
        "  avgSpeed      REAL,"
        "  retryNo       INTEGER"
        ");",
        "CREATE _Records")) return false;

    if (!createSQL(
        "CREATE TABLE IF NOT EXISTS " + stor + " ("
        "  idUser        TEXT NOT NULL,"
        "  idStory       INTEGER NOT NULL,"
        "  idChapter     INTEGER NOT NULL,"
        "  isActivated   INTEGER DEFAULT 0,"
        "  isSelected    INTEGER DEFAULT 0,"
        "  totalRetries  INTEGER DEFAULT 0,"
        "  lastMaxScore  INTEGER DEFAULT 0,"
        "  lastMaxSpeed  REAL    DEFAULT 0,"
        "  PRIMARY KEY (idUser, idStory, idChapter)"
        ");",
        "CREATE _Stories")) return false;

    if (!createSQL(
        "CREATE TABLE IF NOT EXISTS " + sett + " ("
        "  key   TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL DEFAULT ''"
        ");",
        "CREATE _Settings")) return false;

    // ── Group 2: shared_* — Gist JSON import, read-only ─────────────────
    if (!dbExec(
        "CREATE TABLE IF NOT EXISTS shared_data ("
        "  idStory         INTEGER NOT NULL,"
        "  storyName       TEXT NOT NULL,"
        "  idChapter       INTEGER NOT NULL,"
        "  chapterName     TEXT,"
        "  minScore        INTEGER DEFAULT 0,"
        "  minSpeed        REAL    DEFAULT 0,"
        "  minRetries      INTEGER DEFAULT 0,"
        "  requiredStories TEXT    DEFAULT '',"
        "  nextBlockScore  INTEGER DEFAULT 0,"
        "  nextBlockSpeed  REAL    DEFAULT 0,"
        "  tableMatrix     TEXT    DEFAULT '',"
        "  xmlDialogue     TEXT    DEFAULT '',"
        "  thumbnailPath   TEXT    DEFAULT '',"
        "  PRIMARY KEY (idStory, idChapter)"
        ");",
        "CREATE shared_data")) return false;

    // shared_dialogues: per-node dialogue consumed by gameStory V2
    if (!dbExec(
        "CREATE TABLE IF NOT EXISTS shared_dialogues ("
        "  rowId      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  idStory    INTEGER NOT NULL,"
        "  idChapter  INTEGER NOT NULL,"
        "  nodeId     INTEGER NOT NULL,"
        "  speaker    TEXT    DEFAULT '',"
        "  text       TEXT    DEFAULT '',"
        "  imageUrl   TEXT    DEFAULT '',"
        "  bgmUrl     TEXT    DEFAULT '',"
        "  sfxUrl     TEXT    DEFAULT '',"
        "  nextNodeId INTEGER DEFAULT 0,"
        "  hasChoices INTEGER DEFAULT 0,"
        "  UNIQUE(idStory, idChapter, nodeId)"
        ");",
        "CREATE shared_dialogues")) return false;

    // shared_choices: branching choices consumed by gameStory V2
    if (!dbExec(
        "CREATE TABLE IF NOT EXISTS shared_choices ("
        "  rowId      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  idStory    INTEGER NOT NULL,"
        "  idChapter  INTEGER NOT NULL,"
        "  nodeId     INTEGER NOT NULL,"
        "  choiceIdx  INTEGER NOT NULL,"
        "  label      TEXT    DEFAULT '',"
        "  nextNodeId INTEGER DEFAULT 0,"
        "  UNIQUE(idStory, idChapter, nodeId, choiceIdx)"
        ");",
        "CREATE shared_choices")) return false;

    if (!dbExec(
        "CREATE TABLE IF NOT EXISTS shared_meta ("
        "  chapter_id     TEXT PRIMARY KEY,"
        "  sha            TEXT NOT NULL,"
        "  updated_at     INTEGER,"
        "  media_base_url TEXT DEFAULT ''"
        ");",
        "CREATE shared_meta")) return false;

    // ── Group 3: sync_* — Cloudflare D1 1-way sync, read-only ───────────
    if (!dbExec(
        "CREATE TABLE IF NOT EXISTS sync_Records ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  nameUser     TEXT    DEFAULT '',"
        "  totalScore   INTEGER DEFAULT 0,"
        "  totalSeconds INTEGER DEFAULT 0,"
        "  avgSpeed     REAL    DEFAULT 0,"
        "  endTS        INTEGER DEFAULT 0,"
        "  idStory      INTEGER DEFAULT 0,"
        "  idChapter    INTEGER DEFAULT 0"
        ");",
        "CREATE sync_Records")) return false;

    // Seed fake leaderboard data on first init (idempotent)
    {
        sqlite3_stmt* chk = nullptr;
        int cnt = 0;
        if (sqlite3_prepare_v2(g_db,
            "SELECT COUNT(*) FROM sync_Records;", -1, &chk, nullptr) == SQLITE_OK) {
            if (sqlite3_step(chk) == SQLITE_ROW) cnt = sqlite3_column_int(chk, 0);
            sqlite3_finalize(chk);
        }
        if (cnt == 0) {
            const int n = (int)(sizeof(FALLBACK_BOARD_ROWS)/sizeof(FALLBACK_BOARD_ROWS[0]));
            for (int i = 0; i < n; i++) {
                const FallbackRow& r = FALLBACK_BOARD_ROWS[i];
                int64_t ts = parseIso8601(r.iso);
                char sql[256];
                SDL_snprintf(sql, sizeof(sql),
                    "INSERT INTO sync_Records "
                    "(nameUser,totalScore,totalSeconds,avgSpeed,endTS) "
                    "VALUES ('%s',%d,%d,%.2f,%lld);",
                    r.user, r.score, r.score / 2,
                    (float)r.score / (float)(r.score / 2 + 1),
                    (long long)ts);
                dbExec(sql, "seed sync_Records");
            }
            SDL_Log("[gameConsole_db] sync_Records seeded with %d fake rows", n);
            dbSyncToPersistent();
        }
    }

    SDL_Log("[gameConsole_db] schema initialized (9 tables: 3 user + 4 shared + 1 sync + meta)");
    return true;
}

bool dbSeedSharedData() {
    if (!g_db) return false;

    // Data is populated by gameStory sync at startup.
    // Console only verifies rows exist; redirects to gameStory if empty.
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM shared_data;",
                           -1, &st, nullptr) != SQLITE_OK) return false;
    int cnt = 0;
    if (sqlite3_step(st) == SQLITE_ROW) cnt = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    if (cnt > 0) {
        SDL_Log("[gameConsole_db] shared_data %d rows (sync ok)", cnt);
        return true;
    }
    SDL_Log("[gameConsole_db] shared_data empty — gameStory sync required");
    return false;
}

std::vector<StoryRow> dbLoadStories(const char* idUser, int idChapter) {
    std::vector<StoryRow> out;
    if (!g_db || !idUser || !*idUser) return out;

    // LEFT JOIN shared_data (catalogue) with idUser_Stories (per-user overlay).
    // COALESCE turns NULL overlays into safe defaults so untouched stories
    // render as locked/unplayed without separate code paths.
    std::string sql =
        "SELECT s.idStory, s.storyName, s.idChapter, s.chapterName,"
        "       s.minScore, s.minSpeed, s.minRetries, s.requiredStories,"
        "       s.nextBlockScore, s.nextBlockSpeed,"
        "       s.tableMatrix, s.xmlDialogue, s.thumbnailPath,"
        "       COALESCE(u.isActivated,  0),"
        "       COALESCE(u.isSelected,   0),"
        "       COALESCE(u.totalRetries, 0),"
        "       COALESCE(u.lastMaxScore, 0),"
        "       COALESCE(u.lastMaxSpeed, 0.0) "
        "FROM shared_data s "
        "LEFT JOIN " + userTable("Stories") + " u "
        "  ON s.idStory = u.idStory "
        " AND s.idChapter = u.idChapter "
        " AND u.idUser = ?1 "
        "WHERE s.idChapter = ?2 "
        "ORDER BY s.idStory;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbLoadStories prepare fail: %s",
                sqlite3_errmsg(g_db));
        return out;
    }
    sqlite3_bind_text(st, 1, idUser, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 2, idChapter);

    auto colText = [](sqlite3_stmt* s, int i) -> std::string {
        const unsigned char* p = sqlite3_column_text(s, i);
        return p ? std::string((const char*)p) : std::string();
    };

    while (sqlite3_step(st) == SQLITE_ROW) {
        StoryRow r;
        r.idStory         = sqlite3_column_int(st, 0);
        r.storyName       = colText(st, 1);
        r.idChapter       = sqlite3_column_int(st, 2);
        r.chapterName     = colText(st, 3);
        r.minScore        = sqlite3_column_int(st, 4);
        r.minSpeed        = (float)sqlite3_column_double(st, 5);
        r.minRetries      = sqlite3_column_int(st, 6);
        r.requiredStories = colText(st, 7);
        r.nextBlockScore  = sqlite3_column_int(st, 8);
        r.nextBlockSpeed  = (float)sqlite3_column_double(st, 9);
        r.tableMatrix     = colText(st, 10);
        r.xmlDialogue     = colText(st, 11);
        r.thumbnailPath   = colText(st, 12);
        r.isActivated     = sqlite3_column_int(st, 13) != 0;
        r.isSelected      = sqlite3_column_int(st, 14) != 0;
        r.totalRetries    = sqlite3_column_int(st, 15);
        r.lastMaxScore    = sqlite3_column_int(st, 16);
        r.lastMaxSpeed    = (float)sqlite3_column_double(st, 17);

        // Stories with no prerequisites are always open (e.g. intro).
        // Cheap to compute here; saves needing a seed INSERT into
        // idUser_Stories at DB-init time.
        if (r.requiredStories.empty()) r.isActivated = true;

        out.push_back(std::move(r));
    }
    sqlite3_finalize(st);

    SDL_Log("[gameConsole_db] dbLoadStories chapter=%d user=%s rows=%d",
            idChapter, idUser, (int)out.size());
    return out;
}

int dbMaxActivatedChapter(const char* idUser) {
    // Always return >= 1: intro chapter is unconditionally accessible.
    if (!g_db || !idUser || !*idUser) return 1;

    sqlite3_stmt* st = nullptr;
    std::string sql =
        "SELECT COALESCE(MAX(idChapter), 1) FROM " + userTable("Stories") +
        " WHERE idUser = ? AND isActivated = 1;";
    if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbMaxActivatedChapter prepare fail: %s",
                sqlite3_errmsg(g_db));
        return 1;
    }
    sqlite3_bind_text(st, 1, idUser, -1, SQLITE_TRANSIENT);

    int maxCh = 1;
    if (sqlite3_step(st) == SQLITE_ROW) {
        maxCh = sqlite3_column_int(st, 0);
        if (maxCh < 1) maxCh = 1;
    }
    sqlite3_finalize(st);
    return maxCh;
}

int dbCheckAndUnlockStories(const char* idUser) {
    if (!g_db || !idUser || !*idUser) return 0;

    // Pass 1: collect catalogue rows with prerequisites
    std::vector<StoryRow> candidates;
    {
        sqlite3_stmt* st = nullptr;
        std::string sql =
            "SELECT idStory, idChapter, minScore, minSpeed, minRetries, "
            "       requiredStories "
            "FROM shared_data "
            "WHERE requiredStories != '';";
        if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
            SDL_Log("[gameConsole_db] unlock list prepare fail: %s",
                    sqlite3_errmsg(g_db));
            return 0;
        }
        while (sqlite3_step(st) == SQLITE_ROW) {
            StoryRow r;
            r.idStory    = sqlite3_column_int   (st, 0);
            r.idChapter  = sqlite3_column_int   (st, 1);
            r.minScore   = sqlite3_column_int   (st, 2);
            r.minSpeed   = (float)sqlite3_column_double(st, 3);
            r.minRetries = sqlite3_column_int   (st, 4);
            const unsigned char* rs = sqlite3_column_text(st, 5);
            r.requiredStories = rs ? (const char*)rs : "";
            candidates.push_back(std::move(r));
        }
        sqlite3_finalize(st);
    }

    // Pass 2: prepare reusable statements
    sqlite3_stmt* lookupParent = nullptr;
    {
        std::string lpSQL =
            "SELECT lastMaxScore, lastMaxSpeed, totalRetries FROM " +
            userTable("Stories") + " WHERE idUser = ? AND idStory = ?;";
        sqlite3_prepare_v2(g_db, lpSQL.c_str(), -1, &lookupParent, nullptr);
    }

    sqlite3_stmt* checkExisting = nullptr;
    {
        std::string ceSQL =
            "SELECT isActivated FROM " + userTable("Stories") +
            " WHERE idUser = ? AND idStory = ? AND idChapter = ?;";
        sqlite3_prepare_v2(g_db, ceSQL.c_str(), -1, &checkExisting, nullptr);
    }

    sqlite3_stmt* doUnlock = nullptr;
    {
        std::string duSQL =
            "INSERT INTO " + userTable("Stories") +
            " (idUser, idStory, idChapter, isActivated) VALUES (?, ?, ?, 1) "
            "ON CONFLICT(idUser, idStory, idChapter) DO UPDATE SET isActivated = 1;";
        sqlite3_prepare_v2(g_db, duSQL.c_str(), -1, &doUnlock, nullptr);
    }

    int unlocked = 0;

    for (const auto& c : candidates) {
        // Parse parent CSV (idStory IDs)
        std::vector<int> parents;
        const std::string& s = c.requiredStories;
        size_t pos = 0;
        while (pos < s.size()) {
            size_t comma = s.find(',', pos);
            if (comma == std::string::npos) comma = s.size();
            std::string tok = s.substr(pos, comma - pos);
            while (!tok.empty() && (tok.front() == ' ' || tok.front() == '\t'))
                tok.erase(0, 1);
            while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\t'))
                tok.pop_back();
            if (!tok.empty()) {
                int pid = std::atoi(tok.c_str());
                if (pid > 0) parents.push_back(pid);
            }
            pos = comma + 1;
        }
        if (parents.empty()) continue;

        // Every parent must satisfy this child's min thresholds
        bool allPass = true;
        for (int pid : parents) {
            sqlite3_reset(lookupParent);
            sqlite3_bind_text(lookupParent, 1, idUser, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (lookupParent, 2, pid);
            if (sqlite3_step(lookupParent) != SQLITE_ROW) {
                allPass = false; break;
            }
            int   pScore   = sqlite3_column_int   (lookupParent, 0);
            float pSpeed   = (float)sqlite3_column_double(lookupParent, 1);
            int   pRetries = sqlite3_column_int   (lookupParent, 2);
            if (pScore   < c.minScore   ||
                pSpeed   < c.minSpeed   ||
                pRetries < c.minRetries) {
                allPass = false; break;
            }
        }
        if (!allPass) continue;

        // Skip if already unlocked (idempotent re-runs on Console re-entry)
        sqlite3_reset(checkExisting);
        sqlite3_bind_text(checkExisting, 1, idUser, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (checkExisting, 2, c.idStory);
        sqlite3_bind_int (checkExisting, 3, c.idChapter);
        int already = 0;
        if (sqlite3_step(checkExisting) == SQLITE_ROW) {
            already = sqlite3_column_int(checkExisting, 0);
        }
        if (already == 1) continue;

        // Unlock
        sqlite3_reset(doUnlock);
        sqlite3_bind_text(doUnlock, 1, idUser, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (doUnlock, 2, c.idStory);
        sqlite3_bind_int (doUnlock, 3, c.idChapter);
        if (sqlite3_step(doUnlock) == SQLITE_DONE) {
            unlocked++;
            SDL_Log("[gameConsole_db] unlock story=%d chapter=%d",
                    c.idStory, c.idChapter);
        }
    }

    sqlite3_finalize(lookupParent);
    sqlite3_finalize(checkExisting);
    sqlite3_finalize(doUnlock);

    if (unlocked > 0) {
        SDL_Log("[gameConsole_db] dbCheckAndUnlockStories: %d newly unlocked",
                unlocked);
        dbSyncToPersistent();   // [F.7] only sync if rows actually changed
    }
    return unlocked;
}

bool dbInsertRecord(const GameRecord& rec) {
    if (!g_db) return false;
    std::string sql =
        "INSERT INTO " + userTable("Records") + " "
        "(idUser,startTS,endTS,idStory,idChapter,"
        " totalScore,totalSeconds,avgSpeed,retryNo) "
        "VALUES (?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbInsertRecord prepare fail: %s", sqlite3_errmsg(g_db));
        return false;
    }
    sqlite3_bind_text  (st, 1, rec.idUser.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64 (st, 2, rec.startTS);
    sqlite3_bind_int64 (st, 3, rec.endTS);
    sqlite3_bind_int   (st, 4, rec.idStory);
    sqlite3_bind_int   (st, 5, rec.idChapter);
    sqlite3_bind_int   (st, 6, rec.totalScore);
    sqlite3_bind_int   (st, 7, rec.totalSeconds);
    sqlite3_bind_double(st, 8, (double)rec.avgSpeed);
    sqlite3_bind_int   (st, 9, rec.retryNo);
    bool ok = (sqlite3_step(st) == SQLITE_DONE);
    sqlite3_finalize(st);
    if (ok) {
        SDL_Log("[gameConsole_db] dbInsertRecord: user=%s score=%d story=%d",
                rec.idUser.c_str(), rec.totalScore, rec.idStory);
        dbSyncToPersistent();
    }
    return ok;
}

bool dbUpsertStoryProgress(const char* idUser, int idStory, int idChapter,
                           bool isActivated, bool isSelected) {
    if (!g_db || !idUser || !*idUser) return false;
    std::string sql =
        "INSERT INTO " + userTable("Stories") + " "
        "(idUser,idStory,idChapter,isActivated,isSelected) "
        "VALUES (?,?,?,?,?) "
        "ON CONFLICT(idUser,idStory,idChapter) DO UPDATE SET "
        "  isActivated = MAX(isActivated, excluded.isActivated),"
        "  isSelected  = excluded.isSelected;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbUpsertStoryProgress prepare fail: %s", sqlite3_errmsg(g_db));
        return false;
    }
    sqlite3_bind_text(st, 1, idUser, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (st, 2, idStory);
    sqlite3_bind_int (st, 3, idChapter);
    sqlite3_bind_int (st, 4, isActivated ? 1 : 0);
    sqlite3_bind_int (st, 5, isSelected  ? 1 : 0);
    bool ok = (sqlite3_step(st) == SQLITE_DONE);
    sqlite3_finalize(st);
    if (ok) dbSyncToPersistent();
    return ok;
}

// gameconsole-nut-setting-10 / Issue 2.2-2.3
// Persist SettingsConfig to user_settings table (INSERT OR REPLACE per key).
bool dbSaveSettings(const SettingsConfig& cfg) {
    if (!g_db) return false;
    std::string sql =
        "INSERT OR REPLACE INTO " + userTable("Settings") +
        " (key, value) VALUES (?, ?);";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbSaveSettings prepare fail: %s",
                sqlite3_errmsg(g_db));
        return false;
    }
    auto kv = [&](const char* k, const char* v) {
        sqlite3_reset(st);
        sqlite3_bind_text(st, 1, k, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(st, 2, v, -1, SQLITE_TRANSIENT);
        sqlite3_step(st);
    };
    char buf[32];
    SDL_snprintf(buf, sizeof(buf), "%.4f", (double)cfg.volume);
    kv("volume", buf);
    for (int i = 0; i < 7; i++) {
        char key[12];
        SDL_snprintf(key, sizeof(key), "color%d", i);
        kv(key, cfg.colorEnabled[i] ? "1" : "0");
    }
    SDL_snprintf(buf, sizeof(buf), "%d", cfg.storyId);   kv("storyId",   buf);
    SDL_snprintf(buf, sizeof(buf), "%d", cfg.chapterId); kv("chapterId", buf);
    sqlite3_finalize(st);
    dbSyncToPersistent();
    SDL_Log("[gameConsole_db] settings saved");
    return true;
}

// Load SettingsConfig from user_settings table.
// Returns false when table has no rows (caller keeps struct defaults).
bool dbLoadSettings(SettingsConfig& cfg) {
    if (!g_db) return false;
    sqlite3_stmt* st = nullptr;
    std::string sql = "SELECT key, value FROM " + userTable("Settings") + ";";
    if (sqlite3_prepare_v2(g_db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) return false;
    bool found = false;
    while (sqlite3_step(st) == SQLITE_ROW) {
        found = true;
        const char* k = (const char*)sqlite3_column_text(st, 0);
        const char* v = (const char*)sqlite3_column_text(st, 1);
        if (!k || !v) continue;
        if (SDL_strcmp(k, "volume") == 0) {
            float vol = (float)SDL_atof(v);
            if (vol < 0.0f) vol = 0.0f;
            if (vol > 1.0f) vol = 1.0f;
            cfg.volume = vol;
        } else if (SDL_strcmp(k, "storyId")   == 0) { cfg.storyId   = SDL_atoi(v); }
        else if (SDL_strcmp(k, "chapterId") == 0) { cfg.chapterId = SDL_atoi(v); }
        else {
            for (int i = 0; i < 7; i++) {
                char expect[12];
                SDL_snprintf(expect, sizeof(expect), "color%d", i);
                if (SDL_strcmp(k, expect) == 0) {
                    cfg.colorEnabled[i] = (SDL_atoi(v) != 0);
                    break;
                }
            }
        }
    }
    sqlite3_finalize(st);
    if (found) SDL_Log("[gameConsole_db] settings loaded");
    return found;
}

// Issue 2.7: Load leaderboard from sync_Records (replaces board.json).
std::vector<BoardRecord> dbLoadRecords(int limit) {
    std::vector<BoardRecord> out;
    if (!g_db) return out;
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_db,
    "SELECT nameUser, totalScore, totalSeconds, avgSpeed, endTS "
    "FROM sync_Records ORDER BY totalScore DESC LIMIT ?;",
        -1, &st, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbLoadRecords prepare fail: %s",
                sqlite3_errmsg(g_db));
        return out;
    }
    sqlite3_bind_int(st, 1, limit);
    while (sqlite3_step(st) == SQLITE_ROW) {
        BoardRecord r;
        const unsigned char* u = sqlite3_column_text(st, 0);
        r.idUser       = u ? (const char*)u : "";
        r.totalScore   = sqlite3_column_int   (st, 1);
        r.totalSeconds = sqlite3_column_int   (st, 2);
        r.avgSpeed     = (float)sqlite3_column_double(st, 3);
        r.endTS        = (int64_t)sqlite3_column_int64(st, 4);
        out.push_back(std::move(r));
    }
    sqlite3_finalize(st);
    if (out.empty()) {
        const int n = (int)(sizeof(FALLBACK_BOARD_ROWS) / sizeof(FALLBACK_BOARD_ROWS[0]));
        out.reserve(n);
        for (int i = 0; i < n; i++) {
            const FallbackRow& r = FALLBACK_BOARD_ROWS[i];
            BoardRecord row;
            row.idUser = r.user;
            row.totalScore = r.score;
            row.totalSeconds = r.score / 2;
            row.avgSpeed = (float)r.score / (float)(r.score / 2 + 1);
            row.endTS = parseIso8601(r.iso);
            out.push_back(std::move(row));
        }
    }
    SDL_Log("[gameConsole_db] dbLoadRecords: %d rows", (int)out.size());
    return out;
}

static bool dbSelectStory(const char* idUser, int idStory, int idChapter) {
    if (!g_db || !idUser || !*idUser) return false;

    // Single-select semantics: clear any prior selection for this user
    sqlite3_stmt* clr = nullptr;
    {
        std::string clrSQL = "UPDATE " + userTable("Stories") +
            " SET isSelected = 0 WHERE idUser = ? AND isSelected = 1;";
        if (sqlite3_prepare_v2(g_db, clrSQL.c_str(), -1, &clr, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbSelectStory clear prepare fail: %s",
                sqlite3_errmsg(g_db));
        return false;
        }
    }
    sqlite3_bind_text(clr, 1, idUser, -1, SQLITE_TRANSIENT);
    sqlite3_step(clr);
    sqlite3_finalize(clr);

    // UPSERT new selection. Force isActivated=1 too, so selecting the intro
    // (which has no idUser_Stories row before its first play) creates the
    // row correctly and the radio "stays" highlighted across reopen.
    sqlite3_stmt* up = nullptr;
    {
        std::string upSQL =
            "INSERT INTO " + userTable("Stories") +
            "  (idUser, idStory, idChapter, isActivated, isSelected) "
            "VALUES (?, ?, ?, 1, 1) "
            "ON CONFLICT(idUser, idStory, idChapter) DO UPDATE SET "
            "  isActivated = 1, isSelected = 1;";
        if (sqlite3_prepare_v2(g_db, upSQL.c_str(), -1, &up, nullptr) != SQLITE_OK) {
        SDL_Log("[gameConsole_db] dbSelectStory upsert prepare fail: %s",
                sqlite3_errmsg(g_db));
        return false;
        }
    }
    sqlite3_bind_text(up, 1, idUser, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (up, 2, idStory);
    sqlite3_bind_int (up, 3, idChapter);
    sqlite3_step(up);
    sqlite3_finalize(up);

    SDL_Log("[gameConsole_db] select story=%d chapter=%d", idStory, idChapter);
    dbSyncToPersistent();   // [F.7]
    return true;
}

// =========================================================
// [A.2] Main button list -- 5 buttons (was 4 in v1)
// Insert SETTINGS between PLAY and QUIT to keep QUIT as last.
// Layout math: each button 30px tall. y values stay in 30px stride
// (not 40 as the popup-plan suggested) to fit within 160..350 band
// without overlapping the bottom hint text starting at y=380.
//   GUIDE    y=160 .. 190
//   BOARD    y=200 .. 230
//   PLAY     y=240 .. 270
//   SETTINGS y=280 .. 310
//   QUIT     y=320 .. 350
//   (gap 30px) ; hint text starts y=380
// =========================================================
// [F.2] 6 buttons -- y range 130..360. Stride 40px keeps original rhythm;
// start y shifted up from 160 to 130 to fit STORIES insertion without
// pushing QUIT into the hint-text band (y=380..425).
static const float BTN_W = 140.0f;
static const float BTN_H = 30.0f;
static const float BTN_X = (CONSOLE_SCREEN_WIDTH - BTN_W) / 2.0f;
static const Button MAIN_BUTTONS[] = {
    { { BTN_X, 130.0f, BTN_W, BTN_H }, "GUIDE",    { 70,  90, 160, 255} },
    { { BTN_X, 170.0f, BTN_W, BTN_H }, "BOARD",    { 70, 130,  90, 255} },
    { { BTN_X, 210.0f, BTN_W, BTN_H }, "PLAY",     {180,  70,  70, 255} },
    { { BTN_X, 250.0f, BTN_W, BTN_H }, "STORIES",  {170, 110,  70, 255} },
    { { BTN_X, 290.0f, BTN_W, BTN_H }, "SETTINGS", {120,  90, 150, 255} },
    { { BTN_X, 330.0f, BTN_W, BTN_H }, "QUIT",     {110, 110, 110, 255} }
};
static const int NUM_MAIN_BUTTONS = (int)(sizeof(MAIN_BUTTONS)/sizeof(MAIN_BUTTONS[0]));
// Index constants -- dung trong activateButton + ESC-jump-to-quit
static const int BTN_IDX_GUIDE    = 0;
static const int BTN_IDX_BOARD    = 1;
static const int BTN_IDX_PLAY     = 2;
static const int BTN_IDX_STORIES  = 3;   // [F.2] new
static const int BTN_IDX_SETTINGS = 4;
static const int BTN_IDX_QUIT     = 5;

static const SDL_FRect GUIDE_POPUP = { 5.0f, 20.0f, 260.0f, 440.0f };
static const SDL_FRect GUIDE_CLOSE = { GUIDE_POPUP.x + GUIDE_POPUP.w - 22.0f,
                                       GUIDE_POPUP.y + 4.0f, 18.0f, 18.0f };
static const SDL_FRect BOARD_POPUP = { 5.0f, 30.0f, 260.0f, 420.0f };
static const SDL_FRect BOARD_CLOSE = { BOARD_POPUP.x + BOARD_POPUP.w - 22.0f,
                                       BOARD_POPUP.y + 4.0f, 18.0f, 18.0f };

// [A.3] Settings popup geometry -- mirrors GUIDE_POPUP for consistency.
// Future tasks (D.x volume, E.x colors, I.x load/save) populate the body.
static const SDL_FRect SETTINGS_POPUP = { 5.0f, 20.0f, 260.0f, 440.0f };
static const SDL_FRect SETTINGS_CLOSE = { SETTINGS_POPUP.x + SETTINGS_POPUP.w - 22.0f,
                                          SETTINGS_POPUP.y + 4.0f, 18.0f, 18.0f };

// [F.2] Stories popup geometry -- same footprint as Settings/Guide for visual
// consistency. Body (thumbnail, story list, chapter navigator) is populated
// by drawStoriesLightbox in Step 2.6.7. This step ships a placeholder body.
static const SDL_FRect STORIES_POPUP = { 5.0f, 20.0f, 260.0f, 440.0f };
static const SDL_FRect STORIES_CLOSE = { STORIES_POPUP.x + STORIES_POPUP.w - 22.0f,
                                         STORIES_POPUP.y + 4.0f, 18.0f, 18.0f };

// [F.3] Stories popup body layout
//   y range inside popup (popup y=20..460):
//     20..50   title band
//     60..210  thumbnail (150x150, centered)
//    220..400  9 story rows (each 20px tall)
//    408..446  chapter navigator (title + arrows + n/x)
static const float STORIES_THUMB_Y = STORIES_POPUP.y + 40.0f;       // 60
static const float STORIES_THUMB_W = 150.0f;
static const float STORIES_THUMB_H = 150.0f;
static const float STORIES_THUMB_X = STORIES_POPUP.x +
                                     (STORIES_POPUP.w - STORIES_THUMB_W) / 2.0f;
static const SDL_FRect STORIES_THUMB = { STORIES_THUMB_X, STORIES_THUMB_Y,
                                         STORIES_THUMB_W, STORIES_THUMB_H };

static const int   STORIES_LIST_VISIBLE = 10;
static const float STORIES_LIST_Y       = STORIES_THUMB_Y + STORIES_THUMB_H + 10.0f; // 220
static const float STORIES_ROW_H        = 18.0f;
static const float STORIES_ROW_X        = STORIES_POPUP.x + 8.0f;
static const float STORIES_ROW_W        = STORIES_POPUP.w - 16.0f;

static SDL_FRect storiesRowRect(int i) {
    return SDL_FRect{
        STORIES_ROW_X,
        STORIES_LIST_Y + i * STORIES_ROW_H,
        STORIES_ROW_W,
        STORIES_ROW_H - 1.0f
    };
}
static SDL_FRect storiesRadioRect(int i) {
    SDL_FRect row = storiesRowRect(i);
    const float sz = 12.0f;
    return SDL_FRect{ row.x + 4.0f,
                      row.y + (row.h - sz) / 2.0f,
                      sz, sz };
}
static SDL_FRect storiesPlayRect(int i) {
    SDL_FRect row = storiesRowRect(i);
    const float sz = 14.0f;
    return SDL_FRect{ row.x + row.w - sz - 4.0f,
                      row.y + (row.h - sz) / 2.0f,
                      sz, sz };
}

// Chapter navigator (bottom band)
static const float STORIES_NAV_TITLE_Y = STORIES_LIST_Y +
                                         STORIES_LIST_VISIBLE * STORIES_ROW_H + 8.0f; // 408
static const float STORIES_NAV_BTN_Y   = STORIES_NAV_TITLE_Y + 22.0f;                  // 430
static const SDL_FRect STORIES_NAV_LEFT  = {
    STORIES_POPUP.x + 50.0f, STORIES_NAV_BTN_Y, 20.0f, 16.0f
};
static const SDL_FRect STORIES_NAV_RIGHT = {
    STORIES_POPUP.x + STORIES_POPUP.w - 70.0f, STORIES_NAV_BTN_Y, 20.0f, 16.0f
};

// =========================================================
// [D.1] Volume slider geometry (absolute coords trong popup body)
//   y=50  : "VOLUME XX%" label
//   y=72  : track 220x6 (mau xam)
//   y=66  : thumb 12x18 (rectangular, drag-able)
// Hit rect cua thumb tinh dong moi frame: rect = thumb_pos +- 4px slack
// de touch interaction de hit hon (mobile).
// =========================================================
static const float VOL_LABEL_Y = SETTINGS_POPUP.y + 50.0f;
static const SDL_FRect VOL_TRACK = {
    SETTINGS_POPUP.x + 20.0f,
    SETTINGS_POPUP.y + 80.0f,
    220.0f, 6.0f
};
static const float VOL_THUMB_W = 12.0f;
static const float VOL_THUMB_H = 18.0f;

// Compute current thumb rect from volume value [0..1]
static SDL_FRect volThumbRect(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    float trackUsableW = VOL_TRACK.w - VOL_THUMB_W;
    return SDL_FRect{
        VOL_TRACK.x + trackUsableW * volume,
        VOL_TRACK.y + VOL_TRACK.h / 2.0f - VOL_THUMB_H / 2.0f,
        VOL_THUMB_W,
        VOL_THUMB_H
    };
}

// =========================================================
// [E.1] BLOCK_PALETTE -- 7 mau khoi nguoi choi co the bat/tat.
// Thu tu MUST khop voi SettingsConfig.colorEnabled[] (gameConsole_layout.h):
//   0=red 1=orange 2=pink 3=yellow 4=green 5=blue 6=purple
// gameCore se chi pick mau o cac index co colorEnabled[i]=true.
// =========================================================
static const int BLOCK_PALETTE_N = 7;
static const SDL_Color BLOCK_PALETTE[BLOCK_PALETTE_N] = {
    {220,  60,  60, 255},  // red
    {240, 140,  40, 255},  // orange
    {240, 130, 200, 255},  // pink
    {240, 220,  60, 255},  // yellow
    { 80, 200,  90, 255},  // green
    { 70, 130, 230, 255},  // blue
    {170,  90, 220, 255}   // purple
};

// [E.2] Swatch geometry: 7 × 28px o vuong, gap 3px, can giua trong popup.
// Header label "BLOCK COLORS" o y=110, swatches o y=130.
static const float SWATCH_LABEL_Y = SETTINGS_POPUP.y + 110.0f;
static const float SWATCH_W       = 28.0f;
static const float SWATCH_H       = 28.0f;
static const float SWATCH_GAP     = 3.0f;
static const float SWATCH_TOTAL_W = BLOCK_PALETTE_N * SWATCH_W +
                                    (BLOCK_PALETTE_N - 1) * SWATCH_GAP;
static const float SWATCH_X0      = SETTINGS_POPUP.x +
                                    (SETTINGS_POPUP.w - SWATCH_TOTAL_W) / 2.0f;
static const float SWATCH_Y       = SETTINGS_POPUP.y + 130.0f;

static SDL_FRect swatchRect(int i) {
    return SDL_FRect{
        SWATCH_X0 + i * (SWATCH_W + SWATCH_GAP),
        SWATCH_Y,
        SWATCH_W, SWATCH_H
    };
}

// Count of currently enabled colors (E.4 guard: must stay >= 1)
static int countEnabled(const SettingsConfig& cfg) {
    int n = 0;
    for (int i = 0; i < BLOCK_PALETTE_N; i++) if (cfg.colorEnabled[i]) n++;
    return n;
}

// =========================================================
// [D.5] Audio stream + gain plumbing.
// V1 stub: open a SILENT playback stream so SDL_SetAudioStreamGain has a
// valid target (volume slider feels real, gain calls return true). V2
// will queue actual music samples to this same stream -- gain control
// will then audibly affect playback with zero rework.
// Best-effort: if no audio device (CI / headless), g_bgmStream stays
// null and applyVolumeToStream becomes a no-op.
// =========================================================
static SDL_AudioStream* g_bgmStream = nullptr;

static void applyVolumeToStream(float vol) {
    if (!g_bgmStream) return;
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    SDL_SetAudioStreamGain(g_bgmStream, vol);
}

static void closeAudioStream() {
    if (g_bgmStream) {
        SDL_DestroyAudioStream(g_bgmStream);
        g_bgmStream = nullptr;
    }
}

static const float GUIDE_SB_X = GUIDE_POPUP.x + GUIDE_POPUP.w - 12.0f;
static const float GUIDE_SB_Y = GUIDE_POPUP.y + 30.0f;
static const float GUIDE_SB_H = GUIDE_POPUP.h - 40.0f;
static const float BOARD_ROW_H = 36.0f;
static const float BOARD_SB_X  = BOARD_POPUP.x + BOARD_POPUP.w - 12.0f;
static const float BOARD_SB_Y  = BOARD_POPUP.y + 60.0f;
static const float BOARD_SB_H  = BOARD_VISIBLE * BOARD_ROW_H;

// [G/H] Clickable column-header buttons. Positions are computed from the
// 8px-per-char SDL_RenderDebugText grid so the rect aligns with the
// rendered "SCORE" / "TIME" text below.
static const float    BOARD_HDR_Y           = BOARD_POPUP.y + 38.0f;
static const float    BOARD_HDR_HASH_X      = BOARD_POPUP.x + 8.0f;
static const float    BOARD_HDR_USER_X      = BOARD_POPUP.x + 8.0f + 24.0f;
static const float    BOARD_HDR_SCORE_X     = BOARD_POPUP.x + 8.0f + 128.0f;
static const float    BOARD_HDR_SCORE_ARR_X = BOARD_HDR_SCORE_X + 40.0f;
static const float    BOARD_HDR_TIME_X      = BOARD_POPUP.x + 8.0f + 184.0f;
static const float    BOARD_HDR_TIME_ARR_X  = BOARD_HDR_TIME_X + 32.0f;
static const SDL_FRect BOARD_HDR_SCORE_BTN  = {
    BOARD_HDR_SCORE_X - 2.0f, BOARD_HDR_Y - 3.0f, 50.0f, 14.0f
};
static const SDL_FRect BOARD_HDR_TIME_BTN   = {
    BOARD_HDR_TIME_X - 2.0f, BOARD_HDR_Y - 3.0f, 42.0f, 14.0f
};

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

static const char* GUIDE_LINES[] = {
    "HOW TO PLAY",
    "",
    "Movement keys:",
    " - LEFT / A : move left",
    " - RIGHT / D : move right",
    " - UP / W : rotate CCW",
    " - DOWN / S : rotate CW",
    " - SPACE : speed boost x5",
    " - ENTER : pause / resume",
    " - ESC : open quit menu",
    "",
    "Sidebar (12 components):",
    "  1 QUIT  : open quit menu",
    "  2 PAUSE : stop / play",
    "  3 SCORE : current points",
    "  4 TIMER : total play time",
    "  5 NEXT-1: upcoming piece",
    "  6 NEXT-2: reserved (v2)",
    "  7 NEXT-3: reserved (v3)",
    "  8 ARR UP    = key UP/W",
    "  9 ARR DOWN  = key DOWN/S",
    " 10 ARR LEFT  = key LEFT/A",
    " 11 ARR RIGHT = key RIGHT/D",
    " 12 SPEED BOOST: hold=SPACE",
    "",
    "Quit popup options:",
    " - Restart: new game, score 0",
    " - Console: back to setting",
    " - Quit   : close app",
    " - Cancel : keep paused",
    "",
    "Tip: hold SPEED BOOST",
    "to make piece drop 5x faster.",
    "",
    "Mobile swipe gestures:",
    " - Swipe piece L: move L",
    " - Swipe piece R: move R",
    " - Swipe board U: rotate CCW",
    " - Swipe board D: rotate CW",
    " (piece = falling block)",
    " (board = empty cells)",
    "",
    "Press X or ESC to close.",
};
static const int GUIDE_LINE_COUNT = (int)(sizeof(GUIDE_LINES)/sizeof(GUIDE_LINES[0]));
static const int GUIDE_VISIBLE_LINES = 24;

// [B.3] drawBackground: lazy-init bg texture lan goi dau, sau do moi frame
// chi can SDL_RenderTexture vao dst rect = full screen logical (270x480).
// SVG nguon 270x480 cung ti le 9:16 voi screen -> dst rect = entire renderer
// area, KHONG can letterbox. Neu sau nay doi SVG sang aspect khac, them
// math letterbox tai day (so sanh g_consoleBg.w/h voi screen w/h).
//
// Fallback: neu loadSvgTextureFromMem fail (out-of-mem hoac SVG hong),
// drop ve hanh vi V1 (slate fill thuan) de UI van usable.
static void drawBackground(SDL_Renderer* renderer) {
    // Lazy init: chi rasterize 1 lan duy nhat trong vong doi runGameConsole
    if (!g_consoleBg.texture) {
        g_consoleBg = loadSvgTextureFromMem(renderer,
                                            GAMECONSOLE_BG_SVG_DATA,
                                            CONSOLE_SCREEN_WIDTH);
    }

    if (g_consoleBg.texture) {
        // Black clear -- only seen at letterbox edges if aspect ever drifts
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_FRect dst = { 0, 0,
                          (float)CONSOLE_SCREEN_WIDTH,
                          (float)CONSOLE_SCREEN_HEIGHT };
        SDL_RenderTexture(renderer, g_consoleBg.texture, NULL, &dst);
    } else {
        // Fallback V1: slate fill thuan -- texture init failed
        SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
        SDL_RenderClear(renderer);
    }

    // UI text overlay (giu nguyen tu V1, render len tren bg)
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    const char* title = "C T E T R I S";
    int titleLen = (int)SDL_strlen(title);
    float titleX = (CONSOLE_SCREEN_WIDTH - titleLen * 8.0f) / 2.0f;
    SDL_RenderDebugText(renderer, titleX, 60.0f, title);
    float subX = (CONSOLE_SCREEN_WIDTH - 18 * 8.0f) / 2.0f;
    SDL_RenderDebugText(renderer, subX, 90.0f, "-- GAME CONSOLE --");

    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderDebugText(renderer, 10.0f, 380.0f, "TAB/DOWN/LEFT(S/A): next");
    SDL_RenderDebugText(renderer, 10.0f, 395.0f, "SHIFT+TAB/UP/RIGHT(W/D):prev");
    SDL_RenderDebugText(renderer, 10.0f, 410.0f, "ENTER/SPACE: select");
    SDL_RenderDebugText(renderer, 10.0f, 425.0f, "ESC: close / cancel");
}

static void drawCloseButton(SDL_Renderer* renderer, const SDL_FRect& r) {
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &r);
    SDL_RenderDebugText(renderer, r.x + r.w/2 - 4, r.y + r.h/2 - 4, "X");
}

static void drawGuideLightbox(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 70, 80, 110, 255);
    SDL_RenderFillRect(renderer, &GUIDE_POPUP);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &GUIDE_POPUP);

    const float CONTENT_X = GUIDE_POPUP.x + 8;
    const float CONTENT_Y0 = GUIDE_POPUP.y + 30;
    const float ROW_H = 14.0f;
    const int   MAX_CHARS = 28;
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    int rowDrawn = 0;
    for (int i = 0; i < GUIDE_VISIBLE_LINES && rowDrawn < GUIDE_VISIBLE_LINES; i++) {
        int idx = state.guideScroll + i;
        if (idx >= GUIDE_LINE_COUNT) break;
        int used = drawWrappedText(renderer, GUIDE_LINES[idx],
                                   CONTENT_X, CONTENT_Y0 + rowDrawn * ROW_H,
                                   MAX_CHARS, 2);
        rowDrawn += (used > 0) ? used : 1;
    }

    SBLayout sb = layoutSB(GUIDE_SB_X, GUIDE_SB_Y, GUIDE_SB_H,
                           GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES, state.guideScroll);
    drawSB(renderer, sb, GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES,
           state.sb.upHeld, state.sb.downHeld, state.sb.dragging);

    drawCloseButton(renderer, GUIDE_CLOSE);
}

static void drawBoardLightbox(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    SDL_SetRenderDrawColor(renderer, 50, 100, 70, 255);
    SDL_RenderFillRect(renderer, &BOARD_POPUP);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &BOARD_POPUP);

    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 80, BOARD_POPUP.y + 14, "LEADERBOARD");

    // [G/H] Column header row with clickable SCORE / TIME buttons.
    bool scoreActive = (state.boardSortMode == BOARD_SORT_SCORE_DESC ||
                        state.boardSortMode == BOARD_SORT_SCORE_ASC);
    bool timeActive  = (state.boardSortMode == BOARD_SORT_TIME_DESC  ||
                        state.boardSortMode == BOARD_SORT_TIME_ASC);
    const char* scoreArrow =
        (state.boardSortMode == BOARD_SORT_SCORE_DESC) ? "v" :
        (state.boardSortMode == BOARD_SORT_SCORE_ASC ) ? "^" : " ";
    const char* timeArrow  =
        (state.boardSortMode == BOARD_SORT_TIME_DESC ) ? "v" :
        (state.boardSortMode == BOARD_SORT_TIME_ASC  ) ? "^" : " ";

    // Static columns
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer, BOARD_HDR_HASH_X, BOARD_HDR_Y, "#");
    SDL_RenderDebugText(renderer, BOARD_HDR_USER_X, BOARD_HDR_Y, "USER");

    // SCORE button background (only when active)
    if (scoreActive) {
        SDL_SetRenderDrawColor(renderer, 50, 130, 50, 255);
        SDL_RenderFillRect(renderer, &BOARD_HDR_SCORE_BTN);
    }
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer, BOARD_HDR_SCORE_X,     BOARD_HDR_Y, "SCORE");
    SDL_RenderDebugText(renderer, BOARD_HDR_SCORE_ARR_X, BOARD_HDR_Y, scoreArrow);

    // TIME button background (only when active)
    if (timeActive) {
        SDL_SetRenderDrawColor(renderer, 50, 130, 50, 255);
        SDL_RenderFillRect(renderer, &BOARD_HDR_TIME_BTN);
    }
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer, BOARD_HDR_TIME_X,     BOARD_HDR_Y, "TIME");
    SDL_RenderDebugText(renderer, BOARD_HDR_TIME_ARR_X, BOARD_HDR_Y, timeArrow);

    const float ROW_Y0 = BOARD_SB_Y;
    char line[64];
    for (int i = 0; i < BOARD_VISIBLE; i++) {
        int idx = state.boardScroll + i;
        if (idx >= g_boardTotal) break;
        const BoardEntry& e = g_board[idx];
        float y = ROW_Y0 + i * BOARD_ROW_H;
        // Keep the compact HH:MM display used by the popup while sorting
        // still uses the full timeEpoch value.
        const char* timeText = e.time.c_str();
        size_t timeLen = e.time.size();
        if (timeLen >= 5) timeText += timeLen - 5;
        SDL_snprintf(line, sizeof(line), "%2d %-12.12s %5d %s",
                     idx + 1, e.user.c_str(), e.score, timeText);
        SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8, y, line);
    }

    SBLayout sb = layoutSB(BOARD_SB_X, BOARD_SB_Y, BOARD_SB_H,
                           g_boardTotal, BOARD_VISIBLE, state.boardScroll);
    drawSB(renderer, sb, g_boardTotal, BOARD_VISIBLE,
           state.sb.upHeld, state.sb.downHeld, state.sb.dragging);

    SDL_RenderDebugText(renderer, BOARD_POPUP.x + 8,
                        BOARD_POPUP.y + BOARD_POPUP.h - 14,
                        "W/S to scroll, ESC close");

    drawCloseButton(renderer, BOARD_CLOSE);
}

// [A.3/D.2/E.2] Settings popup body. Volume slider + color swatches.
// Future: stories button (F), load/save (I) added below color row.
static void drawSettingsLightbox(SDL_Renderer* renderer, const AppState& state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    // Panel -- mau tim nhe de phan biet voi GUIDE (xanh duong) & BOARD (xanh la)
    SDL_SetRenderDrawColor(renderer, 80, 60, 100, 255);
    SDL_RenderFillRect(renderer, &SETTINGS_POPUP);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &SETTINGS_POPUP);

    // Header
    const char* title = "SETTINGS";
    int tl = (int)SDL_strlen(title);
    SDL_RenderDebugText(renderer,
                        SETTINGS_POPUP.x + (SETTINGS_POPUP.w - tl * 8.0f) / 2.0f,
                        SETTINGS_POPUP.y + 14, title);

    // ===== [D.2] VOLUME SECTION =====
    if (state.cfg) {
        char volLine[24];
        int volPct = (int)(state.cfg->volume * 100.0f + 0.5f);
        if (volPct < 0)   volPct = 0;
        if (volPct > 100) volPct = 100;
        SDL_snprintf(volLine, sizeof(volLine), "VOLUME %3d%%", volPct);
        SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
        SDL_RenderDebugText(renderer, SETTINGS_POPUP.x + 20.0f, VOL_LABEL_Y, volLine);

        // Track (xam toi)
        SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
        SDL_RenderFillRect(renderer, &VOL_TRACK);
        // Filled portion (highlight) tu trai den thumb
        SDL_FRect thumb = volThumbRect(state.cfg->volume);
        SDL_FRect filled = { VOL_TRACK.x, VOL_TRACK.y,
                             thumb.x + thumb.w / 2.0f - VOL_TRACK.x,
                             VOL_TRACK.h };
        if (filled.w < 0) filled.w = 0;
        SDL_SetRenderDrawColor(renderer, 120, 200, 120, 255);
        SDL_RenderFillRect(renderer, &filled);
        // Thumb (yellow when dragging, white otherwise -- match scrollbar UX)
        SDL_Color thumbColor = state.draggingVolume ? HIGHLIGHT_Y : SOFT_WHITE;
        SDL_SetRenderDrawColor(renderer, thumbColor.r, thumbColor.g, thumbColor.b, 255);
        SDL_RenderFillRect(renderer, &thumb);
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_RenderRect(renderer, &thumb);
    }

    // ===== [E.2] BLOCK COLORS SECTION =====
    if (state.cfg) {
        SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
        SDL_RenderDebugText(renderer, SETTINGS_POPUP.x + 20.0f,
                            SWATCH_LABEL_Y, "BLOCK COLORS");

        for (int i = 0; i < BLOCK_PALETTE_N; i++) {
            SDL_FRect r = swatchRect(i);
            SDL_Color c = BLOCK_PALETTE[i];
            if (!state.cfg->colorEnabled[i]) {
                // Disabled: dim fill (50% mau goc) + no ring
                SDL_SetRenderDrawColor(renderer,
                                       (Uint8)(c.r / 3),
                                       (Uint8)(c.g / 3),
                                       (Uint8)(c.b / 3), 255);
                SDL_RenderFillRect(renderer, &r);
                // Dashed border de bao "co the click bat lai"
                SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
                SDL_RenderRect(renderer, &r);
            } else {
                // Enabled: full color fill + white outline ring (2px)
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
                SDL_RenderFillRect(renderer, &r);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                for (int k = 0; k < 2; k++) {
                    SDL_FRect ring = { r.x - k, r.y - k,
                                       r.w + 2 * k, r.h + 2 * k };
                    SDL_RenderRect(renderer, &ring);
                }
            }
        }

        // [E.4] Helper text neu user co the bi block
        if (countEnabled(*state.cfg) <= 1) {
            SDL_SetRenderDrawColor(renderer, 200, 180, 80, 255);
            SDL_RenderDebugText(renderer, SETTINGS_POPUP.x + 20.0f,
                                SWATCH_Y + SWATCH_H + 8.0f,
                                "(at least 1 color required)");
        }
    }

    // Hint footer
    SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
    SDL_RenderDebugText(renderer, SETTINGS_POPUP.x + 20.0f,
                        SETTINGS_POPUP.y + SETTINGS_POPUP.h - 28.0f,
                        "LEFT/RIGHT: volume +-5%");
    SDL_RenderDebugText(renderer, SETTINGS_POPUP.x + 20.0f,
                        SETTINGS_POPUP.y + SETTINGS_POPUP.h - 14.0f,
                        "Click swatch: toggle color");

    drawCloseButton(renderer, SETTINGS_CLOSE);
}

static void drawStoriesLightbox(SDL_Renderer* renderer, const AppState& state) {
    // [F.4] Stories popup -- DB-driven body. Three visual states per row:
    //   LOCKED    : isActivated == false  -> dim bg, dim outline radio,
    //                                        no play button, muted text
    //   ACTIVE    : isActivated, !played  -> normal row, white outline radio,
    //                                        green play button
    //   COMPLETED : isActivated, played   -> green-tinted bg, green-filled radio,
    //                                        green play button, green text
    // Play button hidden on locked rows per task.md spec.
    enum StoryUiState { STORY_UI_LOCKED, STORY_UI_ACTIVE, STORY_UI_COMPLETED };

    auto classifyStory = [](const StoryRow& r) -> StoryUiState {
        if (!r.isActivated) return STORY_UI_LOCKED;
        // V2 heuristic: any row with prior play history counts as completed.
        // 2.6.10 will refine via dbCheckAndUnlockStories() unlock cascade.
        if (r.totalRetries > 0 || r.lastMaxScore > 0) return STORY_UI_COMPLETED;
        return STORY_UI_ACTIVE;
    };

    // Background dimmer
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen = { 0, 0, (float)CONSOLE_SCREEN_WIDTH, (float)CONSOLE_SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &screen);

    // Panel (warm terracotta)
    SDL_SetRenderDrawColor(renderer, 90, 70, 60, 255);
    SDL_RenderFillRect(renderer, &STORIES_POPUP);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &STORIES_POPUP);

    // ===== Title =====
    const char* title = "STORIES";
    int tl = (int)SDL_strlen(title);
    SDL_RenderDebugText(renderer,
                        STORIES_POPUP.x + (STORIES_POPUP.w - tl * 8.0f) / 2.0f,
                        STORIES_POPUP.y + 14, title);

    // ===== Thumbnail (placeholder until thumbnail texture loading lands) =====
    SDL_SetRenderDrawColor(renderer, 50, 40, 35, 255);
    SDL_RenderFillRect(renderer, &STORIES_THUMB);
    SDL_SetRenderDrawColor(renderer, 160, 130, 100, 255);
    SDL_RenderRect(renderer, &STORIES_THUMB);
    const char* thumbLbl = "THUMBNAIL";
    int thl = (int)SDL_strlen(thumbLbl);
    SDL_RenderDebugText(renderer,
                        STORIES_THUMB.x + (STORIES_THUMB.w - thl * 8.0f) / 2.0f,
                        STORIES_THUMB.y + STORIES_THUMB.h / 2.0f - 4.0f,
                        thumbLbl);

    // ===== Story rows =====
    int rowsToFill = (int)state.storiesCache.size();
    if (rowsToFill > STORIES_LIST_VISIBLE) rowsToFill = STORIES_LIST_VISIBLE;

    for (int i = 0; i < STORIES_LIST_VISIBLE; i++) {
        SDL_FRect row = storiesRowRect(i);

        if (i >= rowsToFill) {
            // Empty slot (chapter has fewer stories than visible rows)
            if (i & 1) {
                SDL_SetRenderDrawColor(renderer, 75, 60, 55, 255);
                SDL_RenderFillRect(renderer, &row);
            }
            continue;
        }

        const StoryRow& sr = state.storiesCache[i];
        StoryUiState us = classifyStory(sr);

        // Row background per state
        bool fillBg = false;
        SDL_Color bgColor = {0,0,0,0};
        if (us == STORY_UI_LOCKED) {
            bgColor = SDL_Color{60, 50, 45, 255};
            fillBg = true;
        } else if (us == STORY_UI_COMPLETED) {
            bgColor = SDL_Color{55, 85, 60, 255};
            fillBg = true;
        } else if (i & 1) {
            bgColor = SDL_Color{70, 55, 50, 255};
            fillBg = true;
        }
        if (fillBg) {
            SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 255);
            SDL_RenderFillRect(renderer, &row);
        }

        // Radio button
        SDL_FRect radio = storiesRadioRect(i);
        if (us == STORY_UI_LOCKED) {
            SDL_SetRenderDrawColor(renderer, 100, 90, 85, 255);
            SDL_RenderRect(renderer, &radio);
        } else if (us == STORY_UI_COMPLETED) {
            SDL_SetRenderDrawColor(renderer, 90, 200, 100, 255);
            SDL_RenderFillRect(renderer, &radio);
            SDL_SetRenderDrawColor(renderer, 220, 240, 220, 255);
            SDL_RenderRect(renderer, &radio);
        } else {
            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
            SDL_RenderRect(renderer, &radio);
        }
        // Selected indicator -- yellow ring (story-click wiring lands in 2.6.11)
        if (sr.isSelected) {
            SDL_SetRenderDrawColor(renderer, HIGHLIGHT_Y.r, HIGHLIGHT_Y.g,
                                   HIGHLIGHT_Y.b, 255);
            for (int k = 0; k < 2; k++) {
                SDL_FRect ring = { radio.x - 1 - k, radio.y - 1 - k,
                                   radio.w + 2 + 2*k, radio.h + 2 + 2*k };
                SDL_RenderRect(renderer, &ring);
            }
        }

        // Story name (truncated to fit if needed; row width ~244, minus
        // 26 left margin and 20 right margin = ~198px usable = ~24 chars).
        SDL_Color tc;
        if      (us == STORY_UI_LOCKED)    tc = SDL_Color{125, 115, 105, 255};
        else if (us == STORY_UI_COMPLETED) tc = SDL_Color{200, 240, 200, 255};
        else                                tc = SOFT_WHITE;
        SDL_SetRenderDrawColor(renderer, tc.r, tc.g, tc.b, 255);

        char buf[28];
        int n = (int)sr.storyName.size();
        if (n > 24) n = 24;
        SDL_memcpy(buf, sr.storyName.c_str(), (size_t)n);
        buf[n] = '\0';
        SDL_RenderDebugText(renderer, row.x + 22.0f, row.y + 5.0f, buf);

        // Play button -- HIDDEN for locked rows (per task.md spec)
        if (us != STORY_UI_LOCKED) {
            SDL_FRect play = storiesPlayRect(i);
            SDL_SetRenderDrawColor(renderer, 100, 160, 100, 255);
            SDL_RenderFillRect(renderer, &play);
            SDL_SetRenderDrawColor(renderer, 220, 240, 220, 255);
            SDL_RenderRect(renderer, &play);
        }
    }

    // ===== Chapter navigator =====
    // Real chapterName from first cached row; fallback if empty.
    std::string chapTitle;
    if (!state.storiesCache.empty() && !state.storiesCache[0].chapterName.empty()) {
        chapTitle = state.storiesCache[0].chapterName;
    } else {
        chapTitle = "(chapter)";
    }
    char chapBuf[48];
    SDL_snprintf(chapBuf, sizeof(chapBuf), "%s", chapTitle.c_str());
    int ntl = (int)SDL_strlen(chapBuf);
    if (ntl > 30) ntl = 30;  // clamp to popup width
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderDebugText(renderer,
                        STORIES_POPUP.x + (STORIES_POPUP.w - ntl * 8.0f) / 2.0f,
                        STORIES_NAV_TITLE_Y, chapBuf);

    // Arrow buttons (click handling lands in 2.6.9)
    SDL_SetRenderDrawColor(renderer, 130, 100, 80, 255);
    SDL_RenderFillRect(renderer, &STORIES_NAV_LEFT);
    SDL_RenderFillRect(renderer, &STORIES_NAV_RIGHT);
    SDL_SetRenderDrawColor(renderer, SOFT_WHITE.r, SOFT_WHITE.g, SOFT_WHITE.b, 255);
    SDL_RenderRect(renderer, &STORIES_NAV_LEFT);
    SDL_RenderRect(renderer, &STORIES_NAV_RIGHT);
    SDL_RenderDebugText(renderer,
                        STORIES_NAV_LEFT.x + STORIES_NAV_LEFT.w / 2.0f - 4.0f,
                        STORIES_NAV_LEFT.y + STORIES_NAV_LEFT.h / 2.0f - 4.0f, "<");
    SDL_RenderDebugText(renderer,
                        STORIES_NAV_RIGHT.x + STORIES_NAV_RIGHT.w / 2.0f - 4.0f,
                        STORIES_NAV_RIGHT.y + STORIES_NAV_RIGHT.h / 2.0f - 4.0f, ">");

    // Page indicator from real maxChapter
    int maxCh = state.storiesMaxChapter < 1 ? 1 : state.storiesMaxChapter;
    int curCh = state.currentStoryChapter;
    if (curCh < 1)     curCh = 1;
    if (curCh > maxCh) curCh = maxCh;
    char pageLbl[16];
    SDL_snprintf(pageLbl, sizeof(pageLbl), "%d/%d", curCh, maxCh);
    int pll = (int)SDL_strlen(pageLbl);
    SDL_RenderDebugText(renderer,
                        STORIES_POPUP.x + (STORIES_POPUP.w - pll * 8.0f) / 2.0f,
                        STORIES_NAV_BTN_Y + 4.0f, pageLbl);

    drawCloseButton(renderer, STORIES_CLOSE);
}

// [D.5] Open silent BGM stream best-effort. V1 doesnt feed any samples
// (no music files yet), but volume slider's gain calls are tracked so
// that V2 -- when actual music is queued -- inherits volume control
// without rework. Headless / no-audio environments degrade gracefully:
// stream stays null, applyVolumeToStream becomes a no-op.
static void playBackgroundMusic() {
    if (g_bgmStream) return;
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 44100 };
    g_bgmStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                            &spec, NULL, NULL);
    if (!g_bgmStream) {
        SDL_Log("[gameConsole] khong mo duoc audio stream (BGM): %s",
                SDL_GetError());
        return;
    }
    SDL_ResumeAudioStreamDevice(g_bgmStream);
    SDL_Log("[gameConsole] BGM audio stream san sang");
}

// [A.2/A.3] activateButton -- 5 cases: GUIDE, BOARD, PLAY, SETTINGS, QUIT
static void activateButton(AppState& state, int index) {
    switch (index) {
        case BTN_IDX_GUIDE:
            state.showGuide = true; state.guideScroll = 0;
            sbResetInteraction(state.sb); break;
        case BTN_IDX_BOARD:
            state.showBoard = true; state.boardScroll = 0;
            sbResetInteraction(state.sb); break;
        case BTN_IDX_PLAY:
            state.isRunning = false; state.nextScene = 1; break;
        case BTN_IDX_STORIES:
            // [F.4] Open stories popup. Load DB rows for current chapter +
            // refresh max-chapter counter. dbLoadStories already logs counts.
            state.showStories = true;
            if (state.currentStoryChapter < 1) state.currentStoryChapter = 1;
            state.storiesCache        = dbLoadStories("default",
                                                     state.currentStoryChapter);
            state.storiesCacheChapter = state.currentStoryChapter;
            state.storiesMaxChapter   = dbMaxActivatedChapter("default");
            if (state.storiesMaxChapter < 1) state.storiesMaxChapter = 1;
            sbResetInteraction(state.sb); break;
        case BTN_IDX_SETTINGS:
            // [A.3] Open settings popup
            state.showSettings = true;
            sbResetInteraction(state.sb); break;
        case BTN_IDX_QUIT:
            // QUIT: tren WASM hien shutdown screen thay vi thoat lam canvas trang.
            // Tren native tra ve 0 binh thuong.
#ifdef __EMSCRIPTEN__
            state.wasmShutdown = true;
#else
            state.isRunning = false; state.nextScene = 0;
#endif
            break;
        default: break;
    }
}

// [D.6] Signature now accepts SettingsConfig& (in/out). UI mutations
// inside settings popup persist across Console <-> Core round-trips.
int runGameConsole(SDL_Window* window, SDL_Renderer* renderer,
                   SettingsConfig& cfgInOut) {
    (void)window;
    AppState state;
    state.cfg = &cfgInOut;   // borrow, do NOT delete
    SDL_Event event;

    // Issue 2.1: Guard -- if DB file does not exist, redirect to gameStory
    // so it can create and init the DB before Console renders any UI.
    // Return code 3 = SCREEN_GAMESTORY_INIT (handled in main.cpp).
    {
        char* pref = SDL_GetPrefPath("uit", "cTetris");
        bool dbExists = false;
        if (pref) {
            std::string probe = std::string(pref) + "default.sqlite";
            SDL_free(pref);
            SDL_IOStream* f = SDL_IOFromFile(probe.c_str(), "rb");
            if (f) {
                dbExists = (SDL_GetIOSize(f) > 100); // >100 bytes = not empty stub
                SDL_CloseIO(f);
            }
        }
        if (!dbExists) {
            SDL_Log("[gameConsole] DB not found -> redirect to gameStory (init)");
            return 3;   // SCREEN_GAMESTORY_INIT
        }
    }

    playBackgroundMusic();
    applyVolumeToStream(cfgInOut.volume);   // [D.5] sync gain to current vol

    // [F.1] Open DB + init schema + seed shared_data from JSON.
    // Hardcoded user id "default" for V2; V3 (Step 3.1.x) replaces with
    // email-based id after OTP flow. Seed is idempotent (skips if rows exist).
    if (dbOpen("default")) {
        dbInitSchema();
        dbSeedSharedData();
        dbLoadSettings(cfgInOut);   // Issue 2.2: restore volume, colors, storyId
        applyVolumeToStream(cfgInOut.volume);   // re-apply after load
        dbCheckAndUnlockStories("default");   // [F.5] cascade-unlock on entry
        state.storiesMaxChapter = dbMaxActivatedChapter("default");

        // [F.6] Restore last-selected story into SettingsConfig so the
        // Console -> Core handoff has the right idStory/idChapter
        // even after a full app restart (cfg is reset by main.cpp).
        sqlite3_stmt* st = nullptr;
        std::string selSQL =
            "SELECT idStory, idChapter FROM " + userTable("Stories") +
            " WHERE idUser = ? AND isSelected = 1 LIMIT 1;";
        if (sqlite3_prepare_v2(g_db, selSQL.c_str(), -1, &st, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(st, 1, "default", -1, SQLITE_TRANSIENT);
            if (sqlite3_step(st) == SQLITE_ROW) {
                cfgInOut.storyId   = sqlite3_column_int(st, 0);
                cfgInOut.chapterId = sqlite3_column_int(st, 1);
                SDL_Log("[gameConsole] restore selection: story=%d chapter=%d",
                        cfgInOut.storyId, cfgInOut.chapterId);
            }
            sqlite3_finalize(st);
        }
    }

    // [C.5] Load leaderboard tu JSON; fallback ve hardcoded array neu fail.
    // Re-load moi lan vao Console -- cho phep refresh data sau khi V3 add
    // sync tu MongoDB. Cost ~300us, khong noticeable.
    loadBoardWithFallback();
    // [G/H] Apply default sort (SCORE_DESC) right after load -- locks in
    // a deterministic order regardless of whether JSON or fallback array
    // was the source, and logs the algorithm the router picked.
    applyBoardSort(state);

    while (state.isRunning || state.wasmShutdown) {
        Uint32 nowMs = SDL_GetTicks();

        // ============= WASM shutdown screen =============
        if (state.wasmShutdown) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    state.wasmShutdown = false;
                    state.isRunning = false;
                    state.nextScene = 0;
                    break;
                }
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    state.reloadHover = hitTest(CONSOLE_RELOAD_BTN,
                                                event.motion.x, event.motion.y);
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    if (hitTest(CONSOLE_RELOAD_BTN, event.button.x, event.button.y)) {
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
            }
            drawConsoleWasmShutdown(renderer, state.reloadHover);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
            continue;
        }

        // ============= Game thuong =============
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.isRunning = false; state.nextScene = 0; break;
            }

            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                if (state.showBoard) {
                    state.boardScroll = clampScroll(state.boardScroll - (int)event.wheel.y,
                                                    g_boardTotal, BOARD_VISIBLE);
                } else if (state.showGuide) {
                    state.guideScroll = clampScroll(state.guideScroll - (int)event.wheel.y,
                                                    GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES);
                }
                // [A.4] Settings popup khong co scroll trong V1 -- bo qua wheel
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                bool shiftHeld = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                bool isUp    = (key == SDLK_UP    || key == SDLK_W);
                bool isDown  = (key == SDLK_DOWN  || key == SDLK_S);
                bool isLeft  = (key == SDLK_LEFT  || key == SDLK_A);
                bool isRight = (key == SDLK_RIGHT || key == SDLK_D);

                if (key == SDLK_ESCAPE) {
                    // [A.4/F.2] ESC closes any open lightbox first; only when
                    // none are open does it jump focus to QUIT button.
                    if (state.showGuide)         { state.showGuide = false;    sbResetInteraction(state.sb); }
                    else if (state.showBoard)    { state.showBoard = false;    sbResetInteraction(state.sb); }
                    else if (state.showSettings) {
                        state.showSettings = false;
                        sbResetInteraction(state.sb);
                        if (state.cfg) dbSaveSettings(*state.cfg);   // Issue 2.3
                    }
                    else if (state.showStories)  { state.showStories = false;  sbResetInteraction(state.sb); }
                    else                          state.focusIndex = BTN_IDX_QUIT;
                }
                else if (state.showBoard && (isUp || isDown)) {
                    int delta = isUp ? -1 : 1;
                    state.boardScroll = clampScroll(state.boardScroll + delta,
                                                    g_boardTotal, BOARD_VISIBLE);
                }
                else if (state.showGuide && (isUp || isDown)) {
                    int delta = isUp ? -1 : 1;
                    state.guideScroll = clampScroll(state.guideScroll + delta,
                                                    GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES);
                }
                // [D.4] Settings popup: LEFT/RIGHT = volume +-5%
                else if (state.showSettings && state.cfg && (isLeft || isRight)) {
                    float delta = isRight ? 0.05f : -0.05f;
                    float v = state.cfg->volume + delta;
                    if (v < 0.0f) v = 0.0f;
                    if (v > 1.0f) v = 1.0f;
                    state.cfg->volume = v;
                    applyVolumeToStream(state.cfg->volume);
                }
                // [A.4] Block main-button TAB/arrow nav while ANY popup open
                else if (!state.showGuide && !state.showBoard && !state.showSettings && !state.showStories) {
                    if ((key == SDLK_TAB && !shiftHeld) || isLeft || isDown)
                        state.focusIndex = (state.focusIndex + 1) % NUM_MAIN_BUTTONS;
                    else if ((key == SDLK_TAB && shiftHeld) || isRight || isUp)
                        state.focusIndex = (state.focusIndex - 1 + NUM_MAIN_BUTTONS) % NUM_MAIN_BUTTONS;
                    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
                        activateButton(state, state.focusIndex);
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT) {
                float mx = event.button.x;
                float my = event.button.y;

                if (state.showGuide) {
                    if (hitTest(GUIDE_CLOSE, mx, my)) {
                        state.showGuide = false;
                        sbResetInteraction(state.sb);
                    } else {
                        SBLayout sb = layoutSB(GUIDE_SB_X, GUIDE_SB_Y, GUIDE_SB_H,
                                               GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES,
                                               state.guideScroll);
                        sbOnMouseDown(state.sb, sb, mx, my, state.guideScroll,
                                      GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES, nowMs);
                    }
                } else if (state.showBoard) {
                    if (hitTest(BOARD_CLOSE, mx, my)) {
                        state.showBoard = false;
                        sbResetInteraction(state.sb);
                    }
                    // [G] gameconsole-sort-by-time-in-board-14
                    else if (hitTest(BOARD_HDR_TIME_BTN, mx, my)) {
                        state.boardSortMode =
                            (state.boardSortMode == BOARD_SORT_TIME_DESC)
                                ? BOARD_SORT_TIME_ASC
                                : BOARD_SORT_TIME_DESC;
                        applyBoardSort(state);
                    }
                    // [H] gameconsole-sort-by-score-in-board-15
                    else if (hitTest(BOARD_HDR_SCORE_BTN, mx, my)) {
                        state.boardSortMode =
                            (state.boardSortMode == BOARD_SORT_SCORE_DESC)
                                ? BOARD_SORT_SCORE_ASC
                                : BOARD_SORT_SCORE_DESC;
                        applyBoardSort(state);
                    }
                    else {
                        SBLayout sb = layoutSB(BOARD_SB_X, BOARD_SB_Y, BOARD_SB_H,
                                               g_boardTotal, BOARD_VISIBLE,
                                               state.boardScroll);
                        sbOnMouseDown(state.sb, sb, mx, my, state.boardScroll,
                                      g_boardTotal, BOARD_VISIBLE, nowMs);
                    }
                } else if (state.showSettings) {
                    // [A.3] Close X
                    if (hitTest(SETTINGS_CLOSE, mx, my)) {
                        state.showSettings = false;
                        state.draggingVolume = false;
                        sbResetInteraction(state.sb);
                        if (state.cfg) dbSaveSettings(*state.cfg);   // Issue 2.3
                    }
                    // [D.3] Volume slider drag-start: hit thumb OR click on track
                    else if (state.cfg) {
                        SDL_FRect thumb = volThumbRect(state.cfg->volume);
                        // Inflate hit zone by 4px each side for easier touch
                        SDL_FRect thumbHit = { thumb.x - 4, thumb.y - 4,
                                               thumb.w + 8, thumb.h + 8 };
                        if (hitTest(thumbHit, mx, my)) {
                            state.draggingVolume = true;
                        } else if (hitTest(VOL_TRACK, mx, my)) {
                            // Click-to-jump on track: snap thumb to click pos
                            float trackUsableW = VOL_TRACK.w - VOL_THUMB_W;
                            float thumbCenterX = mx;
                            float ratio = (thumbCenterX - VOL_TRACK.x - VOL_THUMB_W / 2.0f)
                                          / trackUsableW;
                            if (ratio < 0.0f) ratio = 0.0f;
                            if (ratio > 1.0f) ratio = 1.0f;
                            state.cfg->volume = ratio;
                            state.draggingVolume = true;
                            applyVolumeToStream(state.cfg->volume);
                        } else {
                            // [E.3+E.4] Color swatch toggle with at-least-1 guard
                            for (int i = 0; i < BLOCK_PALETTE_N; i++) {
                                SDL_FRect r = swatchRect(i);
                                if (hitTest(r, mx, my)) {
                                    bool currentlyOn = state.cfg->colorEnabled[i];
                                    if (currentlyOn && countEnabled(*state.cfg) <= 1) {
                                        // [E.4] Reject: would leave 0 enabled
                                        SDL_Log("[gameConsole] need >=1 color enabled");
                                    } else {
                                        state.cfg->colorEnabled[i] = !currentlyOn;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                } else if (state.showStories) {
                    if (hitTest(STORIES_CLOSE, mx, my)) {
                        state.showStories = false;
                        sbResetInteraction(state.sb);
                    }
                    // [F.5] Chapter navigator arrows. Clamp to [1, maxChapter];
                    // reload cache when chapter changes so the row list refreshes.
                    else if (hitTest(STORIES_NAV_LEFT, mx, my)) {
                        if (state.currentStoryChapter > 1) {
                            state.currentStoryChapter--;
                            state.storiesCache        = dbLoadStories("default",
                                                            state.currentStoryChapter);
                            state.storiesCacheChapter = state.currentStoryChapter;
                        }
                    }
                    else if (hitTest(STORIES_NAV_RIGHT, mx, my)) {
                        if (state.currentStoryChapter < state.storiesMaxChapter) {
                            state.currentStoryChapter++;
                            state.storiesCache        = dbLoadStories("default",
                                                            state.currentStoryChapter);
                            state.storiesCacheChapter = state.currentStoryChapter;
                        }
                    }
                    // [F.6] Row clicks: radio OR play -> select this story.
                    // Locked rows ignore clicks. Play's "replay gameStory"
                    // semantics remains TODO -- both buttons share selection
                    // behaviour for now.
                    else {
                        int rowsCheckable = (int)state.storiesCache.size();
                        if (rowsCheckable > STORIES_LIST_VISIBLE)
                            rowsCheckable = STORIES_LIST_VISIBLE;
                        for (int i = 0; i < rowsCheckable; i++) {
                            const StoryRow& sr = state.storiesCache[i];
                            if (!sr.isActivated) continue;   // locked
                            SDL_FRect radio = storiesRadioRect(i);
                            SDL_FRect play  = storiesPlayRect(i);
                            if (hitTest(radio, mx, my) || hitTest(play, mx, my)) {
                                // Select story in memory + DB
                                for (auto& r : state.storiesCache)
                                    r.isSelected = false;
                                state.storiesCache[i].isSelected = true;
                                dbSelectStory("default", sr.idStory, sr.idChapter);
                                if (state.cfg) {
                                    state.cfg->storyId        = sr.idStory;
                                    state.cfg->chapterId      = sr.idChapter;
                                    state.cfg->nextBlockScore = sr.nextBlockScore;
                                    state.cfg->nextBlockSpeed = sr.nextBlockSpeed;
                                    state.cfg->tableMatrix    = sr.tableMatrix;
                                }
                                // [E.3] Play button -> preview story dialogue
                                // Radio button -> select only, stay in popup
                                if (hitTest(play, mx, my)) {
                                    state.showStories = false;
                                    state.isRunning   = false;
                                    state.nextScene   = 2;   // signal: play story preview
                                }
                                break;
                            }
                        }
                    }
                } else {
                    for (int i = 0; i < NUM_MAIN_BUTTONS; i++) {
                        if (hitTest(MAIN_BUTTONS[i].rect, mx, my)) {
                            state.focusIndex = i;
                            activateButton(state, i);
                            break;
                        }
                    }
                }
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (state.sb.dragging) {
                    if (state.showGuide) {
                        SBLayout sb = layoutSB(GUIDE_SB_X, GUIDE_SB_Y, GUIDE_SB_H,
                                               GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES,
                                               state.guideScroll);
                        sbOnMouseMotion(state.sb, sb, event.motion.y,
                                        state.guideScroll, GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES);
                    } else if (state.showBoard) {
                        SBLayout sb = layoutSB(BOARD_SB_X, BOARD_SB_Y, BOARD_SB_H,
                                               g_boardTotal, BOARD_VISIBLE,
                                               state.boardScroll);
                        sbOnMouseMotion(state.sb, sb, event.motion.y,
                                        state.boardScroll, g_boardTotal, BOARD_VISIBLE);
                    }
                }
                // [D.3] Volume thumb drag -- compute new volume from x-coord
                if (state.draggingVolume && state.cfg) {
                    float trackUsableW = VOL_TRACK.w - VOL_THUMB_W;
                    float ratio = (event.motion.x - VOL_TRACK.x - VOL_THUMB_W / 2.0f)
                                  / trackUsableW;
                    if (ratio < 0.0f) ratio = 0.0f;
                    if (ratio > 1.0f) ratio = 1.0f;
                    state.cfg->volume = ratio;
                    applyVolumeToStream(state.cfg->volume);
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                event.button.button == SDL_BUTTON_LEFT) {
                sbResetInteraction(state.sb);
                state.draggingVolume = false;  // [D.3] release thumb
            }
        }

        if (state.showGuide && (state.sb.upHeld || state.sb.downHeld)) {
            sbAutoRepeat(state.sb, state.guideScroll,
                         GUIDE_LINE_COUNT, GUIDE_VISIBLE_LINES, nowMs);
        }
        if (state.showBoard && (state.sb.upHeld || state.sb.downHeld)) {
            sbAutoRepeat(state.sb, state.boardScroll,
                         g_boardTotal, BOARD_VISIBLE, nowMs);
        }

        drawBackground(renderer);
        // [A.4/F.2] Main buttons hidden whenever ANY popup is open
        if (!state.showGuide && !state.showBoard && !state.showSettings && !state.showStories) {
            for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
                drawButton(renderer, MAIN_BUTTONS[i], i == state.focusIndex);
        }
        if (state.showGuide)    drawGuideLightbox(renderer, state);
        if (state.showBoard)    drawBoardLightbox(renderer, state);
        if (state.showSettings) drawSettingsLightbox(renderer, state);
        if (state.showStories)  drawStoriesLightbox(renderer, state);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // [B.4] Don dep bg texture truoc khi roi scene -- gameConsole CO THE
    // duoc re-enter (Core back to Console qua nut poweroff), neu khong reset
    // pointer thi lan goi sau drawBackground se dung texture cu da bi
    // SDL_DestroyRenderer invalidate -> garbage pixel hoac crash. Reset
    // ve {nullptr, 0, 0} dam bao lazy-init re-tao tu dau.
    if (g_consoleBg.texture) {
        SDL_DestroyTexture(g_consoleBg.texture);
        g_consoleBg.texture = nullptr;
        g_consoleBg.w = 0;
        g_consoleBg.h = 0;
    }

    // [D.5] Don dep audio stream -- moi lan exit + re-enter Console se
    // mo lai stream moi. Tranh leak khi user back-and-forth nhieu lan.
    closeAudioStream();

    // [F.1] Close DB. Re-opened on next runGameConsole entry. Safe to
    // call even if dbOpen failed earlier.
    dbClose();

    return state.nextScene;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);   // [D.5] need AUDIO subsystem
    SDL_Window* window = SDL_CreateWindow("Game Console \xC2\xA9 - Standalone",
                                          CONSOLE_SCREEN_WIDTH, CONSOLE_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SettingsConfig cfg;   // [D.6] standalone owns its own config
    runGameConsole(window, renderer, cfg);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif
