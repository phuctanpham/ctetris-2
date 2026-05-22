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
#ifndef __EMSCRIPTEN__
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <atomic>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

// stb_image: single-header PNG/JPG decoder (vendored from SDL3 source)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

// stb_truetype: single-header TTF rasterizer. Used to render Unicode
// (Vietnamese) dialogue text -- SDL_RenderDebugText is ASCII-only.
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "gameStory_font.h"   // embedded Liberation Sans Bold (Vietnamese-capable)

// dr_mp3: single-header MP3 decoder. Dialogue BGM assets are .mp3 (ID3), which
// SDL_LoadWAV cannot decode -- decode to PCM here, then feed an SDL_AudioStream.
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"

#include "ctetris_debug.h"

#include <map>

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

// ---------------------------------------------------------------------------
// TTF (stb_truetype) Unicode text rendering. SDL_RenderDebugText is ASCII-only,
// so Vietnamese dialogue (speaker, body, choices) is rendered through here.
// Glyphs are rasterized on first use and cached as SDL textures for the app
// lifetime. Color is applied per draw via texture color/alpha modulation.
// ---------------------------------------------------------------------------
static const float    TTF_PIXEL_H = 13.0f;
static stbtt_fontinfo g_ttf;
static bool           g_ttfReady   = false;
static float          g_ttfScale   = 1.0f;
static int            g_ttfAscent  = 0, g_ttfDescent = 0, g_ttfLineGap = 0;

struct TtfGlyph {
    SDL_Texture* tex = nullptr;
    int   w = 0, h = 0, xoff = 0, yoff = 0;
    float advance = 0.0f;
};
static std::map<int, TtfGlyph> g_ttfCache;

static void ttfInit() {
    if (g_ttfReady) return;
    if (!stbtt_InitFont(&g_ttf, g_dialogueFontData,
                        stbtt_GetFontOffsetForIndex(g_dialogueFontData, 0))) {
        SDL_Log("[gameStory] stbtt_InitFont failed");
        return;
    }
    g_ttfScale = stbtt_ScaleForPixelHeight(&g_ttf, TTF_PIXEL_H);
    stbtt_GetFontVMetrics(&g_ttf, &g_ttfAscent, &g_ttfDescent, &g_ttfLineGap);
    g_ttfReady = true;
}

// Decode one UTF-8 codepoint starting at *i; advances *i past the sequence.
static int utf8Next(const char* s, int len, int* i) {
    unsigned char c = (unsigned char)s[*i];
    int cp, n;
    if      (c < 0x80)      { cp = c;        n = 1; }
    else if ((c >> 5) == 0x6)  { cp = c & 0x1F; n = 2; }
    else if ((c >> 4) == 0xE)  { cp = c & 0x0F; n = 3; }
    else if ((c >> 3) == 0x1E) { cp = c & 0x07; n = 4; }
    else                       { cp = 0xFFFD;   n = 1; }
    int j = *i + 1;
    for (int k = 1; k < n && j < len; k++, j++) {
        unsigned char cc = (unsigned char)s[j];
        if ((cc & 0xC0) != 0x80) break;
        cp = (cp << 6) | (cc & 0x3F);
    }
    *i += n;
    return cp;
}

static TtfGlyph* ttfGlyph(SDL_Renderer* r, int cp) {
    auto it = g_ttfCache.find(cp);
    if (it != g_ttfCache.end()) return &it->second;
    TtfGlyph g;
    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(&g_ttf, cp, &adv, &lsb);
    g.advance = adv * g_ttfScale;
    int w = 0, h = 0, xo = 0, yo = 0;
    unsigned char* bmp = stbtt_GetCodepointBitmap(&g_ttf, 0, g_ttfScale,
                                                  cp, &w, &h, &xo, &yo);
    if (bmp && w > 0 && h > 0) {
        std::vector<unsigned char> rgba((size_t)w * h * 4);
        for (int p = 0; p < w * h; p++) {
            rgba[p*4+0] = 255; rgba[p*4+1] = 255;
            rgba[p*4+2] = 255; rgba[p*4+3] = bmp[p];
        }
        SDL_Surface* surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32,
                                                  rgba.data(), w * 4);
        if (surf) {
            g.tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_DestroySurface(surf);
            if (g.tex) SDL_SetTextureBlendMode(g.tex, SDL_BLENDMODE_BLEND);
        }
        g.w = w; g.h = h; g.xoff = xo; g.yoff = yo;
    }
    if (bmp) stbtt_FreeBitmap(bmp, nullptr);
    auto res = g_ttfCache.emplace(cp, g);
    return &res.first->second;
}

// Draw UTF-8 text with top-left at (x,y); returns advance width in pixels.
static float ttfDrawText(SDL_Renderer* r, float x, float y,
                         const char* text, SDL_Color color) {
    ttfInit();
    if (!g_ttfReady || !text) return 0.0f;
    float baseline = y + g_ttfAscent * g_ttfScale;
    float pen = x;
    int len = (int)SDL_strlen(text);
    int i = 0, prev = 0;
    while (i < len) {
        int cp = utf8Next(text, len, &i);
        if (cp == '\n') continue;
        TtfGlyph* g = ttfGlyph(r, cp);
        if (prev) pen += stbtt_GetCodepointKernAdvance(&g_ttf, prev, cp) * g_ttfScale;
        if (g->tex) {
            SDL_SetTextureColorMod(g->tex, color.r, color.g, color.b);
            SDL_SetTextureAlphaMod(g->tex, color.a);
            SDL_FRect dst = { pen + g->xoff, baseline + g->yoff,
                              (float)g->w, (float)g->h };
            SDL_RenderTexture(r, g->tex, nullptr, &dst);
        }
        pen += g->advance;
        prev = cp;
    }
    return pen - x;
}

// Measure pixel width of a UTF-8 substring [text, text+len).
static float ttfMeasure(SDL_Renderer* r, const char* text, int len) {
    ttfInit();
    if (!g_ttfReady) return 0.0f;
    float pen = 0; int i = 0, prev = 0;
    while (i < len) {
        int cp = utf8Next(text, len, &i);
        TtfGlyph* g = ttfGlyph(r, cp);
        if (prev) pen += stbtt_GetCodepointKernAdvance(&g_ttf, prev, cp) * g_ttfScale;
        pen += g->advance; prev = cp;
    }
    return pen;
}

// Greedy word-wrap by pixel width. Returns the number of lines drawn.
static int ttfDrawWrapped(SDL_Renderer* r, const char* text,
                          float x, float y, float maxW,
                          float lineH, int maxLines, SDL_Color color) {
    ttfInit();
    if (!g_ttfReady || !text) return 0;
    int len = (int)SDL_strlen(text);
    int lines = 0, i = 0, wordStart = 0;
    std::string cur;
    auto flush = [&](const std::string& s) {
        ttfDrawText(r, x, y + lines * lineH, s.c_str(), color);
        lines++;
    };
    while (i <= len && lines < maxLines) {
        if (i == len || text[i] == ' ') {
            std::string word(text + wordStart, i - wordStart);
            std::string trial = cur.empty() ? word : cur + " " + word;
            float w = ttfMeasure(r, trial.c_str(), (int)trial.size());
            if (w <= maxW || cur.empty()) cur = trial;
            else { flush(cur); cur = word; }
            wordStart = i + 1;
            if (i == len) break;
        }
        i++;
    }
    if (lines < maxLines && !cur.empty()) flush(cur);
    return lines;
}

// Word-wrap renderer -- now Unicode-capable via stb_truetype.
static int diagDrawWrapped(SDL_Renderer* r, const char* text,
                            float x, float y, SDL_Color color) {
    return ttfDrawWrapped(r, text, x, y,
                          DIAG_TB_W - 16.0f, DIAG_LINE_H,
                          DIAG_MAX_LINES, color);
}

static void drawDialoguePage(SDL_Renderer* renderer,
                              const DialogueNode&              node,
                              const std::vector<DialogueChoice>& choices,
                              int   selChoice,
                              bool  skipHover,
                              SDL_Texture* imgTex = nullptr) {
    int nc = (int)choices.size();

    // background
    SDL_SetRenderDrawColor(renderer,
        DIAG_BG.r, DIAG_BG.g, DIAG_BG.b, DIAG_BG.a);
    SDL_RenderClear(renderer);

    // image area: render texture if available, else placeholder
    SDL_SetRenderDrawColor(renderer,
        DIAG_IMG_BG.r, DIAG_IMG_BG.g, DIAG_IMG_BG.b, 255);
    SDL_RenderFillRect(renderer, &DIAG_IMG_AREA);
    SDL_SetRenderDrawColor(renderer,
        DIAG_IMG_BORDER.r, DIAG_IMG_BORDER.g, DIAG_IMG_BORDER.b, 255);
    SDL_RenderRect(renderer, &DIAG_IMG_AREA);
    if (imgTex) {
        SDL_RenderTexture(renderer, imgTex, nullptr, &DIAG_IMG_AREA);
    } else if (node.imageUrl.empty()) {
        const char* lbl = "[ IMAGE ]";
        int ll = (int)SDL_strlen(lbl);
        SDL_SetRenderDrawColor(renderer, 75, 78, 98, 255);
        SDL_RenderDebugText(renderer,
            DIAG_IMG_AREA.x + (DIAG_IMG_AREA.w - ll * 8.0f) / 2.0f,
            DIAG_IMG_AREA.y + (DIAG_IMG_AREA.h - 8.0f)  / 2.0f,
            lbl);
    } else {
        const char* lbl = "[ loading... ]";
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

    // speaker name (Unicode via TTF)
    if (!node.speaker.empty()) {
        ttfDrawText(renderer, DIAG_TXT_X, DIAG_SPK_Y,
                    node.speaker.c_str(), DIAG_SPEAKER);
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
            SDL_Color lblColor = (i == selChoice) ? DIAG_SPEAKER : DIAG_TEXT;
            ttfDrawText(renderer, cr.x + 8.0f,
                        cr.y + (cr.h - TTF_PIXEL_H) / 2.0f,
                        choices[i].label.c_str(), lblColor);
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

// forward declaration — defined later in this file
static std::string httpGetSync(const char* url);

// ---------------------------------------------------------------------------
// Dialogue media: image texture cache + BGM audio stream
// ---------------------------------------------------------------------------
static std::map<std::string, SDL_Texture*> g_imgCache;
// BGM (looping-feel, replaced per node) and SFX (one-shot) share ONE audio
// device, each with its own stream bound to it (SDL mixes them). Opening the
// default device twice is unreliable; a single device is robust on native and
// WASM. The device is opened lazily and resumed (WASM needs a gesture to
// actually start, which the first dialogue click provides).
static SDL_AudioDeviceID g_audioDev  = 0;
static SDL_AudioSpec     g_devSpec   = { SDL_AUDIO_S16, 2, 44100 };
static SDL_AudioStream*  g_bgmStream = nullptr;
static SDL_AudioStream*  g_sfxStream = nullptr;
static std::string       g_bgmUrl;

static bool dialEnsureAudioDevice() {
    if (g_audioDev) return true;
    SDL_AudioSpec want = { SDL_AUDIO_S16, 2, 44100 };
    g_audioDev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want);
    if (!g_audioDev) return false;
    SDL_GetAudioDeviceFormat(g_audioDev, &g_devSpec, NULL);
    SDL_ResumeAudioDevice(g_audioDev);
    return true;
}

static void dialStopBgm() {
    if (g_bgmStream) { SDL_DestroyAudioStream(g_bgmStream); g_bgmStream = nullptr; }
    g_bgmUrl.clear();
}

static void dialStopSfx() {
    if (g_sfxStream) { SDL_DestroyAudioStream(g_sfxStream); g_sfxStream = nullptr; }
}

// Full teardown on dialogue exit: drop both streams + close the shared device.
static void dialCloseAudio() {
    dialStopBgm();
    dialStopSfx();
    if (g_audioDev) { SDL_CloseAudioDevice(g_audioDev); g_audioDev = 0; }
}

// Decode WAV (RIFF) or MP3 (dr_mp3) bytes to PCM. Returns true on success;
// caller frees pcm with drmp3_free (isMp3) or SDL_free (WAV).
static bool decodeAudio(const std::string& bytes, SDL_AudioSpec* spec,
                        Uint8** pcm, Uint32* pcmLen, bool* isMp3) {
    *pcm = nullptr; *pcmLen = 0; *isMp3 = false;
    if (bytes.size() < 12) return false;
    if (SDL_memcmp(bytes.data(), "RIFF", 4) == 0) {
        SDL_IOStream* io = SDL_IOFromMem((void*)bytes.data(), (int)bytes.size());
        if (!io || !SDL_LoadWAV_IO(io, true, spec, pcm, pcmLen)) return false;
        return true;
    }
    drmp3_config cfg{};
    drmp3_uint64 frames = 0;
    drmp3_int16* s16 = drmp3_open_memory_and_read_pcm_frames_s16(
        bytes.data(), bytes.size(), &cfg, &frames, nullptr);
    if (!s16 || cfg.channels == 0 || frames == 0) {
        if (s16) drmp3_free(s16, nullptr);
        return false;
    }
    spec->format   = SDL_AUDIO_S16;
    spec->channels = (int)cfg.channels;
    spec->freq     = (int)cfg.sampleRate;
    *pcm    = (Uint8*)s16;
    *pcmLen = (Uint32)(frames * cfg.channels * sizeof(drmp3_int16));
    *isMp3  = true;
    return true;
}

// Play a one-shot sound effect (fire-and-forget; replaces any prior SFX).
static void dialPlaySfx(const std::string& url) {
    if (url.empty()) return;
    if (!dialEnsureAudioDevice()) return;
    std::string bytes = httpGetSync(url.c_str());
    if (bytes.empty()) return;
    SDL_AudioSpec spec{}; Uint8* pcm = nullptr; Uint32 pcmLen = 0; bool isMp3 = false;
    if (!decodeAudio(bytes, &spec, &pcm, &pcmLen, &isMp3)) {
        SDL_Log("[gameStory] SFX decode failed: %s", url.c_str());
        return;
    }
    dialStopSfx();
    g_sfxStream = SDL_CreateAudioStream(&spec, &g_devSpec);
    if (g_sfxStream) {
        SDL_BindAudioStream(g_audioDev, g_sfxStream);
        SDL_PutAudioStreamData(g_sfxStream, pcm, (int)pcmLen);
        SDL_ResumeAudioDevice(g_audioDev);
    }
    if (isMp3) drmp3_free(pcm, nullptr); else SDL_free(pcm);
}

static SDL_Texture* dialLoadImage(SDL_Renderer* renderer, const std::string& url) {
    if (url.empty()) return nullptr;
    auto it = g_imgCache.find(url);
    if (it != g_imgCache.end()) return it->second;

    std::string bytes = httpGetSync(url.c_str());
    if (bytes.empty()) return nullptr;

    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load_from_memory(
        (const unsigned char*)bytes.data(), (int)bytes.size(), &w, &h, &n, 4);
    if (!px) return nullptr;

    SDL_Surface* surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4);
    SDL_Texture* tex = surf ? SDL_CreateTextureFromSurface(renderer, surf) : nullptr;
    SDL_DestroySurface(surf);
    stbi_image_free(px);

    if (tex) g_imgCache[url] = tex;
    return tex;
}

static void dialPlayBgm(const std::string& url) {
    if (url == g_bgmUrl) return;
    dialStopBgm();
    if (url.empty()) return;

    if (!dialEnsureAudioDevice()) { g_bgmUrl = url; return; }

    std::string bytes = httpGetSync(url.c_str());
    if (bytes.empty()) { g_bgmUrl = url; return; }

    // Decode to PCM (WAV or MP3 -- dialogue BGM assets are .mp3).
    SDL_AudioSpec spec{};
    Uint8* pcm = nullptr; Uint32 pcmLen = 0; bool isMp3 = false;
    if (!decodeAudio(bytes, &spec, &pcm, &pcmLen, &isMp3)) {
        SDL_Log("[gameStory] BGM decode failed: %s", url.c_str());
        g_bgmUrl = url; return;
    }

    g_bgmStream = SDL_CreateAudioStream(&spec, &g_devSpec);
    if (g_bgmStream) {
        SDL_BindAudioStream(g_audioDev, g_bgmStream);
        SDL_PutAudioStreamData(g_bgmStream, pcm, (int)pcmLen);   // copies data
        SDL_ResumeAudioDevice(g_audioDev);
    }
    if (isMp3) drmp3_free(pcm, nullptr); else SDL_free(pcm);
    g_bgmUrl = url;
}

// ---------------------------------------------------------------------------
// Minimal HTTP GET — returns body string or empty on failure.
// Native:  libcurl synchronous (blocking, acceptable for startup sync).
// WASM:    emscripten_fetch synchronous mode (uses Asyncify to avoid blocking
//          the browser event loop; requires -sASYNCIFY in link flags).
// Offline: returns "" — all callers treat empty response as offline.
// ---------------------------------------------------------------------------
#ifdef __EMSCRIPTEN__
// Async emscripten_fetch made synchronous via Asyncify. Synchronous
// emscripten_fetch is ILLEGAL on the browser main thread (a synchronous XHR
// cannot use arraybuffer responseType -> instant status 0 failure), which is
// why the old SYNCHRONOUS path reported "offline" even when online. Instead we
// issue an ASYNC fetch and pump the JS event loop with emscripten_sleep until
// the success/error callback flips `done`. Requires -sASYNCIFY=1 (set).
struct EmFetchResult { std::string body; int status = 0; bool done = false; };
static void emFetchOk(emscripten_fetch_t* f) {
    EmFetchResult* r = (EmFetchResult*)f->userData;
    if (f->numBytes > 0) r->body.assign(f->data, (size_t)f->numBytes);
    r->status = (int)f->status;
    r->done   = true;
    emscripten_fetch_close(f);
}
static void emFetchErr(emscripten_fetch_t* f) {
    EmFetchResult* r = (EmFetchResult*)f->userData;
    r->status = (int)f->status;
    r->done   = true;
    emscripten_fetch_close(f);
}
#endif

static std::string httpGetSync(const char* url) {
    if (!url || url[0] == '\0') return "";

    CTDBG_REQ("GET", url);

#ifdef __EMSCRIPTEN__
    EmFetchResult res;
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    SDL_strlcpy(attr.requestMethod, "GET", sizeof(attr.requestMethod));
    attr.attributes   = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;   // async (no SYNCHRONOUS)
    attr.timeoutMSecs = 20000;
    attr.onsuccess    = emFetchOk;
    attr.onerror      = emFetchErr;
    attr.userData     = &res;
    emscripten_fetch(&attr, url);
    while (!res.done) emscripten_sleep(8);   // Asyncify pumps the JS event loop
    if (res.status == 200 && !res.body.empty()) {
        CTDBG_RES("GET", url, 200, res.body.size());
        CTDBG_BODY(res.body);
        return res.body;
    }
    CTDBG_RES("GET", url, res.status, 0);
    CTDBG_ERR("emscripten_fetch(async): non-200 or empty");
    SDL_Log("[gameStory] httpGetSync WASM: HTTP %d for %s", res.status, url);
    return std::string();

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

static void diagUpdateBgm(const std::string& fullUrl) {
    // Empty bgmUrl on a node means "no change" -> keep the current track
    // playing through to dialogue end (do NOT stop). Only a new, different
    // non-empty URL switches tracks.
    if (fullUrl.empty()) return;
    dialPlayBgm(fullUrl);
}

// Resolve a media filename to a full URL using shared_meta.media_base_url.
static std::string dialMediaUrl(const std::string& filename, int chapterId) {
    if (filename.empty()) return "";
    if (filename.rfind("http", 0) == 0) return filename;   // already absolute
    char cid[8];
    SDL_snprintf(cid, sizeof(cid), "c%03d", chapterId);
    std::string base = g_storyDb ? metaGetMediaBaseUrl(g_storyDb, cid) : "";
    if (base.empty()) return "";
    if (base.back() != '/') base += '/';
    return base + filename;
}

// Chapter thumbnail used as fallback when a dialogue node has no imageUrl.
static std::string dialChapterThumbnailUrl(int storyId, int chapterId) {
    if (!g_storyDb) return "";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_storyDb,
        "SELECT thumbnailPath FROM shared_data "
        "WHERE idStory=? AND idChapter=?;",
        -1, &st, nullptr) != SQLITE_OK) return "";
    sqlite3_bind_int(st, 1, storyId);
    sqlite3_bind_int(st, 2, chapterId);
    std::string thumb;
    if (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char* p = sqlite3_column_text(st, 0);
        if (p) thumb = (const char*)p;
    }
    sqlite3_finalize(st);
    return dialMediaUrl(thumb, chapterId);
}

// Picks node image URL with chapter-thumbnail fallback when node has none.
static std::string dialNodeImageUrl(const DialogueNode& node,
                                     int storyId, int chapterId) {
    if (!node.imageUrl.empty())
        return dialMediaUrl(node.imageUrl, chapterId);
    return dialChapterThumbnailUrl(storyId, chapterId);
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
    SDL_Texture* curImg = nullptr;

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

    // Media for the first node is fetched/decoded synchronously (image + BGM
    // can be hundreds of KB), which would otherwise freeze on the previous
    // screen. Paint a "Loading dialogue..." frame first so the transition is
    // visible and it's clear the app is progressing, not stuck.
    {
        SDL_SetRenderDrawColor(renderer, 18, 18, 28, 255);
        SDL_RenderClear(renderer);
        const char* lm = "Loading dialogue...";
        SDL_SetRenderDrawColor(renderer, 180, 190, 210, 255);
        SDL_RenderDebugText(renderer,
            (STORY_SCREEN_WIDTH - (int)SDL_strlen(lm) * 8.0f) / 2.0f,
            232.0f, lm);
        SDL_RenderPresent(renderer);
    }
    SDL_Log("[gameStory] dialogue load story=%d ch=%d nodes=%d",
            storyId, chapterId, (int)nodes.size());

    // Load choices and media for first node
    if (nodes[0].hasChoices)
        curChoices = storyDbLoadChoices(storyId, chapterId, nodes[0].nodeId);
    curImg = dialLoadImage(renderer, dialNodeImageUrl(nodes[0], storyId, chapterId));

    // D.6 initial BGM + SFX
    diagUpdateBgm(dialMediaUrl(nodes[0].bgmUrl, chapterId));
    dialPlaySfx(dialMediaUrl(nodes[0].sfxUrl, chapterId));
    SDL_Log("[gameStory] dialogue first-node media loaded");

    while (!quit && curIdx < (int)nodes.size()) {
        const DialogueNode& node = nodes[curIdx];  // valid until curIdx changes
        bool waiting = true;

        while (!quit && waiting) {
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_EVENT_QUIT) {
                    quit = true;
                    break;
                }
                // On the browser, the audio context is suspended until a user
                // gesture. The dialogue can auto-start after sync (no gesture
                // yet), so resume on the first key/click to make BGM+SFX audible.
                if (ev.type == SDL_EVENT_KEY_DOWN ||
                    ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    if (g_audioDev) SDL_ResumeAudioDevice(g_audioDev);
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
            drawDialoguePage(renderer, node, curChoices, selCh, skipHover, curImg);
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

        // Load choices + media for new node
        if (nodes[curIdx].hasChoices)
            curChoices = storyDbLoadChoices(storyId, chapterId,
                                            nodes[curIdx].nodeId);
        curImg = dialLoadImage(renderer, dialNodeImageUrl(nodes[curIdx], storyId, chapterId));
        diagUpdateBgm(dialMediaUrl(nodes[curIdx].bgmUrl, chapterId));
        dialPlaySfx(dialMediaUrl(nodes[curIdx].sfxUrl, chapterId));
    }

    dialCloseAudio();   // drop streams + close shared device on exit
}

// =============================================================================
// Startup flow — deterministic, step-by-step intro replacing the timed bar.
// Each step is shown with a status (running "...", ok, fail, info, skip) so the
// player can actually see every background job instead of a single "100%".
// Flow:
//   1. init db setup          (existed / created)
//   2. shared db check        (existed / unexisted)
//   3. current manifest ver   (from DB; 00.00 if none)
//   4. internet connection    (passed / fail -> confirm before console)
//   5. latest manifest ver    (if == current -> launch automatically)
//   6. shared data fetch      (recursive: manifest -> each c{id}.json)
//   7. shared db setup        (import rows) -> recheck version -> launch
// Single-threaded + frame-driven so it animates identically on native + WASM
// (the WASM async fetch path keeps the event loop alive during requests).
// =============================================================================
enum FlowStepStatus { FS_RUN, FS_OK, FS_FAIL, FS_INFO, FS_SKIP };
struct FlowStep { std::string label; std::string detail; FlowStepStatus status; };
static std::vector<FlowStep> g_flow;
// Progress bar: g_flowPctTarget is set at each milestone; g_flowPct eases
// toward it every frame so the bar visibly increments as child tasks finish.
static float g_flowPct       = 0.0f;
static float g_flowPctTarget = 0.0f;

static int flowAdd(const std::string& label, FlowStepStatus st,
                   const std::string& detail = "") {
    g_flow.push_back({ label, detail, st });
    return (int)g_flow.size() - 1;
}
static void flowSet(int i, FlowStepStatus st, const std::string& detail = "") {
    if (i < 0 || i >= (int)g_flow.size()) return;
    g_flow[i].status = st;
    if (!detail.empty()) g_flow[i].detail = detail;
}
static void flowProgress(float target) {
    if (target < 0.0f) target = 0.0f;
    if (target > 1.0f) target = 1.0f;
    g_flowPctTarget = target;
}

// Single-line status console + percentage progress bar (replaces the old
// multi-line task list). The bar eases toward the latest milestone so the
// percent climbs as each child task completes.
static void flowRender(SDL_Renderer* r, Uint32 elapsed) {
    SDL_SetRenderDrawColor(r, 30, 30, 30, 255);
    SDL_RenderClear(r);
    drawLogo(r, elapsed);

    // Ease displayed percent toward target.
    g_flowPct += (g_flowPctTarget - g_flowPct) * 0.18f;
    if (g_flowPct > g_flowPctTarget - 0.001f) g_flowPct = g_flowPctTarget;

    const float barW = 180.0f, barH = 14.0f;
    const float barX = (STORY_SCREEN_WIDTH - barW) / 2.0f;
    const float barY = STORY_SCREEN_HEIGHT - 96.0f;

    SDL_FRect bg = { barX, barY, barW, barH };
    SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
    SDL_RenderFillRect(r, &bg);
    float fill = g_flowPct; if (fill < 0) fill = 0; if (fill > 1) fill = 1;
    if (fill > 0.0f) {
        SDL_FRect fg = { barX, barY, barW * fill, barH };
        SDL_SetRenderDrawColor(r, 0, 200, 80, 255);
        SDL_RenderFillRect(r, &fg);
    }
    char pctBuf[8];
    SDL_snprintf(pctBuf, sizeof(pctBuf), "%d%%", (int)(fill * 100.0f));
    SDL_SetRenderDrawColor(r, 10, 10, 10, 220);
    SDL_RenderDebugText(r, barX + (barW - (int)SDL_strlen(pctBuf) * 8.0f) / 2.0f,
                        barY + (barH - 8.0f) / 2.0f, pctBuf);

    // Single status line = the most recent task, with animated dots if running.
    if (!g_flow.empty()) {
        const FlowStep& s = g_flow.back();
        const int dots = (int)((elapsed / 250) % 4);
        SDL_Color c;
        switch (s.status) {
            case FS_OK:   c = {120, 200, 120, 255}; break;
            case FS_RUN:  c = {230, 210, 120, 255}; break;
            case FS_FAIL: c = {220, 110, 110, 255}; break;
            case FS_SKIP: c = {135, 135, 145, 255}; break;
            default:      c = {170, 185, 210, 255}; break;
        }
        std::string line = s.label;
        if (s.status == FS_RUN)     line += std::string(dots, '.');
        else if (!s.detail.empty()) line += ": " + s.detail;
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
        SDL_RenderDebugText(r,
            (STORY_SCREEN_WIDTH - (int)line.size() * 8.0f) / 2.0f,
            barY + barH + 8.0f, line.c_str());
    }
    drawCorpCredit(r, elapsed, barY + barH + 26.0f);
}

// Render frames for `ms`, animating dots and pumping events. QUIT exits.
static void flowDwell(SDL_Renderer* r, Uint32 t0, Uint32 ms) {
    Uint32 s = SDL_GetTicks();
    SDL_Event e;
    do {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) std::exit(0);
        }
        flowRender(r, SDL_GetTicks() - t0);
        SDL_RenderPresent(r);
        SDL_Delay(16);
    } while (SDL_GetTicks() - s < ms);
}

// Offline confirmation: block until the player acknowledges, then proceed.
static void flowConfirmOffline(SDL_Renderer* r, Uint32 t0) {
    SDL_Event e;
    bool waiting = true;
    while (waiting) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) std::exit(0);
            if (e.type == SDL_EVENT_KEY_DOWN &&
                (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER ||
                 e.key.key == SDLK_SPACE  || e.key.key == SDLK_ESCAPE))
                waiting = false;
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) waiting = false;
        }
        flowRender(r, SDL_GetTicks() - t0);
        const char* m1 = "No internet connection.";
        const char* m2 = "Press ENTER to continue offline";
        SDL_SetRenderDrawColor(r, 230, 200, 110, 255);
        SDL_RenderDebugText(r,
            (STORY_SCREEN_WIDTH - (int)SDL_strlen(m1) * 8.0f) / 2.0f,
            STORY_SCREEN_HEIGHT - 64.0f, m1);
        SDL_RenderDebugText(r,
            (STORY_SCREEN_WIDTH - (int)SDL_strlen(m2) * 8.0f) / 2.0f,
            STORY_SCREEN_HEIGHT - 50.0f, m2);
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }
}

static bool sharedDataHasRows(sqlite3* db) {
    if (!db) return false;
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM shared_data;",
                           -1, &st, nullptr) != SQLITE_OK) return false;
    bool has = false;
    if (sqlite3_step(st) == SQLITE_ROW) has = sqlite3_column_int(st, 0) > 0;
    sqlite3_finalize(st);
    return has;
}

// "__manifest__" pseudo-chapter row in shared_meta stores the synced manifest
// version string in its sha column (reuses metaGetSha/metaSetSha).
static const char* MANIFEST_META_KEY = "__manifest__";

static void runStartupFlow(SDL_Renderer* r, sqlite3* db, bool dbFileExisted) {
    g_flow.clear();
    g_flowPct = 0.0f; g_flowPctTarget = 0.0f;
    const Uint32 t0   = SDL_GetTicks();
    const Uint32 DW   = 280;   // min dwell so a step is actually seen
    const Uint32 ANIM = 420;   // animated "running" dwell
    SDL_Log("[gameStory] startup flow begin");

    // 1. init db setup
    int s1 = flowAdd("init db setup", dbFileExisted ? FS_OK : FS_RUN,
                     dbFileExisted ? "existed" : "");
    if (!dbFileExisted) {
        flowDwell(r, t0, ANIM);            // animate while "setting up"
        flowSet(s1, FS_OK, "created");     // schema created by caller
    }
    flowProgress(0.10f);
    flowDwell(r, t0, DW);

    // 2. shared db check
    bool sharedHasRows = sharedDataHasRows(db);
    flowAdd("shared db check", FS_INFO, sharedHasRows ? "existed" : "unexisted");
    flowProgress(0.20f);
    flowDwell(r, t0, DW);

    // 3. current manifest version
    std::string curVer = db ? metaGetSha(db, MANIFEST_META_KEY) : "";
    if (curVer.empty()) curVer = "00.00";
    flowAdd("current manifest version", FS_INFO, curVer);
    flowProgress(0.30f);
    flowDwell(r, t0, DW);

    // 4. internet connection (fetch manifest)
    int s4 = flowAdd("internet connection", FS_RUN);
    flowProgress(0.42f);
    flowDwell(r, t0, ANIM);
    const char* manifestUrl = MANIFEST_GIST_URL;
    std::string manifestBody =
        (manifestUrl && manifestUrl[0]) ? httpGetSync(manifestUrl) : "";
    nlohmann::json manifest;
    bool online = false;
    if (!manifestBody.empty()) {
        try { manifest = nlohmann::json::parse(manifestBody); online = true; }
        catch (...) { online = false; }
    }
    if (!online) {
        flowSet(s4, FS_FAIL, "no connection");
        flowProgress(1.0f);
        flowConfirmOffline(r, t0);
        return;                            // proceed to console with local data
    }
    flowSet(s4, FS_OK, "passed");
    flowProgress(0.50f);
    flowDwell(r, t0, DW);

    // 5. latest manifest version
    std::string latest = manifest.value("latest", std::string());
    if (latest.empty() || !manifest.contains(latest) ||
        !manifest[latest].is_object()) {
        flowAdd("latest manifest version", FS_FAIL, "invalid manifest");
        flowProgress(1.0f);
        flowConfirmOffline(r, t0);
        return;
    }
    flowAdd("latest manifest version", FS_INFO, latest);
    flowProgress(0.58f);
    flowDwell(r, t0, DW);

    if (latest == curVer && sharedHasRows) {
        flowAdd("up to date - launching", FS_OK, "");
        flowProgress(1.0f);
        flowDwell(r, t0, 600);
        SDL_Log("[gameStory] startup flow: up to date, launching");
        return;                            // versions match -> auto proceed
    }

    // 6 + 7. recursive fetch + import per chapter. Distribute 0.58..0.95 of
    // the bar across chapters so percent climbs with each fetch/import.
    const auto& lv = manifest[latest];
    int nChapters = 0;
    for (auto it = lv.begin(); it != lv.end(); ++it) nChapters++;
    if (nChapters < 1) nChapters = 1;
    int idx = 0;
    for (auto it = lv.begin(); it != lv.end(); ++it, ++idx) {
        std::string cid     = it.key();
        std::string sha     = it.value().value("latestCommitId", std::string());
        std::string gistUrl = it.value().value("gistUrl",        std::string());
        std::string mbu     = it.value().value("mediaBaseUrl",   std::string());
        float base = 0.58f + (0.37f * idx) / nChapters;
        float next = 0.58f + (0.37f * (idx + 1)) / nChapters;
        if (cid.empty() || sha.empty() || gistUrl.empty()) continue;

        if (metaGetSha(db, cid.c_str()) == sha && sharedHasRows) {
            flowAdd(cid + " fetch", FS_SKIP, "up to date");
            flowProgress(next);
            flowDwell(r, t0, 160);
            continue;
        }
        int sf = flowAdd(cid + " fetch", FS_RUN);
        flowProgress(base + (next - base) * 0.4f);
        flowDwell(r, t0, ANIM);
        std::string body = httpGetSync(gistUrl.c_str());
        if (body.empty()) { flowSet(sf, FS_FAIL, "fetch failed");
                            flowDwell(r, t0, DW); continue; }
        flowSet(sf, FS_OK, "fetched");

        int si = flowAdd(cid + " import rows", FS_RUN);
        flowDwell(r, t0, ANIM);
        SDL_Log("[gameStory] flow import %s begin", cid.c_str());
        if (applyChapterJson(db, body, cid.c_str(), mbu.c_str())) {
            metaSetSha(db, cid.c_str(), sha.c_str(), mbu.c_str());
            flowSet(si, FS_OK, "imported");
            SDL_Log("[gameStory] flow import %s done", cid.c_str());
#ifdef __EMSCRIPTEN__
            EM_ASM({ if (Module['FS'] && FS.syncfs) FS.syncfs(false, function(){}); });
#endif
        } else {
            flowSet(si, FS_FAIL, "import error");
        }
        flowProgress(next);
        flowDwell(r, t0, DW);
    }

    // Persist current version = latest, then recheck (now matches) -> launch.
    if (db) metaSetSha(db, MANIFEST_META_KEY, latest.c_str(), "");
    flowAdd("recheck version", FS_OK, latest + " == current");
    flowProgress(1.0f);
    flowDwell(r, t0, 700);
    SDL_Log("[gameStory] startup flow complete -> launching");
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

    // Probe whether the init DB file pre-existed BEFORE storyDbOpen creates it
    // (drives the "init db setup: existed / created" step).
    bool dbFileExisted = false;
    {
        char* pref = SDL_GetPrefPath("uit", "cTetris");
        if (pref) {
            std::string probe = std::string(pref) + "default.sqlite";
            SDL_free(pref);
            SDL_IOStream* f = SDL_IOFromFile(probe.c_str(), "rb");
            if (f) { dbFileExisted = (SDL_GetIOSize(f) > 100); SDL_CloseIO(f); }
        }
    }

    // Open (or reuse) the DB for sync and post-sync check
    bool dbOpenedHere = false;
    if (!g_storyDb) {
        dbOpenedHere = storyDbOpen();
    } else {
        dbOpenedHere = true;
    }

    // --- Ensure shared_* tables exist so sync can write on first startup ---
    // gameConsole.dbInitSchema() also creates these (idempotent CREATE IF NOT EXISTS).
    // Creating them here means sync can succeed before gameConsole has ever run.
    if (g_storyDb) {
        sqlite3_exec(g_storyDb,
            "CREATE TABLE IF NOT EXISTS shared_meta ("
            "  chapter_id TEXT PRIMARY KEY, sha TEXT NOT NULL,"
            "  updated_at INTEGER, media_base_url TEXT DEFAULT '');",
            nullptr, nullptr, nullptr);
        sqlite3_exec(g_storyDb,
            "CREATE TABLE IF NOT EXISTS shared_data ("
            "  idStory INTEGER NOT NULL, storyName TEXT NOT NULL,"
            "  idChapter INTEGER NOT NULL, chapterName TEXT,"
            "  minScore INTEGER DEFAULT 0, minSpeed REAL DEFAULT 0,"
            "  minRetries INTEGER DEFAULT 0, requiredStories TEXT DEFAULT '',"
            "  nextBlockScore INTEGER DEFAULT 0, nextBlockSpeed REAL DEFAULT 0,"
            "  tableMatrix TEXT DEFAULT '', xmlDialogue TEXT DEFAULT '',"
            "  thumbnailPath TEXT DEFAULT '',"
            "  PRIMARY KEY (idStory, idChapter));",
            nullptr, nullptr, nullptr);
        sqlite3_exec(g_storyDb,
            "CREATE TABLE IF NOT EXISTS shared_dialogues ("
            "  rowId INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  idStory INTEGER NOT NULL, idChapter INTEGER NOT NULL,"
            "  nodeId INTEGER NOT NULL, speaker TEXT DEFAULT '',"
            "  text TEXT DEFAULT '', imageUrl TEXT DEFAULT '',"
            "  bgmUrl TEXT DEFAULT '', sfxUrl TEXT DEFAULT '',"
            "  nextNodeId INTEGER DEFAULT 0, hasChoices INTEGER DEFAULT 0,"
            "  UNIQUE(idStory, idChapter, nodeId));",
            nullptr, nullptr, nullptr);
        sqlite3_exec(g_storyDb,
            "CREATE TABLE IF NOT EXISTS shared_choices ("
            "  rowId INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  idStory INTEGER NOT NULL, idChapter INTEGER NOT NULL,"
            "  nodeId INTEGER NOT NULL, choiceIdx INTEGER NOT NULL,"
            "  label TEXT DEFAULT '', nextNodeId INTEGER DEFAULT 0,"
            "  UNIQUE(idStory, idChapter, nodeId, choiceIdx));",
            nullptr, nullptr, nullptr);
        // default_Stories (per-user overlay) -- gameConsole owns this table, but
        // on first launch gameStory runs BEFORE gameConsole, so create it here
        // too (idempotent). Without it, postSyncConditionCheck's JOIN query
        // fails to prepare and returns 0 -> the intro dialogue never plays.
        sqlite3_exec(g_storyDb,
            "CREATE TABLE IF NOT EXISTS default_Stories ("
            "  idUser TEXT NOT NULL, idStory INTEGER NOT NULL,"
            "  idChapter INTEGER NOT NULL, isActivated INTEGER DEFAULT 0,"
            "  isSelected INTEGER DEFAULT 0, totalRetries INTEGER DEFAULT 0,"
            "  lastMaxScore INTEGER DEFAULT 0, lastMaxSpeed REAL DEFAULT 0,"
            "  PRIMARY KEY (idUser, idStory, idChapter));",
            nullptr, nullptr, nullptr);
    }

    // --- Startup flow: deterministic step-by-step intro (replaces the old
    // timed bar + background sync thread). Runs the whole init/check/sync
    // sequence on the main thread, rendering each step so the player sees it.
    runStartupFlow(renderer, g_storyDb, dbFileExisted);
    SDL_Log("[gameStory] post-flow: running condition check");

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
    SDL_Log("[gameStory] postSyncConditionCheck -> %d", nextStory);

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
    SDL_Event event;
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