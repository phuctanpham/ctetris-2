// integration/v1
#include "gameStory_layout.h"

// nanosvg single-header library: parser + rasterizer.
// Define implementation TRUOC khi include de bung phan code thuc thi.
// NANOSVG_ALL_COLOR_KEYWORDS bat ho tro toan bo ten mau theo chuan SVG.
#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

// =========================================================
// LUU Y QUAN TRONG VE 2 LOGO SU DUNG TRONG MODULE NAY:
//   - LOGO_SVG_DATA  : LOGO CUA GAME cTetris (3x3 o vuong nhieu mau,
//                      mo phong phong cach Tetris). Day la BRAND LOGO
//                      cua game, KHONG phai logo UIT.
//   - CORP_SVG_DATA  : LOGO TRUONG DAI HOC UIT (corp credit) -- hien thi
//                      kem dong "Powered up by ..." o cuoi man hinh intro.
// CAC FILE SVG VAT LY (gameStory_logo.svg, gameStory_corp.svg) DA BI XOA
// theo chien luoc moi: khong phat sinh them file hinh anh trong build.
// Noi dung SVG da duoc embed truc tiep trong 2 header file ben duoi
// duoi dang raw string literal -- compile-time, khong can I/O luc runtime.
// =========================================================
#include "gameStory_logo_svg.h"   // LOGO_SVG_DATA: logo cua GAME cTetris
#include "gameStory_corp_svg.h"   // CORP_SVG_DATA: logo UIT (truong DH)

#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstring>

// Loading bar duration: tang them 5 giay (3000 -> 8000) de hien thi cot truyen
// dai hon va cho asset/network kip load tren WASM.
const int INTRO_DURATION = 8000;

// gamestory-logo-intro-01
// Cache 2 texture (logo + corp) de chi rasterize 1 lan duy nhat
// va tai su dung cho moi frame ke tiep -> tiet kiem CPU.
struct SvgTexture {
    SDL_Texture* texture = nullptr;
    int          w       = 0;
    int          h       = 0;
};
static SvgTexture g_logo;   // Texture logo cua GAME cTetris (LOGO_SVG_DATA)
static SvgTexture g_corp;   // Texture logo UIT (CORP_SVG_DATA)

// Helper chung: rasterize 1 SVG (raw string) ra SDL_Texture co chieu rong
// = targetW pixel, giu aspect ratio cua SVG goc.
// Quy trinh: copy SVG ra buffer mutable -> nsvgParse -> rasterize ra
// pixel buffer RGBA -> tao SDL_Surface -> upload thanh SDL_Texture.
static SvgTexture createSvgTexture(SDL_Renderer* renderer,
                                   const char* svgData,
                                   int targetW) {
    SvgTexture result;

    // nanosvgParse SE ghi de len buffer (modify in-place) nen phai copy
    // data ra mot vung nho mutable, tranh sua chuoi const cua chuong trinh.
    size_t svgLen = SDL_strlen(svgData);
    char* svgCopy = (char*)SDL_malloc(svgLen + 1);
    if (!svgCopy) return result;
    SDL_memcpy(svgCopy, svgData, svgLen + 1);

    NSVGimage* image = nsvgParse(svgCopy, "px", 96.0f);
    SDL_free(svgCopy);
    if (!image || image->width <= 0 || image->height <= 0) {
        if (image) nsvgDelete(image);
        SDL_Log("Khong the parse SVG");
        return result;
    }

    // Tinh ti le scale theo chieu rong mong muon, giu aspect ratio
    float scale = (float)targetW / image->width;
    int outW = targetW;
    int outH = (int)(image->height * scale + 0.5f);

    // Cap phat buffer pixel RGBA (4 byte/pixel), khoi tao trong suot
    size_t pixelBytes = (size_t)outW * (size_t)outH * 4;
    unsigned char* pixels = (unsigned char*)SDL_malloc(pixelBytes);
    if (!pixels) { nsvgDelete(image); return result; }
    SDL_memset(pixels, 0, pixelBytes);

    // Rasterize: nanosvg ho tro fill, transform matrix, antialias.
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) { SDL_free(pixels); nsvgDelete(image); return result; }
    nsvgRasterize(rast, image, 0.0f, 0.0f, scale,
                  pixels, outW, outH, outW * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    // Tao SDL_Surface tu pixel buffer (byte order R-G-B-A khop nanosvg)
    SDL_Surface* surface = SDL_CreateSurfaceFrom(outW, outH,
                                                 SDL_PIXELFORMAT_RGBA32,
                                                 pixels, outW * 4);
    if (!surface) { SDL_free(pixels); return result; }

    // CreateTextureFromSurface COPY pixel data sang GPU memory ->
    // sau buoc nay free surface va buffer deu an toan.
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    SDL_free(pixels);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        result.texture = texture;
        result.w       = outW;
        result.h       = outH;
    } else {
        SDL_Log("Khong the tao texture tu SVG: %s", SDL_GetError());
    }
    return result;
}

