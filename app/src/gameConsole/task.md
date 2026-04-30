## Tasks:
### V1
[x] Task 1.1: viết v1 gameConsole/app.cpp - tạo layout.h đảm bảo tỷ lệ màn hình 9:16 và thiết lập chế độ BUILD_STANDALONE để test độc lập module trên hệ điều hành.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-tao-giao-dien-169-00
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và cài đặt thư viện cần thiết.
[x] Task 1.2: viết v1 gameConsole/app.cpp - vẽ giao diện nền trơn cơ bản (sử dụng màu sắc và chữ ASCII) tạm thay thế cho ảnh tĩnh.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-giao-dien-nen-01
    - Đặt vị trí phù hợp để làm nền tảng hiển thị các nút và popup.
[x] Task 1.3: viết v1 gameConsole/app.cpp - xây dựng hệ thống Scrollbar tương tác toàn diện (Interactive Scrollbar).
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-interactive-scrollbar-02
    - Mô tả: Hệ thống hỗ trợ người chơi kéo thả (drag) thanh trượt (thumb), click vào track để nhảy cóc đến vị trí, click mũi tên lên/xuống và giữ chuột để cuộn liên tục (auto-repeat).
[x] Task 1.4: viết v1 gameConsole/app.cpp - tạo thuật toán tự động ngắt dòng văn bản (Word Wrapping).
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-word-wrapping-03
    - Mô tả: Hàm helper tự động tính toán, ngắt từ và đưa xuống dòng dựa trên số ký tự tối đa để text hiển thị vừa vặn vào khung hẹp mà không bị cắt ngang từ.
[x] Task 1.5: viết v1 gameConsole/app.cpp - tạo nút guide, hiển thị popup hướng dẫn và nút close.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-nut-guide-04
    - Mô tả: Áp dụng kỹ thuật lightbox (phủ mờ nền sau). Tích hợp thuật toán Word Wrapping và Scrollbar cho nội dung hướng dẫn (di chuyển, xoay khối, phím chức năng...).
[x] Task 1.6: viết v1 gameConsole/app.cpp - tạo nút board và hiển thị popup leaderboard bằng dữ liệu cứng (hardcoded array).
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-nut-board-05
    - Mô tả: Tái sử dụng kỹ thuật lightbox và Scrollbar. Tạm thời sử dụng mảng dữ liệu C++ (`BOARD_DATA`) gồm thông tin người dùng, điểm, thời gian để hiển thị danh sách (không dùng file json lúc này).
[x] Task 1.7: viết v1 gameConsole/app.cpp - chèn nhạc nền trong quá trình ở màn hình game console.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-chen-nhac-06
    - Mô tả: Tối ưu kỹ thuật chèn media (hiện tại gọi stub log "Phat nhac nen console") để tái sử dụng lại cho ở các version sau.
[x] Task 1.8: viết v1 gameConsole/app.cpp - điều hướng nút bằng bàn phím, con lăn chuột (mouse wheel) và bổ sung nút quit.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-dieu-huong-ban-phim-07
    - Bắt sự kiện cuộn con lăn chuột (mouse wheel) để điều khiển thanh trượt.
    - Cho phép thêm các phương thức điều khiển bằng bàn phím song song với click chuột:
        + Phím tab, phím left (A) và phím down (S) để dịch chuyển focus tiến tới giữa các nút.
        + Phím right (D) và phím up (W) để dịch chuyển focus lùi giữa các nút.
        + Phím enter hoặc phím space để kích hoạt nút đang được focus.
        + Phím esc để đóng lightbox guide/board; khi ở màn hình chính nhảy focus về nút quit.
        + Trong lightbox board/guide, các phím W/S đồng thời cũng dùng để scroll danh sách như UP/DOWN.
    - Trạng thái nút đang focus phải được hiển thị rõ ràng (ví dụ khung viền vàng) để người chơi nhận biết.
[x] Task 1.9: viết v1 gameConsole/app.cpp - tạo nút PLAY (Start) và tích hợp với các modules còn lại trong app/src qua file app/main.cpp.
    - Comment codeblock này trong gameConsole/app.cpp là: integration/v1
    - Tạo nút PLAY, xử lý chuyển cảnh sang giao diện gameCore mượt mà khi người dùng kích hoạt (nhấn Start/Play).
    - Khởi động chương trình luôn bắt đầu bằng màn hình của gameStory.
    - Sau khi kết thúc màn hình gameStory sẽ chuyển tiếp qua gameConsole.
    - Đưa về 1 file CMakeLists.txt để hỗ trợ build wasm từ macOS / Ubuntu / Window. 
    - Viết script build.sh và build.ps1 phục vụ kiểm tra môi trường, chọn chế độ build module đơn lẻ hoặc tích hợp.
### V2
[ ] Task 2.1: viết v2 gameConsole/app.cpp - chèn 1 hình ảnh làm background đẹp và tự full-fit theo tỷ lệ phóng lớn thu nhỏ.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-chen-backgound-08
    - Kỹ thuật chèn background và resize đảm bảo màn hình kéo giãn luôn theo 9:16 để không bể điểm ảnh. Áp dụng chung cho các app/src/game* khác.
[ ] Task 2.2: viết v2 gameConsole/app.cpp - sử dụng file JSON để trình diễn cấu trúc thông tin thay vì mảng C++ cứng. (Dời từ v1)
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-doc-du-lieu-json-09
    - Lấy dữ liệu từ file `gameConsole/gameConsole_board.json`. Tìm hướng đặt file json hợp lý để sau này chuyển đổi sang gọi API lấy về lưu trữ tạm.
[ ] Task 2.3: viết v2 gameConsole/app.cpp - tạo nút setting và nút close popup setting.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-nut-setting-10
    - Nơi chứa các cấu hình khác cho setting.
[ ] Task 2.4: viết v2 gameConsole/app.cpp - điều chỉnh âm lượng.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-dieu-chinh-am-luong-11
    - Chú ý thông số vì đây có thể sẽ là input đầu cho cấu hình về âm lượng ở gameCore.
[ ] Task 2.5: viết v2 gameConsole/app.cpp - tuỳ chỉnh màu cho các khối xếp hình.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-tuy-chinh-mau-cho-cac-khoi-xep-hinh-12
    - Cho phép click multi choice (red, orange, pink, yellow, green, blue, purple) làm thông số cấu hình đầu vào cho gameCore.
[ ] Task 2.6: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsonle/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1
[ ] Task 3.1: viết v3 gameConsole/app.cpp - gọi API lấy data leaderboard từ mongo atlas.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-tich-hop-backend-13
    - Chỉ cần lấy và đọc (read) từ API thay vì dùng file `gameConsole_board.json`.
### V3
[ ] Task 3.2: viết v3 gameConsole/app.cpp - tạo sort theo time trong popup board.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-sort-by-time-in-board-14
[ ] Task 3.3: viết v3 gameConsole/app.cpp - tạo sort theo score trong popup board.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-sort-by-score-in-board-15
[ ] Task 3.3: tích hợp v3 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsole/app.cpp là: integration/v3
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v2
## Issues (Changes, Bugs and Usabilities)
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