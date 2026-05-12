## Tasks:
### V1
[x] Task 1.1: viết v1 các file class/struct trong gameCore/include  
    - Định nghĩa các cấu trúc dữ liệu cơ bản như Point, Tetromino, GameState.  
[x] Task 1.2: viết v1 gameCore/app.cpp - tạo layout.h và build .exe đảm bảo màn hình 9:16 trước khi viết code tiếp  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-giao-dien-169-00  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và trên 01.  
    - Cấu hình layout chi tiết: bảng game 240x480 và một sidebar 30x480 bên phải.  
[x] Task 1.3: viết v1 gameCore/app.cpp - tạo 6 khối hình học cơ bản (L, I, T, S, Z, O)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-cac-khoi-xep-hinh-LITZO-01  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 00 và trên 02.  
[x] Task 1.4: viết v1 gameCore/app.cpp - đổ màu ngẫu nhiên (Blue, Red, Green, Yellow, Orange, Purple)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-do-mau-02  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 01 và trên 03.  
[x] Task 1.5: viết v1 gameCore/app.cpp - cho khối rơi tự do ở 1 vị trí bất kỳ (on top)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-roi-03  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 02 và trên 04.  
[x] Task 1.6: viết v1 gameCore/app.cpp - điều khiển khối bằng các mũi tên và WASD  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-phim-04  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 03 và trên 05.  
    - Lật theo chiều kim đồng hồ: mũi tên (up) và phím W.  
    - Lật ngược chiều kim đồng hồ: mũi tên (down) và phím S.  
    - Di chuyển trái: mũi tên (left) và phím A.  
    - Di chuyển phải: mũi tên (right) và phím D.  
[x] Task 1.7: viết v1 gameCore/app.cpp - hiệu ứng chạm và dừng rơi khối  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-cham-05  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trên 06.  
[x] Task 1.8: viết v1 gameCore/app.cpp - hiệu ứng xoá dòng  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-xoa-dong-06  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05 và trên 07.  
[x] Task 1.9: viết v1 gameCore/app.cpp - tính điểm với quy tắc định dạng 6 chữ số  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tinh-diem-07  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trên 08.  
    - Quy tắc điểm: mỗi dòng xoá = 1 điểm. Giới hạn điểm số tối đa là 999999.  
    - Điểm được hiển thị ở định dạng chuỗi 6 chữ số (ví dụ "000005").  
[x] Task 1.10: viết v1 gameCore/app.cpp - xây dựng Sidebar UI và Procedural Icons  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-sidebar-ui-08  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trên 09.  
    - Chia sidebar 30x480 thành 12 component đồng nhất (30x40).  
    - Các icon (Quit, Pause, Arrows, Speed Boost) phải được vẽ thủ công bằng code (hình học SDL) thay vì dùng ảnh tĩnh bitmap.  
[x] Task 1.11: viết v1 gameCore/app.cpp - tương tác UI bằng chuột (Click & Hold)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-mouse-interaction-09  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 08 và trên 10.  
    - Cho phép click/giữ chuột lên các icon ở sidebar để điều khiển khối tương tự phím cứng.  
    - Nút đang được giữ (chuột hoặc phím) phải đổi sang màu vàng highlight để báo hiệu trạng thái active.  
[x] Task 1.12: viết v1 gameCore/app.cpp - tính năng rơi nhanh (Soft Drop)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-soft-drop-10  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 09 và trên 11.  
    - Sử dụng phím SPACE hoặc giữ chuột vào nút Speed Booster trên sidebar để tăng tốc độ rơi của khối.  
[x] Task 1.13: viết v1 gameCore/app.cpp - đồng hồ tính giờ chơi (Play Timer)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-play-timer-11  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 10 và trên 12.  
    - Hiển thị tổng thời gian chơi thực tế (loại trừ thời gian Pause) dưới định dạng HH:MM trên ô số 4 của sidebar.  
[x] Task 1.14: viết v1 gameCore/app.cpp - hiển thị trước 1 khối tiếp theo (Preview Next Block)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-preview-next-block-12  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 11 và trên 13.  
    - Hiển thị khối sắp rơi tiếp theo ở ô NEXT-1 trên sidebar.  
