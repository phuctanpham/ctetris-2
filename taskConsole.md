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
    - Mô tả: Hệ thống hỗ trợ người chơi kéo thả (drag) thanh trượt (thumb), click vào track để nhảy cóc đến vị trí, click mũi tên lên/xuống và giữ chuột để cuộn liên tục (auto-repeat).  
[x] Task 1.4: viết v1 gameConsole/app.cpp - tạo thuật toán tự động ngắt dòng văn bản (Word Wrapping).  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-word-wrapping-03  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 02 và trên 04.  
    - Mô tả: Hàm helper tự động tính toán, ngắt từ và đưa xuống dòng dựa trên số ký tự tối đa để text hiển thị vừa vặn vào khung hẹp mà không bị cắt ngang từ.  
[x] Task 1.5: viết v1 gameConsole/app.cpp - tạo nút guide, hiển thị popup hướng dẫn và nút close.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nut-guide-04  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 03 và trên 05.  
    - Mô tả: Áp dụng kỹ thuật lightbox (phủ mờ nền sau). Tích hợp thuật toán Word Wrapping và Scrollbar cho nội dung hướng dẫn (di chuyển, xoay khối, phím chức năng...).  
[x] Task 1.6: viết v1 gameConsole/app.cpp - tạo nút board và hiển thị popup leaderboard bằng dữ liệu cứng (hardcoded array).  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nut-board-05  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trên 06.  
    - Mô tả: Tái sử dụng kỹ thuật lightbox và Scrollbar. Tạm thời sử dụng mảng dữ liệu C++ (`BOARD_DATA`) gồm thông tin người dùng, điểm, thời gian để hiển thị danh sách (không dùng file json lúc này).  
[x] Task 1.7: viết v1 gameConsole/app.cpp - chèn nhạc nền trong quá trình ở màn hình game console.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-chen-nhac-06  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05 và trên 07.  
    - Mô tả: Tối ưu kỹ thuật chèn media (hiện tại gọi stub log "Phat nhac nen console") để tái sử dụng lại cho ở các version sau.  
[x] Task 1.8: viết v1 gameConsole/app.cpp - điều hướng nút bằng bàn phím, con lăn chuột (mouse wheel) và bổ sung nút quit.  
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-dieu-huong-ban-phim-07  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trên 08.  
    - Bắt sự kiện cuộn con lăn chuột (mouse wheel) để điều khiển thanh trượt.  
    - Cho phép thêm các phương thức điều khiển bằng bàn phím song song với click chuột:  
        + Phím tab, phím left (A) và phím down (S) để dịch chuyển focus tiến tới giữa các nút.  
        + Phím right (D) và phím up (W) để dịch chuyển focus lùi giữa các nút.  
        + Phím enter hoặc phím space để kích hoạt nút đang được focus.  
        + Phím esc để đóng lightbox guide/board; khi ở màn hình chính nhảy focus về nút quit.  
        + Trong lightbox board/guide, các phím W/S đồng thời cũng dùng để scroll danh sách như UP/DOWN.  
    - Trạng thái nút đang focus phải được hiển thị rõ ràng (ví dụ khung viền vàng) để người chơi nhận biết.  
[x] Task 1.9: tích hợp v1 với các modules khác trong app/src  
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsole/app.cpp là: integration/v1  
    - Khởi động chương trình luôn bắt đầu bằng màn hình của module gameStory.  
    - Sau khi kết thúc màn hình gameStory sẽ chuyển tiếp qua gameConsole.  
    - Sau khi bấm nút start bên màn hình gameConsole chuyển tiếp qua màn hình gameCore. Ngược lại, từ màn hình gameCore vẫn có thể về lại màn hình gameConsole qua cơ chế bên trong của nút có biểu tượng poweroff.  
    - Đưa về 1 file CMakeLists.txt để hỗ trợ progressive web application và desktop application.  
    - Viết và kiểm tra build.sh (macOS/Linux) và build.ps1 (Windows) để tự động xác định thư viện phù hợp với hệ điều hành tạo ứng dụng, validate môi trường, kiểm tra version các công cụ (cmake, emsdk, SDL3, git, python, curl), và hướng dẫn cài đặt nếu thiếu.  
    - Chuyển toàn bộ code từ SFML sang SDL3 để đảm bảo chạy đa nền tảng (desktop, web/wasm).  
    - Viết deploy-pages.yml để dùng GitHub Actions CI/CD build và deploy bản WASM lên GitHub Pages, kiểm tra cache, validate log, và cảnh báo nếu thiếu dependency.  
    - Biên dịch từ C++ qua wasm để chạy trên môi trường đám mây, đảm bảo các asset (favicon, manifest, sw.js) được copy đúng vào build output.  
    - Thống nhất bộ nhận diện thương hiệu (logo, icon, màu sắc, tên game) cho toàn bộ ứng dụng và cho phép tuỳ chỉnh cho từng nền tảng nếu cần.  
    - Đảm bảo các file header SVG, layout, asset cần thiết đã được commit sẵn, không sinh động trong script build.  
    - Kiểm tra lại flow chuyển đổi giữa các màn hình, đảm bảo không bị deadlock hoặc lỗi logic khi chuyển cảnh.  
    - Viết hướng dẫn sử dụng/tích hợp cho developer mới (README hoặc comment chi tiết trong code).

