## Tasks:
### V1
[x] Task 1.1: viết v1 các file class/struct trong gameCore/include  
    - Định nghĩa các cấu trúc dữ liệu cơ bản như Point, Tetromino, GameState.  
[x] Task 1.2: viết v1 gameCore/app.cpp - tạo layout.h và build .exe đảm bảo màn hình 9:16 trước khi viết code tiếp  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-giao-dien-169-00  
[x] Task 1.3: viết v1 gameCore/app.cpp - tạo 5 khối hình học cơ bản (L, I, T, Z, O)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tao-cac-khoi-xep-hinh-LITZO-01  
[x] Task 1.4: viết v1 gameCore/app.cpp - đổ màu ngẫu nhiên (Blue, Red, Green, Yellow, Orange, Purple)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-do-mau-02  
[x] Task 1.5: viết v1 gameCore/app.cpp - cho khối rơi tự do ở 1 vị trí bất kỳ (on top)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-roi-03  
[x] Task 1.6: viết v1 gameCore/app.cpp - điều khiển khối bằng các mũi tên và WASD  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-phim-04  
[x] Task 1.7: viết v1 gameCore/app.cpp - hiệu ứng chạm và dừng rơi khối  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-cham-05  
[x] Task 1.8: viết v1 gameCore/app.cpp - hiệu ứng xoá dòng  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-xu-ly-xoa-dong-06  
[x] Task 1.9: viết v1 gameCore/app.cpp - tính điểm với quy tắc định dạng 5 chữ số  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-tinh-diem-07  
[x] Task 1.10: viết v1 gameCore/app.cpp - xây dựng Sidebar UI và Procedural Icons  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-sidebar-ui-08  
[x] Task 1.11: viết v1 gameCore/app.cpp - tương tác UI bằng chuột (Click & Hold)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-mouse-interaction-09  
[x] Task 1.12: viết v1 gameCore/app.cpp - tính năng rơi nhanh (Soft Drop)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-soft-drop-10  
[x] Task 1.13: viết v1 gameCore/app.cpp - đồng hồ tính giờ chơi (Play Timer)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-play-timer-11  
[x] Task 1.14: viết v1 gameCore/app.cpp - hiển thị trước 1 khối tiếp theo (Preview Next Block)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-preview-next-block-12  
[x] Task 1.15: viết v1 gameCore/app.cpp - tính năng Pause / Play  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-pause-vs-play-13  
[x] Task 1.16: viết v1 gameCore/app.cpp - popup Quit / Game Over (Lightbox)  
    - Comment codeblock này trong gameCore/app.cpp là: gamecore-popup-quit-14  
[x] Task 1.17: tích hợp v1 với các modules khác trong app/src  
    - Comment codeblock này trong gameCore/app.cpp là: integration/v1  

### V2
[x] Task P1: mở rộng SettingsConfig trong gameConsole_layout.h
[x] Task P2: implement dbInsertRecord() trong gameConsole/app.cpp
[x] Task P3: implement dbUpsertStoryProgress() trong gameConsole/app.cpp
[x] Task P4: gameConsole populate cfg.nextBlockScore/nextBlockSpeed/tableMatrix khi chọn story
[x] Task 2.1: viết v2 gameCore/app.cpp - nhạc nền trong suốt tiến trình
    - Comment: gamecore-chen-nhac-nen-15
[x] Task 2.2: viết v2 gameCore/app.cpp - tốc độ rơi nhanh dần theo điểm số
    - Comment: gamecore-tang-do-kho-16
[x] Task 2.3: viết v2 gameCore/app.cpp - dự báo 3 khối liên tiếp với góc xoay ngẫu nhiên
    - Comment: gamecore-du-bao-ba-khoi-xep-hinh-lien-tiep-17
[x] Task 2.4: viết v2 gameCore/app.cpp - hiệu ứng chớp tắt khi xoá dòng
    - Comment: gamecore-hieu-ung-khi-xoa-dong-18
[x] Task 2.5: viết v2 gameCore/app.cpp - tăng điểm thưởng khi xoá nhiều dòng cùng thời điểm
    - Comment: gamecore-diem-thuong-khi-xoa-nhieu-dong-19
[x] Task 2.6: viết v2 gameCore/app.cpp - dùng cfg.colorEnabled[] để chọn màu khối
    - Comment: gamecore-color-palette-cfg-20
[x] Task 2.7: viết v2 gameCore/app.cpp - tableMatrix pre-populate board
    - Comment: gamecore-table-matrix-21
[x] Task 2.8: viết v2 gameCore/app.cpp - ghi idUser_Records khi game over
    - Comment: gamecore-save-record-22
[x] Task 2.9: viết v2 gameCore/app.cpp - cập nhật idUser_Stories sau game over
    - Comment: gamecore-update-story-progress-23
[x] Task 2.10: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - Comment: integration/v2

### V3
[x] Task 3.1: Tự động đối chiếu thành tích Game Over (hoặc restart / quit)
    - Comment: gamecore-game-over-screen-24
    - onGameOver() queries MAX(total_score) FROM sync_Records before inserting.
    - If session score > max: state.isNewRecord = true.
    - drawQuitPopup() shows gold title + "* NEW RECORD! Sync via Board." hint at y+120.
    - isNewRecord reset to false in resetGame() so retry sessions start clean.

[x] Task 3.2: Tích hợp V3
    - Comment: integration/v3 in gameCore/app.cpp.
    - Flow verified: Game Over → onGameOver() → new-record flag → popup hint.

## Issues (Changes, Bugs and Usabilities)
### V1
[x] Issue 1.1: wasmShutdown flag + RELOAD_BTN screen.
[x] Issue 1.2: 5 shapes mirror flip 50%.
[x] Issue 1.3: "%05d" cap 99999.
[x] Issue 1.4: "Total time: HH:MM:SS" in popup.
[x] Issue 1.5: SOFT_WHITE + BLOCK_PAD=1.
[x] Issue 1.6: swipe gesture threshold 15px.

### V2
[x] Issue 2.1–2.8: all resolved (see taskCore.md V2 section above).

### V3
[x] Issue 3.1: isNewRecord reset on resetGame() — verified.

## Rules:
    - Chỉ có 1 file c++ (app/src/gameCore/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameCore/include).
    - DB lifecycle: gameCore KHÔNG sở hữu DB connection. Gọi dbOpen → work → dbClose trong cùng 1 transaction block khi game over.
