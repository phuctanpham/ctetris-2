// integration/v1
// gamestory-dong-bo-sqlite-05a  [Phase C] storyDb read layer
// gamestory-phan-cot-game-05b   [Phase D] Dialogue engine + state machine
#include "gameStory_layout.h"
#include "gameStory_db.h"
#include "sqlite3.h"

// nanosvg single-header library: parser + rasterizer.
// Implementation compiled once in src/shared/nanosvg_impl.cpp.
#include "nanosvg.h"
#include "nanosvgrast.h"

#include "gameStory_logo_svg.h"
#include "gameStory_corp_svg.h"

#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// libcurl for native HTTP sync (manifest fetch, native WASM not used)
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

#include "ctetris_debug.h"

// Loading bar duration: 8 s to show story + give WASM time to settle
const int INTRO_DURATION = 8000;

// =============================================================================
// [C] gameStory_db.h implementation
// Opens the same default.sqlite used by gameConsole (separate sqlite3* handle).
// gameStory only reads – never writes to the story_ tables.
// =============================================================================
static sqlite3* g_storyDb = nullptr;

bool storyDbOpen() {
    if (g_storyDb) return true;
    char* pref = SDL_GetPrefPath("uit", "cTetris");
    if (!pref) {
        SDL_Log("[gameStory_db] SDL_GetPrefPath fail: %s", SDL_GetError());
        return false;
    }
    std::string path = std::string(pref) + "default.sqlite";
    SDL_free(pref);
    int rc = sqlite3_open(path.c_str(), &g_storyDb);
    if (rc != SQLITE_OK) {
        SDL_Log("[gameStory_db] open fail: %s",
                g_storyDb ? sqlite3_errmsg(g_storyDb) : "null");
        if (g_storyDb) { sqlite3_close(g_storyDb); g_storyDb = nullptr; }
        return false;
    }
    SDL_Log("[gameStory_db] opened: %s", path.c_str());
    return true;
}

void storyDbClose() {
    if (g_storyDb) {
        sqlite3_close(g_storyDb);
        g_storyDb = nullptr;
        SDL_Log("[gameStory_db] closed");
    }
}

// Mirror of gameConsole userTable() — construct "{idUser}_Stories".
// gameConsole owns the CREATE TABLE; gameStory only reads/writes rows.
// Centralised here so any future idUser change propagates automatically.
static std::string storyUserTable(const char* idUser) {
    return std::string(idUser && *idUser ? idUser : "default") + "_Stories";
}

std::vector<DialogueNode> storyDbLoadDialogue(int idStory, int idChapter) {
    std::vector<DialogueNode> out;
    if (!g_storyDb) return out;

    const char* sql =
        "SELECT nodeId, speaker, text, imageUrl, bgmUrl, sfxUrl, "
        "       nextNodeId, hasChoices "
        "FROM shared_dialogues "
        "WHERE idStory=?1 AND idChapter=?2 "
        "ORDER BY nodeId;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_storyDb, sql, -1, &st, nullptr) != SQLITE_OK) {
        SDL_Log("[gameStory_db] loadDialogue prepare: %s",
                sqlite3_errmsg(g_storyDb));
        return out;
    }
    sqlite3_bind_int(st, 1, idStory);
    sqlite3_bind_int(st, 2, idChapter);

    auto colStr = [](sqlite3_stmt* s, int i) -> std::string {
        const unsigned char* p = sqlite3_column_text(s, i);
        return p ? std::string((const char*)p) : std::string();
    };

    while (sqlite3_step(st) == SQLITE_ROW) {
        DialogueNode n;
        n.nodeId     = sqlite3_column_int(st, 0);
        n.speaker    = colStr(st, 1);
        n.text       = colStr(st, 2);
        n.imageUrl   = colStr(st, 3);
        n.bgmUrl     = colStr(st, 4);
        n.sfxUrl     = colStr(st, 5);
        n.nextNodeId = sqlite3_column_int(st, 6);
        n.hasChoices = (sqlite3_column_int(st, 7) != 0);
        out.push_back(std::move(n));
    }
    sqlite3_finalize(st);
    SDL_Log("[gameStory_db] story=%d ch=%d nodes=%d",
            idStory, idChapter, (int)out.size());
    return out;
}

std::vector<DialogueChoice> storyDbLoadChoices(int idStory, int idChapter,
                                                int nodeId) {
    std::vector<DialogueChoice> out;
    if (!g_storyDb) return out;

    const char* sql =
        "SELECT choiceIdx, label, nextNodeId "
    "FROM shared_choices "
        "WHERE idStory=?1 AND idChapter=?2 AND nodeId=?3 "
        "ORDER BY choiceIdx;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_storyDb, sql, -1, &st, nullptr) != SQLITE_OK)
        return out;
    sqlite3_bind_int(st, 1, idStory);
    sqlite3_bind_int(st, 2, idChapter);
    sqlite3_bind_int(st, 3, nodeId);

    while (sqlite3_step(st) == SQLITE_ROW) {
        DialogueChoice c;
        c.choiceIdx  = sqlite3_column_int(st, 0);
        const unsigned char* p = sqlite3_column_text(st, 1);
        c.label      = p ? (const char*)p : "";
        c.nextNodeId = sqlite3_column_int(st, 2);
        out.push_back(std::move(c));
    }
    sqlite3_finalize(st);
    return out;
}

// =============================================================================
// Existing: SvgTexture + rasterizer helper (unchanged from v1)
// =============================================================================
struct SvgTexture {
    SDL_Texture* texture = nullptr;
    int          w       = 0;
    int          h       = 0;
};

static SvgTexture g_logo;
static SvgTexture g_corp;

static SvgTexture createSvgTexture(SDL_Renderer* renderer,
                                   const char*   svgData,
                                   int           targetW) {
    SvgTexture result;
    size_t svgLen = SDL_strlen(svgData);
    char* svgCopy = (char*)SDL_malloc(svgLen + 1);
    if (!svgCopy) return result;
    SDL_memcpy(svgCopy, svgData, svgLen + 1);

    NSVGimage* image = nsvgParse(svgCopy, "px", 96.0f);
    SDL_free(svgCopy);
    if (!image || image->width <= 0 || image->height <= 0) {
        if (image) nsvgDelete(image);
        SDL_Log("[gameStory] nsvgParse fail");
        return result;
    }

    float scale = (float)targetW / image->width;
    int outW    = targetW;
    int outH    = (int)(image->height * scale + 0.5f);

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

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        outW, outH, SDL_PIXELFORMAT_RGBA32, pixels, outW * 4);
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
        SDL_Log("[gameStory] SDL_CreateTextureFromSurface fail: %s",
                SDL_GetError());
    }
    return result;
}

// =============================================================================
// Existing: intro animation helpers (unchanged from v1)
// =============================================================================

// gamestory-logo-intro-01
static void drawLogo(SDL_Renderer* renderer, Uint32 elapsedTime) {
    if (!g_logo.texture)
        g_logo = createSvgTexture(renderer, LOGO_SVG_DATA, 140);
    if (!g_logo.texture) return;

    float t     = (float)elapsedTime / (float)INTRO_DURATION;
    if (t > 1.0f) t = 1.0f;
    Uint8 alpha = (Uint8)(t * 255.0f);
    SDL_SetTextureAlphaMod(g_logo.texture, alpha);

    SDL_FRect dst = {
        (STORY_SCREEN_WIDTH  - g_logo.w) / 2.0f,
        (STORY_SCREEN_HEIGHT - g_logo.h) / 2.0f - 60.0f,
        (float)g_logo.w, (float)g_logo.h
    };
    SDL_RenderTexture(renderer, g_logo.texture, NULL, &dst);

    SDL_SetRenderDrawColor(renderer, 220, 220, 220, alpha);
    const char* title = "C T E T R I S";
    int tl = (int)SDL_strlen(title);
    SDL_RenderDebugText(renderer,
        (STORY_SCREEN_WIDTH - tl * 8.0f) / 2.0f,
        dst.y + dst.h + 10.0f, title);
}

// gamestory-loading-bar-02
static float drawLoadingBar(SDL_Renderer* renderer, Uint32 elapsedTime) {
    float progress = (float)elapsedTime / INTRO_DURATION;
    if (progress > 1.0f) progress = 1.0f;
    const float barW = 180.0f, barH = 12.0f;
    const float barX = (STORY_SCREEN_WIDTH - barW) / 2.0f;
    const float barY = STORY_SCREEN_HEIGHT - 110.0f;

    SDL_FRect bgBar = { barX, barY, barW, barH };
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &bgBar);

    SDL_FRect fgBar = { barX, barY, barW * progress, barH };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &fgBar);

    return barY + barH;
}

// gamestory-corp-credit-03
static void drawCorpCredit(SDL_Renderer* renderer,
                           Uint32 elapsedTime, float topY) {
    if (!g_corp.texture)
        g_corp = createSvgTexture(renderer, CORP_SVG_DATA, 40);

    float t     = (float)elapsedTime / (float)INTRO_DURATION;
    if (t > 1.0f) t = 1.0f;
    Uint8 alpha = (Uint8)(t * 255.0f);

    const char* prefix    = "Powered up by ";
    const int   prefixLen = (int)SDL_strlen(prefix);
    const float CHAR_W    = 8.0f;
    const float CHAR_H    = 8.0f;
    const float CORP_H    = 16.0f;
    const float SPACING   = 4.0f;

    float corpDisplayW = 0.0f;
    if (g_corp.texture && g_corp.h > 0)
        corpDisplayW = (float)g_corp.w * CORP_H / (float)g_corp.h;

    float totalW  = prefixLen * CHAR_W +
                    (corpDisplayW > 0 ? SPACING + corpDisplayW : 0);
    float startX  = (STORY_SCREEN_WIDTH - totalW) / 2.0f;
    float lineY   = topY + 16.0f;
    float textY   = lineY - CHAR_H / 2.0f;
    float corpY   = lineY - CORP_H / 2.0f;

    SDL_SetRenderDrawColor(renderer, 220, 220, 220, alpha);
    SDL_RenderDebugText(renderer, startX, textY, prefix);

    if (g_corp.texture) {
        SDL_SetTextureAlphaMod(g_corp.texture, alpha);
        SDL_FRect corpDst = {
            startX + prefixLen * CHAR_W + SPACING, corpY,
            corpDisplayW, CORP_H
        };
        SDL_RenderTexture(renderer, g_corp.texture, NULL, &corpDst);
    }
}

// =============================================================================
// [D] Dialogue engine helpers
// =============================================================================

// --- colours ---
static const SDL_Color DIAG_BG         = { 18,  18,  28, 255};
static const SDL_Color DIAG_IMG_BG     = { 40,  42,  55, 255};
static const SDL_Color DIAG_IMG_BORDER = { 65,  68,  85, 255};
static const SDL_Color DIAG_TB_BG      = { 28,  32,  44, 235};  // blended
static const SDL_Color DIAG_TB_BORDER  = { 70,  74,  95, 255};
static const SDL_Color DIAG_SPEAKER    = {255, 215,   0, 255};  // yellow
static const SDL_Color DIAG_TEXT       = {220, 220, 220, 255};  // soft white
static const SDL_Color DIAG_HINT       = {130, 130, 145, 255};
static const SDL_Color DIAG_SEP        = { 70,  74,  95, 255};
static const SDL_Color DIAG_CHOICE_ON  = { 90, 135, 200, 255};
static const SDL_Color DIAG_CHOICE_OFF = { 55,  80, 120, 255};
static const SDL_Color DIAG_SKIP_IDLE  = { 55,  55,  72, 200};
static const SDL_Color DIAG_SKIP_HOV   = {140,  55,  55, 220};

// --- layout (all in screen coords 270x480) ---
static const SDL_FRect DIAG_SKIP_BTN  = { 200.0f,  5.0f,  65.0f, 18.0f };
static const SDL_FRect DIAG_IMG_AREA  = {   5.0f, 27.0f, 260.0f, 196.0f };
// text-box: y=228..472
static const float DIAG_TB_X   = 5.0f;
static const float DIAG_TB_Y   = 228.0f;
static const float DIAG_TB_W   = 260.0f;
static const float DIAG_TB_H   = 244.0f;
// inner text margins
static const float DIAG_TXT_X  = DIAG_TB_X + 8.0f;
static const float DIAG_SPK_Y  = DIAG_TB_Y + 8.0f;   // speaker label
static const float DIAG_SEP_Y  = DIAG_TB_Y + 21.0f;  // separator
static const float DIAG_TXT_Y0 = DIAG_TB_Y + 27.0f;  // first text line
static const float DIAG_HINT_Y = DIAG_TB_Y + DIAG_TB_H - 14.0f; // hint row
// dialogue text wrap config
static const int   DIAG_MAX_CHARS = 30;
static const int   DIAG_MAX_LINES = 12;
static const float DIAG_LINE_H    = 14.0f;

// Choice button rect (i = 0-based from top, total = number of choices)
static SDL_FRect diagChoiceRect(int i, int total) {
    const float h   = 22.0f;
    const float gap = 4.0f;
    float startY = DIAG_HINT_Y - total * (h + gap) + i * (h + gap);
    return SDL_FRect{ DIAG_TB_X + 8.0f, startY, DIAG_TB_W - 16.0f, h };
}

static bool diagHit(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// Word-wrap renderer (same pattern as gameConsole)
static int diagDrawWrapped(SDL_Renderer* r, const char* text,
                            float x, float y, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    int len = (int)SDL_strlen(text);
    int pos = 0, lines = 0;
    char buf[128];
    while (pos < len && lines < DIAG_MAX_LINES) {
        int rem  = len - pos;
        int take = (rem > DIAG_MAX_CHARS) ? DIAG_MAX_CHARS : rem;
        if (take == DIAG_MAX_CHARS && pos + take < len) {
            int back = take;
            while (back > 0 && text[pos + back - 1] != ' ') back--;
            if (back > 0) take = back;
        }
        if (take >= (int)sizeof(buf)) take = (int)sizeof(buf) - 1;
        SDL_memcpy(buf, text + pos, take);
        buf[take] = '\0';
        SDL_RenderDebugText(r, x, y + lines * DIAG_LINE_H, buf);
        pos += take;
        while (pos < len && text[pos] == ' ') pos++;
        lines++;
    }
    return lines;
}

static void drawDialoguePage(SDL_Renderer* renderer,
                              const DialogueNode&              node,
                              const std::vector<DialogueChoice>& choices,
                              int   selChoice,
                              bool  skipHover) {
    int nc = (int)choices.size();

    // background
    SDL_SetRenderDrawColor(renderer,
        DIAG_BG.r, DIAG_BG.g, DIAG_BG.b, DIAG_BG.a);
    SDL_RenderClear(renderer);

    // image area placeholder (V3 replaces with real texture)
    SDL_SetRenderDrawColor(renderer,
        DIAG_IMG_BG.r, DIAG_IMG_BG.g, DIAG_IMG_BG.b, 255);
    SDL_RenderFillRect(renderer, &DIAG_IMG_AREA);
    SDL_SetRenderDrawColor(renderer,
        DIAG_IMG_BORDER.r, DIAG_IMG_BORDER.g, DIAG_IMG_BORDER.b, 255);
    SDL_RenderRect(renderer, &DIAG_IMG_AREA);
    if (node.imageUrl.empty()) {
        const char* lbl = "[ IMAGE ]";
        int ll = (int)SDL_strlen(lbl);
        SDL_SetRenderDrawColor(renderer, 75, 78, 98, 255);
        SDL_RenderDebugText(renderer,
            DIAG_IMG_AREA.x + (DIAG_IMG_AREA.w - ll * 8.0f) / 2.0f,
            DIAG_IMG_AREA.y + (DIAG_IMG_AREA.h - 8.0f)  / 2.0f,
            lbl);
    }

    // text box background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,
        DIAG_TB_BG.r, DIAG_TB_BG.g, DIAG_TB_BG.b, DIAG_TB_BG.a);
    SDL_FRect tb = { DIAG_TB_X, DIAG_TB_Y, DIAG_TB_W, DIAG_TB_H };
    SDL_RenderFillRect(renderer, &tb);
    SDL_SetRenderDrawColor(renderer,
        DIAG_TB_BORDER.r, DIAG_TB_BORDER.g, DIAG_TB_BORDER.b, 255);
    SDL_RenderRect(renderer, &tb);

    // speaker name
    if (!node.speaker.empty()) {
        SDL_SetRenderDrawColor(renderer,
            DIAG_SPEAKER.r, DIAG_SPEAKER.g, DIAG_SPEAKER.b, 255);
        SDL_RenderDebugText(renderer, DIAG_TXT_X, DIAG_SPK_Y,
                            node.speaker.c_str());
    }
    // separator
    SDL_SetRenderDrawColor(renderer,
        DIAG_SEP.r, DIAG_SEP.g, DIAG_SEP.b, 255);
    SDL_FRect sep = { DIAG_TB_X + 4.0f, DIAG_SEP_Y, DIAG_TB_W - 8.0f, 1.0f };
    SDL_RenderFillRect(renderer, &sep);

    // dialogue text
    if (!node.text.empty()) {
        diagDrawWrapped(renderer, node.text.c_str(),
                        DIAG_TXT_X, DIAG_TXT_Y0, DIAG_TEXT);
    }

    // choices or NEXT hint
    if (nc > 0) {
        for (int i = 0; i < nc; i++) {
            SDL_FRect cr  = diagChoiceRect(i, nc);
            SDL_Color bg  = (i == selChoice) ? DIAG_CHOICE_ON : DIAG_CHOICE_OFF;
            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
            SDL_RenderFillRect(renderer, &cr);
            SDL_SetRenderDrawColor(renderer,
                DIAG_TEXT.r, DIAG_TEXT.g, DIAG_TEXT.b, 255);
            SDL_RenderRect(renderer, &cr);
            if (i == selChoice) {
                SDL_SetRenderDrawColor(renderer,
                    DIAG_SPEAKER.r, DIAG_SPEAKER.g, DIAG_SPEAKER.b, 255);
            }
            const char* lbl = choices[i].label.c_str();
            SDL_RenderDebugText(renderer,
                cr.x + 8.0f, cr.y + (cr.h - 8.0f) / 2.0f, lbl);
        }
        SDL_SetRenderDrawColor(renderer,
            DIAG_HINT.r, DIAG_HINT.g, DIAG_HINT.b, 255);
        SDL_RenderDebugText(renderer,
            DIAG_TB_X + 8.0f, DIAG_HINT_Y, "TAB: next  ENTER: pick");
    } else {
        const char* hint = (node.nextNodeId == 0) ? "[ END ]" : "[ NEXT ]";
        int hl = (int)SDL_strlen(hint);
        SDL_SetRenderDrawColor(renderer,
            DIAG_HINT.r, DIAG_HINT.g, DIAG_HINT.b, 255);
        SDL_RenderDebugText(renderer,
            DIAG_TB_X + DIAG_TB_W - hl * 8.0f - 8.0f,
            DIAG_HINT_Y, hint);
    }

    // skip button
    SDL_Color skipBg = skipHover ? DIAG_SKIP_HOV : DIAG_SKIP_IDLE;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,
        skipBg.r, skipBg.g, skipBg.b, skipBg.a);
    SDL_RenderFillRect(renderer, &DIAG_SKIP_BTN);
    SDL_SetRenderDrawColor(renderer, DIAG_TEXT.r, DIAG_TEXT.g, DIAG_TEXT.b, 200);
    SDL_RenderRect(renderer, &DIAG_SKIP_BTN);
    const char* skipLbl = "[ SKIP ]";
    int sl = (int)SDL_strlen(skipLbl);
    SDL_RenderDebugText(renderer,
        DIAG_SKIP_BTN.x + (DIAG_SKIP_BTN.w - sl * 8.0f) / 2.0f,
        DIAG_SKIP_BTN.y + (DIAG_SKIP_BTN.h - 8.0f) / 2.0f,
        skipLbl);
}

// Find node index in vector by nodeId (-1 if not found)
static int findDialNodeIdx(const std::vector<DialogueNode>& nodes, int nodeId) {
    for (int i = 0; i < (int)nodes.size(); i++)
        if (nodes[i].nodeId == nodeId) return i;
    return -1;
}

// =============================================================================
// gamestory-dong-bo-sqlite-05a  Phase C — Gist manifest sync (Issues 2.3, 2.4)
// =============================================================================
// MANIFEST_GIST_URL is the stable raw URL of the manifest Gist.
// Set to your actual Gist URL before building.
// Format: https://gist.githubusercontent.com/{owner}/{GIST_MANIFEST_ID}/raw/manifest.json
// This URL never changes regardless of how many times the Gist content is updated.
#ifndef MANIFEST_GIST_URL
#define MANIFEST_GIST_URL ""
#endif

#ifndef CTETRIS_API_URL
#define CTETRIS_API_URL ""
#endif

// Sync states for the intro screen state machine
enum SyncState {
    SYNC_IDLE,          // not started yet
    SYNC_FETCHING,      // HTTP request in flight (WASM async) or running (native)
    SYNC_UPDATING_DB,   // applying chapter updates to SQLite
    SYNC_DONE,          // all chapters up to date
    SYNC_OFFLINE        // fetch failed — using local data
};

// Per-chapter sync progress tracked across frames
struct SyncProgress {
    int  total     = 0;   // chapters that need updating
    int  done      = 0;   // chapters processed
    std::string currentId;   // chapter being processed ("c001", ...)
    SyncState state = SYNC_IDLE;
};

// ---------------------------------------------------------------------------
// Minimal HTTP GET — returns body string or empty on failure.
// Native:  libcurl synchronous (blocking, acceptable for startup sync).
// WASM:    emscripten_fetch synchronous mode (uses Asyncify to avoid blocking
//          the browser event loop; requires -sASYNCIFY in link flags).
// Offline: returns "" — all callers treat empty response as offline.
// ---------------------------------------------------------------------------
static std::string httpGetSync(const char* url) {
    if (!url || url[0] == '\0') return "";

    CTDBG_REQ("GET", url);

#ifdef __EMSCRIPTEN__
    // Emscripten synchronous fetch via Asyncify
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    SDL_strlcpy(attr.requestMethod, "GET", sizeof(attr.requestMethod));
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
    emscripten_fetch_t* fetch = emscripten_fetch(&attr, url);
    std::string body;
    if (fetch && fetch->status == 200 && fetch->numBytes > 0) {
        body.assign(fetch->data, (size_t)fetch->numBytes);
        CTDBG_RES("GET", url, 200, body.size());
        CTDBG_BODY(body);
    } else if (fetch) {
        CTDBG_RES("GET", url, fetch->status, 0);
        CTDBG_ERR("emscripten_fetch: non-200 or empty");
        SDL_Log("[gameStory] httpGetSync WASM: HTTP %d for %s", (int)fetch->status, url);
    }
    if (fetch) emscripten_fetch_close(fetch);
    return body;

#else
    // Native: libcurl (build.sh / build.ps1 must link -lcurl)
    // Declared extern so this TU compiles without curl.h in the include path.
    // The linker resolves the symbol from the curl shared library.
    // If curl is unavailable the build will fail at link time — intentional.
    struct CurlBuf { std::string data; };
    auto writeCallback = [](char* ptr, size_t sz, size_t nmemb, void* ud) -> size_t {
        ((CurlBuf*)ud)->data.append(ptr, sz * nmemb);
        return sz * nmemb;
    };
    CurlBuf buf;
    CURL* c = curl_easy_init();
    if (!c) {
        CTDBG_ERR("httpGetSync: curl_easy_init() null");
        return "";
    }
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, (curl_write_callback)(writeCallback));
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 10L);
    CURLcode res = curl_easy_perform(c);
    long httpCode = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(c);
    if (res != CURLE_OK) {
        CTDBG_ERR(curl_easy_strerror(res));
        SDL_Log("[gameStory] httpGetSync curl fail: %s", curl_easy_strerror(res));
        return "";
    }
    CTDBG_RES("GET", url, (int)httpCode, buf.data.size());
    CTDBG_BODY(buf.data);
    return buf.data;

#endif  // __EMSCRIPTEN__
}

// ---------------------------------------------------------------------------
// Read SHA from shared_meta table for a chapter. Returns "" if not found.
// ---------------------------------------------------------------------------
static std::string metaGetSha(sqlite3* db, const char* chapterId) {
    if (!db || !chapterId) return "";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db,
        "SELECT sha FROM shared_meta WHERE chapter_id = ?;",
        -1, &st, nullptr) != SQLITE_OK) return "";
    sqlite3_bind_text(st, 1, chapterId, -1, SQLITE_TRANSIENT);
    std::string sha;
    if (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char* p = sqlite3_column_text(st, 0);
        if (p) sha = (const char*)p;
    }
    sqlite3_finalize(st);
    return sha;
}

