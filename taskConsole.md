## Tasks:
### V1
[x] Task 1.1: viết v1 gameConsole/app.cpp - tạo layout.h đảm bảo tỷ lệ màn hình 9:16 và thiết lập chế độ BUILD_STANDALONE để test độc lập module trên hệ điều hành.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tao-giao-dien-169-00  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và trên 01.  
[x] Task 1.2: viết v1 gameConsole/app.cpp - vẽ giao diện nền trơn cơ bản (sử dụng màu sắc và chữ ASCII) tạm thay thế cho ảnh tĩnh.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-giao-dien-nen-01  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 00 và trên 02.  
    - Đặt vị trí phù hợp để làm nền tảng hiển thị các nút và popup.  
[x] Task 1.3: viết v1 gameConsole/app.cpp - xây dựng hệ thống Scrollbar tương tác toàn diện (Interactive Scrollbar).  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-interactive-scrollbar-02  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 01 và trên 03.  
[x] Task 1.4: viết v1 gameConsole/app.cpp - tạo thuật toán tự động ngắt dòng văn bản (Word Wrapping).  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-word-wrapping-03  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 02 và trên 04.  
[x] Task 1.5: viết v1 gameConsole/app.cpp - tạo nút guide, hiển thị popup hướng dẫn và nút close.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nut-guide-04  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 03 và trên 05.  
[x] Task 1.6: viết v1 gameConsole/app.cpp - tạo nút board và hiển thị popup leaderboard bằng dữ liệu cứng (hardcoded array).  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nut-board-05  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trên 06.  
[x] Task 1.7: viết v1 gameConsole/app.cpp - chèn nhạc nền trong quá trình ở màn hình game console.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-chen-nhac-06  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05 và trên 07.  
[x] Task 1.8: viết v1 gameConsole/app.cpp - điều hướng nút bằng bàn phím, con lăn chuột (mouse wheel) và bổ sung nút quit.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-dieu-huong-ban-phim-07  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trên 08.  
[x] Task 1.9: tích hợp v1 với các modules khác trong app/src  
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsole/app.cpp là: integration/v1  

### V2
[x] Task 2.1: viết v2 gameConsole/app.cpp - chèn 1 hình ảnh làm background đẹp và tự full-fit theo tỷ lệ phóng lớn thu nhỏ.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-chen-backgound-08
    - SVG background lazy-rasterize qua nanosvg, reset texture khi exit để re-entry rebuild.
    - GAMECONSOLE_BG_SVG_DATA embed compile-time trong gameConsole_bg_svg.h.

[x] Task 2.2: viết v2 gameConsole/app.cpp - sử dụng file JSON để trình diễn cấu trúc thông tin thay vì mảng C++ cứng.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-doc-du-lieu-json-09
    - SUPERSEDED bởi Issue 2.7: Board popup đọc từ sync_Records (SQLite Group 3) thay vì JSON file.
    - loadBoardWithFallback() + dbLoadRecords() đã implement, seed 30 rows từ FALLBACK_BOARD_ROWS.
    - gameConsole_board.json không còn là data source chính.

[x] Task 2.3: viết v2 gameConsole/app.cpp - tạo nút setting và nút close popup setting.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nut-setting-10
    - Settings popup với volume slider + 7 color swatches.
    - dbSaveSettings() gọi khi đóng popup (X button và ESC).

[x] Task 2.4: viết v2 gameConsole/app.cpp - điều chỉnh âm lượng.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-dieu-chinh-am-luong-11
    - SDL_AudioStream + SDL_SetAudioStreamGain, drag/click track, LEFT/RIGHT ±5%.
    - cfg.volume persist qua dbSaveSettings/dbLoadSettings.

[x] Task 2.5: viết v2 gameConsole/app.cpp - tuỳ chỉnh màu cho các khối xếp hình.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tuy-chinh-mau-cho-cac-khoi-xep-hinh-12
    - 7 color swatches toggle multi-select, guard tối thiểu 1 màu (countEnabled guard).
    - cfg.colorEnabled[7] truyền sang gameCore qua SettingsConfig.

[x] Task 2.6: viết v2 gameConsole/app.cpp - hiển thị background nền ứng nội dung tuyến truyện + popup Stories.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nen-nhan-vat-12
    - Stories popup: thumbnail placeholder, 10 story rows, chapter navigator, 3-state rows.
    - SQLite schema 9 bảng (3 user + 4 shared + 1 sync + meta) đã implement.
    - dbSelectStory() + cfg.storyId/chapterId/nextBlockScore/nextBlockSpeed/tableMatrix populated.
    - BUG PENDING (Issue 2.4): thumbnail fade + "Best: N" label → fix ở bước D1.

[x] Task 2.7: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - integration/v2 comment block added in gameConsole/app.cpp (step A4).
    - DB lifecycle verified: Console closes DB before returning, Core opens/closes per onGameOver().
    - SettingsConfig flow: Console populates storyId/chapterId/nextBlockScore/tableMatrix → Core reads.

### V3
[ ] Task 3.1: viết v3 gameConsole/app.cpp - gọi API lấy data leaderboard từ Cloudflare (D1 + Durable Objects).
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tich-hop-backend-13
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 12 và trên 14.
    - Nút "load"/"save" trong Settings popup với OTP/email authentication.
    - Cloudflare D1 + Durable Objects: collections user, board, story.

[ ] Task 3.2: viết v3 gameConsole/app.cpp - tạo sort theo time trong popup board.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-sort-by-time-in-board-14
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 13 và trên 15.