[x] Task 1.15: viết v1 gameCore/app.cpp - tính năng Pause / Play  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-pause-vs-play-13  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 12 và trên 14.  
    - Tạm dừng/tiếp tục trò chơi bằng phím Enter, phím KP_ENTER hoặc click vào icon Pause.  
    - Thay đổi icon tương ứng (Vuông: Stop/Đang chạy, Tam giác: Play/Đang dừng).  
    - Ngừng đếm thời gian chơi khi đang Pause.  
[x] Task 1.16: viết v1 gameCore/app.cpp - popup Quit / Game Over (Lightbox)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-popup-quit-14  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 13 và trên 15.  
    - Kích hoạt bằng phím ESC, click icon Nguồn, hoặc khi Game Over.  
    - Áp dụng kỹ thuật lightbox (phủ mờ nền đen) để hiển thị popup xác nhận.  
    - Cung cấp 4 nút chức năng: Restart (chơi lại), Console (về menu), Quit (thoát app), và Cancel.  
[x] Task 1.17: tích hợp v1 với các modules khác trong app/src  
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameCore/app.cpp là: integration/v1  

### V2

> **Cross-module dependency note (đọc trước khi làm):**
>
> gameConsole V2 đã implement xong các tính năng sau (verified từ source):
> - `SettingsConfig` struct với `volume`, `colorEnabled[7]`, `storyId`, `chapterId`
> - SQLite DB với 3 bảng: `idUser_Records`, `idUser_Stories`, `shared_data`
> - `shared_data` chứa `nextBlockScore`, `nextBlockSpeed`, `tableMatrix` per story
> - `dbOpen/dbClose/dbInitSchema/dbSeedSharedData/dbLoadStories/dbCheckAndUnlockStories/dbMaxActivatedChapter` đã implement
> - `dbSelectStory` đã implement (trong app.cpp, không có trong header)
>
> **Những gì gameConsole V2 CHƯA implement (cần làm trước gameCore V2):**
> - `dbInsertRecord()` — khai báo trong header nhưng THIẾU body trong app.cpp
> - `dbUpsertStoryProgress()` — khai báo trong header nhưng THIẾU body trong app.cpp
> - `SettingsConfig` THIẾU 3 fields: `nextBlockScore`, `nextBlockSpeed`, `tableMatrix`
> - gameConsole chưa populate 3 fields trên vào cfg khi user chọn story
>
> **Hiện trạng gameCore V2:** `runGameCore()` nhận `const SettingsConfig& cfg` nhưng toàn bộ là `(void)cfg` — chưa dùng gì từ cfg cả.
>
> **Thứ tự thực hiện bắt buộc:** P1 → P2 → P3 → C1 → C2 → C3 → C4 → C5 → C6 → C7 → C8 → C9 → C10

---

#### Prerequisite Tasks (sửa shared files trước — không phải gameCore/app.cpp)

[ ] Task P1: mở rộng SettingsConfig trong gameConsole_layout.h
    - File: `app/src/gameConsole/include/gameConsole_layout.h`
    - Thêm 3 fields mới vào struct:
      ```cpp
      int         nextBlockScore = 0;   // ngưỡng điểm để mở NEXT-2
      float       nextBlockSpeed = 0.0f; // ngưỡng speed để mở NEXT-2
      std::string tableMatrix    = "";   // trạng thái bàn cờ ban đầu (từ shared_data)
      ```
    - I/O validation: default values → blank board, NEXT-2/NEXT-3 không hiện.
    - Không thay đổi signature `runGameCore()` hay `runGameConsole()` — default values đảm bảo backward compat.

[ ] Task P2: implement dbInsertRecord() trong gameConsole/app.cpp
    - File: `app/src/gameConsole/app.cpp`
    - Thêm body cho hàm đã khai báo trong gameConsole_db.h:
      ```cpp
      bool dbInsertRecord(const GameRecord& rec) { ... }
      ```
    - INSERT vào bảng `idUser_Records` (schema đã có).
    - I/O validation: `SELECT COUNT(*) FROM idUser_Records` trước và sau = tăng 1.

[ ] Task P3: implement dbUpsertStoryProgress() trong gameConsole/app.cpp
    - File: `app/src/gameConsole/app.cpp`
    - Thêm body cho hàm đã khai báo trong gameConsole_db.h:
      ```cpp
      bool dbUpsertStoryProgress(const char* idUser, int idStory, int idChapter,
                                  bool isActivated, bool isSelected) { ... }
      ```
    - INSERT OR REPLACE vào bảng `idUser_Stories`.
    - I/O validation: upsert → re-query shows updated isActivated/isSelected.

[ ] Task P4: gameConsole populate cfg.nextBlockScore/nextBlockSpeed/tableMatrix khi chọn story
    - File: `app/src/gameConsole/app.cpp`
    - Trong event handler click story row (dbSelectStory call), thêm:
      ```cpp
      cfg->nextBlockScore = sr.nextBlockScore;
      cfg->nextBlockSpeed = sr.nextBlockSpeed;
      cfg->tableMatrix    = sr.tableMatrix;
      ```
    - I/O validation: chọn story 1 (nextBlockScore=50) → cfg.nextBlockScore==50 khi vào gameCore.

---

#### gameCore V2 Tasks

[ ] Task 2.1: viết v2 gameCore/app.cpp - nhạc nền trong suốt tiến trình
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-chen-nhac-nen-15
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 14 và trên 16.
    - Mở SDL_AudioStream silent stream khi vào runGameCore (giống pattern gameConsole).
    - Áp dụng `SDL_SetAudioStreamGain(stream, cfg.volume)` ngay khi entry — tôn trọng volume slider đã set ở Console.
    - Đóng stream khi exit runGameCore.
    - Depends on: P1 (để dùng cfg.volume đúng cách).
    - I/O validation: volume=0.0 → gain=0.0; volume=0.5 → gain=0.5; volume=1.0 → gain=1.0.

[ ] Task 2.2: viết v2 gameCore/app.cpp - tốc độ rơi nhanh dần theo điểm số
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tang-do-kho-16
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 15 và trên 17.
    - Thay thế hằng số `FALL_INTERVAL_NORMAL = 500` bằng hàm tính động theo state.score:
      ```
      score  0..4  → 500ms
      score  5..14 → 400ms
      score 15..29 → 300ms
      score 30..49 → 200ms
      score 50+    → 100ms
      ```
    - Soft drop interval = fallInterval / 5 (giữ tỷ lệ x5 từ v1).
    - I/O validation: score=0 → fall mỗi 500ms; score=50 → fall mỗi 100ms; soft drop luôn = fallInterval/5.

[ ] Task 2.3: viết v2 gameCore/app.cpp - dự báo 3 khối liên tiếp với góc xoay ngẫu nhiên
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-du-bao-ba-khoi-xep-hinh-lien-tiep-17
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 16 và trên 18.
    - Thêm `nextBlock2`, `nextBlock3` vào GameState (gameCore_layout.h).
    - Cập nhật `resetGame()` và `lockBlock()` để duy trì queue 3 khối.
    - **Unlock logic (dùng cfg từ P1/P4):**
        + NEXT-1: luôn hiện (v1 behavior).
        + NEXT-2: hiện khi `cfg.nextBlockScore > 0 && state.score >= cfg.nextBlockScore`.
        + NEXT-3: hiện khi NEXT-2 đã mở và `state.score >= cfg.nextBlockScore * 2` (fallback nếu story không set riêng).
        + Nếu `cfg.nextBlockScore == 0` (no story selected): chỉ NEXT-1 hiện — giữ nguyên v1.
    - Cập nhật `drawNextPreview()` để render NEXT-2/NEXT-3 slot khi unlocked.
    - Depends on: P1, P4.
    - I/O validation: story 1 (nextBlockScore=50) → NEXT-2 ẩn lúc đầu → xuất hiện khi đạt 50 điểm.

[ ] Task 2.4: viết v2 gameCore/app.cpp - hiệu ứng chớp tắt khi xoá dòng
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-hieu-ung-khi-xoa-dong-18
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 17 và trên 19.
    - Thêm flash state vào GameState: `flashRows[4]` (indices of rows to clear), `flashCount`, `flashTimer`, `pendingClear` flag.
    - Flow: phát hiện full rows → lưu vào flashRows → set flashCount=6 (3 blinks × 2 frames) → trong khi flashCount>0: render rows luân phiên trắng/tối mỗi ~80ms, block fall tạm dừng → sau khi flashCount==0: thực hiện xoá dòng thật.
    - I/O validation: xoá 1 dòng → quan sát 3 lần chớp trước khi dòng biến mất.

[ ] Task 2.5: viết v2 gameCore/app.cpp - tăng điểm thưởng khi xoá nhiều dòng cùng thời điểm
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-diem-thuong-khi-xoa-nhieu-dong-19
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 18 và trên 20.
    - Sửa hàm `clearLines()`: `state.score += linesCleared * linesCleared`.
    - Bảng điểm: 1 dòng=1pt, 2 dòng=4pt, 3 dòng=9pt, 4 dòng=16pt.
    - Giữ cap 99999 (5 chữ số từ v1 Issue 1.3).
    - Depends on: Task 2.4 (clearLines() sẽ được gọi sau flash phase).
    - I/O validation: xoá 4 dòng cùng lúc → score tăng 16 điểm.

[ ] Task 2.6: viết v2 gameCore/app.cpp - dùng cfg.colorEnabled[] để chọn màu khối
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-color-palette-cfg-20
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 19 và trên 21.
    - Sửa `spawnBlock()`: build danh sách indices enabled từ `cfg.colorEnabled[7]`, chọn ngẫu nhiên trong danh sách đó.
    - Map `colorEnabled[0..6]` → `COLORS[1..7]` (COLORS[0] = black/empty, COLORS[1..6] = game colors; palette có 7 entries nhưng COLORS[] có 7 entries indexed 0-6 → cần align đúng).
    - `spawnBlock()` cần nhận cfg hoặc dùng global/static pointer; vì cfg là const ref trong runGameCore, truyền qua parameter là sạch nhất.
    - Depends on: P1.
    - I/O validation: disable tất cả trừ red (colorEnabled[0]=true, rest=false) → mọi khối đều đỏ.

[ ] Task 2.7: viết v2 gameCore/app.cpp - tableMatrix pre-populate board
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-table-matrix-21
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 20 và trên 22.
    - Parse `cfg.tableMatrix` (format: CSV rows, e.g. "0,0,1,2,0,..." × BOARD_ROWS rows, hoặc empty string).
    - Gọi parser trong `resetGame()` trước khi spawn first block; nếu tableMatrix rỗng thì blank board như v1.
    - Format tableMatrix (cần thống nhất trong shared_data seed): `"r0c0,r0c1,...,r0c9;r1c0,...;..."` (semicolon-separated rows, comma-separated cols, values = colorID 0-6).
    - Depends on: P1, P4.
    - I/O validation: story với tableMatrix có 2 dòng đáy pre-filled → bàn cờ bắt đầu với 2 dòng đó; empty tableMatrix → blank board.

[ ] Task 2.8: viết v2 gameCore/app.cpp - ghi idUser_Records khi game over
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-save-record-22
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 21 và trên 23.
    - Include `gameConsole_db.h` và `sqlite3.h` trong gameCore/app.cpp.
    - Khi `isGameOver = true` (trong `lockBlock()`): tạo `GameRecord`, điền từ state + cfg, gọi `dbOpen("default")` → `dbInsertRecord(rec)` → `dbClose()`.
    - Tính `avgSpeed = (float)state.score / (float)max(1, totalSeconds)`.
    - Depends on: P1, P2.
    - I/O validation: game over → `SELECT COUNT(*) FROM idUser_Records` tăng 1; record có đúng score/storyId.

[ ] Task 2.9: viết v2 gameCore/app.cpp - cập nhật idUser_Stories sau game over
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-update-story-progress-23
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 22 và trên 24.
    - Sau khi `dbInsertRecord()` thành công, gọi `dbUpsertStoryProgress()` để cập nhật `lastMaxScore`, `lastMaxSpeed`, `totalRetries`.
    - Gọi `dbCheckAndUnlockStories("default")` để cascade-unlock stories mới.
    - Depends on: P2, P3, Task 2.8.
    - I/O validation: chơi story 1 đạt score >= minScore của story 2 → re-open Console Stories popup → story 2 đổi sang trạng thái active.

[ ] Task 2.10: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - Comment codeblock này trong gameCore/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1.
    - Kiểm tra SettingsConfig flow: Console chọn story → cfg populated → Core dùng đúng → back to Console.
    - Kiểm tra DB lifecycle: dbOpen/dbClose không bị double-open giữa Console và Core.
    - CMakeLists.txt: đảm bảo `sqlite3.c` + `gameConsole_db.h` include path accessible từ gameCore/app.cpp.

### V3
[ ] Task 3.1: viết v3 gameCore/app.cpp - cho phép nhập tên khi game over
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-game-over-screen-20
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 19 và trên 21.
    - Bổ sung ô nhập ký tự (text input) vào lightbox Game Over để lưu lại tên người chơi với điểm số tương ứng. Thoát game là mất data nếu không ghi.
    - Dùng `SDL_StartTextInput()` + `SDL_EVENT_TEXT_INPUT` để bắt ký tự.
    - Lưu tên vào `GameRecord.idUser` trước khi gọi `dbInsertRecord()`.
    - I/O validation: nhập "PLAYER1" → dbInsertRecord với idUser="PLAYER1".