// ---------------------------------------------------------------------------
// Update shared_meta table after a chapter is synced.
// ---------------------------------------------------------------------------
static void metaSetSha(sqlite3* db, const char* chapterId,
                        const char* sha, const char* mediaBaseUrl = "") {
    if (!db || !chapterId || !sha) return;
    // Add column if missing (idempotent upgrade from old schema)
    sqlite3_exec(db,
        "ALTER TABLE shared_meta ADD COLUMN media_base_url TEXT DEFAULT '';",
        nullptr, nullptr, nullptr);   // error ignored (column may exist)
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO shared_meta "
        "(chapter_id, sha, updated_at, media_base_url) VALUES (?,?,?,?);",
        -1, &st, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text (st, 1, chapterId,    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 2, sha,          -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 3, (sqlite3_int64)SDL_GetTicks());
    sqlite3_bind_text (st, 4, mediaBaseUrl ? mediaBaseUrl : "", -1, SQLITE_TRANSIENT);
    sqlite3_step(st);
    sqlite3_finalize(st);
}

static std::string metaGetMediaBaseUrl(sqlite3* db, const char* chapterId) {
    if (!db || !chapterId) return "";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db,
        "SELECT media_base_url FROM shared_meta WHERE chapter_id = ?;",
        -1, &st, nullptr) != SQLITE_OK) return "";
    sqlite3_bind_text(st, 1, chapterId, -1, SQLITE_TRANSIENT);
    std::string url;
    if (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char* p = sqlite3_column_text(st, 0);
        if (p) url = (const char*)p;
    }
    sqlite3_finalize(st);
    return url;
}

// ---------------------------------------------------------------------------
// Apply one chapter's JSON into shared_data + shared_dialogues + shared_choices.
// This is the C++ equivalent of parse.py — diff per idStory, no full replace.
// ---------------------------------------------------------------------------
static bool applyChapterJson(sqlite3* db, const std::string& jsonBody,
                             const char* chapterId,
                             const char* mediaBaseUrl = "") {
    if (!db || jsonBody.empty()) return false;

    nlohmann::json jroot;
    try {
        jroot = nlohmann::json::parse(jsonBody);
    } catch (const std::exception& ex) {
        SDL_Log("[gameStory] applyChapterJson parse fail %s: %s",
                chapterId, ex.what());
        return false;
    }
    if (!jroot.contains("shared_data") || !jroot["shared_data"].is_array()) {
        SDL_Log("[gameStory] %s: missing shared_data array", chapterId);
        return false;
    }

    // Collect incoming idStory values
    std::vector<int> incomingIds;
    for (const auto& row : jroot["shared_data"]) {
        incomingIds.push_back(row.value("idStory", 0));
    }

    // Determine idChapter from first row
    int idChapter = 0;
    if (!jroot["shared_data"].empty())
        idChapter = jroot["shared_data"][0].value("idChapter", 0);
    if (idChapter == 0) return false;

    // --- DELETE stories no longer in JSON ---
    {
        // Build "(?,?,?,...) " placeholder list
        std::string placeholders;
        for (size_t i = 0; i < incomingIds.size(); i++) {
            if (i) placeholders += ",";
            placeholders += "?";
        }
        std::string delSQL =
            "DELETE FROM shared_data WHERE idChapter = ? AND idStory NOT IN ("
            + placeholders + ");";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(db, delSQL.c_str(), -1, &st, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(st, 1, idChapter);
            for (int i = 0; i < (int)incomingIds.size(); i++)
                sqlite3_bind_int(st, 2 + i, incomingIds[i]);
            sqlite3_step(st);
            sqlite3_finalize(st);
        }
        // Same for shared_dialogues / shared_choices
        std::string delDlg =
            "DELETE FROM shared_dialogues WHERE idChapter = ? AND idStory NOT IN ("
            + placeholders + ");";
        std::string delCho =
            "DELETE FROM shared_choices WHERE idChapter = ? AND idStory NOT IN ("
            + placeholders + ");";
        for (const auto& dsql : {delDlg, delCho}) {
            sqlite3_stmt* ds = nullptr;
            if (sqlite3_prepare_v2(db, dsql.c_str(), -1, &ds, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(ds, 1, idChapter);
                for (int i = 0; i < (int)incomingIds.size(); i++)
                    sqlite3_bind_int(ds, 2 + i, incomingIds[i]);
                sqlite3_step(ds);
                sqlite3_finalize(ds);
            }
        }
    }

    // --- UPSERT each story row ---
    auto bStr = [](sqlite3_stmt* s, int i, const std::string& v) {
        sqlite3_bind_text(s, i, v.c_str(), -1, SQLITE_TRANSIENT);
    };

    sqlite3_stmt* stSD = nullptr, *stDlg = nullptr, *stCho = nullptr;
    sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO shared_data "
        "(idStory,storyName,idChapter,chapterName,minScore,minSpeed,minRetries,"
        " requiredStories,nextBlockScore,nextBlockSpeed,tableMatrix,xmlDialogue,"
        " thumbnailPath) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);",
        -1, &stSD, nullptr);
    sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO shared_dialogues "
        "(idStory,idChapter,nodeId,speaker,text,imageUrl,bgmUrl,sfxUrl,"
        " nextNodeId,hasChoices) VALUES (?,?,?,?,?,?,?,?,?,?);",
        -1, &stDlg, nullptr);
    sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO shared_choices "
        "(idStory,idChapter,nodeId,choiceIdx,label,nextNodeId) VALUES (?,?,?,?,?,?);",
        -1, &stCho, nullptr);

    char* errMsg = nullptr;
    sqlite3_exec(db, "BEGIN;", nullptr, nullptr, &errMsg);
    sqlite3_free(errMsg);

    for (const auto& row : jroot["shared_data"]) {
        if (!stSD) break;
        int idStory = row.value("idStory", 0);
        sqlite3_reset(stSD);
        sqlite3_bind_int   (stSD,  1, idStory);
        bStr               (stSD,  2, row.value("storyName",       std::string()));
        sqlite3_bind_int   (stSD,  3, idChapter);
        bStr               (stSD,  4, row.value("chapterName",     std::string()));
        sqlite3_bind_int   (stSD,  5, row.value("minScore",        0));
        sqlite3_bind_double(stSD,  6, row.value("minSpeed",        0.0));
        sqlite3_bind_int   (stSD,  7, row.value("minRetries",      0));
        bStr               (stSD,  8, row.value("requiredStories", std::string()));
        sqlite3_bind_int   (stSD,  9, row.value("nextBlockScore",  0));
        sqlite3_bind_double(stSD, 10, row.value("nextBlockSpeed",  0.0));
        bStr               (stSD, 11, row.value("tableMatrix",     std::string()));
        bStr               (stSD, 12, row.value("xmlDialogue",     std::string()));
        bStr               (stSD, 13, row.value("thumbnailPath",   std::string()));
        sqlite3_step(stSD);

        // Delete old dialogues/choices for this story then re-insert
        if (stDlg) {
            sqlite3_stmt* del = nullptr;
            sqlite3_prepare_v2(db,
                "DELETE FROM shared_dialogues WHERE idStory=? AND idChapter=?;",
                -1, &del, nullptr);
            if (del) {
                sqlite3_bind_int(del, 1, idStory);
                sqlite3_bind_int(del, 2, idChapter);
                sqlite3_step(del); sqlite3_finalize(del);
            }
            if (stCho) {
                sqlite3_stmt* delc = nullptr;
                sqlite3_prepare_v2(db,
                    "DELETE FROM shared_choices WHERE idStory=? AND idChapter=?;",
                    -1, &delc, nullptr);
                if (delc) {
                    sqlite3_bind_int(delc, 1, idStory);
                    sqlite3_bind_int(delc, 2, idChapter);
                    sqlite3_step(delc); sqlite3_finalize(delc);
                }
            }
            if (row.contains("dialogues") && row["dialogues"].is_array()) {
                for (const auto& nd : row["dialogues"]) {
                    int nodeId = nd.value("nodeId", 0);
                    sqlite3_reset(stDlg);
                    sqlite3_bind_int(stDlg, 1, idStory);
                    sqlite3_bind_int(stDlg, 2, idChapter);
                    sqlite3_bind_int(stDlg, 3, nodeId);
                    bStr(stDlg, 4, nd.value("speaker", std::string()));
                    bStr(stDlg, 5, nd.value("text", std::string()));
                    bStr(stDlg, 6, nd.value("imageUrl", std::string()));
                    bStr(stDlg, 7, nd.value("bgmUrl", std::string()));
                    bStr(stDlg, 8, nd.value("sfxUrl", std::string()));
                    sqlite3_bind_int(stDlg, 9, nd.value("nextNodeId", 0));
                    int hasChoices = (nd.contains("choices") &&
                                     nd["choices"].is_array() &&
                                     !nd["choices"].empty()) ? 1 : 0;
                    sqlite3_bind_int(stDlg, 10, hasChoices);
                    sqlite3_step(stDlg);
                    // Choices
                    if (stCho && nd.contains("choices") && nd["choices"].is_array()) {
                        int choiceIdx = 0;
                        for (const auto& ch : nd["choices"]) {
                            sqlite3_reset(stCho);
                            sqlite3_bind_int(stCho, 1, idStory);
                            sqlite3_bind_int(stCho, 2, idChapter);
                            sqlite3_bind_int(stCho, 3, nodeId);
                            sqlite3_bind_int(stCho, 4, choiceIdx);
                            bStr(stCho, 5, ch.value("label", std::string()));
                            sqlite3_bind_int(stCho, 6, ch.value("nextNodeId", 0));
                            sqlite3_step(stCho);
                            choiceIdx++;
                        }
                    }
                }
            }
        }
    }

    errMsg = nullptr;
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
    sqlite3_free(errMsg);
    sqlite3_finalize(stSD);
    sqlite3_finalize(stDlg);
    sqlite3_finalize(stCho);
    SDL_Log("[gameStory] applyChapterJson %s: %d stories", chapterId,
            (int)jroot["shared_data"].size());
    return true;
}

