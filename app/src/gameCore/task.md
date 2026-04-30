## Tasks:
### V1
[x] Task 1.1: viết v1 các file class/struct trong gameCore/include
    - Định nghĩa các cấu trúc dữ liệu cơ bản như Point, Tetromino, GameState.
[x] Task 1.2: viết v1 gameCore/app.cpp - tạo layout.h và build .exe đảm bảo màn hình 9:16 trước khi viết code tiếp
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-giao-dien-169-00
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và cài đặt thư viện cần thiết.
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
    - Quy tắc điểm: mỗi dòng xoá = 1 điểm. Giới hạn điểm số tối đa là 999999.
    - Điểm được hiển thị ở định dạng chuỗi 6 chữ số (ví dụ "000005").
[x] Task 1.10: viết v1 gameCore/app.cpp - xây dựng Sidebar UI và Procedural Icons
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-sidebar-ui-08
    - Chia sidebar 30x480 thành 12 component đồng nhất (30x40).
    - Các icon (Quit, Pause, Arrows, Speed Boost) phải được vẽ thủ công bằng code (hình học SDL) thay vì dùng ảnh tĩnh bitmap.
[x] Task 1.11: viết v1 gameCore/app.cpp - tương tác UI bằng chuột (Click & Hold)
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-mouse-interaction-09
    - Cho phép click/giữ chuột lên các icon ở sidebar để điều khiển khối tương tự phím cứng.
    - Nút đang được giữ (chuột hoặc phím) phải đổi sang màu vàng highlight để báo hiệu trạng thái active.
[x] Task 1.12: viết v1 gameCore/app.cpp - tính năng rơi nhanh (Soft Drop)
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-soft-drop-10
    - Sử dụng phím SPACE hoặc giữ chuột vào nút Speed Booster trên sidebar để tăng tốc độ rơi của khối.
[x] Task 1.13: viết v1 gameCore/app.cpp - đồng hồ tính giờ chơi (Play Timer)
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-play-timer-11
    - Hiển thị tổng thời gian chơi thực tế (loại trừ thời gian Pause) dưới định dạng HH:MM trên ô số 4 của sidebar.
[x] Task 1.14: viết v1 gameCore/app.cpp - hiển thị trước 1 khối tiếp theo (Preview Next Block)
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-preview-next-block-12
    - Hiển thị khối sắp rơi tiếp theo ở ô NEXT-1 trên sidebar.
[x] Task 1.15: viết v1 gameCore/app.cpp - tính năng Pause / Play
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-pause-vs-play-13
    - Tạm dừng/tiếp tục trò chơi bằng phím Enter, phím KP_ENTER hoặc click vào icon Pause.
    - Thay đổi icon tương ứng (Vuông: Stop/Đang chạy, Tam giác: Play/Đang dừng).
    - Ngừng đếm thời gian chơi khi đang Pause.
[x] Task 1.16: viết v1 gameCore/app.cpp - popup Quit / Game Over (Lightbox)
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-popup-quit-14
    - Kích hoạt bằng phím ESC, click icon Nguồn, hoặc khi Game Over.
    - Áp dụng kỹ thuật lightbox (phủ mờ nền đen) để hiển thị popup xác nhận.
    - Cung cấp 4 nút chức năng: Restart (chơi lại), Console (về menu), Quit (thoát app), và Cancel.
[x] Task 1.17: tích hợp v1 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameCore/app.cpp là: integration/v1
    - Khởi động chương trình luôn bắt đầu bằng màn hình của gameStory.
    - Sau khi kết thúc màn hình gameStory sẽ chuyển tiếp qua gameConsole.
    - Sau khi bấm nút start bên màn hình gameConsole chuyển tiếp qua gameCore.
    - Đưa về 1 file CMakeLists.txt để hỗ trợ build wasm từ macOS / Ubuntu / Window. 
    - Viết và kiểm tra build.sh và build.ps1 để tự động xác định môi trường và build file.
### V2
[ ] Task 2.1: viết v2 gameCore/app.cpp - nhạc nền trong suốt tiến trình
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-chen-nhac-nen-15
[ ] Task 2.2: viết v2 gameCore/app.cpp - tốc độ rơi nhanh dần theo điểm số.
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tang-do-kho-16
    - Logic tăng độ khó linh động dựa trên mốc điểm thay vì rơi đều đặn với `FALL_INTERVAL_NORMAL = 500`.
[ ] Task 2.3: viết v3 gameCore/app.cpp - dự báo 3 khối liên tiếp
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-du-bao-ba-khoi-xep-hinh-lien-tiep-17
    - Nâng cấp từ dự báo 1 khối (v1) lên hiển thị 3 khối ở các slot NEXT-1, NEXT-2, NEXT-3.
