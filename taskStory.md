## Tasks:
### V1
[x] Task 1.1: viết v1 gameStory/app.cpp - tạo layout.h đảm bảo màn hình 9:16 và thiết lập chế độ BUILD_STANDALONE để test độc lập module trên macOS.  
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-tao-giao-dien-169-00  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí đầu tiên sau các khai báo và trên 01.  

[x] Task 1.2: viết v1 gameStory/app.cpp - tích hợp thư viện nanosvg để đọc/rasterize file SVG trực tiếp (không dùng ảnh bitmap) và áp dụng cơ chế Caching (Lazy Init) cho texture để tối ưu hiệu năng.  
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-xu-ly-svg-caching-01  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 00 và trên 02.  
    - Đặt ngay sau khai báo thư viện nanosvg và trước các logic UI chính.  

[x] Task 1.3: viết v1 gameStory/app.cpp - làm hiển thị logo UIT (dùng raw SVG) có hiệu ứng fade-in tự chọn, âm thanh và hiển thị tên game "C T E T R I S" ngay bên dưới logo.  
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-logo-intro-02  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 01 và trên 03.  

[x] Task 1.4: viết v1 gameStory/app.cpp - tạo loading bar theo thời gian hiệu ứng của logo.  
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-loading-bar-03  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 02 và trên 04.  

[x] Task 1.5: viết v1 gameStory/app.cpp - hiển thị dòng credit "Powered up by" kèm logo công ty phụ (corp logo) căn giữa, nằm ngay dưới thanh loading bar.  
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-corp-credit-04  
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 03 và trên 05.  

[x] Task 1.6: tích hợp v1 với các modules khác trong app/src  
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameStory/app.cpp là: integration/v1  
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
[ ] Task 2.1: viết v2 gameStory/app.cpp - tạo dialogue story đơn giản để giới thiệu game kèm nhạc nền phù hợp.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-phan-cot-game-05
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trên 06.
    - Cần hỗ trợ phím enter và space thay vì chỉ click next để chuyển tiếp cảnh.
    - Cần đa dạng chuyển cảnh, ví dụ làm 2 route đơn giản cho kịch bản truyện, chọn A thì ra phân cảnh khác, chọn B thì ra phân cảnh khác. Hỗ trợ phím tab để chuyển đổi các option.

[ ] Task 2.2: viết v2 gameStory/app.cpp - nút skip để bỏ qua phần cốt truyện.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-nut-bo-qua-cot-truyen-06
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05 và trên 07.
    - Sau khi skip chuyển qua nhịp nhàng phần game console.

[ ] Task 2.3: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameStory/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1

### V3
[ ] Task 3.1: viết v3 gameStory/app.cpp - download âm thanh và các hình ảnh trong story mỗi lần khởi động qua API thay vì build trực tiếp vào .exe file.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-tich-hop-backend-07
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trên 08.
    - Trừ loading bar, không để các file ảnh nặng và nhạc nền trong file .exe nữa mà sẽ phải gọi API để download các hình ảnh rồi mới chạy được phần cốt game.
    - Tự động skip nếu không kết nối internet.

[ ] Task 3.2: viết v3 gameStory/app.cpp - thay tốc độ loading bar bằng tốc độ download, hiển thị phần dowload và tốc độ download và repeat hiệu ứng logo cho đến khi download xong hết.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-hieu-chinh-loading-bar-theo-download-speed-08
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trên 09.
    - Như tên task, tác vụ này nhằm đảm bảo cốt truyện tải đầy đủ rồi mới chạy.
    - Mục tiêu đưa các media lên đám mây để lưu trữ, không build trực tiếp vào file.

[ ] Task 3.3: tích hợp v3 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameStory/app.cpp là: integration/v3
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v2

## Issues (Changes, Bugs and Usabilities)
### V1
[x] Issue 1.1: Cơ chế tự động sinh file header SVG (gameStory_corp_svg.h, gameStory_logo_svg.h) thông qua build.sh.
    - Code thực tế đã nhúng cứng mã SVG (raw string literal) tại thời điểm compile-time thay vì load file vật lý lúc runtime.
    - Các file `gameStory_logo.svg` và `gameStory_corp.svg` đã bị xóa để thực thi chiến lược không phát sinh file hình ảnh khi build.
[x] Issue 1.2: Bổ sung ký tự Copyright (©) trên thanh tiêu đề của hệ điều hành.
    - Tên cửa sổ được ghép mã UTF-8 `\xC2\xA9` (thành "cTetris ©" và "Game Story © - Standalone").
    - Ký tự này được đẩy cho window manager của hệ điều hành xử lý trực tiếp thay vì vẽ bằng SDL_RenderDebugText (do giới hạn chỉ hỗ trợ bảng mã ASCII).
[x] Issue 1.3: Tinh chỉnh độ dày font chữ (Thinning Effect) bằng màu sắc.
    - Text hiển thị tên game ("C T E T R I S") và credit ("Powered up by ") sử dụng màu xám nhạt (RGB: 220, 220, 220) thay vì màu trắng (255, 255, 255).
    - Kỹ thuật này được áp dụng để tạo cảm giác nét chữ "mỏng" và "nhẹ" hơn khi render bằng font mặc định của hệ thống SDL.
[x] Issue 1.4: Tăng thời lượng Loading Bar (INTRO_DURATION) lên 8 giây.
    - Hằng số INTRO_DURATION được thiết lập là 8000ms thay vì 3000ms như dự kiến ban đầu.
    - Sự thay đổi này nhằm đáp ứng thời gian hiển thị cốt truyện dài hơn, đồng thời đảm bảo đủ thời gian chờ tải assets/network khi chạy trên môi trường WASM.
[x] Issue 1.5: Sai lệch vai trò giữa Logo Game và Logo UIT so với Task 1.3.
    - Task 1.3 ghi nhận Logo UIT là logo chính ở giữa màn hình.
    - Tuy nhiên, mã nguồn thiết lập `LOGO_SVG_DATA` (logo game cTetris - 3x3 ô vuông) làm logo chính cho hiệu ứng intro, trong khi `CORP_SVG_DATA` (logo trường đại học UIT) bị đẩy xuống làm logo phụ đính kèm ở phần credit "Powered up by".

### V2
[ ] bổ sung sau

### V3
[ ] bổ sung sau

## Rules:
    - Chỉ có 1 file c++ (app/src/gameStory/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameStory/include).
    - Cần tách 1 file layout.h (app/src/gameStory/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16.
    - Các file hình ảnh, âm thanh, phim... phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameStory thì phải để trong app/src/gameStory/gameStory_music.mp3.