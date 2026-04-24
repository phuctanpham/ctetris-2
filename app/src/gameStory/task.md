## Tasks:
[x] Task 1.1: viết v1 gameStory/app.cpp - tạo layout.h và file build trên macOS đảm bảo màn hình 9:16 trước khi viết code tiếp
    - Comment codeblock này trong gamestory/app.cpp là: gameconsole-tao-giao-dien-169-00
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và cài đặt thư viện cần thiét
[x] Task 1.2: viết v1 gameStory/app.cpp - làm hiển thị logo UIT có hiệu ứng tự chọn hiển thị và âm thanh
    - Comment codeblock này trong gamestory/app.cpp là: gameconsole-logo-intro-01
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau 00 và trước 02
[x] Task 1.3: viết v1 gameStory/app.cpp - tạo loading bar theo theo thời gian hiệu ứng của logo
    - Comment codeblock này trong gamestory/app.cpp là: gamestory-loading-bar-02
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau 01 và trước 03
[x] Task 1.4: tích hợp v1 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameStory/app.cpp là:  integration/v1
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí trên 00
    - Khơi động chương trình luôn bắt đầu bằng màn hình của gameStory.
    - Sau khi kết thúc màn hình của phần gameStory sẽ chuyển tiếp qua khởi động màn hình gameConsole
    - Sau khi bấm nút start bên màn hình gameConsole chuyển tiếp qua khởi động màn hình gameCore
    - Đưa về các file CMakeLists.txt để hỗ trợ build wasm từ macOS / Ubuntu / Window. 
    - viết và kiểm tra build.sh và build.ps1 để:
        + hỏi build đơn lẻ từng module hay tích hợp toàn bộ.
        + với build.sh tự xác định hệ điều hành  (macos hay ubuntu) , build.ps1 thì mặc định là windown rồi chạy chạy các cài đặt tiếp 
        + kiểm tra môi trường hệ điều hành hiện tại đã cài các công cụ cần thiết đúng version trên hệ điều hành chưa? nếu chưa hoặc thấp hơn version tối thiếu thì dừng script yêu cầu cài đặt.
        + nếu các công cụ cài đặt đầy đủ thì tiến hành build và để trong build/wasm, kết thúc có hướng dẫn cách chạy trên file build trên local host.
[ ] Task 2.1: viết v2 gameStory/app.cpp - tạo dialogue story đơn giản để giới thiệu game kèm nhạc nền phù hợp
    - Comment codeblock này trong gamestory/app.cpp là: gamestory-phan-cot-game-03
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau 02 và trước 04
    - Cần hỗ trợ phím enter và space thay vì chỉ click next để chuyển tiếp cảnh
    - Cần đa dạng chuyển cảnh, ví dụ làm 2 route đơn giản cho kịch bản truyện, chọn A thì ra phân cảnh khác khác, chọn B thì ra phân cảnh khác. Hỗ trợ phím tab để chuyển đổi các option.
[ ] Task 2.2:  viết v2 gameStory/app.cpp - nút skip để bỏ qua phần cốt truyện
    - Comment codeblock này trong gamestory/app.cpp là: gamestory-nut-bo-qua-cot-truyen-04
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí trước 03 và sau 05
    - Sau khi skip chuyển qua nhịp nhàng phần game console.
[ ] Task 3.1: viết v3 gameStory/app.cpp - download âm thanh và các hình ảnh trong story mỗi lần khởi động qua API thay vì build trực tiếp vào .exe file
    - Comment codeblock này trong gamestory/app.cpp là: gamestory-tich-hop-backend-04
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau 04 và trước 06
    - Trừ loading bar, Không để các file ảnh nặng và nhạc nền trong file .exe nữa mà sẽ phải gọi API để download các hình ảnh rồi mới chạy được phần cốt game
    - Tự động skip nếu không kêt nối internet.
[ ] Task 3.2: viết v3 gameStory/app.cpp - thay tốc độ loading bar bằng tốc độ download và repeat hiệu ứng logo cho đến khi download xong hết
    - Comment codeblock này trong gamestory/app.cpp là: gamestory-hieu-chinh-loading-bar-theo-download-speed-06
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí cuối cùng
    - Như tên task, tác vụ này nhằm đảm bảo cốt chuyện đầy đủ rồi mới chạy
    - Mục tiêu đưa các media lên đám mây để lưu trữ không build trực tiếp vào file
## Rules:
    - Chỉ có 1 file c++ (app/src/gameStory/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameStory/include).
    - Cần tách 1 file layout.h (app/src/gameStory/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16 trên windown.
    - Cần tạo thành công build file từ app.cpp để chạy trên macos và ubuntu.
    - Các file hình ảnh, âm thanh, phim ... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameStory thì phải để trong app/src/gameStory/gameStory_music.mp3