[ ] Task 2.4: viết v3 gameCore/app.cpp - hiệu ứng chớp tắt khi xoá dòng
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-hieu-ung-khi-xoa-dong-18
    - tạo hiệu ứng chớp tắt 3 lần báo hiệu để người chơi cảm nhận là dòng sắp bị xoá.
[ ] Task 2.5: viết v3 gameCore/app.cpp - tăng điểm thưởng khi xoá nhiều dòng cùng thời điểm.
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-diem-thuong-khi-xoa-nhieu-dong-19
    - tổng số hàng được được xoá nhân với tổng số hàng được xoá. Ví dụ: mỗi hàng đang 1 điểm, xoá 2 hàng cùng lúc là (2x2) 4 điểm, xoá 3 hàng cùng lúc là (3x3) 9 điểm, xoá 4 hàng cùng lúc là (3x3) 16 điểm.
[ ] Task 2.6: tích hợp v3 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameCore/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1
### V3
[ ] Task 3.1: viết v3 gameCore/app.cpp - cho phép nhập tên khi game over
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-game-over-screen-20
    - Bổ sung ô nhập ký tự (text input) vào lightbox Game Over để lưu lại tên người chơi với điểm số tương ứng. Thoát game là mất data nếu không ghi.
[ ] Task 3.2: viết v3 gameCore/app.cpp - gọi API và gửi tên và điểm lên Mongo atlas
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tich-hop-backend-21
    - Chỉ cần quan tâm bài toán ghi dữ liệu. Thêm nút retry hoặc resync ở màn hình game over nếu ghi không thành công.
[ ] Task 3.3: tích hợp v3 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameCore/app.cpp là: integration/v3
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v2
## Issues (Changes, Bugs and Usabilities)
### V1
[x] Issue 1.1: Xử lý màn hình Shutdown và Reload riêng cho môi trường web (WASM)
    - Do lệnh `exit()` không hoạt động tốt trên trình duyệt, mã nguồn đã thêm cờ `wasmShutdown` để hiển thị một màn hình đen với nút "RELOAD" lớn ở giữa thay vì tắt game hoàn toàn.
    - Hỗ trợ gọi JS bridge (`emscripten_run_script("window.location.reload();")`) để làm mới trang khi người dùng click chuột hoặc bấm các phím F5/Enter/Space.
[x] Issue 1.2: Loại bỏ khối hình "S" và thêm cơ chế lật gương (Mirror Flip) ngẫu nhiên
    - Biến `NUM_SHAPES` thực tế chỉ cấu hình 5 khối cơ bản (L, I, Z, O, T), chủ động loại bỏ khối S.
    - Hàm `spawnBlock` bổ sung logic lật gương (mirror flip) theo trục X với xác suất 50% khi sinh khối, giúp tạo ra các biến thể (như J, S) linh hoạt mà không cần phải khai báo tĩnh thêm dữ liệu hình khối.
[x] Issue 1.3: Giảm giới hạn hiển thị điểm số từ 6 xuống 5 chữ số
    - Giới hạn điểm số tối đa được hạ từ 999999 xuống 99999 (định dạng 5 chữ số, ví dụ "00000").
    - Mục đích để có thể đồng bộ kích thước font chữ (`SCALE = 0.65f`) cho cả ô Score và ô Timer, giúp các con số hiển thị to rõ, cân đối và dễ đọc hơn trong không gian hẹp của slot 30x40.
[x] Issue 1.4: Bổ sung thông tin "Total time" vào Popup Quit / Game Over
    - Hàm `drawQuitPopup` được nâng cấp để nhận thêm biến `elapsedMs`.
    - Popup giờ đây sẽ hiển thị thêm dòng báo cáo tổng thời gian chơi thực tế (loại trừ thời gian đã pause) theo định dạng `HH:MM:SS` nằm ngay bên dưới thông tin điểm số.
[x] Issue 1.5: Tinh chỉnh độ dày nét chữ (Thinning) và viền (Stroke) cho khối Preview
    - Toàn bộ màu text được đổi sang mã màu `SOFT_WHITE` (RGB 220) thay vì trắng tinh, tạo hiệu ứng thị giác giúp nét chữ bitmap trông mỏng và nhẹ nhàng hơn.
    - Khung Preview (NEXT-1) tăng kích thước mỗi ô (`CELL = 5.0f`) và khe hở (`CELL_GAP = 1.5f`) để đường viền (stroke) chia cắt giữa các ô vuông nhỏ trong khối Tetromino hiển thị sắc nét và rõ rệt hơn.
### V2
[ ] bổ sung sau
### V3
[ ] bổ sung sau
## Rules:
    - Chỉ có 1 file c++ (app/src/gameCore/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameCore/include).
    - Cần tách 1 file layout.h (app/src/gameCore/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên window.
    - Cần tạo thành công build file từ app.cpp để chạy trên macos và wasm.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameCore thì phải để trong app/src/gameCore/gameCore_music.mp3.