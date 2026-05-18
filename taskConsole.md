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

## Rules:
    - Chỉ có 1 file c++ (app/src/gameConsole/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameConsole/include).
    - OTP token source: 3rd-party service. Worker only validates Bearer non-empty.
    - CTETRIS_API_URL must be set in app/.env before build for API sync to work.
