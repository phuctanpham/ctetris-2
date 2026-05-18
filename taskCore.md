## Tasks:
### V1
[x] Task 1.1: viết v1 các file class/struct trong gameCore/include  
    - Định nghĩa các cấu trúc dữ liệu cơ bản như Point, Tetromino, GameState.  
[x] Task 1.2: viết v1 gameCore/app.cpp - tạo layout.h và build .exe đảm bảo màn hình 9:16 trước khi viết code tiếp  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-giao-dien-169-00  
[x] Task 1.3: viết v1 gameCore/app.cpp - tạo 5 khối hình học cơ bản (L, I, T, Z, O)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-cac-khoi-xep-hinh-LITZO-01  
[x] Task 1.4: viết v1 gameCore/app.cpp - đổ màu ngẫu nhiên (Blue, Red, Green, Yellow, Orange, Purple)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-do-mau-02  
[x] Task 1.5: viết v1 gameCore/app.cpp - cho khối rơi tự do ở 1 vị trí bất kỳ (on top)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-roi-03  
[x] Task 1.6: viết v1 gameCore/app.cpp - điều khiển khối bằng các mũi tên và WASD  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-phim-04  
[x] Task 1.7: viết v1 gameCore/app.cpp - hiệu ứng chạm và dừng rơi khối  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-cham-05  
[x] Task 1.8: viết v1 gameCore/app.cpp - hiệu ứng xoá dòng  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-xoa-dong-06  
[x] Task 1.9: viết v1 gameCore/app.cpp - tính điểm với quy tắc định dạng 5 chữ số  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tinh-diem-07  
[x] Task 1.10: viết v1 gameCore/app.cpp - xây dựng Sidebar UI và Procedural Icons  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-sidebar-ui-08  
[x] Task 1.11: viết v1 gameCore/app.cpp - tương tác UI bằng chuột (Click & Hold)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-mouse-interaction-09  
[x] Task 1.12: viết v1 gameCore/app.cpp - tính năng rơi nhanh (Soft Drop)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-soft-drop-10  
[x] Task 1.13: viết v1 gameCore/app.cpp - đồng hồ tính giờ chơi (Play Timer)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-play-timer-11  
[x] Task 1.14: viết v1 gameCore/app.cpp - hiển thị trước 1 khối tiếp theo (Preview Next Block)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-preview-next-block-12  
[x] Task 1.15: viết v1 gameCore/app.cpp - tính năng Pause / Play  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-pause-vs-play-13  
[x] Task 1.16: viết v1 gameCore/app.cpp - popup Quit / Game Over (Lightbox)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-popup-quit-14  
[x] Task 1.17: tích hợp v1 với các modules khác trong app/src  
    - Comment codeblock này trong gameCore/app.cpp là: integration/v1  

### V2

> **Cross-module dependency note:**
> P1–P4 đã hoàn thành — xem chi tiết bên dưới.
> Thứ tự thực hiện ban đầu: P1 → P2 → P3 → P4 → C1..C9 → C10.
> Tất cả C1–C9 đã implement xong. C10 (integration/v2 comment) pending bước A4.

---

#### Prerequisite Tasks

[x] Task P1: mở rộng SettingsConfig trong gameConsole_layout.h
    - File: `app/src/gameConsole/include/gameConsole_layout.h`
    - 3 fields đã có: nextBlockScore=0, nextBlockSpeed=0.0f, tableMatrix="".
    - Default values đảm bảo backward compat với V1.

[x] Task P2: implement dbInsertRecord() trong gameConsole/app.cpp
    - File: `app/src/gameConsole/app.cpp`
    - Body đã implement: INSERT INTO default_Records với 9 bound params.
    - dbSyncToPersistent() gọi sau INSERT thành công.

[x] Task P3: implement dbUpsertStoryProgress() trong gameConsole/app.cpp
    - File: `app/src/gameConsole/app.cpp`
    - Body đã implement: INSERT INTO default_Stories ON CONFLICT DO UPDATE.
    - dbSyncToPersistent() gọi sau upsert thành công.

[x] Task P4: gameConsole populate cfg.nextBlockScore/nextBlockSpeed/tableMatrix khi chọn story
    - File: `app/src/gameConsole/app.cpp`
    - Stories popup click handler: cfg->nextBlockScore/Speed/tableMatrix populated từ sr.*.
    - I/O: chọn story 1 (nextBlockScore=30) → cfg.nextBlockScore==30 khi vào gameCore. ✓

---

[x] Task 2.1: viết v2 gameCore/app.cpp - nhạc nền trong suốt tiến trình
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-chen-nhac-nen-15
    - coreOpenBgm(cfg.volume) mở SDL_AudioStream silent stream khi entry.
    - SDL_SetAudioStreamGain áp dụng cfg.volume ngay lập tức.
    - coreCloseBgm() đóng stream khi exit runGameCore.
    - NOTE: silent stub (không có audio file). Playback thật ở V3 Task 3.x.

[x] Task 2.2: viết v2 gameCore/app.cpp - tốc độ rơi nhanh dần theo điểm số
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tang-do-kho-16
    - getFallInterval(score): 500→400→300→200→100ms tại ngưỡng 0/5/15/30/50.
    - Soft drop = getFallInterval(score) / 5 (tỷ lệ x5 nhất quán).

[x] Task 2.3: viết v2 gameCore/app.cpp - dự báo 3 khối liên tiếp với góc xoay ngẫu nhiên
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-du-bao-ba-khoi-xep-hinh-lien-tiep-17
    - nextBlock2, nextBlock3 trong GameState + gameCore_layout.h.
    - lockBlock() + finishLineClear() duy trì queue 3 khối.
    - NEXT-2: hiện khi cfg.nextBlockScore>0 && score >= nextBlockScore.
    - NEXT-3: hiện khi NEXT-2 open && score >= nextBlockScore*2 (fallback V2).

[x] Task 2.4: viết v2 gameCore/app.cpp - hiệu ứng chớp tắt khi xoá dòng
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-hieu-ung-khi-xoa-dong-18
    - pendingClear, clearRows[4], flashTick, flashOn, flashTimer trong GameState.
    - 6 ticks × 80ms → white/dark flash → finishLineClear(). Fall blocked trong suốt phase.

[x] Task 2.5: viết v2 gameCore/app.cpp - tăng điểm thưởng khi xoá nhiều dòng cùng thời điểm
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-diem-thuong-khi-xoa-nhieu-dong-19
    - score += clearRowCount * clearRowCount (n²). Cap 99999.
    - 1→1pt, 2→4pt, 3→9pt, 4→16pt.

[x] Task 2.6: viết v2 gameCore/app.cpp - dùng cfg.colorEnabled[] để chọn màu khối
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-color-palette-cfg-20
    - spawnBlock() build enabledIds[] từ s_cfg->colorEnabled[7] + PALETTE_TO_COLOR[7].
    - Fallback: toàn bộ palette nếu không có màu nào enabled.

[x] Task 2.7: viết v2 gameCore/app.cpp - tableMatrix pre-populate board
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-table-matrix-21
    - applyTableMatrix(state, cfg.tableMatrix) trong resetGame() trước spawnBlock().
    - Format: semicolon rows, comma colorID values. Empty string = blank board.
    - Parse error per cell → silently skip (no crash).

[x] Task 2.8: viết v2 gameCore/app.cpp - ghi idUser_Records khi game over
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-save-record-22
    - onGameOver(): dbOpen("default") → dbInsertRecord(rec) → dbClose().
    - gameOverRecorded guard: fires exactly once per session.

[x] Task 2.9: viết v2 gameCore/app.cpp - cập nhật idUser_Stories sau game over
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-update-story-progress-23
    - onGameOver(): dbUpsertStoryProgress() + dbCheckAndUnlockStories() sau dbInsertRecord().
    - Cascade unlock chạy sau mỗi game-over nếu storyId > 0.

[x] Task 2.10: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - integration/v2 comment block added in gameCore/app.cpp (step A4).
    - DB lifecycle: Core opens/closes per onGameOver() only; coreOpenedDb guard in place (B3).
    - CMakeLists.txt: sqlite3.c + gameConsole_db.h include path accessible from gameCore.
    - Console tự mở lại DB khi re-enter — no double-open conflict.

### V3
[ ] Task 3.1: Tự động đối chiếu thành tích Game Over (hoặc restart / quit)
    - Comment: `gamecore-game-over-screen-24`
    - Vị trí: sau `gamecore-update-story-progress-23`.
    - Đối chiếu thành tích bảng, nếu phá vỡ kỷ lục lớn nhất lần cuối thì nhắc nhở nên vào board nhấn nút sync để đồng bộ.

[ ] Task 3.2: Tích hợp V3
    - Comment: `integration/v3` trong `gameCore/app.cpp`.
    - Sau `integration/v2`.
    - Kiểm tra flow: Game Over → tự động submit SQLite.

## Issues (Changes, Bugs and Usabilities)
### V1
[x] Issue 1.1: Xử lý màn hình Shutdown và Reload riêng cho môi trường web (WASM)
    - wasmShutdown flag + RELOAD_BTN screen. emscripten_run_script reload.
