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

// Du lieu SVG cua logo UIT (raw string literal)
#include "gameStory_logo_svg.h"

#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstring>

const int INTRO_DURATION = 3000;

// gamestory-logo-intro-01
// Cache texture logo de chi rasterize 1 lan duy nhat (frame dau tien)
// va tai su dung cho moi frame ke tiep -> tiet kiem CPU.
static SDL_Texture* g_logoTexture = nullptr;
static int          g_logoW       = 0;
static int          g_logoH       = 0;

// Rasterize SVG ra texture co chieu rong = targetW (pixel), giu nguyen ti le.
// Quy trinh: parse SVG -> rasterize ra buffer RGBA -> tao SDL_Surface
// -> chuyen thanh SDL_Texture roi giai phong buffer trung gian.
static SDL_Texture* createLogoTexture(SDL_Renderer* renderer, int targetW) {
    // nanosvgParse SE ghi de len buffer (modify in-place) nen phai copy data ra
    // mot vung nho mutable, tranh sua chuoi const cua chuong trinh.
    size_t svgLen = SDL_strlen(LOGO_SVG_DATA);
    char* svgCopy = (char*)SDL_malloc(svgLen + 1);
    if (!svgCopy) return nullptr;
    SDL_memcpy(svgCopy, LOGO_SVG_DATA, svgLen + 1);

    NSVGimage* image = nsvgParse(svgCopy, "px", 96.0f);
    SDL_free(svgCopy);
    if (!image || image->width <= 0 || image->height <= 0) {
        if (image) nsvgDelete(image);
        SDL_Log("Khong the parse SVG logo");
        return nullptr;
    }

    // Tinh ti le scale theo chieu rong mong muon, giu aspect ratio
    float scale = (float)targetW / image->width;
    int outW = targetW;
    int outH = (int)(image->height * scale + 0.5f);

    // Cap phat buffer pixel RGBA (4 byte/pixel)
    size_t pixelBytes = (size_t)outW * (size_t)outH * 4;
    unsigned char* pixels = (unsigned char*)SDL_malloc(pixelBytes);
    if (!pixels) { nsvgDelete(image); return nullptr; }
    SDL_memset(pixels, 0, pixelBytes); // nen trong suot

    // Rasterize: nanosvg ho tro fill, transform matrix, antialias - ket qua
    // dep, sat voi SVG goc (ke ca cac path Bezier phuc tap cua shield UIT).
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) { SDL_free(pixels); nsvgDelete(image); return nullptr; }
    nsvgRasterize(rast, image, 0.0f, 0.0f, scale,
                  pixels, outW, outH, outW * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    // Tao SDL_Surface tu pixel buffer (byte order R-G-B-A khop nanosvg output)
    SDL_Surface* surface = SDL_CreateSurfaceFrom(outW, outH,
                                                 SDL_PIXELFORMAT_RGBA32,
                                                 pixels, outW * 4);
    if (!surface) { SDL_free(pixels); return nullptr; }

    // CreateTextureFromSurface se COPY pixel data sang GPU memory,
    // nen sau buoc nay free buffer va surface deu an toan.
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    SDL_free(pixels);

    if (texture) {
        // Bat blend mode de hieu ung fade-in (alpha mod) hoat dong
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        g_logoW = outW;
        g_logoH = outH;
    } else {
        SDL_Log("Khong the tao texture tu logo SVG: %s", SDL_GetError());
    }
    return texture;
}

// Ve logo voi hieu ung fade-in (alpha tang dan theo thoi gian intro)
static void drawLogo(SDL_Renderer* renderer, Uint32 elapsedTime) {
    // Lazy init: chi tao texture lan dau goi (sau khi renderer da san sang)
    if (!g_logoTexture) {
        g_logoTexture = createLogoTexture(renderer, 140);
    }
    if (!g_logoTexture) return; // an toan: bo qua neu loi

    // Tinh alpha fade-in 0..255 theo tien do thoi gian
    float t = (float)elapsedTime / (float)INTRO_DURATION;
    if (t > 1.0f) t = 1.0f;
    Uint8 alpha = (Uint8)(t * 255.0f);
    SDL_SetTextureAlphaMod(g_logoTexture, alpha);

    // Canh giua man hinh, hoi lech len tren de chua thanh loading ben duoi
    SDL_FRect dst = {
        (STORY_SCREEN_WIDTH  - g_logoW) / 2.0f,
        (STORY_SCREEN_HEIGHT - g_logoH) / 2.0f - 40.0f,
        (float)g_logoW,
        (float)g_logoH
    };
    SDL_RenderTexture(renderer, g_logoTexture, NULL, &dst);

    // Tieu de game ngay duoi logo
    SDL_SetRenderDrawColor(renderer, 230, 230, 230, alpha);
    const char* title = "C T E T R I S";
    int titleLen = (int)SDL_strlen(title);
    SDL_RenderDebugText(renderer,
                        (STORY_SCREEN_WIDTH - titleLen * 8.0f) / 2.0f,
                        dst.y + dst.h + 10.0f, title);
}

// gamestory-loading-bar-02
// Thanh tien trinh loading chay song song voi hieu ung fade-in cua logo.
static void drawLoadingBar(SDL_Renderer* renderer, Uint32 elapsedTime) {
    float progress = (float)elapsedTime / INTRO_DURATION;
    if (progress > 1.0f) progress = 1.0f;
    const float barW = 180.0f, barH = 12.0f;
    const float barX = (STORY_SCREEN_WIDTH - barW) / 2.0f;
    const float barY = STORY_SCREEN_HEIGHT - 100.0f;

    // Nen thanh tien trinh (mau xam toi)
    SDL_FRect bgBar = { barX, barY, barW, barH };
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &bgBar);

    // Phan tien trinh da chay (mau xanh la)
    SDL_FRect fgBar = { barX, barY, barW * progress, barH };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &fgBar);
}

int runGameStory(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    bool running = true;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        // Xu ly su kien thoat
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) std::exit(0);
        }
        // Het thoi gian intro -> chuyen tiep sang gameConsole (return)
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        if (elapsedTime > (Uint32)INTRO_DURATION) running = false;

        // Nen toi de logo xanh duong noi bat
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        drawLogo(renderer, elapsedTime);
        drawLoadingBar(renderer, elapsedTime);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Don dep texture truoc khi roi scene (tranh leak)
    if (g_logoTexture) {
        SDL_DestroyTexture(g_logoTexture);
        g_logoTexture = nullptr;
    }
    return 0;
}

#ifdef BUILD_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game Story - Standalone",
                                          STORY_SCREEN_WIDTH, STORY_SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    runGameStory(window, renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif