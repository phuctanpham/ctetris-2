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
[ ] Task 2.1: viết v2 gameConsole/app.cpp - chèn 1 hình ảnh làm background đẹp và tự full-fit theo tỷ lệ phóng lớn thu nhỏ.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-chen-backgound-08
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trên 09.
    - Kỹ thuật chèn background và resize đảm bảo màn hình kéo giãn luôn theo 9:16 để không bể điểm ảnh. Áp dụng chung cho các app/src/game* khác.
[ ] Task 2.2: viết v2 gameConsole/app.cpp - sử dụng file JSON để trình diễn cấu trúc thông tin thay vì mảng C++ cứng. (Dời từ v1)
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-doc-du-lieu-json-09
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 08 và trên 10.
    - Lấy dữ liệu từ file `gameConsole/gameConsole_board.json`. Tìm hướng đặt file json hợp lý để sau này chuyển đổi sang gọi API lấy về lưu trữ tạm.
[ ] Task 2.3: viết v2 gameConsole/app.cpp - tạo nút setting và nút close popup setting.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-nut-setting-10
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 09 và trên 11.
    - Nơi chứa các cấu hình khác cho setting.
[ ] Task 2.4: viết v2 gameConsole/app.cpp - điều chỉnh âm lượng.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-dieu-chinh-am-luong-11
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 10 và trên 12.
    - Chú ý thông số vì đây có thể sẽ là input đầu cho cấu hình về âm lượng ở gameCore.
[ ] Task 2.5: viết v2 gameConsole/app.cpp - tuỳ chỉnh màu cho các khối xếp hình.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tuy-chinh-mau-cho-cac-khoi-xep-hinh-12
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 11 và trên 13.
    - Cho phép click multi choice (red, orange, pink, yellow, green, blue, purple) làm thông số cấu hình đầu vào cho gameCore.
[ ] Task 2.6: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsole/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1

### V3
[ ] Task 3.1: viết v3 gameConsole/app.cpp - gọi API lấy data leaderboard từ mongo atlas.
    - Comment codeblock này trong gameConsole/app.cpp là: gameconsole-tich-hop-backend-13
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 12 và trên 14.
    - Chỉ cần lấy và đọc (read) từ API thay vì dùng file `gameConsole_board.json`.
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
[ ] bổ sung sau
### V3
[ ] bổ sung sau
## Rules:
    - Chỉ có 1 file c++ (app/src/gameConsole/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameConsole/include).
    - Cần tách 1 file layout.h (app/src/gameConsole/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên window.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameConsole thì phải để trong app/src/gameConsole/gameConsole_music.mp3.