### V2
[x] Task 2.1: viết v2 gameConsole/app.cpp - chèn 1 hình ảnh làm background đẹp và tự full-fit theo tỷ lệ phóng lớn thu nhỏ.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-chen-backgound-08
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trên 09.
    - Kỹ thuật chèn background và resize đảm bảo màn hình kéo giãn luôn theo 9:16 để không bể điểm ảnh. Áp dụng chung cho các app/src/game* khác.
[x] Task 2.2: viết v2 gameConsole/app.cpp - sử dụng file JSON để trình diễn cấu trúc thông tin thay vì mảng C++ cứng. (Dời từ v1)
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-doc-du-lieu-json-09
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 08 và trên 10.
    - Lấy dữ liệu từ file `gameConsole/gameConsole_board.json`. Tìm hướng đặt file json hợp lý để sau này chuyển đổi sang gọi API lấy về lưu trữ tạm.
[x] Task 2.3: viết v2 gameConsole/app.cpp - tạo nút setting và nút close popup setting.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nut-setting-10
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 09 và trên 11.
    - Nơi chứa các cấu hình khác cho setting.
[x] Task 2.4: viết v2 gameConsole/app.cpp - điều chỉnh âm lượng.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-dieu-chinh-am-luong-11
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 10 và trên 12.
    - Chú ý thông số vì đây có thể sẽ là input đầu cho cấu hình về âm lượng ở gameCore.
[x] Task 2.5: viết v2 gameConsole/app.cpp - tuỳ chỉnh màu cho các khối xếp hình.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tuy-chinh-mau-cho-cac-khoi-xep-hinh-12
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 11 và trên 13.
    - Cho phép click multi choice (red, orange, pink, yellow, green, blue, purple) làm thông số cấu hình đầu vào cho gameCore.
[x] Task 2.6: viết v2 gameConsole/app.cpp - hiển thị background nền ứng nội dung tuyến truyện
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nen-nhan-vat-12
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 12 và trên 14.
    - Dựa vào sự lựa chọn tuyến truyện ở gameStory để hiện hình nền tương ứng.
    - Xây dựng thêm nút "Stories", khi bấm vào sẽ hiển thị popup với các component như mô tả bên dưới:
        - Nút Đóng (X): Nằm ở góc trên cùng bên phải, chiếm tỷ lệ kích thước rất nhỏ.
        - Khung Ảnh (Thumbnail): Nằm ngay dưới nút đóng, được thu hẹp chiều ngang (dạng hình vuông hoặc gần vuông) và căn giữa màn hình, chiếm khoảng 35-40% chiều cao màn hình.
        - Danh sách Truyện: Nằm ngay dưới khung ảnh, chiếm khoảng 40-45% chiều cao. Gồm 9 dòng, mỗi dòng chứa: Nút hình tròn (trái) + Tên truyện (giữa) + Nút Play (phải, để chạy lại xem lại phần gameStory tương ứng). Đặc điểm: phân biệt cách hiển thị giữa 3 trạng thái: đã hoàn thành thử thách's tuyến truyện, chưa mở thử thách's tuyến truyện, đang mở thử thách's tuyến truyện. Nút play không hiển thị với trạng thái "chưa mở thử thách's tuyến truyện".
        - Khu vực Điều hướng: Nằm ở dưới cùng, chiếm khoảng 15% chiều cao. Bao gồm tiêu đề chương ("chapper's title") nằm trên, bên dưới là mũi tên trái/phải để chuyển chương. Vì chỉ có 3 chương nên hiển thị số trang ("1/x") nằm chính giữa. Với x là số lượng chương đã mở ra.
        - Sử dụng sqlite làm database và đặt tên file database theo idUser lưu trữ gồm 3 bảng:
            + idUser_Records: Mỗi lần chơi lại sẽ dựa vào nhóm thông số gồm: idUsers, starting timestamp, ending timestamp, idStory, idChapter, totalScores, totalSeconds, avgSpeed (totalScores/totalSeconds), retryNo. Mỗi lần kết thúc chơi lại sẽ tạo mới 1 hàng lưu.  
            + idUser_Stories: Mỗi lần khởi động gameStory sẽ kiểm tra bảng này để sửa cập nhật lại cột isActivatied và isSeclected cho tuyến truyện mới hoặc chơi lại tuyến truyên cũ. Bẳng này gồm các cột: idUser, idStory, idChapter, isActivated, isSelected, totalRetries, lastMaxScore, lastMaxSpeed.
            + shared_data: idStory, storyName, idChapter,chapterName, minScore, minSpeed, minRetries, requiredStories, nextBlockScore, nextBlockSpeed, tableMatrix, xmlDialogue và thubmnailPath. Trong đó, nextBlockScore và nextBlockSpeed tương ứng với điều kiện để mở ra nextBLock_II và nextBlock_III trong gameCore. tableMatrix chứa vị trí các ô và màu sắc cần hiển thị sẵn trên bàn cờ khi bắt đầu game.
