[x] Task 1.1: viết v1 các file class trong gameCore/include
[x] Task 1.2: viết v1 gameCore/app.cpp - tạo layout.h và build .exe đảm bảo màn hình 9:16 trước khi viết code tiếp
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-giao-dien-169-00
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và cài đặt thư viện cần thiét
[x] Task 1.3: viết v1 gameCore/app.cpp - tạo 1 trong 6 khối (L,I,T,Z letf, Z right, O) đơn sắc
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-cac-khoi-xep-hinh-LITZO-01
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 00 và trên 02.
[x] Task 1.4: viết v1 gameCore/app.cpp - đổ màu ngẫu nhiên (Blue, Red, Green,Yellow, Orange, Purple)
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-do-mau-02
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 01 và trên 03
[x] Task 1.5: viết v1 gameCore/app.cpp - cho khối rơi tự do ở 1 vị trí bất kỳ (on top)
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-roi-03
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 02 và trên 04.
[x] Task 1.6: viết v1 gameCore/app.cpp - điều khiển khối bằng các mũi tên và WASD
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-xu-ly-phim-04
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 03 và trên 05
    - Lật theo chiều kim đồng hồ:  mũi tên (up) và phím W
    - Di chuyển trái: mũi trên (left) và phím A
    - Di chuyển trái: mũi trên (right) và phím D
    - Lật ngược chiều kim đồng hồh: mũi trên (down) và phím S
[x] Task 1.7: viết v1 gameCore/app.cpp - hiệu ứng chạm và dừng rơi khối
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-xu-ly-cham-05
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trên 06
[x] Task 1.8: viết v1 gameCore/app.cpp - hiệu ứng xoá dòng
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-xu-ly-xoa-dong-06
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05 và trên 07
[x] Task 1.9: viết v1 gameCore/app.cpp - tính điểm
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-tinh-diem-07
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trên 08
[ ] Task 1.10: tích hợp v1 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameCore/app.cpp là:  integration/v1
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí trên 00
    - Khơi động chương trình luôn bắt đầu bằng màn hình của gameStory.
    - Sau khi kết thúc màn hình của phần gameStory sẽ chuyển tiếp qua khởi động màn hình gameConsole
    - Sau khi bấm nút start bên màn hình gameConsole chuyển tiếp qua khởi động màn hình gameCore
        - Đưa về các file CMakeLists.txt trong các modules về 1 một file duy nhất đặt ở app/ để hỗ trợ build chương trình chạy độc lập từng module hoặc tích hợp toàn bộ modules. Kiểm tra tính tồn tại của thư mục build trong thư mục app. Hỗ trợ build chương trình chạy trên các platfom: macos, windown, linux, ios và android, mỗi platform là 1 tên thư mục chứa các file build trong thư mục app/build. Ví dụ: build file cài đặt exe chạy trên windown thì là app/build/windown/ctetris.msi. File exe sẽ cần kiểm tra thư viện và môi trường windown để tiến hành cài đặt thư viện cần thiết rồi cài đặt game trước khởi động trò chơi. Macos các cũng tương tự các file như ctetris.app (macos). Nếu build riêng lẻ từng module để kiểm thử thì moduldeName.platformTailName, ví dụ: gameStory.exe
  - Đưa về các file build.sh trong các modules về 1 một file duy nhất đặt ở app/ . File build.sh sẽ:
    + hỏi build đơn lẻ từng module hay tích hợp toàn bộ.
    + hỏi build trên nền tảng nào? windown và macos
    + kiểm tra môi trường hệ điều hành hiện tại đã cài cmake đúng version chưa? nếu chưa có hoặc chưa đúng version sẽ hỏi để gỡ bỏ một cách sạch sẽ và cài lại
    + kiểm tra môi trường hệ điều hành hiện tại đã cài sfml version được thống nhất trong CMakeLists.txt chưa? nếu chưa có hoặc chưa đúng version sẽ hỏi để gỡ bỏ một cách sạch sẽ và cài lại cho môi trường hệ điều hành hiện tại
    + kiểm tra môi trường hệ điều hành hiện tại đã cài các thư viện và gói cài đặt cần thiết để build file cho nền tảng đã chọn chưa? nếu chưa có hoặc chưa đúng version sẽ hỏi để gỡ bỏ một cách sạch sẽ và cài lại.
[ ] Task 2.1: viết v2 gameCore/app.cpp - tốc độ rơi nhanh dần theo điểm số.
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-tang-do-kho-09
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trên 09
[ ] Task 2.2: viết v2 gameCore/app.cpp - nhạc nền trong suốt tiến trình
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-chen-nhac-nen-09
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 08 và trên 10
[ ] Task 3.1: viết v3 gameCore/app.cpp - dự báo 3 khối
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-du-bao-ba-khoi-xep-hinh-lien-tiep-08
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 09 và trên 11
[ ] Task 3.2: viết v3 gameCore/app.cpp - các phím (pause/play)
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-pause-vs-play-11
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 10 và trên 12
    - Mô tả: nếu ấn phím enter, phím space hoặc click icon pause thì icon pause sẽ chuyển qua icon play và toàn bộ trò chơi tạm dừng (trừ nhạc nền). Khi ở trạng thái tạm dừng,  nếu ấn phím enter, phím space hoặc click icon play thì toàn bộ thoát trạng thái tạm dừng.
[ ] Task 3.3: viết v3 gameCore/app.cpp - nút quit
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-quit-button-12
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 11 và trên 13
    - Mô tả: nếu ấn phím esc hoặc click icon shutdown thì toàn bộ game vào trạng thái tạm dừng đồng thời màn hình game mờ tối để làm nổi bật lên 1 lightbox popup ở giữa có màu sắc và độ sáng cao hơn xung quanh bên ngoài, bên trong lightbox xác nhận lại việc thoát game. Nếu nhấn đồng ý thì thoát game , nếu nhấn huỷ thì trở về trang thái trước đó và chơi game tiép, có thể sử dụng phím tab để di chuyển giữa 2 lựa chọn thay vì click chuột.
[ ] Task 3.4: viết v3 gameCore/app.cpp - cho phép nhập tên khi game over
    - Comment codeblock này trong gameCore/app.cpp là:  gamecore-game-over-screen-13
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 12 và trên 14
    - Không cần xử lý lưu, nhập tên tổng kết điểm là xong, thoát game là mất data. Tại màn hình game over,  nên có thêm 1 nút restart, nút reset (để chuyển qua màn hình của game console)  và nút quit (để thoát game hoàng toàn).
[ ] Task 3.5: viết v3 gameCore/app.cpp - gọi API và gửi tên và điểm lên Mongo atlas
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tich-hop-backend-14
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí cuối cùng.
    - chỉ cần quan tâm bài toán ghi, truyền data vào api được là xong, chuyện có query data ra được leaderboard để hiển thị hay không là của bên gameConsole. Cho 1 thêm nút retry hoặc nút resync ở màn hình game over nếu ghi lên không thành công.