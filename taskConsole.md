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
[ ] Task 2.2: viết v2 gameConsole/app.cpp - sử dụng file JSON để trình diễn cấu trúc thông tin thay vì mảng C++ cứng. (Dời từ v1)
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
[ ] Task 3.1: viết v3 gameConsole/app.cpp - gọi API lấy data leaderboard từ Cloudflare (D1 + Durable Objects).
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tich-hop-backend-13
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 12 và trên 14.
    - Chỉ cần lấy và đọc (read) từ API thay vì dùng file `gameConsole_board.json`.
    - Làm nút "load" trong popup của setting để lấy đồng bộ file sqllite lên Cloudflare (D1 + Durable Objects). Không dùng cơ chế login, yêu cầu nhập email (hoặc username) và dùng OTP. Tìm document được tạo mới nhất có chứa email và để download file sqlite tường ứng.
    - Làm nút "save" trong popup của setting để lấy cập nhật file sqllite lên Cloudflare (D1 + Durable Objects). Không dùng cơ chế login, yêu cầu nhập email hoặc username. Nếu idUser khớp với idUser trong document mới nhât khi tìm kiếm bằng email hoặc không tìm ra bất kỳ document nào theo email cung cấp, và xác lập đồng ý bằng otp rồi tạo doucment mới với và upload file sql tương ứng.
    - Trong Cloudflare (D1 + Durable Objects) gồm 3 collections: 
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

[ ] Issue 2.4: Stories popup — thumbnail fade theo story hover; 3 trạng thái row rõ ràng.
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

[ ] Issue 2.11: Board data source (migrate from gameConsole_board.json into runtime sync)
    - Bỏ hoàn toàn file `app/src/gameConsole/gameConsole_board.json` khỏi repo và khỏi artefacts deliverable.
    - Thêm biến môi trường `CTETRIS_BOARD_URL` chứa URL tới JSON board (raw JSON file).
    - Thời điểm runtime (khi khởi chạy Console hoặc khi click "Board") nếu có Internet: fetch payload JSON, tính checksum (ví dụ SHA-256) của payload; KHÔNG được download/ghi file vào artefacts tái biên dịch.
    - Nếu checksum khác với checksum đã lưu (store key `board_checksum` trong `shared_meta` hoặc tương đương), parse JSON và cập nhật `sync_Records` bằng thao tác INSERT/UPDATE (không ghi file); lưu checksum mới vào DB.
    - Nếu fetch thất bại hoặc offline → fallback đọc `sync_Records` từ DB; nếu DB rỗng → dùng `FALLBACK_BOARD_ROWS`.
    - Build-time (`build.sh`, `build.ps1`, CMakeLists) phải kiểm tra và validate `CTETRIS_BOARD_URL` (biến tồn tại, link reachable, trả về JSON tối thiểu). Nếu không thỏa → emit warning hoặc fail build theo chế độ package/recompile.
    - Tạm bỏ qua OTP/Email flows cho V2.

[ ] Issue 2.12: Stories thumbnails — async fetch, cache and hover-fade
    - Implement async thumbnail fetch + per-story texture cache (native: libcurl wrapper; WASM: emscripten_fetch/asyncify). Store cached binary or URL meta in `shared_data.thumbnailPath` or an appropriate cache table.
    - On story row hover/focus: perform fade-out 150ms → swap texture → fade-in 150ms. Ensure smooth timing and cancellation if user moves away.
    - Graceful fallback to placeholder thumbnail when fetch fails or times out.
    - Ensure WASM-specific async flow does not block main loop and uses proper callbacks/asyncify.

## Rules:
    - Chỉ có 1 file c++ (app/src/gameConsole/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameConsole/include).
    - Cần tách 1 file layout.h (app/src/gameConsole/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên window.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameConsole thì phải để trong app/src/gameConsole/gameConsole_music.mp3.