[ ] Task 3.3: viết v3 gameConsole/app.cpp - tạo sort theo score trong popup board.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-sort-by-score-in-board-15
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 14 và trên 16.

[ ] Task 3.4: tích hợp v3 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsole/app.cpp là: integration/v3
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v2

## Issues (Changes, Bugs and Usabilities)
### V1
[x] Issue 1.1: Tinh chỉnh màu chữ để làm "mỏng" nét font (Thinning Effect).
    - SOFT_WHITE = {220, 220, 220, 255} đã định nghĩa và dùng xuyên suốt.
[x] Issue 1.2: Bổ sung ký tự Copyright (©) trên thanh tiêu đề của hệ điều hành.
    - UTF-8 \xC2\xA9 trong SDL_CreateWindow title.
[x] Issue 1.3: Cập nhật nội dung hướng dẫn (Guide) - Thay đổi hệ số Speed Boost.
    - Guide text: "SPACE : speed boost x5".
[x] Issue 1.4: Đồng bộ màu Highlight (HIGHLIGHT_Y) cho các trạng thái tương tác.
    - HIGHLIGHT_Y = {255, 215, 0, 255} dùng cho focus border + scrollbar thumb dragging.

### V2
[x] Issue 2.1: `runGameConsole()` entry kiểm tra DB tồn tại — return 3 nếu thiếu.
    - Probe SDL_GetPrefPath + "default.sqlite" size > 100 bytes.
    - DB không tồn tại → return 3 → main.cpp gọi runGameStory(0,0) để init.

[x] Issue 2.2: Load `default_Settings` từ DB khi khởi động.
    - dbLoadSettings(cfgInOut) sau dbInitSchema() + dbSeedSharedData().
    - applyVolumeToStream() re-apply sau load.

[x] Issue 2.3: Save `default_Settings` khi đóng Settings popup.
    - dbSaveSettings(*state.cfg) khi X click và ESC trong Settings popup.
    - WASM: dbSyncToPersistent() → idbfs_save_to_idb() sau mỗi save.

[x] Issue 2.4: Stories popup — thumbnail fade theo story hover; "Best: N" label.
    - Fixed in D1: "B:N" label (e.g. B:8540) rendered right of name for COMPLETED rows.
    - Name cap reduced 24→16 chars for COMPLETED rows to guarantee 16px clear gap before label.
    - Colour: muted green {150,220,150,255} matching COMPLETED row tint.
    - Thumbnail fade (150ms) → DEFERRED TO V3 Issue 2.12 (needs async fetch).

[x] Issue 2.5: Nút Play trong Stories popup — update `isSelected` DB rồi chuyển sang gameStory.
    - dbSelectStory() SET isSelected=1 story mới, isSelected=0 story cũ.
    - cfg.storyId/chapterId/nextBlockScore/nextBlockSpeed/tableMatrix populated.
    - Return 2 từ runGameConsole() → main.cpp gọi runGameStory(cfg.storyId, cfg.chapterId).

[x] Issue 2.6: Tự động đồng bộ `isSelected` từ gameStory khi re-enter Console.
    - Mỗi lần vào runGameConsole(): query default_Stories WHERE isSelected=1 → cập nhật cfg.
    - Popup Stories luôn phản chiếu DB.

[x] Issue 2.7: Board popup đọc từ `sync_Records` (Group 3) thay vì JSON file.
    - dbLoadRecords() query sync_Records ORDER BY totalScore DESC LIMIT 50.
    - Fallback: FALLBACK_BOARD_ROWS[] constants nếu sync_Records rỗng.
    - sync_Records seed 30 fake rows từ FALLBACK_BOARD_ROWS khi dbInitSchema().

[x] Issue 2.8: Chuẩn hoá tên bảng DB thành 3 nhóm.
    - userTable(suffix) helper: g_dbCurrentUser + "_" + suffix.
    - Group 1 default_*: Records, Stories, Settings.
    - Group 2 shared_*: data, dialogues, choices, meta.
    - Group 3 sync_*: Records.

[x] Issue 2.9: `dbInitSchema()` tạo 9 bảng; `sync_Records` seed fake data.
    - Tất cả CREATE TABLE IF NOT EXISTS — idempotent.
    - sync_Records seeded 30 rows nếu COUNT(*) == 0.
    - Log: "schema initialized (9 tables: 3 user + 4 shared + 1 sync + meta)".

[x] Issue 2.10: `dbSeedSharedData()` trở thành no-op skeleton.
    - Không đọc file JSON/SQL từ disk.
    - shared_data có rows → return true (sync đã chạy từ gameStory).
    - shared_data rỗng → return false → runGameConsole() return 3 → gameStory sync.

[ ] Issue 2.11: Board data source (migrate from JSON into runtime sync via CTETRIS_BOARD_URL).
    - → DEFERRED TO V3: Cần thêm env var build-time + HTTP fetch layer + checksum logic.
    - V2: Board dùng sync_Records (seeded) đã đủ cho gameplay.

[ ] Issue 2.12: Stories thumbnails — async fetch, cache and hover-fade.
    - → DEFERRED TO V3: Cần libcurl/emscripten_fetch + SDL_Texture async decode.
    - V2: placeholder grey box khi không có media.
    - "Best: N" label (non-async part) → fix ở bước D1.

### V3
[ ] bổ sung sau

## Rules:
    - Chỉ có 1 file c++ (app/src/gameConsole/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameConsole/include).
    - Cần tách 1 file layout.h (app/src/gameConsole/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên window.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục.
