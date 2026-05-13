// integration/v1
// gamestory-dong-bo-sqlite-05a  [Phase C] storyDb read layer
// gamestory-phan-cot-game-05b   [Phase D] Dialogue engine + state machine
#include "gameStory_layout.h"
#include "gameStory_db.h"   // Phase C: DialogueNode, DialogueChoice, storyDb*
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

std::vector<DialogueNode> storyDbLoadDialogue(int idStory, int idChapter) {
    std::vector<DialogueNode> out;
    if (!g_storyDb) return out;

    const char* sql =
        "SELECT nodeId, speaker, text, imageUrl, bgmUrl, sfxUrl, "
        "       nextNodeId, hasChoices "
        "FROM story_dialogues "
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
        "FROM story_choices "
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

    // -------------------------------------------------------------------------
    // gamestory-logo-intro-01  +  gamestory-loading-bar-02
    // gamestory-corp-credit-03: Original intro (storyId == 0)
    // gamestory-nut-bo-qua-cot-truyen-06 [E.1+E.2]: SKIP button on intro screen
    // -------------------------------------------------------------------------
    bool running    = true;
    bool skipHoverI = false;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) std::exit(0);
            // ESC = skip intro [E.2]
            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.key == SDLK_ESCAPE) {
                running = false;
            }
            // SKIP button hover + click [E.2]
            if (event.type == SDL_EVENT_MOUSE_MOTION)
                skipHoverI = diagHit(DIAG_SKIP_BTN,
                                     event.motion.x, event.motion.y);
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT &&
                diagHit(DIAG_SKIP_BTN, event.button.x, event.button.y)) {
                running = false;
            }
        }
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime > (Uint32)INTRO_DURATION) running = false;

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        drawLogo(renderer, elapsedTime);
        float barBottomY = drawLoadingBar(renderer, elapsedTime);
        drawCorpCredit(renderer, elapsedTime, barBottomY);

        // SKIP button [E.1] — same rect as dialogue mode for visual consistency
        {
            SDL_Color skipBg = skipHoverI ? DIAG_SKIP_HOV : DIAG_SKIP_IDLE;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer,
                skipBg.r, skipBg.g, skipBg.b, skipBg.a);
            SDL_RenderFillRect(renderer, &DIAG_SKIP_BTN);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 180);
            SDL_RenderRect(renderer, &DIAG_SKIP_BTN);
            const char* sl = "[ SKIP ]";
            int sll = (int)SDL_strlen(sl);
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 200);
            SDL_RenderDebugText(renderer,
                DIAG_SKIP_BTN.x + (DIAG_SKIP_BTN.w - sll * 8.0f) / 2.0f,
                DIAG_SKIP_BTN.y + (DIAG_SKIP_BTN.h - 8.0f) / 2.0f,
                sl);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

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