[ ] Task 3.2: viết v3 gameCore/app.cpp - gọi API và gửi tên và điểm lên Mongo atlas
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tich-hop-backend-21
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 20 và trên 22.
    - Chỉ cần quan tâm bài toán ghi dữ liệu. Thêm nút retry hoặc resync ở màn hình game over nếu ghi không thành công.
    - Native: libcurl POST to MongoDB Atlas Data API.
    - WASM: `emscripten_fetch` async call.
    - I/O validation: network available → record appears in Atlas `board` collection; network fail → retry button shown.
[ ] Task 3.3: tích hợp v3 với các modules còn lại trong app/src qua file app/main.cpp
    - Comment codeblock này trong gameCore/app.cpp là: integration/v3
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v2

## Issues (Changes, Bugs and Usabilities)
### V1
[x] Issue 1.1: Xử lý màn hình Shutdown và Reload riêng cho môi trường web (WASM)
[x] Issue 1.2: Loại bỏ khối hình "S" và thêm cơ chế lật gương (Mirror Flip) ngẫu nhiên
[x] Issue 1.3: Giảm giới hạn hiển thị điểm số từ 6 xuống 5 chữ số
[x] Issue 1.4: Bổ sung thông tin "Total time" vào Popup Quit / Game Over
[x] Issue 1.5: Tinh chỉnh độ dày nét chữ (Thinning) và viền (Stroke) cho khối Preview
[x] Issue 1.6: Thêm cử chỉ vuốt cảm ứng (Swipe Gesture) trên vùng bảng game
### V2
[ ] Issue 2.1: `spawnBlock()` hiện dùng `std::rand() % 6 + 1` hardcode — sau Task 2.6 phải dùng enabled palette.
[ ] Issue 2.2: `FALL_INTERVAL_FAST = 500 / 5` hardcode — sau Task 2.2 phải = `getFallInterval(score) / 5`.
[ ] Issue 2.3: `dbOpen/dbClose` trong gameCore cần tránh double-open nếu Console đã mở DB (kiểm tra `g_db != nullptr` trước khi open lại).
[ ] Issue 2.4: tableMatrix format cần được chuẩn hoá trong `c001.sql` — hiện tại cột `tableMatrix` là empty string cho tất cả stories.
### V3
[ ] bổ sung sau

## Execution Order (V2)
```
P1 (SettingsConfig expand)
  → P2 (dbInsertRecord impl)
  → P3 (dbUpsertStoryProgress impl)
  → P4 (Console populate cfg fields)
  → C1=Task 2.1 (BGM stream + volume)
  → C2=Task 2.2 (dynamic fall speed)
  → C3=Task 2.3 (3-block preview + unlock)
  → C4=Task 2.4 (flash effect)
  → C5=Task 2.5 (multi-line bonus score)
  → C6=Task 2.6 (colorEnabled in spawnBlock)
  → C7=Task 2.7 (tableMatrix board init)
  → C8=Task 2.8 (write idUser_Records)
  → C9=Task 2.9 (update idUser_Stories + unlock)
  → C10=Task 2.10 (integration/v2)
```

## Rules:
    - Chỉ có 1 file c++ (app/src/gameCore/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameCore/include).
    - Cần tách 1 file layout.h (app/src/gameCore/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên window.
    - Cần tạo thành công build file từ app.cpp để chạy trên macos và wasm.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameCore thì phải để trong app/src/gameCore/gameCore_music.mp3.
    - Prerequisite tasks (P1-P4) chạm vào shared files (gameConsole_layout.h, gameConsole/app.cpp) — phải hoàn thành trước bất kỳ gameCore V2 task nào.
    - DB lifecycle: gameCore KHÔNG sở hữu DB connection. Gọi `dbOpen` → work → `dbClose` trong cùng 1 transaction block khi game over. Console tự mở lại DB của nó khi re-enter.
    - tableMatrix format (chuẩn V2): semicolon-separated rows, comma-separated colorID values. Empty string = blank board. Ví dụ: `"0,0,1,2,0,0,3,0,0,1;0,2,2,1,3,1,2,0,1,2"` = 2 dòng pre-filled.