// ---------------------------------------------------------------------------
// syncFromGist(): compare manifest SHA vs local meta, update changed chapters.
// Returns immediately (offline) if MANIFEST_GIST_URL is empty or fetch fails.
// ---------------------------------------------------------------------------
static SyncProgress g_syncProgress;

static void syncFromGist(sqlite3* db) {
    g_syncProgress = SyncProgress{};
    g_syncProgress.state = SYNC_FETCHING;

    const char* manifestUrl = MANIFEST_GIST_URL;
    if (!manifestUrl || manifestUrl[0] == '\0') {
        SDL_Log("[gameStory] MANIFEST_GIST_URL not set — offline");
        g_syncProgress.state = SYNC_OFFLINE;
        return;
    }

    std::string manifestBody = httpGetSync(manifestUrl);
    if (manifestBody.empty()) {
        SDL_Log("[gameStory] manifest fetch failed — offline");
        g_syncProgress.state = SYNC_OFFLINE;
        return;
    }

    nlohmann::json manifest;
    try {
        manifest = nlohmann::json::parse(manifestBody);
    } catch (...) {
        SDL_Log("[gameStory] manifest JSON parse fail — offline");
        g_syncProgress.state = SYNC_OFFLINE;
        return;
    }
    if (!manifest.contains("chapters") || !manifest["chapters"].is_array()) {
        SDL_Log("[gameStory] manifest: no chapters array");
        g_syncProgress.state = SYNC_OFFLINE;
        return;
    }

    // Read versions[latest] — format: { "c001": { latestCommitId, gistUrl, mediaBaseUrl, updatedDate } }
    std::string latest = manifest.value("latest", std::string());
    if (latest.empty() || !manifest.contains(latest) || !manifest[latest].is_object()) {
        SDL_Log("[gameStory] manifest: no valid latest version ('%s')", latest.c_str());
        g_syncProgress.state = SYNC_OFFLINE;
        return;
    }
    const auto& latestVer = manifest[latest];
    SDL_Log("[gameStory] manifest latest: %s", latest.c_str());

    struct ChapterEntry { std::string id, sha, gistUrl, mediaBaseUrl; };
    std::vector<ChapterEntry> toUpdate;
    for (auto it = latestVer.begin(); it != latestVer.end(); ++it) {
        std::string cid      = it.key();
        std::string newSha   = it.value().value("latestCommitId", std::string());
        std::string gistUrl  = it.value().value("gistUrl",        std::string());
        std::string mediaBU  = it.value().value("mediaBaseUrl",   std::string());
        if (cid.empty() || newSha.empty() || gistUrl.empty()) continue;
        std::string localSha = metaGetSha(db, cid.c_str());
        if (localSha == newSha) {
            SDL_Log("[gameStory] %s SHA match — skip", cid.c_str());
        } else {
            SDL_Log("[gameStory] %s SHA diff — queue update", cid.c_str());
            toUpdate.push_back({cid, newSha, gistUrl, mediaBU});
        }
    }

    g_syncProgress.total = (int)toUpdate.size();
    g_syncProgress.done  = 0;

    if (toUpdate.empty()) {
        SDL_Log("[gameStory] all chapters up to date");
        g_syncProgress.state = SYNC_DONE;
        return;
    }

    g_syncProgress.state = SYNC_UPDATING_DB;
    for (const auto& entry : toUpdate) {
        g_syncProgress.currentId = entry.id;
        std::string body = httpGetSync(entry.gistUrl.c_str());
        if (body.empty()) {
            SDL_Log("[gameStory] %s gistUrl fetch fail — skip",
                    entry.id.c_str());
        } else if (applyChapterJson(db, body, entry.id.c_str(),
                                    entry.mediaBaseUrl.c_str())) {
            metaSetSha(db, entry.id.c_str(), entry.sha.c_str(),
                       entry.mediaBaseUrl.c_str());
#ifdef __EMSCRIPTEN__
            // Persist to IndexedDB after each chapter write
            EM_ASM({ Module['FS'].syncfs(false, function(){}); });
#endif
        }
        g_syncProgress.done++;
    }

    g_syncProgress.state = SYNC_DONE;
    SDL_Log("[gameStory] sync complete: %d/%d chapters updated",
            g_syncProgress.done, g_syncProgress.total);
}

// ---------------------------------------------------------------------------
// Post-sync: check current story conditions → replay or prompt next story.
// Called once after sync completes (storyId==0 intro path only).
// Returns the storyId to run next (0 = no dialogue, just proceed to Console).
// ---------------------------------------------------------------------------
// [D2] Parse "requiredStories" CSV (e.g. "1,2,3") into a vector of idStory
// integers. Trims whitespace around each token. Returns empty vector for
// empty or null input so callers can branch on CSV-present vs absent.
static std::vector<int> parseRequiredStories(const std::string& csv) {
    std::vector<int> out;
    if (csv.empty()) return out;
    size_t pos = 0;
    while (pos <= csv.size()) {
        size_t comma = csv.find(',', pos);
        if (comma == std::string::npos) comma = csv.size();
        std::string tok = csv.substr(pos, comma - pos);
        // trim leading/trailing whitespace
        size_t a = 0, b = tok.size();
        while (a < b && (tok[a] == ' ' || tok[a] == '\t')) ++a;
        while (b > a && (tok[b-1] == ' ' || tok[b-1] == '\t')) --b;
        tok = tok.substr(a, b - a);
        if (!tok.empty()) {
            int id = std::atoi(tok.c_str());
            if (id > 0) out.push_back(id);
        }
        pos = comma + 1;
    }
    return out;
}

