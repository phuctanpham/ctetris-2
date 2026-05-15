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
[x] Task 2.1: Tổ chức lại cấu trúc thư mục theo chuẩn mới.
    - Di chuyển nội dung chapter ra ngoài thư mục `app/`, đặt `chapters/` cùng cấp với `app/`.
    - Cấu trúc thư mục chuẩn sau khi hoàn thành:
      ```
      /
      ├── app/
      └── chapters/
          ├── manifest.json        ← auto-generated bởi CI, không commit tay
          ├── prompts/
          │   ├── json.md          ← prompt định nghĩa cấu trúc JSON và quy tắc thư mục src
          │   ├── c001.md          ← prompt tuyến truyện, nhân vật, hội thoại, hình ảnh, âm thanh
          │   └── c002.md
          └── src/
              ├── c001/
              │   ├── c001.json
              │   └── media/
              │       ├── intro_scene.png
              │       └── opening_bgm.mp3
              └── c002/
                  ├── c002.json
                  └── media/
      ```
    - Cập nhật toàn bộ đường dẫn liên quan trong logic xử lý của module gameStory.
    - Cập nhật CMakeLists.txt và build script để phản ánh đường dẫn mới.

[ ] Task 2.2: Thiết lập quy chuẩn nội dung chapter và quy trình CI/CD qua GitHub Actions.

    **Cấu trúc file trong mỗi chapter:**
    - `chapters/prompts/json.md`: Prompt dùng cho GenAI để định nghĩa cấu trúc file JSON và quy tắc đặt tên trong thư mục src. File này là nguồn tham chiếu chính khi tạo chapter mới.
    - `chapters/prompts/c{id}.md`: Prompt tuyến truyện của từng chapter — nhân vật, hội thoại, lựa chọn, mô tả hình ảnh và âm thanh.
    - `chapters/src/c{id}/c{id}.json`: Dữ liệu gốc của chapter, sinh ra từ prompt. Cấu trúc bắt buộc gồm object gốc chứa mảng `shared_data`.
    - `chapters/src/c{id}/media/`: Chứa tất cả media của chapter. **Tên file không chứa ID chapter hay ID story** — đặt tên theo nội dung gợi nhớ (ví dụ: `intro_scene.png`, `battle_bgm.mp3`, `hero_enter_sfx.mp3`). Phân biệt âm thanh nền và hiệu ứng qua suffix `_bgm` và `_sfx`.

    **Cấu trúc JSON mỗi chapter (`c{id}.json`):**
    ```json
    {
      "shared_data": [
        {
          "idStory": 1,
          "thumbnail": "media/intro_scene.png",
          "dialogues": [
            {
              "id": 1,
              "speaker": "Narrator",
              "text": "...",
              "image": "media/scene_01.png",
              "sfx": "media/door_open_sfx.mp3",
              "bgm": "media/town_bgm.mp3",
              "next": 2
            },
            {
              "id": 2,
              "speaker": "Hero",
              "text": "...",
              "image": "media/hero_speak.png",
              "choices": [
                { "label": "Đồng ý",   "next": 3 },
                { "label": "Từ chối",  "next": 5 }
              ]
            }
          ]
        }
      ]
    }
    ```
    - Trường `next` dùng cho thoại tuyến tính. Trường `choices` kích hoạt phân nhánh — không dùng cả hai cùng lúc trong một dialogue node.
    - Trường media (`image`, `sfx`, `bgm`) chứa đường dẫn tương đối từ gốc chapter, ví dụ: `"media/intro_scene.png"`.

    **Quy trình CI/CD (GitHub Actions — `sync-chapters.yml`):**
    - Trigger: push có thay đổi trong `chapters/src/**/*.json` hoặc `chapters/src/**/media/**`.
    - Bước 1 — Parse JSON thành SQL: chạy `parse.py`, sinh file SQL với câu lệnh `INSERT OR REPLACE INTO dialogues(...)`. URL media được chuyển thành raw GitHub URL đầy đủ: `https://raw.githubusercontent.com/{owner}/{repo}/main/chapters/src/c{id}/media/{filename}`.
    - Bước 2 — Rebuild `chapters/manifest.json`: lấy git commit SHA của từng file `c{id}.json` (dùng `git log -1 --format=%H -- chapters/src/c{id}/c{id}.json`), ghi vào manifest.
    - Bước 3 — Commit lại: commit toàn bộ file SQL và manifest.json được sinh ra vào repo với message `[skip ci] sync chapters`. Dùng điều kiện `if: github.actor != 'github-actions[bot]'` để chống vòng lặp vô hạn.
    - Cấu trúc `chapters/manifest.json` sau khi CI chạy:
      ```json
      {
        "chapters": [
          { "id": "c001", "sha": "a1b2c3d4..." },
          { "id": "c002", "sha": "e5f6g7h8..." }
        ]
      }
      ```

[x] Task 2.3: Viết v2 gameStory/app.cpp — Cơ chế đồng bộ CSDL SQLite từ xa dùng manifest.json.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-dong-bo-sqlite-05a
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trước gamestory-phan-cot-game-05b.

    **Schema bảng meta trong SQLite local:**
    ```sql
    CREATE TABLE IF NOT EXISTS meta (
        chapter_id TEXT PRIMARY KEY,
        sha        TEXT NOT NULL,
        updated_at INTEGER
    );
    ```

    **Luồng xử lý khi khởi động module gameStory:**
    1. Dùng `libcurl` fetch `chapters/manifest.json` từ raw GitHub URL.
    2. Parse JSON manifest, duyệt từng entry `{ "id": "c001", "sha": "..." }`.
    3. Với mỗi chapter: đọc `sha` hiện tại trong bảng `meta` của SQLite local.
    4. Nếu SHA khớp → bỏ qua chapter đó.
    5. Nếu SHA mới hơn hoặc chapter chưa có trong local → fetch file SQL của chapter từ raw URL → chạy `sqlite3_exec()` vào DB local → cập nhật bảng `meta` với SHA mới.
    6. Nếu không có kết nối mạng (fetch manifest thất bại) → bỏ qua toàn bộ bước sync, dùng dữ liệu local đang có, tiếp tục khởi động bình thường.

    **Tách biệt Desktop và WASM:**
    - Desktop: dùng `libcurl` + file SQLite thông thường như mô tả trên.
    - WASM (`#ifdef __EMSCRIPTEN__`): không gọi `libcurl` trực tiếp. Thay vào đó bundle sẵn file SQL vào build output của Emscripten, hoặc dùng `emscripten_fetch` kết hợp IndexedDB. Tách logic này thành hàm riêng để dễ bảo trì.

[x] Task 2.4: viết v2 gameStory/app.cpp - tạo dialogue story engine để hiển thị cốt truyện kèm nhạc nền.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-phan-cot-game-05b
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05a và trên 06.
    - Đọc dữ liệu hội thoại từ SQLite local (đã được đồng bộ bởi Task 2.3).
    - Hỗ trợ phím Enter và Space để chuyển thoại tiếp theo (ngoài click chuột).
    - Hỗ trợ tuyến tính (`next`) và phân nhánh (`choices`). Khi có `choices`, dùng phím Tab để chuyển đổi giữa các lựa chọn, Enter/Space để xác nhận.
    - Mỗi dialogue node hiển thị: hình ảnh scene, tên nhân vật, nội dung thoại, nhạc nền nếu có `bgm`.

[x] Task 2.5: viết v2 gameStory/app.cpp - nút skip để bỏ qua phần cốt truyện.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-nut-bo-qua-cot-truyen-06
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05b và trên 07.
    - Sau khi skip, chuyển nhịp nhàng sang màn hình gameConsole.

[x] Task 2.6: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp.
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameStory/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1.

### V3
[ ] Task 3.1: viết v3 gameStory/app.cpp - Cơ chế tải file Media độc lập từ Raw URL (Trace theo Commit ID).
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-tich-hop-backend-07
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trên 08.
    - Hệ thống C++ đọc các URL media từ SQLite (đã được đồng bộ bởi V2), dùng `libcurl` để tải từng file về cache local.
    - Nếu file đã tồn tại trên disk với đúng tên → bỏ qua tải lại.
    - Tự động skip nếu không kết nối internet.

[ ] Task 3.2: viết v3 gameStory/app.cpp - thay tốc độ loading bar bằng tốc độ download, hiển thị phần download và tốc độ download và repeat hiệu ứng logo cho đến khi download xong hết.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-hieu-chinh-loading-bar-theo-download-speed-08
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trên 09.
    - Theo dõi tiến trình tải các file đơn lẻ (từ Task 3.1) và cập nhật thanh loading.
    - Đảm bảo cốt truyện và hình ảnh/âm thanh tải đầy đủ rồi mới bắt đầu chạy kịch bản.

[ ] Task 3.3: tích hợp v3 với các modules còn lại trong app/src qua file app/main.cpp.
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameStory/app.cpp là: integration/v3
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v2

---

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
[ ] Issue 2.1: Khởi tạo DB client khi chưa tồn tại — Prefix theo Device ID.
    - Nếu không tìm thấy file SQLite (`SDL_GetPrefPath("uit","cTetris")` + device-id prefix), tạo mới `{deviceId}.sqlite`.
    - Device ID = hash ổn định từ `SDL_GetPlatform()` + `SDL_GetBasePath()`.
    - Sau khi init, fetch `manifest.json` qua hằng số `MANIFEST_GIST_URL` → lấy danh sách gist link từng chapter → import tuần tự vào DB.
    - Story mặc định sau init: story có `idStory` nhỏ nhất của chapter có `idChapter` nhỏ nhất trong manifest.
    - I/O validation: xoá DB → chạy lại → DB mới tên chứa device-id, bảng `meta` có ít nhất 1 row sau sync.

[x] Issue 2.2: So sánh phiên bản manifest và cập nhật có chọn lọc — Flow A (DB đã tồn tại).
    - Fetch `manifest.json` gist → so sánh từng `{ "id", "sha" }` với bảng `meta` local.
    - SHA khớp → bỏ qua chapter. SHA khác → fetch `c{id}.json` từ gist tương ứng → parse JSON trong C++ (không qua `parse.py`) → diff từng object `idStory`:
        + `idStory` mới → INSERT vào `stories`, `dialogues`, `choices`.
        + `idStory` thay đổi → UPDATE các row tương ứng.
        + `idStory` bị xoá → DELETE các row tương ứng.
    - Sau mỗi chapter xử lý xong, UPDATE `meta.sha` và `meta.updated_at`.
    - I/O validation: sửa 1 dialogue trong `c001.json` → push gist → chạy lại → row `dialogues` đã update, chapter khác không bị chạm.

[ ] Issue 2.3: GitHub Action chỉ cập nhật Gist — KHÔNG commit vào git repo.
    - Workflow trigger: thay đổi `chapters/src/**/*.json`.
    - Với mỗi file thay đổi: publish JSON lên Gist tương ứng qua GitHub Gist API (dùng secret `GIST_TOKEN`).
    - Sau khi cập nhật tất cả chapter Gist, rebuild manifest Gist với `{ id, sha, gistUrl }` mới.
    - Không có bước `git add` / `git commit` nào trong workflow.
    - Điều kiện guard: `if: github.actor != 'github-actions[bot]'`.
    - I/O validation: push `c001.json` → workflow chạy → Gist có nội dung mới → manifest Gist có SHA mới → `git log` không có commit mới.