[ ] Task 2.7: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsole/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1

### V3
[ ] Task 3.1: viết v3 gameConsole/app.cpp - gọi API lấy data leaderboard từ mongo atlas.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tich-hop-backend-13
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 12 và trên 14.
    - Chỉ cần lấy và đọc (read) từ API thay vì dùng file `gameConsole_board.json`.
    - Làm nút "load" trong popup của setting để lấy đồng bộ file sqllite lên mongo atlas. Không dùng cơ chế login, yêu cầu nhập email (hoặc username) và dùng OTP. Tìm document được tạo mới nhất có chứa email và để download file sqlite tường ứng.
    - Làm nút "save" trong popup của setting để lấy cập nhật file sqllite lên mongo atlas. Không dùng cơ chế login, yêu cầu nhập email hoặc username. Nếu idUser khớp với idUser trong document mới nhât khi tìm kiếm bằng email hoặc không tìm ra bất kỳ document nào theo email cung cấp, và xác lập đồng ý bằng otp rồi tạo doucment mới với và upload file sql tương ứng.
    - Trong mongo gồm 3 collections: 
        - user: chứa các document lưu trữ việc upload file sqllite của các user gồm: sqlFile, 
        - board: chứa các document thành tích kỷ lục mới mỗi khi có user upload file idUser, nameUser, sqlFite, emailUser.
        - story: chứa các document mô tả các story, mỗi document gồm các phần: idChapter, titleChapter, data. Trong data là chứa dữ liệu để gameStory download và import vào bảng shared_data ở lần đầu mở game hoặc không có cache.
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
    - Thay vì dùng màu trắng tinh (255, 255, 255), mã nguồn đã định nghĩa hằng số `SOFT_WHITE = {220, 220, 220, 255}`.
    - Giảm độ sáng của text giúp font bitmap (SDL_RenderDebugText) trông mỏng và nhẹ hơn, khắc phục hạn chế không thể đổi font weight động.
[x] Issue 1.2: Bổ sung ký tự Copyright (©) trên thanh tiêu đề của hệ điều hành.
    - Tên cửa sổ được ghép mã UTF-8 `\xC2\xA9` (thành "cTetris ©" và "Game Console © - Standalone").
    - Ký tự này được đẩy cho window manager của hệ điều hành xử lý trực tiếp do SDL_RenderDebugText chỉ hỗ trợ bảng mã ASCII.
[x] Issue 1.3: Cập nhật nội dung hướng dẫn (Guide) - Thay đổi hệ số Speed Boost.
    - Hệ số rơi nhanh (Speed Boost) khi giữ phím SPACE đã được điều chỉnh tăng lên mức x5.
    - Nội dung trong popup Guide đã được cập nhật thành: " - SPACE : speed boost x5" và "Tip: hold SPEED BOOST to make piece drop 5x faster."
[x] Issue 1.4: Đồng bộ màu Highlight (HIGHLIGHT_Y) cho các trạng thái tương tác.
    - Mã nguồn định nghĩa hằng số `HIGHLIGHT_Y = {255, 215, 0, 255}`.
    - Màu vàng này được dùng thống nhất để tạo viền cho nút bấm khi đang được focus bằng bàn phím, đồng thời làm màu cho thanh trượt (thumb) của scrollbar khi người chơi đang kéo thả (dragging).
### V2
[x] Issue 2.1: `runGameConsole()` entry kiểm tra DB tồn tại — return 3 nếu thiếu.
    - Kiểm tra `{devicePrefPath}/default.sqlite` qua `SDL_IOFromFile`.
    - File không tồn tại hoặc size < 100 bytes → return 3 (SCREEN_GAMESTORY_INIT).
    - `main.cpp` bắt `next == 3` → gọi `runGameStory(0,0)` để init DB + sync, rồi loop lại Console.
    - Không render bất kỳ UI Console nào trước khi kiểm tra xong.

