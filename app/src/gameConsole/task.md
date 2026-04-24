## Task:
[x] Task 1.1: viết v1 gameConsole/app.cpp - tạo layout.h và build .exe đảm bảo màn hình 9:16 trước khi viết code tiếp
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-tao-giao-dien-169-00
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và cài đặt thư viện cần thiét
[x] Task 1.2: viết v1 gameConsole/app.cpp - chèn 1 hình ảnh làm background đẹp và tự full-fit theo tỷ lệ phóng lớn thu nhỏ
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-chen-backgound-01
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 00 và trước 02
    - Kỹ thuật chèn backgound và resize này, nếu thành công sẽ lấy để áp dụng cho các app/src/game* khác, mục tiêu là khổng bể ảnh và layout tỷ lệ 16:9 khi người chơi kéo giãn màn hình, đảm bảo màn hình kéo giãn luôn theo 16:9 để không bể điểm ảnh.
[x] Task 1.3: viết v1 gameConsole/app.cpp - tạo nút guide và nút close popup guide
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-nut-guide-02
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 01 và trước 03
    - Kỹ thuật lightbox: màn hình background bị phủ 1 lớp mờ tối để làm nổi bật lên 1 lightbox popup ở giữa có màu sắc và độ sáng cao hơn xung quanh bên ngoài, bên trong lightbox nội dung hướng dẫn như bên dưới, trong khung lightbox cần có 1 nút close để thoát trạng thái lightbox và trở về màn hình game console. Kỹ thuật này nếu làm thành công thì sẽ đưa vào tái sử dụng cho các app khác.
    - Nội dung hướng dẫn cần ghi trong nút guide:
        + Lật theo chiều kim đồng hồ:  mũi tên (up) và phím W
        + Di chuyển trái: mũi trên (left) và phím A
        + Di chuyển trái: mũi trên (right) và phím D
        + Lật ngược chiều kim đồng hồ: mũi trên (down) và phím S
[x] Task 1.4: viết v1 gameConsole/app.cpp - tạo nút board và nút close popup board
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-nut-board-03
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 02 và trước 04
    - Tái sử dụng lại kỹ thuật lightbox cho nút board.
    - Thử sử dụng json để trình diễn cấu trúc thông tin trước
    - Tìm hướng để file json ở đâu cho hợp lý để sau này gọi API thì có chỗ lưu các file json
    Hướng dẫn khác:
    - lấy file data giả bằng gameConsole/gameConsole_board.json để hiển thị, chỉ hiện tối 7 cái từ trên xuống, scroll lên xuống để xem thêm, scroll theo row, đừng rồi chỉ để thấy nửa dòng chữ.
[x] Task 1.5: viết v1 gameConsole/app.cpp - chèn nhạc nền trong quá trình ở màn hình game console
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-chen-nhac-05
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 03 và trước 05
    - Tương tự kỹ thuật lightbox, việc làm trước chèn nhạc nền trong v1 nhằm tối ưu kỹ thuật chèn media để tái sử dụng lại cho ở các version sau.
[x] Task 1.6: tích hợp v1 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameConsole/app.cpp là:  integration/v1
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí trên 00
    - Khởi động chương trình luôn bắt đầu bằng màn hình của gameStory.
    - làm tạm nút play ở task 2.4 
    - Sau khi kết thúc màn hình của phần gameStory sẽ chuyển tiếp qua khởi động màn hình gameConsole
    - Sau khi bấm nút start bên màn hình gameConsole chuyển tiếp qua khởi động màn hình gameCore
    - Đưa về 1 file CMakeLists.txt để hỗ trợ build wasm từ macOS / Ubuntu / Window. 
     - viết và kiểm tra build.sh và build.ps1 để:
        + hỏi build đơn lẻ từng module hay tích hợp toàn bộ.
        + với build.sh tự xác định hệ điều hành  (macos hay ubuntu) , build.ps1 thì mặc định là windown rồi chạy chạy các cài đặt tiếp 
        + kiểm tra môi trường hệ điều hành hiện tại đã cài các công cụ cần thiết đúng version trên hệ điều hành chưa? nếu chưa hoặc thấp hơn version tối thiếu thì dừng script yêu cầu cài đặt.
        + nếu các công cụ cài đặt đầy đủ thì tiến hành build và để trong build/wasm, kết thúc có hướng dẫn cách chạy trên file build trên local host.
[ ] Task 2.1: viết v2 gameConsole/app.cpp - tạo nút setting và nút close popup setting.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-chen-nhac-05
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trước 06
    - nơi chứa các cấu hình khác cho setting
[ ] Task 2.2: viết v2 gameConsole/app.cpp - điều chỉnh ẩm lượng
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-dieu-chinh-am-luong-06
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05 và trước 07
    - Chú ý thông số vì đây có thể sẽ là input đầu cho cấu hình về âm lượng ở gameCore
    - Có thể phải lưu tạm để mỗi khi khởi động app không cần điều chỉnh lại, các phần của các app/src/game* khác cũng dựa vào file lưu tạm này mà cấu hình âm thanh bên modules đó.
    - Đây cũng là kỹ thuật được tái sử dụng để giao tiếp giữa các phần các app/src/game* như 1 public interface hay 1 file kiểu xml hay json lưu tạm làm trung gian giao tiếp.
[ ] Task 2.3: viết v2 gameConsole/app.cpp - tuỳ chỉnh màu cho các khối xếp hình
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-tuy-chinh-mau-cho-cac-khoi-xep-hinh-07
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trước 08
    - Chú ý thông số vì đây có thể sẽ là input đầu cho cấu hình về màu sắc ở gameCore
    - Có thể phải lưu tạm để mỗi khi khởi động app không cần điều chỉnh lại, phần code src/gameCore khác cũng dựa vào file lưu tạm này mà cấu hình màu.
    - Cho phép click multi choice, mỗi choice là 1 trong các màu: red, orange, pink, yellow, green, blue, purple. Các màu này sẽ là đầu vào thuộc tính cho các viên xếp hình.
[ ] Task 2.4: viết v2 gameConsole/app.cpp - tạo nút start và tích hợp với các bên src/game* khác.
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-nut-start-va-tich-hop-voi-cac-ben-khac-08
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trước 09
    - chủ yếu lúc này cần nói chuyện giưã các src/game* để làm sao click nút start (nút skip) mà nó chuyển tiếp qua các phần giao diện khác của src/game* quản lý. Hoặc khi ở màn hình giao diện khác, có nút “setting” thì vòng lại màn hình này nhịp nhàng
[ ] Task 2.5: viết v3 gameConsole/app.cpp - gọi API lấy data từ mongo atlas thay vì dùng gameConsole_board.json
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-tich-hop-backend-09
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 08 và trước 10
    - không cần quan tâm bài toán ghi, chỉ càn quan tâm bài toán lấy và đọc từ API thay vì dùng file gameConsole_board.json
[ ] Task 3.1: viết v3 gameConsole/app.cpp - tạo sort theo time trong popup board
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-sort-by-time-in-board-10
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 09 và trước 11
[ ] Task 3.2: viết v3 gameConsole/app.cpp - tạo sort theo score trong popup board
    - Comment codeblock này trong gameconsole/app.cpp là: gameconsole-sort-by-time-in-board-11
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí cuối cùng
 ## Rules:
    - Chỉ có 1 file c++ (app/src/gameConsole/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameSgameConsoletory/include).
    - Cần tách 1 file layout.h (app/src/gameConsole/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên windown.
    - Cần tạo thành công build file từ app.cpp để chạy trên macos và wasm.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameConsole thì phải để trong app/src/gameConsole/gameConsole_music.mp3