[x] Issue 1.2: Loại bỏ khối hình "S" và thêm cơ chế lật gương (Mirror Flip) ngẫu nhiên
    - 5 shapes (L,I,Z,O,T). 50% xác suất X-mirror: blocks[i].x = maxX - blocks[i].x.
[x] Issue 1.3: Giảm giới hạn hiển thị điểm số từ 6 xuống 5 chữ số
    - "%05d" format, cap 99999.
[x] Issue 1.4: Bổ sung thông tin "Total time" vào Popup Quit / Game Over
    - drawQuitPopup() hiển thị "Total time: HH:MM:SS".
[x] Issue 1.5: Tinh chỉnh độ dày nét chữ (Thinning) và viền (Stroke) cho khối Preview
    - SOFT_WHITE = {220,220,220,255}, CELL inner padding BLOCK_PAD=1.
[x] Issue 1.6: Thêm cử chỉ vuốt cảm ứng (Swipe Gesture) trên vùng bảng game
    - swipeActive, swipeStartX/Y, swipeOnBlock. Threshold 15px. Block zone vs board zone.

### V2
[x] Issue 2.1: `spawnBlock()` dùng enabled palette từ cfg.colorEnabled[].
    - enabledIds[] built từ s_cfg->colorEnabled[7] + PALETTE_TO_COLOR[7].
    - Guard: fallback toàn palette nếu tất cả false (không thể từ Console guard).

[x] Issue 2.2: `FALL_INTERVAL_FAST` hardcode → getFallInterval(score) / 5.
    - softDropInterval = getFallInterval(state.score) / 5 tính lại mỗi frame.
    - Không còn hằng số FALL_INTERVAL_FAST.

[x] Issue 2.3: `dbOpen/dbClose` trong gameCore tránh double-open với Console.
    - onGameOver() gọi dbOpen("default"). Nếu g_db != nullptr và g_dbCurrentUser=="default"
      → dbOpen() return true (reuse). Chỉ dbClose() khi Core là người open.
    - Hai sqlite3* riêng biệt (Console g_db vs Core local) không conflict.

[x] Issue 2.4: tableMatrix format chuẩn hoá — data từ c001.json thay vì dbSeedSharedData().
    - SUPERSEDED: dbSeedSharedData() là no-op skeleton (Issue 2.10 taskConsole).
    - tableMatrix data đến từ chapters/src/c001/c001.json qua Gist sync (bước C1).
    - c001.json đã có 5 stories với tableMatrix mẫu từ blank đến 8-row expert.
    - Format chuẩn trong chapters/prompts/json.md (bước C2).

[x] Issue 2.5: Cập nhật DB khi Quit, Restart và Game Over — không mất dữ liệu.
    - Game Over: onGameOver() tự động.
    - Restart: onGameOver() gọi trước resetGame(), retryCount++.
    - Quit/Console: onGameOver() gọi trước khi set exitCode + quitRequested.
    - gameOverRecorded guard tránh duplicate write.
    - WASM: EM_ASM syncfs sau mỗi write trong dbSyncToPersistent().

[x] Issue 2.6: Parse và hiển thị `tableMatrix` từ cfg — đồng bộ trạng thái bàn cờ.
    - resetGame() gọi applyTableMatrix(state, cfg.tableMatrix) trước spawnBlock().
    - Parse lỗi → log warning, blank board, không crash.

[x] Issue 2.7: Hiển thị thông tin story hiện tại trên Sidebar từ cfg.
    - drawSidebar(): s_cfg->storyId > 0 → render "S{id}-C{id}" overlay trên RECT_NEXT1.
    - storyId == 0 → ô để trống.

[x] Issue 2.8: `nextBlockScore * 2` cho NEXT-3 là fallback — TODO V3.
    - Comment TODO(V3) đã có trong code: add nextBlockScore3 to SettingsConfig.
    - V2 dùng nextBlockScore * 2 làm ngưỡng NEXT-3 khi không có field riêng.

### V3
[ ] bổ sung sau

## Rules:
    - Chỉ có 1 file c++ (app/src/gameCore/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameCore/include).
    - Cần tách 1 file layout.h (app/src/gameCore/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên window.
    - Prerequisite tasks (P1-P4) đã hoàn thành — không cần thực hiện lại.
    - DB lifecycle: gameCore KHÔNG sở hữu DB connection. Gọi dbOpen → work → dbClose trong cùng 1 transaction block khi game over. Console tự mở lại DB của nó khi re-enter.
    - tableMatrix format (chuẩn V2): semicolon-separated rows, comma-separated colorID values 0-7. Empty string = blank board.
