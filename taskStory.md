## Tasks:
### V1
[x] Task 1.1: viết v1 gameStory/app.cpp - tạo layout.h đảm bảo màn hình 9:16 và BUILD_STANDALONE.
[x] Task 1.2: viết v1 gameStory/app.cpp - nanosvg SVG rasterize + Caching (Lazy Init).
[x] Task 1.3: viết v1 gameStory/app.cpp - logo UIT fade-in + "C T E T R I S".
[x] Task 1.4: viết v1 gameStory/app.cpp - loading bar theo thời gian.
[x] Task 1.5: viết v1 gameStory/app.cpp - "Powered up by" corp logo credit.
[x] Task 1.6: tích hợp v1 với các modules khác trong app/src. Comment: integration/v1

### V2
[x] Task 2.1: Tổ chức lại cấu trúc thư mục — chapters/ cùng cấp với app/.
[x] Task 2.2: Thiết lập quy chuẩn chapter + CI/CD sync-chapters.yml.
[x] Task 2.3: Gist manifest sync — syncFromGist() + applyChapterJson() + SHA diff.
    - Comment: gamestory-dong-bo-sqlite-05a
[x] Task 2.4: Dialogue story engine — dialRunLoop() + drawDialoguePage().
    - Comment: gamestory-phan-cot-game-05b
[x] Task 2.5: Nút skip trong dialogue phase.
    - Comment: gamestory-nut-bo-qua-cot-truyen-06
[x] Task 2.6: Tích hợp v2. Comment: integration/v2

### V3
[x] Task 3.1: Media file download từ raw URL (cache local, skip-if-exists).
    - Comment: gamestory-tich-hop-backend-07
    - buildDownloadQueue(): JOIN shared_data + shared_dialogues với shared_meta.media_base_url.
    - tickMediaDownload(): 1 file/frame, httpGetSync() reuse, SDL_IOFromFile wb save.
    - ensureMediaCacheDir(): SDL_CreateDirectory() cross-platform (SDL3).
    - Skip nếu file đã có trong SDL_GetPrefPath()/media/ cache.
    - Offline safe: httpGetSync() trả "" → log + skip, không crash.

[x] Task 3.2: Loading bar theo download speed.
    - Comment: gamestory-hieu-chinh-loading-bar-theo-download-speed-08
    - barFill = max(syncProgress, mediaProgress, timeProgress).
    - Status line: "Media X/Y  N.N KB/s" khi đang download.
    - Logo loop không exit cho đến khi dlDone && elapsedTime > INTRO_DURATION.

[x] Task 3.3: Tích hợp v3. Comment: integration/v3 trong gameStory/app.cpp.

## Issues
### V1
[x] Issue 1.1–1.5: resolved.

### V2
[x] Issue 2.2: SHA diff sync — resolved.
[x] Issue 2.3: GitHub Action không commit — resolved.
[x] Issue 2.6: Skip button chỉ hiện trong dialogue — resolved.
[x] Issue 2.7: requiredStories unlock logic — resolved.
[x] Issue 2.8: postSyncConditionCheck() — resolved.
[x] Issue 2.9: DB update khi user xác nhận — resolved.
[x] Issue 2.12: Skip visibility — resolved.
[x] Issue 2.13: requiredStories CSV enforce — resolved.

[ ] Issue 2.1: DB device-id namespacing → DEFERRED V3 OTP flow.
[ ] Issue 2.4: Stream ảnh từ URL → DEFERRED V3.
[ ] Issue 2.5: Progressive bar granularity → partially done (V3.2 covers speed display).
[ ] Issue 2.10: DB device-id → DEFERRED V3.
[ ] Issue 2.11: In-memory media streaming → DEFERRED V3 (disk cache implemented).

### V3
[x] Issue 3.1: Media cache dir created via SDL_CreateDirectory() before first download.
[x] Issue 3.2: Binary media (PNG/audio) stored correctly via std::string byte buffer.
[x] Issue 3.3: nlohmann/json include fix — added #include <nlohmann/json.hpp> at line 20
               in gameStory/app.cpp for V3 manifest + dialogue JSON parsing.
               - Resolves error C2653: 'nlohmann' is not a class or namespace name (line 644).
               - Required for Task 3.1 (syncFromGist manifest) + Task 3.3 (dialogue JSON).
               - Build: exit code 0, manifest + media download integration complete.
[x] Issue 3.4: Emscripten WASM build fixes — added #include <emscripten.h> and
               #include <emscripten/fetch.h> at lines 21-24 in gameStory/app.cpp.
               - Resolves error: undeclared emscripten_fetch_attr_t, emscripten_fetch_t, etc.
               - Fixes EM_ASM syntax: changed Module.FS.syncfs to Module['FS'].syncfs
                 (bracket notation required in EM_ASM macros at lines 911, 1504).
               - WASM build: exit code 0, media download + IndexedDB persistence complete.

## Rules:
    - Chỉ có 1 file c++ (app/src/gameStory/app.cpp) duy nhất để viết.
    - Nội dung chapter (JSON, media) đặt trong chapters/src/c{id}/, không nằm trong app/.
    - File chapters/prompts/json.md là nguồn tham chiếu duy nhất cho cấu trúc JSON.