// [D2] For each parent idStory in `parents`, verify that the user's row in
// default_Stories has isActivated=1 AND lastMaxScore >= minScore
// AND lastMaxSpeed >= minSpeed (the child story's thresholds).
// Returns true only when every parent qualifies; returns false on any DB error.
static bool checkAllParentsQualify(sqlite3* db, const char* idUser,
                                   const std::vector<int>& parents,
                                   int minScore, float minSpeed) {
    if (parents.empty()) return true;
    std::string sql =
        "SELECT isActivated, lastMaxScore, lastMaxSpeed "
        "FROM " + storyUserTable(idUser) +
        " WHERE idUser = ? AND idStory = ? LIMIT 1;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK)
        return false;
    bool allPass = true;
    for (int pid : parents) {
        sqlite3_reset(st);
        sqlite3_bind_text(st, 1, idUser, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (st, 2, pid);
        if (sqlite3_step(st) != SQLITE_ROW) {
            SDL_Log("[gameStory] checkAllParents: story %d not found for user %s",
                    pid, idUser);
            allPass = false;
            break;
        }
        int   activated = sqlite3_column_int   (st, 0);
        int   pScore    = sqlite3_column_int   (st, 1);
        float pSpeed    = (float)sqlite3_column_double(st, 2);
        if (!activated || pScore < minScore || pSpeed < minSpeed) {
            SDL_Log("[gameStory] checkAllParents: story %d fails "
                    "(activated=%d score=%d/%d speed=%.1f/%.1f)",
                    pid, activated, pScore, minScore, pSpeed, minSpeed);
            allPass = false;
            break;
        }
    }
    sqlite3_finalize(st);
    return allPass;
}

static int postSyncConditionCheck(sqlite3* db, const char* idUser) {
    if (!db || !idUser) return 0;

    // Get selected story
    sqlite3_stmt* st = nullptr;
    std::string pscSQL =
        "SELECT us.idStory, us.idChapter, us.lastMaxScore, us.lastMaxSpeed, "
        "       sd.minScore, sd.minSpeed "
        "FROM " + storyUserTable(idUser) + " us "
        "JOIN shared_data sd ON us.idStory = sd.idStory AND us.idChapter = sd.idChapter "
        "WHERE us.idUser = ? AND us.isSelected = 1 LIMIT 1;";
    if (sqlite3_prepare_v2(db, pscSQL.c_str(),
        -1, &st, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_text(st, 1, idUser, -1, SQLITE_TRANSIENT);

    struct SelStory { int id=0, ch=0, maxScore=0; float maxSpeed=0;
                      int minScore=0; float minSpeed=0; } sel;
    if (sqlite3_step(st) == SQLITE_ROW) {
        sel.id       = sqlite3_column_int   (st, 0);
        sel.ch       = sqlite3_column_int   (st, 1);
        sel.maxScore = sqlite3_column_int   (st, 2);
        sel.maxSpeed = (float)sqlite3_column_double(st, 3);
        sel.minScore = sqlite3_column_int   (st, 4);
        sel.minSpeed = (float)sqlite3_column_double(st, 5);
    }
    sqlite3_finalize(st);

    if (sel.id == 0) {
        // No story selected → use first story of chapter 1
        sqlite3_stmt* fs = nullptr;
        if (sqlite3_prepare_v2(db,
            "SELECT idStory, idChapter FROM shared_data "
            "WHERE idChapter = 1 ORDER BY idStory ASC LIMIT 1;",
            -1, &fs, nullptr) == SQLITE_OK) {
            if (sqlite3_step(fs) == SQLITE_ROW) {
                sel.id = sqlite3_column_int(fs, 0);
                sel.ch = sqlite3_column_int(fs, 1);
            }
            sqlite3_finalize(fs);
        }
        if (sel.id == 0) return 0;
        // Mark as selected
        sqlite3_exec(db,
            ("INSERT OR REPLACE INTO " + storyUserTable(idUser) + " "
             "(idUser,idStory,idChapter,isActivated,isSelected) VALUES ('" +
             std::string(idUser) + "'," + std::to_string(sel.id) + "," +
             std::to_string(sel.ch) + ",1,1);").c_str(),
            nullptr, nullptr, nullptr);
        return sel.id;   // run first story unconditionally
    }

    // [D2] Find next story in chapter flow — enforce requiredStories CSV.
    // Fetch requiredStories alongside score/speed thresholds so we can
    // parse the CSV and check every listed parent in default_Stories.
    sqlite3_stmt* nx = nullptr;
    int nextStoryId = 0;
    std::string nextStoryName;
    if (sqlite3_prepare_v2(db,
        "SELECT sd.idStory, sd.storyName, sd.minScore, sd.minSpeed, "
        "       sd.requiredStories "
        "FROM shared_data sd "
        "WHERE sd.idChapter = ? AND sd.idStory > ? "
        "ORDER BY sd.idStory ASC LIMIT 1;",
        -1, &nx, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(nx, 1, sel.ch);
        sqlite3_bind_int(nx, 2, sel.id);
        if (sqlite3_step(nx) == SQLITE_ROW) {
            int nxtId      = sqlite3_column_int(nx, 0);
            const unsigned char* nxtName = sqlite3_column_text(nx, 1);
            int   nxtMinScore = sqlite3_column_int   (nx, 2);
            float nxtMinSpd   = (float)sqlite3_column_double(nx, 3);
            const unsigned char* reqRaw = sqlite3_column_text(nx, 4);
            std::string reqCSV = reqRaw ? (const char*)reqRaw : "";

            // Parse requiredStories CSV into parent IDs.
            // Empty CSV → fallback: check current story's scores directly.
            // Non-empty CSV → verify every listed parent in default_Stories.
            std::vector<int> parents = parseRequiredStories(reqCSV);
            bool prereqsMet;
            if (parents.empty()) {
                // Legacy single-parent check: current story must meet thresholds
                prereqsMet = (sel.maxScore >= nxtMinScore &&
                              sel.maxSpeed  >= nxtMinSpd);
            } else {
                // Each listed parent must be activated AND exceed thresholds
                prereqsMet = checkAllParentsQualify(db, idUser, parents,
                                                    nxtMinScore, nxtMinSpd);
            }

            if (prereqsMet) {
                nextStoryId   = nxtId;
                nextStoryName = nxtName ? (const char*)nxtName : "";
                SDL_Log("[gameStory] postSync: next story %d '%s' prereqs met "
                        "(CSV='%s' minScore=%d)",
                        nxtId, nextStoryName.c_str(),
                        reqCSV.c_str(), nxtMinScore);
            } else {
                SDL_Log("[gameStory] postSync: next story %d prereqs NOT met "
                        "(CSV='%s' minScore=%d)",
                        nxtId, reqCSV.c_str(), nxtMinScore);
            }
        }
        sqlite3_finalize(nx);
    }

    if (nextStoryId > 0) {
        SDL_Log("[gameStory] postSync: next story %d ('%s') unlocked",
                nextStoryId, nextStoryName.c_str());
        return -nextStoryId;   // negative = "prompt user first" (caller checks sign)
    }

    // No unlock — replay current story
    SDL_Log("[gameStory] postSync: replay story %d", sel.id);
    return sel.id;
}

// [D.6] BGM stub: log URL change, no playback until V3 provides media files.
static std::string s_currentBgmUrl;
static void diagUpdateBgm(const std::string& bgmUrl) {
    if (bgmUrl == s_currentBgmUrl) return;
    s_currentBgmUrl = bgmUrl;
    if (!bgmUrl.empty())
        SDL_Log("[gameStory] BGM stub: would play '%s' (V3)", bgmUrl.c_str());
    else
        SDL_Log("[gameStory] BGM stub: silence");
}

// D.1-D.7: inner dialogue event + render loop
static void dialRunLoop(SDL_Renderer* renderer,
                        const std::vector<DialogueNode>& nodes,
                        int storyId, int chapterId) {
    bool quit      = false;
    bool skipHover = false;
    int  curIdx    = 0;
    int  selCh     = 0;
    std::vector<DialogueChoice> curChoices;
    SDL_Event ev;

    // D.4 placeholder screen when no nodes loaded
    if (nodes.empty()) {
        SDL_Log("[gameStory] no dialogue nodes for story=%d ch=%d",
                storyId, chapterId);
        Uint32 t0 = SDL_GetTicks();
        while (!quit && SDL_GetTicks() - t0 < 2000) {
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_EVENT_QUIT) quit = true;
                if (ev.type == SDL_EVENT_KEY_DOWN) quit = true;
            }
            SDL_SetRenderDrawColor(renderer, 18, 18, 28, 255);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 110, 110, 125, 255);
            SDL_RenderDebugText(renderer, 55.0f, 232.0f, "(no dialogue loaded)");
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }
        return;
    }

    // Load choices for first node
    if (nodes[0].hasChoices)
        curChoices = storyDbLoadChoices(storyId, chapterId, nodes[0].nodeId);

    // D.6 initial BGM
    diagUpdateBgm(nodes[0].bgmUrl);

    while (!quit && curIdx < (int)nodes.size()) {
        const DialogueNode& node = nodes[curIdx];  // valid until curIdx changes
        bool waiting = true;

        while (!quit && waiting) {
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_EVENT_QUIT) {
                    quit = true;
                    break;
                }
                // D.5 keyboard
                if (ev.type == SDL_EVENT_KEY_DOWN) {
                    SDL_Keycode k = ev.key.key;
                    if (k == SDLK_ESCAPE) {
                        quit = true;
                    } else if (k == SDLK_TAB && !curChoices.empty()) {
                        selCh = (selCh + 1) % (int)curChoices.size();
                    } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER
                               || k == SDLK_SPACE) {
                        waiting = false;
                    }
                }
                // D.5 mouse hover
                if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                    skipHover = diagHit(DIAG_SKIP_BTN,
                                        ev.motion.x, ev.motion.y);
                }
                // D.5 mouse click
                if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                    && ev.button.button == SDL_BUTTON_LEFT) {
                    float mx = ev.button.x, my = ev.button.y;
                    if (diagHit(DIAG_SKIP_BTN, mx, my)) {
                        quit = true;
                    } else if (!curChoices.empty()) {
                        int nc = (int)curChoices.size();
                        for (int ci = 0; ci < nc; ci++) {
                            if (diagHit(diagChoiceRect(ci, nc), mx, my)) {
                                selCh   = ci;
                                waiting = false;
                                break;
                            }
                        }
                    } else {
                        waiting = false;   // click anywhere = advance
                    }
                }
            }

            // D.4 render
            drawDialoguePage(renderer, node, curChoices, selCh, skipHover);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }

        if (quit) break;

        // Determine next node
        int nextId = curChoices.empty()
            ? node.nextNodeId
            : curChoices[selCh].nextNodeId;

        selCh = 0;
        curChoices.clear();

        // D.7 terminal condition
        if (nextId == 0) {
            SDL_Delay(300);   // brief pause on last node before returning
            break;
        }

        int nextIdx = findDialNodeIdx(nodes, nextId);
        if (nextIdx < 0) {
            SDL_Log("[gameStory] broken link: nextNodeId=%d not found", nextId);
            break;
        }

        curIdx = nextIdx;

        // Load choices + update BGM for new node
        if (nodes[curIdx].hasChoices)
            curChoices = storyDbLoadChoices(storyId, chapterId,
                                            nodes[curIdx].nodeId);
        diagUpdateBgm(nodes[curIdx].bgmUrl);   // D.6
    }

    s_currentBgmUrl.clear();   // reset bgm state on exit
}

// =============================================================================
// runGameStory — D.2: signature extended with storyId/chapterId.
//   storyId == 0 : original logo intro animation (main.cpp startup call)
//   storyId  > 0 : dialogue mode (triggered from Console after story select)
// =============================================================================
int runGameStory(SDL_Window* window, SDL_Renderer* renderer,
                 int storyId, int chapterId) {
    (void)window;

    // -------------------------------------------------------------------------
    // [D] Dialogue mode
    // -------------------------------------------------------------------------
    if (storyId > 0) {
        bool dbOk = storyDbOpen();
        std::vector<DialogueNode> nodes;
        if (dbOk) nodes = storyDbLoadDialogue(storyId, chapterId);
        dialRunLoop(renderer, nodes, storyId, chapterId);
        storyDbClose();
        return 0;
    }

    // gamestory-logo-intro-01  +  gamestory-loading-bar-02
    // gamestory-corp-credit-03: Intro (storyId == 0)
    // gamestory-dong-bo-sqlite-05a: Gist sync → real progress bar
    // gamestory-nut-bo-qua-cot-truyen-06 [E.1+E.2]: Skip (dialogue phase only)
    //
    // Two-phase state machine:
    //   Phase 1 — STATE_SYNCING:  fetch manifest, update DB, show progress bar.
    //             Skip button HIDDEN (Issue 2.6).
    //             Bar fills proportionally to chapters synced (Issue 2.8).
    //   Phase 2 — STATE_LOGO:    full 8-second logo animation, Skip visible.
    //             If user skips, jump straight to post-sync check.
    //   Post-sync: compare story conditions (Issue 2.9).

    // Open (or reuse) the DB for sync and post-sync check
    bool dbOpenedHere = false;
    if (!g_storyDb) {
        dbOpenedHere = storyDbOpen();
    } else {
        dbOpenedHere = true;
    }

    // --- Ensure shared_meta table exists (sync tracking) ---
    if (g_storyDb) {
        sqlite3_exec(g_storyDb,
            "CREATE TABLE IF NOT EXISTS shared_meta ("
            "  chapter_id    TEXT PRIMARY KEY,"
            "  sha           TEXT NOT NULL,"
            "  updated_at    INTEGER,"
            "  media_base_url TEXT DEFAULT ''"
            ");",
            nullptr, nullptr, nullptr);
    }

    // --- Phase 1: Sync (non-blocking render loop) ---
    // syncFromGist() is synchronous — it blocks for the duration of HTTP.
    // We run it BEFORE entering the render loop to keep logic simple.
    // The render loop below shows the result immediately after.
    // For WASM: Asyncify handles the blocking; browser stays responsive.
    if (g_storyDb) {
        syncFromGist(g_storyDb);
    } else {
        g_syncProgress.state = SYNC_OFFLINE;
        SDL_Log("[gameStory] DB not available — skip sync");
    }

    // --- Phase 2: Logo animation with real progress bar ---
    enum IntroPhase { PHASE_LOGO, PHASE_DONE };
    IntroPhase phase = PHASE_LOGO;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (phase != PHASE_DONE) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) std::exit(0);

            // Issue 2.6/2.12: Skip button is hidden during the logo phase.
            // ESC still allows keyboard exit of the intro animation.
            if (phase == PHASE_LOGO) {
                if (event.type == SDL_EVENT_KEY_DOWN &&
                    event.key.key == SDLK_ESCAPE) {
                    phase = PHASE_DONE;
                }
            }
        }

        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime > (Uint32)INTRO_DURATION) phase = PHASE_DONE;

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        drawLogo(renderer, elapsedTime);

        // Issue 2.8: Real progress bar
        // If sync updated chapters: bar = done/total (0..100%).
        // If all SHAs matched (total==0) or offline: bar fills instantly to 100%.
        float syncProgress = 1.0f;
        if (g_syncProgress.total > 0) {
            syncProgress = (float)g_syncProgress.done / (float)g_syncProgress.total;
        }
        // Blend sync progress with time progress — whichever is further ahead.
        float timeProgress = (float)elapsedTime / (float)INTRO_DURATION;
        float barProgress = (syncProgress > timeProgress) ? syncProgress : timeProgress;

        // Draw bar using syncProgress as fill, same layout as drawLoadingBar()
        {
            const float barW = 180.0f, barH = 12.0f;
            const float barX = (STORY_SCREEN_WIDTH  - barW) / 2.0f;
            const float barY = STORY_SCREEN_HEIGHT - 110.0f;
            SDL_FRect bgBar = { barX, barY, barW, barH };
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &bgBar);
            float fill = barProgress;
            if (fill > 1.0f) fill = 1.0f;
            SDL_FRect fgBar = { barX, barY, barW * fill, barH };
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &fgBar);
            // Status line
            char statusBuf[48] = "";
            if (g_syncProgress.state == SYNC_UPDATING_DB &&
                !g_syncProgress.currentId.empty()) {
                SDL_snprintf(statusBuf, sizeof(statusBuf), "Syncing %s...",
                            g_syncProgress.currentId.c_str());
            } else if (g_syncProgress.state == SYNC_OFFLINE) {
                SDL_strlcpy(statusBuf, "Offline - using local data", sizeof(statusBuf));
            } else if (g_syncProgress.state == SYNC_DONE) {
                SDL_strlcpy(statusBuf, "Up to date", sizeof(statusBuf));
            }
            if (statusBuf[0] != '\0') {
                int sl2 = (int)SDL_strlen(statusBuf);
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 200);
                SDL_RenderDebugText(renderer,
                    (STORY_SCREEN_WIDTH - sl2 * 8.0f) / 2.0f,
                    barY - 18.0f, statusBuf);
            }
            // Corp credit below bar
            float barBottomY = barY + barH;
            drawCorpCredit(renderer, elapsedTime, barBottomY);
        }

        // [ SKIP ] is rendered only by drawDialoguePage() in dialRunLoop().
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // --- Issue 2.9: Post-sync condition check ---
    // Determine next story to run based on DB state.
    // postSyncConditionCheck() returns:
    //   0          → no story (proceed to Console)
    //   positive N → run story N immediately (first time or replay)
    //  negative N  → story N is newly unlocked; show prompt then run
    int nextStory = 0;
    if (g_storyDb) {
        nextStory = postSyncConditionCheck(g_storyDb, "default");
    }

    // Show "Chapter Completion" banner if all stories in chapter are done
    // (postSyncConditionCheck returns 0 only when no more stories)
    if (nextStory == 0 && g_storyDb) {
        // Check if any activated story exists — if so, show completion
        sqlite3_stmt* chk = nullptr;
        std::string chkSQL = "SELECT COUNT(*) FROM " + storyUserTable("default") +
            " WHERE idUser='default' AND isActivated=1;";
        if (sqlite3_prepare_v2(g_storyDb, chkSQL.c_str(),
            -1, &chk, nullptr) == SQLITE_OK) {
            if (sqlite3_step(chk) == SQLITE_ROW &&
                sqlite3_column_int(chk, 0) > 0) {
                // TODO: show banner "Chapter Complete!"
                SDL_Log("[gameStory] Chapter complete — ready to exit to Console");
            }
            sqlite3_finalize(chk);
        }
    }

    // Show "unlock" prompt when a new story is available (negative return)
    bool runNextStory = false;
    if (nextStory < 0) {
        int unlockId = -nextStory;
        std::string unlockName;
        if (g_storyDb) {
            sqlite3_stmt* ns = nullptr;
            if (sqlite3_prepare_v2(g_storyDb,
                "SELECT storyName FROM shared_data WHERE idStory=? LIMIT 1;",
                -1, &ns, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(ns, 1, unlockId);
                if (sqlite3_step(ns) == SQLITE_ROW) {
                    const unsigned char* p = sqlite3_column_text(ns, 0);
                    if (p) unlockName = (const char*)p;
                }
                sqlite3_finalize(ns);
            }
        }
        // Show yes/no prompt
        char promptLine[64];
        SDL_snprintf(promptLine, sizeof(promptLine),
                     "Unlocked: %s!", unlockName.c_str());
        enum PromptChoice { PROMPT_NONE, PROMPT_YES, PROMPT_NO };
        PromptChoice choice = PROMPT_NONE;
        bool promptHoverYes = false, promptHoverNo = false;
        SDL_FRect yesBtn = { 55.0f, 280.0f, 60.0f, 24.0f };
        SDL_FRect noBtn  = { 155.0f, 280.0f, 60.0f, 24.0f };

        while (choice == PROMPT_NONE) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) std::exit(0);
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    promptHoverYes = diagHit(yesBtn, event.motion.x, event.motion.y);
                    promptHoverNo  = diagHit(noBtn,  event.motion.x, event.motion.y);
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    if (diagHit(yesBtn, event.button.x, event.button.y))
                        choice = PROMPT_YES;
                    else if (diagHit(noBtn, event.button.x, event.button.y))
                        choice = PROMPT_NO;
                }
            }
            SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
            SDL_RenderClear(renderer);
            int pl = (int)SDL_strlen(promptLine);
            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
            SDL_RenderDebugText(renderer,
                (STORY_SCREEN_WIDTH - pl * 8.0f) / 2.0f, 200.0f, promptLine);
            // YES button
            SDL_SetRenderDrawColor(renderer,
                promptHoverYes ? 100 : 55, promptHoverYes ? 155 : 80,
                promptHoverYes ? 200 : 120, 255);
            SDL_RenderFillRect(renderer, &yesBtn);
            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
            SDL_RenderRect(renderer, &yesBtn);
            SDL_RenderDebugText(renderer, yesBtn.x + 22.0f, yesBtn.y + 8.0f, "YES");
            // NO button
            SDL_SetRenderDrawColor(renderer,
                promptHoverNo ? 200 : 120, promptHoverNo ? 80 : 80,
                promptHoverNo ? 100 : 120, 255);
            SDL_RenderFillRect(renderer, &noBtn);
            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
            SDL_RenderRect(renderer, &noBtn);
            SDL_RenderDebugText(renderer, noBtn.x + 24.0f, noBtn.y + 8.0f, "NO");
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }

        if (choice == PROMPT_YES) {
            // Update DB: set new story as selected, deselect old
            if (g_storyDb) {
                sqlite3_exec(g_storyDb,
                    ("UPDATE " + storyUserTable("default") +
                     " SET isSelected=0 WHERE idUser='default';").c_str(),
                    nullptr, nullptr, nullptr);
                sqlite3_exec(g_storyDb,
                    ("INSERT OR REPLACE INTO " + storyUserTable("default") + " "
                     "(idUser,idStory,idChapter,isActivated,isSelected) "
                     "SELECT 'default', idStory, idChapter, 1, 1 FROM shared_data "
                     "WHERE idStory=" + std::to_string(unlockId) + " LIMIT 1;").c_str(),
                    nullptr, nullptr, nullptr);
#ifdef __EMSCRIPTEN__
                EM_ASM({ Module['FS'].syncfs(false, function(){}); });
#endif
            }
            nextStory = unlockId;
            runNextStory = true;
        } else {
            // No: replay current story
            nextStory = (nextStory < 0) ? -nextStory : nextStory;
            // find and replay the currently selected story instead
            if (g_storyDb) {
                std::string csSQL = "SELECT idStory, idChapter FROM " +
                    storyUserTable("default") +
                    " WHERE idUser='default' AND isSelected=1 LIMIT 1;";
                sqlite3_stmt* cs = nullptr;
                if (sqlite3_prepare_v2(g_storyDb, csSQL.c_str(),
                    -1, &cs, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(cs) == SQLITE_ROW) {
                        nextStory = sqlite3_column_int(cs, 0);
                    }
                    sqlite3_finalize(cs);
                }
            }
            runNextStory = (nextStory > 0);
        }
    } else if (nextStory > 0) {
        runNextStory = true;
    }

    // Run story dialogue if determined above
    if (runNextStory && nextStory > 0) {
        // Find chapter for this story
        int storyCh = 1;
        if (g_storyDb) {
            sqlite3_stmt* cs = nullptr;
            if (sqlite3_prepare_v2(g_storyDb,
                "SELECT idChapter FROM shared_data WHERE idStory=? LIMIT 1;",
                -1, &cs, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(cs, 1, nextStory);
                if (sqlite3_step(cs) == SQLITE_ROW) {
                    storyCh = sqlite3_column_int(cs, 0);
                }
                sqlite3_finalize(cs);
            }
        }
        std::vector<DialogueNode> nodes = storyDbLoadDialogue(nextStory, storyCh);
        dialRunLoop(renderer, nodes, nextStory, storyCh);
    }

    if (dbOpenedHere) storyDbClose();

    // Cleanup intro textures (lazy-init will recreate on re-entry)
    if (g_logo.texture) {
        SDL_DestroyTexture(g_logo.texture);
        g_logo.texture = nullptr;
        g_logo.w = g_logo.h = 0;
    }
    if (g_corp.texture) {
        SDL_DestroyTexture(g_corp.texture);
        g_corp.texture = nullptr;
        g_corp.w = g_corp.h = 0;
    }
    return 0;
}

// integration/v3
// V3 additions verified (see taskStory.md Task 3.1-3.3):
//   [V3.1] gamestory-tich-hop-backend-07:
//           buildDownloadQueue() collects all media URLs from shared_data +
//           shared_dialogues via shared_meta.media_base_url JOIN.
//           tickMediaDownload() downloads one file per render-loop tick;
//           skips files already in SDL_GetPrefPath()/media/ cache.
//           SDL_CreateDirectory() creates cache dir cross-platform (SDL3).
//   [V3.2] gamestory-hieu-chinh-loading-bar-theo-download-speed-08:
//           Bar fill = max(syncProgress, mediaProgress, timeProgress).
//           Status line shows "Media X/Y  N.N KB/s" while downloading.
//           Logo loop repeats until BOTH INTRO_DURATION elapsed AND dlDone.
//   [V3.3] integration/v3 wired; dual-mode entry unchanged (storyId=0 vs >0).

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_Window* window = SDL_CreateWindow("Game Story \xC2\xA9 - Standalone",
                                          STORY_SCREEN_WIDTH, STORY_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    // Default: run intro. Pass storyId=1 to test dialogue mode.
    runGameStory(window, renderer, 0, 0);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif