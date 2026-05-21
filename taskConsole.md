## Tasks:
### V1
[x] Task 1.1–1.9: all V1 tasks complete. See original taskConsole.md for detail.

### V2
[x] Task 2.1–2.7: all V2 tasks complete. See original taskConsole.md for detail.

### V3
[x] Task 3.1: viết v3 gameConsole/app.cpp - gọi API leaderboard từ Cloudflare Worker.
    - Comment: gameconsole-tich-hop-backend-13
    - consoleHttpGet() / consoleHttpPost(): WASM (emscripten_fetch synchronous) +
      native (libcurl HAVE_LIBCURL guard) + offline graceful fallback.
    - fetchLeaderboardFromApi(): GET /leaderboard?limit=50 → DELETE + INSERT
      sync_Records atomically; reloads board + re-applies sort mode.
    - submitLastRecord(token): POST /record with Bearer token (3rd-party OTP).
    - SYNC BOARD + SUBMIT SCORE buttons added to Settings popup body (y+196, y+224).
    - apiSyncStatus field in AppState: shows feedback below buttons.
    - CTETRIS_API_URL read from app/.env at cmake configure time.

[x] Task 3.2: sort theo time trong popup board.
    - Comment: gameconsole-sort-by-time-in-board-14
    - Implemented in V2 [G]: BOARD_SORT_TIME_DESC / BOARD_SORT_TIME_ASC.
    - TIME column header button toggles asc/desc; active header highlighted.

[x] Task 3.3: sort theo score trong popup board.
    - Comment: gameconsole-sort-by-score-in-board-15
    - Implemented in V2 [H]: BOARD_SORT_SCORE_DESC / BOARD_SORT_SCORE_ASC.
    - SCORE column header button toggles asc/desc; default SCORE_DESC on open.

[x] Task 3.4: tích hợp v3.
    - Comment: integration/v3 trong gameConsole/app.cpp.
    - fetchLeaderboardFromApi() + submitLastRecord() wired to Settings popup clicks.
    - Board reloads + re-sorts immediately after successful API sync.

## Issues (Changes, Bugs and Usabilities)
### V1
[x] Issue 1.1–1.4: resolved.

### V2
[x] Issue 2.1–2.10: resolved (see original).
[ ] Issue 2.11: Board data source migrate → RESOLVED in V3 Task 3.1 (SYNC BOARD button).
[ ] Issue 2.12: Stories thumbnails async fetch → DEFERRED; placeholder box remains.

### V3
[x] Issue 3.1: CTETRIS_API_URL injected via cmake from app/.env — consistent with
               MANIFEST_GIST_URL pattern. Worker URL: ctetris-api.*.workers.dev.
[x] Issue 3.2: SQL injection guard — submitLastRecord() uses SDL_snprintf with
               hardcoded "default" idUser; fetchLeaderboardFromApi() uses
               prepared statement bind for all API-sourced values.
[x] Issue 3.3: SYNC BOARD button disabled label when CTETRIS_API_URL empty —
               shows "SYNC (no API URL)" in muted colour.
[x] Issue 3.4: CTETRIS_API_URL preprocessor scope fix — moved #ifndef CTETRIS_API_URL
               block outside #ifdef __EMSCRIPTEN__ to global scope (line 26-29).
               - Was nested inside platform guard, unavailable on native Windows build.
               - Now accessible for fetchLeaderboardFromApi() + consoleHttpGet() calls.
               - Build: exit code 0, leaderboard API integration complete.
[x] Issue 3.5: emscripten/fetch.h include fix — added #include <emscripten/fetch.h> at line 33
               in gameConsole/app.cpp inside #ifdef __EMSCRIPTEN__ block.
               - Resolves error: undeclared emscripten_fetch_attr_t, emscripten_fetch_t, etc.
               - Required for WASM consoleHttpGet() and consoleHttpPost() implementation.
               - WASM build: exit code 0, HTTP API calls functional.
[x] Issue 3.6: libcurl Windows build (build.ps1 Initialize-Curl) — auto-installs libcurl
               (Schannel backend, no OpenSSL dep) to libs/windows/downloads/curl-native/
               with CURLConfig.cmake + DLL, passed via CMAKE_PREFIX_PATH to CMake.
               - New $CurlVersion pin: 8.10.1
               - Initialize-Curl called from Build-Native after Initialize-Sqlite
               - libcurl DLL POST_BUILD copied next to cTetris.exe (parity with SDL3.dll)
               - CMakeLists.txt: find_package(CURL QUIET CONFIG) + fallback to FindCURL module
               - Windows download only; WASM untouched (uses emscripten_fetch)
               - Build: exit code 0, curl-native installed + linked.
[x] Issue 3.7: libcurl macOS/Linux install (build.sh ensure_curl) — detection first via
               curl-config or pkg-config, then auto-install via brew/apt/dnf/pacman/zypper.
               - No-op when already detected (CLI tool or pkg-config)
               - Returns 0 even if install skipped (CMakeLists.txt warns + offline fallback)
               - macOS: brew install curl; Ubuntu: apt-get install libcurl4-openssl-dev, etc.
               - build_native calls ensure_curl after ensure_sqlite
               - Detects & appends macOS Homebrew keg-only paths (/opt/homebrew/opt/curl)
                 to CMAKE_PREFIX_PATH so find_package(CURL) finds it without env vars
               - Build: exit code 0, curl system/brew package ready.
[x] Issue 3.8: Remove chapters/ + gameConsole_board.json from WASM --preload-file.
               - Both now remote-sourced at runtime: leaderboard via D1 API, chapters via
                 Gist manifest + mediaBaseUrl per chapter.
               - CMakeLists.txt: dropped "SHELL:--preload-file ..." flags from WASM link options
               - WASM binary size reduced; offline fallback still works (API timeouts -> SDK log + skip)
               - Build: exit code 0, WASM HTML/JS/WASM size optimized.
[x] Issue 3.9: Remove native POST_BUILD copy of gameConsole_board.json + chapters/.
               - Both now remote-sourced at runtime; local offline copy no longer needed.
               - CMakeLists.txt: dropped two add_custom_command POST_BUILD copy directives
               - Leaderboard seed via sync_Records DB init (FALLBACK_BOARD_ROWS) still works
               - Build: exit code 0, native executable dir lighter (no chapters/ blob).
[x] Issue 3.10: Remove dead loadBoardFromJson() from gameConsole/app.cpp.
                - Never called after V3 API integration; replaced by fetchLeaderboardFromApi()
                  pulling from D1 into sync_Records, then loadBoardWithFallback() reads DB.
                - FALLBACK_BOARD_ROWS + dbLoadRecords() still seed/fallback (offline safe).
                - Removed ~50 lines of JSON file I/O + nlohmann parsing (only static fallback kept).
                - Build: exit code 0, compile faster, cleaner code.
[x] Issue 3.11: Consolidate duplicate CURL find_package blocks in CMakeLists.txt.
                - Dropped first CURL block (inside "if(NOT BUILD_WASM)" guard at top).
                - Kept consolidated CURL block at bottom with CONFIG mode + module fallback.
                - Added Windows DLL POST_BUILD copy (mirrors SDL3.dll pattern).
                - Updated error message to recommend: Windows: .\build.ps1 native (auto-installs).
                - Build: exit code 0, CURL integration simplified + robust.

## Rules:
    - Chỉ có 1 file c++ (app/src/gameConsole/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameConsole/include).
    - OTP token source: 3rd-party service. Worker only validates Bearer non-empty.
    - CTETRIS_API_URL must be set in app/.env before build for API sync to work.