// Ve LOGO CUA GAME cTetris (LOGO_SVG_DATA) voi hieu ung fade-in
// (alpha tang dan theo thoi gian intro). Day KHONG phai logo UIT --
// logo UIT la corp credit duoc ve boi drawCorpCredit() ben duoi.
static void drawLogo(SDL_Renderer* renderer, Uint32 elapsedTime) {
    // Lazy init: chi tao texture lan dau goi (sau khi renderer da san sang)
    if (!g_logo.texture) {
        g_logo = createSvgTexture(renderer, LOGO_SVG_DATA, 140);
    }
    if (!g_logo.texture) return;

    // Tinh alpha fade-in 0..255 theo tien do thoi gian
    float t = (float)elapsedTime / (float)INTRO_DURATION;
    if (t > 1.0f) t = 1.0f;
    Uint8 alpha = (Uint8)(t * 255.0f);
    SDL_SetTextureAlphaMod(g_logo.texture, alpha);

    // Canh giua man hinh, lech len de chua thanh loading + corp ben duoi
    SDL_FRect dst = {
        (STORY_SCREEN_WIDTH  - g_logo.w) / 2.0f,
        (STORY_SCREEN_HEIGHT - g_logo.h) / 2.0f - 60.0f,
        (float)g_logo.w,
        (float)g_logo.h
    };
    SDL_RenderTexture(renderer, g_logo.texture, NULL, &dst);

    // Tieu de game ngay duoi logo. Su dung mau hoi xam (220) thay vi trang
    // tinh (255) de font xuat hien "mong/nhe" hon (yeu cau giam thickness).
    // KHONG them "(C)" copyright vi SDL_RenderDebugText la bitmap ASCII font,
    // "(C)" trong ngoac don nhin xau. Copyright chinh thuc o OS window title.
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, alpha);
    const char* title = "C T E T R I S";
    int titleLen = (int)SDL_strlen(title);
    SDL_RenderDebugText(renderer,
                        (STORY_SCREEN_WIDTH - titleLen * 8.0f) / 2.0f,
                        dst.y + dst.h + 10.0f, title);
}

// gamestory-loading-bar-02
// Thanh tien trinh loading chay song song voi hieu ung fade-in cua logo.
// Tra ve toa do Y day cua bar de cac thanh phan ben duoi can chinh theo.
static float drawLoadingBar(SDL_Renderer* renderer, Uint32 elapsedTime) {
    float progress = (float)elapsedTime / INTRO_DURATION;
    if (progress > 1.0f) progress = 1.0f;
    const float barW = 180.0f, barH = 12.0f;
    const float barX = (STORY_SCREEN_WIDTH - barW) / 2.0f;
    // Day len 1 chut so voi v1 de chua dong "Powered up by..." ben duoi
    const float barY = STORY_SCREEN_HEIGHT - 110.0f;

    // Nen thanh tien trinh (mau xam toi)
    SDL_FRect bgBar = { barX, barY, barW, barH };
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &bgBar);

    // Phan tien trinh da chay (mau xanh la)
    SDL_FRect fgBar = { barX, barY, barW * progress, barH };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &fgBar);

    return barY + barH;  // y = day cua bar
}