[x] Issue 2.2: Load `{idUser}_Settings` từ DB khi khởi động — không dùng default hardcode.
    - `dbLoadSettings(cfgInOut)` gọi ngay sau `dbInitSchema()` + `dbSeedSharedData()`.
    - Đọc bảng `{idUser}_Settings` (key-value); không có row → giữ default struct values.
    - `applyVolumeToStream()` gọi lại sau load để sync gain với volume từ DB.

[x] Issue 2.3: Save `{idUser}_Settings` khi đóng Settings popup.
    - `dbSaveSettings(*state.cfg)` gọi khi X button click và ESC trong Settings popup.
    - WASM: `EM_ASM({ Module.FS.syncfs(false, function(){}); })` sau mỗi save.
    - I/O: set volume=0.3 → kill app → mở lại → slider tại 30%.

[x] Issue 2.4: Stories popup — thumbnail fade theo story hover; 3 trạng thái row rõ ràng.
    - Hover/focus row → fetch `thumbnailPath` URL → fade-out 150ms → fade-in texture mới.
    - `isActivated=0`: row xám `{80,80,80}`, icon lock, không Play.
    - `isActivated=1, isSelected=0`: row bình thường, Play hiện, radio outline.
    - `isActivated=1, isSelected=1`: row viền `HIGHLIGHT_Y`, Play hiện, radio filled.
    - `lastMaxScore > 0`: hiển thị `"Best: {score}"` cạnh tên story.

[x] Issue 2.5: Nút Play trong Stories popup — update `isSelected` DB rồi chuyển sang gameStory.
    - Click Play: `dbUpsertStoryProgress` SET `isSelected=1` story mới, `isSelected=0` story cũ.
    - Return `SCREEN_GAMESTORY` (=1) từ `runGameConsole()`.
    - `main.cpp`: `next==1` → `runGameStory(cfg.storyId, cfg.chapterId)` → loop về Console.

[x] Issue 2.6: Tự động đồng bộ `isSelected` từ gameStory khi re-enter Console.
    - Mỗi lần vào `runGameConsole()`: query `{idUser}_Stories WHERE isSelected=1` → cập nhật `cfg.storyId/chapterId`.
    - Popup Stories luôn phản chiếu DB, không cache in-memory từ lần mở trước.

[x] Issue 2.7: Board popup đọc từ `sync_Records` (Group 3) thay vì `gameConsole_board.json`.
    - `dbLoadRecords()` (không tham số) query `sync_Records ORDER BY totalScore DESC LIMIT 50`.
    - Fallback: nếu `sync_Records` rỗng dùng `FALLBACK_BOARD_ROWS[]` constants.
    - `sync_Records` được seed 30 fake rows từ `FALLBACK_BOARD_ROWS` lúc `dbInitSchema()` nếu table rỗng.

[x] Issue 2.8: Chuẩn hoá tên bảng DB thành 3 nhóm.
    - `userTable(suffix)` helper: `g_dbCurrentUser + "_" + suffix`.
    - Group 1 `{idUser}_*`: `default_Records`, `default_Stories`, `default_Settings`.
    - Group 2 `shared_*`: `shared_data`, `shared_dialogues`, `shared_choices`, `shared_meta`.
    - Group 3 `sync_*`: `sync_Records`.

[x] Issue 2.9: `dbInitSchema()` tạo 9 bảng; `sync_Records` seed fake data.
    - Tất cả `CREATE TABLE IF NOT EXISTS` — idempotent, an toàn khi gọi lại.
    - `sync_Records` seeded 30 rows từ `FALLBACK_BOARD_ROWS` nếu `COUNT(*) == 0`.
    - Log: `"schema initialized (9 tables: 3 user + 4 shared + 1 sync + meta)"`.

[x] Issue 2.10: `dbSeedSharedData()` trở thành no-op skeleton.
    - Không còn đọc file `.json` hay `.sql` từ `SDL_GetBasePath()`.
    - Nếu `shared_data` có rows → return true (sync đã chạy từ gameStory).
    - Nếu rỗng → log warning + return false → `runGameConsole()` return 3 → gameStory sync.

### V3
[ ] bổ sung sau
## Rules:
    - Chỉ có 1 file c++ (app/src/gameConsole/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameConsole/include).
    - Cần tách 1 file layout.h (app/src/gameConsole/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên window.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameConsole thì phải để trong app/src/gameConsole/gameConsole_music.mp3.

    # gameConsole — Remaining Task Plan (V2 & V3)

> Scope: all remaining tasks in `app/src/gameConsole/task.md`, **excluding** integration-driven tasks (2.7, 3.4).

---

## Overview

| Metric | Value |
|---|---|
| Remaining tasks | 9 |
| V2 tasks | 6 |
| V3 tasks | 3 |
| High complexity | 3 (2.6, 3.1, and the WASM+SQLite cross-concern) |

---

## Recommended execution order

```
① 2.3 Settings popup  →  ② 2.1 Background  →  ③ 2.2 JSON
→  ④ 2.4 Volume  →  ⑤ 2.5 Colors  →  ⑥ 2.6 SQLite/Stories ⚠
→  ⑦ 3.2 Sort time  →  ⑧ 3.3 Sort score  →  ⑨ 3.1 MongoDB/OTP ⚠
```

**Rationale:**
- Build the Settings popup container (2.3) before 2.4 / 2.5 / 3.1, which all live inside it.
- Establish the shared `loadSvgTextureFromMem()` helper in 2.1 before 2.6 reuses it for story thumbnails.
- Complete the JSON data layer (2.2) before the sort tasks (3.2, 3.3) — they need `BoardEntry::timeEpoch`.
- 2.6 is the gating dependency for 3.1; budget ~2× normal time for it.

---

## Dependency map

```
2.1 Background   ──────────────────────→  2.6 (thumbnail loading)
2.2 JSON         ──────────────────────→  3.2 Sort time, 3.3 Sort score
2.3 Settings     ──────────────────────→  2.4 Volume, 2.5 Colors, 3.1 API
2.6 SQLite       ──────────────────────→  3.1 MongoDB API
```

---

## Cross-cutting decisions to make before coding

1. **`SettingsConfig` struct** — finalise in `gameConsole_layout.h` early; it is the data contract between Console and Core. Changing it mid-V2 forces edits in both modules.
2. **HTTP abstraction layer** — define `gameConsole_http.h` with `#ifdef __EMSCRIPTEN__` switching between libcurl and emscripten fetch before writing any 3.1 code.
3. **SQLite IDBFS persistence contract** — decide when `FS.syncfs()` is called (after every INSERT, or batched on popup close) before implementing 2.6; getting this wrong causes silent data loss on browser tab close.

---

## New build-script additions required (both `build.sh` and `build.ps1`)

| # | What | How |
|---|---|---|
| A | `nlohmann/json.hpp` (single header) | Download from `raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp` — same pattern as nanosvg |
| B | SQLite amalgamation (`sqlite3.h` + `sqlite3.c`) | Download from `sqlite.org/download.html`; add to `GAME_SOURCES`; compile with `-DSQLITE_THREADSAFE=0` for WASM |
| C | libcurl (native only) | `find_package(CURL REQUIRED)` + `target_link_libraries(cTetris PRIVATE CURL::libcurl)` in CMakeLists; WASM uses emscripten fetch, no libcurl needed |
| D | WASM link flags | Add `--preload-file gameConsole_board.json` and `-sFETCH=1` to `target_link_options` in CMakeLists |
| E | Deploy pipeline | Copy `gameConsole_board.json` to WASM output dir alongside `cTetris.html` |

---

## V2 Tasks

### Task 2.1 — Background image full-fit 9:16
**Code block tag:** `gameconsole-chen-backgound-08`
**Complexity:** Medium
**Files:** `gameConsole/app.cpp`, `gameConsole/gameConsole_bg.svg` (new asset)
**Dependencies:** nanosvg (already vendored), `SDL_CreateTexture`

**Implementation steps:**

1. Add a helper `loadSvgTexture(renderer, path)` using nanosvg → rasterise at 270×480 → `SDL_CreateTexture(SDL_PIXELFORMAT_RGBA32)` → upload via `SDL_UpdateTexture`. Return `SDL_Texture*`.
2. Load texture once on the first call to `drawBackground()`; store as a file-scope `static SDL_Texture* s_bgTex = nullptr`. Lazy-init pattern avoids re-loading on re-entry.
3. Compute a letterbox/fill rect: keep 9:16 aspect, scale to fill window, centre. Use `SDL_GetWindowSize` each frame for responsive WASM resize.
4. Call `SDL_RenderTexture(renderer, s_bgTex, NULL, &dstRect)` as the first draw call before all UI, then render all existing buttons/panels on top.
5. On module exit (`runGameConsole` returns), call `SDL_DestroyTexture(s_bgTex); s_bgTex = nullptr` to prevent a leak on re-entry.
6. For WASM: embed SVG data as a C string header `gameConsole_bg_svg.h` (same pattern as `gameStory_logo_svg.h`) to avoid Emscripten FS setup.

> ⚠ This task applies to all `gameXxx` modules. Define a shared `loadSvgTextureFromMem()` utility in a common header (e.g. `sdl_svg_utils.h`) to avoid duplication across modules.

---

### Task 2.2 — JSON leaderboard data
**Code block tag:** `gameconsole-doc-du-lieu-json-09`
**Complexity:** Medium
**Files:** `gameConsole/app.cpp`, `gameConsole/gameConsole_board.json` (already exists), `libs/*/downloads/nlohmann/` (via build script)
**Dependencies:** nlohmann/json (header-only), Emscripten FS or `--preload-file`

**Implementation steps:**

1. Add `Initialize-Nlohmann` / `init_nlohmann()` to build scripts — same download pattern as nanosvg, from `https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp`.
2. Native path: read the file relative to the executable via `SDL_GetBasePath()` + `"/gameConsole_board.json"`. Parse with `nlohmann::json::parse()` into a `std::vector<BoardEntry>`.
3. WASM path: use Emscripten `--preload-file gameConsole_board.json` linker flag (add to `target_link_options` in CMakeLists). File is accessible via standard `fopen()` on the virtual FS.
4. Fallback: if JSON fails to load, silently fall back to the existing static `BOARD_DATA[]` array so the app never crashes.
5. Extract ISO 8601 timestamps and convert to display format `MM-DD HH:MM` using a small cross-platform helper — `strptime` is unavailable on MSVC so do not use it directly.
6. Replace all `BOARD_DATA[]` references throughout `drawBoardLightbox` and event handlers with the loaded vector. `BOARD_TOTAL` becomes `(int)g_board.size()`.

---

### Task 2.3 — Settings button & popup
**Code block tag:** `gameconsole-nut-setting-10`
**Complexity:** Low
**Files:** `gameConsole/app.cpp`, `gameConsole/include/gameConsole_layout.h` (add `SettingsConfig` struct)
**Dependencies:** 2.4 (volume slider lives here), 2.5 (color picker lives here)

**Implementation steps:**

1. Add a 5th main button "SETTINGS" (same `BTN` layout, e.g. `y=320`). Update `NUM_MAIN_BUTTONS`; shift QUIT down to `y=360`.
2. Add `bool showSettings` to `AppState`. Wire button index 4 → `state.showSettings = true`.
3. Define `SETTINGS_POPUP` rect (similar size to `GUIDE_POPUP`). Draw background dimmer + popup panel in a new `drawSettingsLightbox()` function.
4. Define a `SettingsConfig` struct in `gameConsole_layout.h`:
   ```cpp
   struct SettingsConfig {
       float volume;
       bool  colorEnabled[7];
       int   storyId;
       int   chapterId;
   };
   ```
5. ESC and the X button close the popup. All existing keyboard navigation (TAB/focus cycle) must correctly skip to QUIT when the settings popup is open.

---

### Task 2.4 — Volume control
**Code block tag:** `gameconsole-dieu-chinh-am-luong-11`
**Complexity:** Medium
**Files:** `gameConsole/app.cpp`, `gameConsole/include/gameConsole_layout.h`
**Dependencies:** 2.3 (Settings popup), SDL3 `SDL_AudioStream` (built-in, no external mixer)

**Implementation steps:**

1. Draw a horizontal slider inside the Settings popup: a track rect and a draggable thumb circle (~8 px radius). Label "VOLUME" with a percentage readout.
2. Track mouse drag on the slider thumb in the existing `SDL_EVENT_MOUSE_BUTTON_DOWN` / `MOUSE_MOTION` handler — add a `bool draggingVolume` field to `AppState`.
3. Map slider position `[0, sliderW]` → volume `[0.0f, 1.0f]`. Store in `SettingsConfig::volume`.
4. Apply to audio: SDL3 uses `SDL_SetAudioStreamGain(stream, volume)`. Call this whenever the slider value changes. Refactor the current stub `playBackgroundMusic()` to return the `SDL_AudioStream*` so the slider can reference it.
5. Keyboard support: when the slider is focused, LEFT/RIGHT arrow keys decrement/increment by 5%.
6. Pass `SettingsConfig::volume` into the `runGameCore()` signature so game sound effects also respect the setting.

> ℹ SDL3 replaced SDL_mixer. Use `SDL_OpenAudioDeviceStream` + `SDL_SetAudioStreamGain`. No external mixer library is needed.

---

### Task 2.5 — Block color customization
**Code block tag:** `gameconsole-tuy-chinh-mau-cho-cac-khoi-xep-hinh-12`
**Complexity:** Low
**Files:** `gameConsole/app.cpp`, `gameConsole/include/gameConsole_layout.h`
**Dependencies:** 2.3 (Settings popup)

**Implementation steps:**

1. Define a palette of 7 `SDL_Color` entries (red, orange, pink, yellow, green, blue, purple) in a static array `BLOCK_PALETTE[7]`.
2. In the Settings popup, render 7 colored square swatches (e.g. 28×28 px each) in a horizontal row. Draw a white outline ring around selected swatches.
3. Multi-select: clicking a swatch toggles its selection. Store as `bool SettingsConfig::colorEnabled[7]`. Guard: at least one color must remain enabled at all times.
4. Pass the palette selection into `runGameCore()` via `SettingsConfig`. gameCore uses only enabled colors when choosing piece colors.
5. Mouse hit test: each swatch is a small `SDL_FRect`; reuse the existing `hitTest()` helper.

---

### Task 2.6 — Stories button + SQLite database
**Code block tag:** `gameconsole-nen-nhan-vat-12`
**Complexity:** High
**Files:**
- `gameConsole/app.cpp`
- `gameConsole/include/gameConsole_db.h` (new SQLite wrapper)
- `libs/*/downloads/sqlite/` (amalgamation, via build script)
- `CMakeLists.txt` (add `sqlite3.c` to `GAME_SOURCES`)
- `build.sh` / `build.ps1` (download sqlite amalgamation)

**Dependencies:** sqlite3 amalgamation, 2.1 (thumbnail texture loading), 2.3 (popup pattern)

#### Database schema

**`idUser_Records`** — per-game session log
```
idUser, startTS, endTS, idStory, idChapter,
totalScore, totalSeconds, avgSpeed, retryNo
```

**`idUser_Stories`** — per-user story progress
```
idUser, idStory, idChapter,
isActivated, isSelected, totalRetries, lastMaxScore, lastMaxSpeed
```

**`shared_data`** — static story catalogue
```
idStory, storyName, idChapter, chapterName,
minScore, minSpeed, minRetries, requiredStories,
nextBlockScore, nextBlockSpeed, tableMatrix, xmlDialogue, thumbnailPath
```

#### Implementation steps

1. Add the sqlite3 amalgamation download to build scripts. Add `sqlite3.c` to `GAME_SOURCES` in CMakeLists. Create a thin `gameConsole_db.h` wrapper exposing: `openDb(idUser)`, `initSchema()`, `loadStories()`, `upsertStoryProgress()`, `insertRecord()`.
2. DB file path: `SDL_GetPrefPath("uit", "cTetris")` + `<idUser>.sqlite`. On WASM, mount Emscripten IDBFS at `/idb/` and call `FS.syncfs(false, cb)` after every write and `FS.syncfs(true, cb)` on mount.
3. Populate `shared_data` from hardcoded C++ seed data on first run (guard with `SELECT COUNT(*) FROM shared_data`). This is the offline catalogue; V3 will download updates from MongoDB.
4. Stories popup layout (top to bottom): Close (X) → Thumbnail (`SDL_Texture`, 270×150 px area) → 9-row story list → Chapter navigator with prev/next arrows and "n/x" counter.
5. Each story row: round radio button (left) + story name (centre) + Play button (right, hidden if locked). Three visual states: grey/dim = locked, blue outline = active, green checkmark = completed.
6. Chapter navigator: query `MAX(idChapter) FROM idUser_Stories WHERE isActivated=1` to compute `x` in "n/x". Left/right arrows change `currentChapter`; reload the story list for that chapter.
7. On startup, call `checkAndUnlockStories()`: read `shared_data.requiredStories`, compare with completed story IDs from `idUser_Stories`, and `UPDATE idUser_Stories SET isActivated=1` for newly unlocked stories.
8. Pass selected `idStory`, `idChapter`, `tableMatrix`, and unlock conditions into gameCore via `SettingsConfig`.

> ⚠ SQLite on WASM: compile `sqlite3.c` with `-DSQLITE_THREADSAFE=0`. Use `emscripten_idbfs` + `Module.FS.syncfs()` after every transaction. This is the most complex sub-task — plan ~2× normal time.

---

## V3 Tasks

### Task 3.1 — MongoDB Atlas API + OTP load/save
**Code block tag:** `gameconsole-tich-hop-backend-13`
**Complexity:** High
**Files:**
- `gameConsole/app.cpp`
- `gameConsole/include/gameConsole_http.h` (new HTTP abstraction)
- `CMakeLists.txt` (add `find_package(CURL)` for native)
- `build.sh` / `build.ps1` (libcurl install)

**Dependencies:** 2.6 (SQLite DB), 2.3 (Settings popup — load/save buttons go here), libcurl (native) / emscripten fetch (WASM), MongoDB Atlas Data API key

#### MongoDB collections

**`user`** — upload records
```
idUser, nameUser, emailUser, sqlFileBase64, uploadedAt
```

**`board`** — public leaderboard (updated on each save)
```
idUser, nameUser, emailUser, maxScore, maxSpeed, updatedAt
```

**`story`** — story catalogue updates (maps to `shared_data` columns)
```
idChapter, titleChapter, data { ...shared_data fields... }
```

#### Implementation steps

1. Create `gameConsole_http.h` with two cross-platform primitives:
   ```cpp
   std::string httpGet(const std::string& url, const Headers& headers);
   std::string httpPost(const std::string& url, const Headers& headers, const std::string& body);
   ```
   Native: libcurl with `curl_easy_perform`. WASM: `emscripten_fetch` — prefer the async callback variant under ASYNCIFY and show a loading spinner rather than blocking.

2. In the Settings popup, add "LOAD" and "SAVE" buttons. On click, open an email input sub-panel. Use `SDL_StartTextInput` + `SDL_EVENT_TEXT_INPUT` events to capture keyboard input (note: this also triggers the software keyboard on mobile WASM).

3. **OTP flow — SAVE:** user enters email → app generates a 6-digit OTP → send via a simple email relay REST API (e.g. EmailJS, no SDK) → show OTP input field → on match, read the SQLite file, base64-encode it, POST to the `user` collection, and POST best scores to the `board` collection.

4. **OTP flow — LOAD:** user enters email → query the `user` collection for the most recent document with that email → if found, send OTP to that email → on match, download `sqlFileBase64`, decode, write to the IDBFS path, and reload all in-memory data.

5. **Story sync:** on startup, GET the `story` collection and compare `idChapter` entries with local `shared_data`. `INSERT OR REPLACE` any new or changed entries. Show a brief "Syncing stories…" status line during the request.

6. **API key:** pass as a compile-time define `-DATLAS_API_KEY="..."` from an environment variable in build scripts. Never commit the key. Document that the WASM bundle exposes it in plaintext — the production mitigation is a Cloudflare Worker proxy (stub the proxy URL as a constant so upgrading is a one-line change).

7. **Offline graceful degradation:** if any HTTP call fails, show "Offline — using local data" and skip the operation silently. All local functionality must remain fully usable without network.

> ⚠ Storing an API key in a WASM bundle is insecure. For production, proxy all Atlas requests through a serverless function (Cloudflare Worker / AWS Lambda). Document this as a known limitation for the V3 demo milestone.

---

### Task 3.2 — Sort by time in board popup
**Code block tag:** `gameconsole-sort-by-time-in-board-14`
**Complexity:** Low
**Files:** `gameConsole/app.cpp`
**Dependencies:** 2.2 (JSON/dynamic board data — `timeEpoch` field required)

**Implementation steps:**

1. Add a sort-mode toggle button inside the Board popup header row (e.g. a small "⇅ TIME" label button). Add `enum SortMode { SCORE_DESC, TIME_DESC, TIME_ASC }` and track the current mode in `AppState`.
2. Parse ISO 8601 timestamps from `BoardEntry::time` into a `time_t` / `int64_t` Unix epoch field (`BoardEntry::timeEpoch`) once during loading in task 2.2.
3. On mode change, call `std::stable_sort(g_board.begin(), g_board.end(), cmpByTime)` and reset `boardScroll = 0`.
4. Display the active sort mode visually: highlight the active sort button with a `HIGHLIGHT_Y` border.

---

### Task 3.3 — Sort by score in board popup
**Code block tag:** `gameconsole-sort-by-score-in-board-15`
**Complexity:** Low
**Files:** `gameConsole/app.cpp`
**Dependencies:** 3.2 (shares the same sort infrastructure)

**Implementation steps:**

1. Extend the `SortMode` enum from 3.2 with `SCORE_ASC` (low → high). The default mode is `SCORE_DESC` (matching the current hardcoded order).
2. Add a "⇅ SCORE" toggle button beside the "⇅ TIME" button. Both are mutually exclusive — clicking one deactivates the other.
3. Implement a `cmpByScore` comparator and call `std::stable_sort` the same way as 3.2. Reset scroll to 0.
4. Re-number the rank column (`#`) after sorting: the displayed rank must always reflect the sorted order position, not the original dataset index.

---

## Risk register

| ID | Severity | Description | Mitigation |
|---|---|---|---|
| R1 | High | SQLite + IDBFS persistence on WASM is browser-specific | Must call `FS.syncfs(false, cb)` after every write and `FS.syncfs(true, cb)` on mount. Test on Chrome, Firefox, and Safari. |
| R2 | High | Atlas API key exposed in WASM binary | Cloudflare Worker proxy for production; stub the proxy URL as a compile-time constant. |
| R3 | Medium | `SDL_StartTextInput` triggers software keyboard on mobile WASM | Test touch + physical keyboard; ensure popup layout is not obscured by the on-screen keyboard. |
| R4 | Medium | `strptime` unavailable on MSVC | Write a small cross-platform ISO 8601 parser — only `YYYY-MM-DDTHH:MM` precision is needed for the format we control. |
| R5 | Low | emscripten fetch under ASYNCIFY can block JS event loop if used synchronously | Use the async callback variant and display a loading spinner for all network operations. |