[ ] Issue 2.4: Stream ảnh trực tiếp từ URL — Không tải về disk.
    - Tất cả `image_url`, `sfx_url`, `bgm_url` trong DB là raw URL. Fetch in-memory → decode → `SDL_CreateTexture`.
    - Không ghi bất kỳ file media nào xuống FS (không có cache disk layer ở V2; V3 Task 3.1 sẽ thêm).
    - Offline fallback: fetch thất bại → thumbnail hiển thị hình chữ nhật xám `{80,80,80,255}`, không crash.
    - I/O validation: tắt mạng → chạy → thumbnail xám hiện, log "image offline fallback", không exception.

[ ] Issue 2.5: Progressive Bar hiển thị tiến trình sync thực tế.
    - Bar CHỈ hiển thị trong phase sync DB (state `STATE_SYNCING`), KHÔNG hiển thị trên màn hình dialogue.
    - Tính `percent = chaptersProcessed / totalChaptersToSync * 100`.
    - Nếu không có chapter nào cần sync (tất cả SHA khớp): bar điền 100% ngay, không delay.
    - Dòng trạng thái bên dưới bar: `"Syncing c{id}... ({done}/{total})"`.
    - I/O validation: 3 chapter cần update → bar tăng qua 3 bước → chuyển tiếp sau 100%.

[ ] Issue 2.6: Button "Skip" CHỈ hiện trong Dialogue Screen — Không hiện khi đang sync/load.
    - `SKIP_BTN` không render và không nhận event trong state `STATE_SYNCING` / `STATE_LOADING`.
    - Skip story hiện tại → chỉ kết thúc dialogue của story đó; nếu chapter flow còn story kế tiếp đã activated → mở dialogue story đó (Skip button hiện lại).
    - Hết story trong flow → chuyển sang gameConsole.
    - I/O validation: đang sync → Skip button ẩn; vào dialogue story 1 → Skip hiện; click → story 2 bắt đầu với Skip; skip story 2 → gameConsole.

[ ] Issue 2.7: Chapter Flow — Story đầu tiên không có điều kiện, các story sau unlock tuần tự.
    - Story đầu tiên của mỗi chapter (`idStory` nhỏ nhất) luôn có `requiredStories = NULL`.
    - Story kế tiếp chỉ chạy khi story trước trong cùng chapter đã có `isActivated=1`.
    - Story có `requiredStories` tham chiếu cross-chapter: chỉ kiểm tra khi đang ở đúng chapter đó; không chạy cross-chapter story nếu chapter hiện tại không phải chapter của story đó.
    - Tất cả story trong chapter hoàn thành → hiển thị `"Chapter Completion: {chapterName}"` → chuyển gameConsole.
    - I/O validation: story 1 xong → story 2 tự chạy; story 2 có cross-chapter requirement chưa đủ → bị bỏ qua → chapter completion hiện.

[ ] Issue 2.8: Kiểm tra điều kiện sau Progressive Bar — Chạy lại hoặc mở story mới.
    - Sau sync 100%: đọc `idUser_Stories` + `idUser_Records` để lấy `lastMaxScore`, `lastMaxSpeed` của story `isSelected=1`.
    - So sánh với `minScore`, `minSpeed` của story kế tiếp trong chapter flow:
        + Chưa đủ điều kiện → chạy lại dialogue của story hiện tại.
        + Đủ điều kiện → hiển thị prompt `"Bạn đã mở ra {nextStoryName}! Tiếp tục?"` → Yes: chạy dialogue mới; No: chạy lại story cũ.
    - Tất cả story chapter completed → `"Chapter Completion: {chapterName}"`.
    - I/O validation: `lastMaxScore=60`, `minScore` story kế tiếp=50 → prompt mở story mới hiện; chọn No → dialogue story cũ chạy.

[ ] Issue 2.9: Cập nhật DB khi user xác nhận mở story mới.
    - Sau khi user chọn "Yes" trên prompt: `UPDATE idUser_Stories SET isSelected=0` story cũ, `isSelected=1` và `isActivated=1` story mới.
    - WASM: `EM_ASM({ Module.FS.syncfs(false, function(){}); })` ngay sau UPDATE.
    - I/O validation: confirm Yes → query `idUser_Stories` → story mới `isSelected=1`, story cũ `isSelected=0`.

[ ] Issue 2.10: DB device-id namespacing and initialization behavior.
    - Current: `storyDbOpen()` opens `default.sqlite` under `SDL_GetPrefPath` and does not namespace DB filename by device id.
    - Desired V2 behavior: create `{deviceId}.sqlite` where `deviceId` is stable hash of platform+basepath. Also, initial sync should seed story selection from manifest-derived data.
    - Task: implement device-id prefixing, ensure `main.cpp` / story code uses same pref path and device-id when opening DB, and update tests for I/O validation.

[ ] Issue 2.11: In-memory media streaming (images/bgm/sfx) — no disk cache.
    - Current: dialogue image area is a placeholder; BGM is a stub logging URL but not played.
    - Desired V2 behavior: fetch image/audio from `media_base_url` or raw URL, decode in-memory and create `SDL_Texture` / audio stream on-the-fly without writing to disk. Provide graceful offline fallback.
    - Task: implement async/native fetch + texture/audio creation, timeouts, and placeholder fallback.

[ ] Issue 2.12: Skip-button visibility differs from spec.
    - Current: Skip button is hidden during sync (correct) but visible during logo intro and dialogue. Spec requires Skip only in Dialogue Screen.
    - Task: change visibility to only render and accept Skip events during dialogue phase; ensure logo phase skip behavior is handled via explicit logo-skip config or UX decision.

[ ] Issue 2.13: `requiredStories` unlock logic not enforced.
    - Current: `postSyncConditionCheck()` only checks `minScore` / `minSpeed` of the next story; it does not evaluate `requiredStories` (cross-story prerequisites).
    - Task: implement parsing and enforcement of `requiredStories` (comma-separated ids), including cross-chapter references per spec, and unit tests for unlock flows.

### V3
[ ] bổ sung sau

---

## Hướng dẫn thực hiện V2

### Thứ tự triển khai khuyến nghị

Thực hiện theo đúng thứ tự sau để tránh phụ thuộc ngược:

```
2.1 (thư mục) → json.md schema → 2.2 (CI/CD) → 2.3 Desktop → 2.4/2.5/2.6 → 2.3 WASM
```

Lý do: CI/CD cần cấu trúc thư mục ổn định (2.1) trước khi viết workflow. Schema JSON cần được định nghĩa trước khi viết `parse.py` trong CI. Runtime sync Desktop (2.3) cần SQLite có dữ liệu mới viết được dialogue engine (2.4). WASM là nhánh riêng, để cuối cùng.

### Kiến trúc dữ liệu

**`chapters/manifest.json`** là điểm kết nối duy nhất giữa CI và runtime. CI ghi, runtime đọc. Không có thành phần nào khác cần biết về nhau.

```
CI pipeline                    Runtime C++
──────────────────────         ──────────────────────
push c001.json          →      fetch manifest.json
  ↓                              ↓
parse.py chạy           →      diff vs meta table
  ↓                              ↓
sinh c001.sql           →      download c001.sql nếu SHA mới
  ↓                              ↓
cập nhật manifest.json  →      sqlite3_exec + update meta
  ↓                              ↓
commit [skip ci]        →      start game story
```

### Thiết kế bảng SQLite

Ngoài bảng `meta` (tracking phiên bản), cần tạo bảng `dialogues` để lưu nội dung hội thoại:

```sql
CREATE TABLE IF NOT EXISTS meta (
    chapter_id TEXT PRIMARY KEY,
    sha        TEXT NOT NULL,
    updated_at INTEGER
);

CREATE TABLE IF NOT EXISTS stories (
    id         INTEGER PRIMARY KEY,
    chapter_id TEXT NOT NULL,
    id_story   INTEGER NOT NULL,
    thumbnail  TEXT
);

CREATE TABLE IF NOT EXISTS dialogues (
    id         INTEGER PRIMARY KEY,
    chapter_id TEXT NOT NULL,
    id_story   INTEGER NOT NULL,
    id_dialogue INTEGER NOT NULL,
    speaker    TEXT,
    text       TEXT,
    image_url  TEXT,
    sfx_url    TEXT,
    bgm_url    TEXT,
    next_id    INTEGER,
    has_choices INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS choices (
    id          INTEGER PRIMARY KEY,
    dialogue_id INTEGER NOT NULL,
    label       TEXT,
    next_id     INTEGER
);
```

### Lưu ý kỹ thuật quan trọng

**CI/CD — chống infinite loop:**  
Workflow commit lại repo sẽ tự trigger chính nó. Dùng điều kiện sau trong job:
```yaml
if: github.actor != 'github-actions[bot]'
```
hoặc kiểm tra message commit:
```yaml
if: "!contains(github.event.head_commit.message, '[skip ci]')"
```

**Raw URL format cho media:**  
```
https://raw.githubusercontent.com/{owner}/{repo}/main/chapters/src/{chapterId}/media/{filename}
```
URL này stable miễn là file không bị đổi tên. `parse.py` sẽ tự động ghép URL đầy đủ khi sinh SQL.

**WASM — không dùng libcurl trực tiếp:**  
Emscripten không hỗ trợ `libcurl` theo cách thông thường do CORS và sandbox. Tách biệt bằng:
```cpp
#ifdef __EMSCRIPTEN__
    // Bundle SQL sẵn vào build, hoặc dùng emscripten_fetch
    syncChapters_WASM();
#else
    // Desktop: libcurl + sqlite file thông thường
    syncChapters_Desktop();
#endif
```

**Dialogue engine — state machine tối giản:**  
Trạng thái runtime chỉ cần lưu `currentDialogueId` (integer). Mỗi frame: đọc row từ SQLite theo ID, render, chờ input. Khi có `choices`, render danh sách options và track `selectedChoiceIndex`.

### Kiểm tra hoàn thành V2

Trước khi đánh dấu V2 done, xác nhận:
- [ ] `chapters/manifest.json` được CI tự động tạo khi push JSON mới
- [ ] Thêm `c003/` mới vào repo, runtime tự phát hiện và tải về khi khởi động
- [ ] Sửa nội dung `c001.json`, runtime nhận SHA mới và cập nhật DB
- [ ] Mất kết nối mạng → app vẫn chạy bình thường với dữ liệu local
- [ ] Dialogue hiển thị đúng, phân nhánh `choices` hoạt động, Tab/Enter/Space phản hồi đúng
- [ ] Nút skip chuyển cảnh mượt sang gameConsole

---

## Rules:
    - Chỉ có 1 file c++ (app/src/gameStory/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameStory/include).
    - Cần tách 1 file layout.h (app/src/gameStory/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16.
    - Các file hình ảnh, âm thanh, phim... của module app phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục. VD: cần thêm 1 file nhạc nền tên music.mp3 cho gameStory thì phải để trong app/src/gameStory/gameStory_music.mp3.
    - Nội dung chapter (JSON, media) được đặt trong `chapters/src/c{id}/`, không nằm trong thư mục `app/`. Tên file media không chứa ID chapter hay ID story — đặt tên theo nội dung gợi nhớ kèm suffix `_sfx` hoặc `_bgm` để phân biệt loại âm thanh.
    - File `chapters/manifest.json` và các file `*.sql` được tự động sinh bởi CI. Không commit hoặc chỉnh sửa tay các file này.
    - File `chapters/prompts/json.md` là nguồn tham chiếu duy nhất cho cấu trúc JSON và quy tắc thư mục. Khi thay đổi schema, cập nhật file này trước, sau đó mới chỉnh `parse.py` và code C++.