// gamestory-corp-credit-03
// Hang chu "Powered up by" + LOGO UIT (CORP_SVG_DATA) nho ben canh,
// can giua theo chieu ngang. Y duoc xac dinh ngay duoi loading bar.
//
// LUU Y: g_corp = logo UIT (truong dai hoc), KHONG phai logo cua game.
// Logo cua game la g_logo da ve o tren.
//
// Can bang kich thuoc: SDL_RenderDebugText dung font 8x8 pixel/ky tu.
// Mau text dung 220 (xam nhat) de tao cam giac mong/nhe hon trang tinh.
static void drawCorpCredit(SDL_Renderer* renderer,
                           Uint32 elapsedTime,
                           float topY) {
    // Lazy init corp texture: render mot lan voi chieu cao tham chieu 40px
    // (do hoa chuan: ~2x chieu cao text de noi bat nhe nhung khong ap text).
    if (!g_corp.texture) {
        g_corp = createSvgTexture(renderer, CORP_SVG_DATA, 40);
    }

    // Fade-in dong bo voi cac thanh phan khac
    float t = (float)elapsedTime / (float)INTRO_DURATION;
    if (t > 1.0f) t = 1.0f;
    Uint8 alpha = (Uint8)(t * 255.0f);

    // Cau hinh layout: text scale 1.0 (font 8x8) co kich thuoc moi ky tu = 8px
    // de gon nhe; corp logo display 16px chieu cao de can voi text
    const char* prefix = "Powered up by ";
    const int   prefixLen = (int)SDL_strlen(prefix);
    const float CHAR_W = 8.0f;        // SDL_RenderDebugText pixel/ky tu
    const float CHAR_H = 8.0f;
    const float CORP_DISPLAY_H = 16.0f;
    const float SPACING = 4.0f;        // khoang cach giua text va logo

    // Tinh chieu rong corp display de can giua tong the
    float corpDisplayW = 0.0f;
    if (g_corp.texture && g_corp.h > 0) {
        // Giu aspect ratio: scale corp.w theo ti le CORP_DISPLAY_H / corp.h
        corpDisplayW = (float)g_corp.w * CORP_DISPLAY_H / (float)g_corp.h;
    }

    // Tong width = text width + spacing + corp display width
    float totalW = prefixLen * CHAR_W
                 + (corpDisplayW > 0 ? SPACING + corpDisplayW : 0);
    float startX = (STORY_SCREEN_WIDTH - totalW) / 2.0f;

    // Y can giua doc giua text va logo: text dat sao cho center y = top + half
    // cua phan tu cao nhat (corp logo). Logo dat ngay phai text.
    float lineCenterY = topY + 16.0f;  // cach loading bar 16px
    float textY = lineCenterY - CHAR_H / 2.0f;
    float corpY = lineCenterY - CORP_DISPLAY_H / 2.0f;

    // Ve text mau xam nhat (220) thay vi trang tinh (255) de tao cam giac
    // chu mong/nhe hon (yeu cau giam font thickness).
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, alpha);
    SDL_RenderDebugText(renderer, startX, textY, prefix);

    // Ve LOGO UIT ngay sau text, can theo chieu doc
    if (g_corp.texture) {
        SDL_SetTextureAlphaMod(g_corp.texture, alpha);
        SDL_FRect corpDst = {
            startX + prefixLen * CHAR_W + SPACING,
            corpY,
            corpDisplayW,
            CORP_DISPLAY_H
        };
        SDL_RenderTexture(renderer, g_corp.texture, NULL, &corpDst);
    }
}

int runGameStory(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    bool running = true;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) std::exit(0);
        }
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime > (Uint32)INTRO_DURATION) running = false;

        // Nen toi de logo va text noi bat
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        drawLogo(renderer, elapsedTime);
        float barBottomY = drawLoadingBar(renderer, elapsedTime);
        drawCorpCredit(renderer, elapsedTime, barBottomY);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Don dep ca 2 texture truoc khi roi scene (tranh leak)
    if (g_logo.texture) {
        SDL_DestroyTexture(g_logo.texture);
        g_logo.texture = nullptr;
    }
    if (g_corp.texture) {
        SDL_DestroyTexture(g_corp.texture);
        g_corp.texture = nullptr;
    }
    return 0;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    // Standalone window title: them suffix copyright (UTF-8: \xC2\xA9 = ©)
    SDL_Window* window = SDL_CreateWindow("Game Story \xC2\xA9 - Standalone",
                                          STORY_SCREEN_WIDTH, STORY_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameStory(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif