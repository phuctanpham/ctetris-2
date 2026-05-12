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
[ ] Task 2.1: Tổ chức lại cấu trúc thư mục dự án, di chuyển thư mục "chapter" ra ngoài "app" để tách biệt nội dung (content) và mã nguồn (source code).
    - Đảm bảo cấu trúc phân cấp mới: `/chapter` nằm cùng cấp với `/app`.
    - Cập nhật các đường dẫn liên quan trong logic xử lý của module gameStory.

[ ] Task 2.2: Thiết lập quy chuẩn thư mục nội dung chương, cấu trúc dữ liệu và quy trình CI/CD qua GitHub.
    - **Cấu trúc thư mục chuẩn**: Mỗi chương mới phải được đặt trong một thư mục con riêng biệt theo cú pháp `chapter/c{chapterID}` (Ví dụ: `chapter/c001`).
    - **Quy tắc các file văn bản và dữ liệu**:
        - **File Markdown (`c{chapterID}.md`)**: Đóng vai trò là Prompt cho GenAI để thiết kế cốt truyện, sinh lời thoại và prompt hình ảnh.
        - **File JSON (`c{chapterID}.json`)**: Nguồn dữ liệu gốc. Cấu trúc bắt buộc có object gốc chứa mảng `shared_data`.
        - **File SQL (`c{chapterID}.sql`)**: File đầu ra được tự động generate từ JSON.
    - **Quy tắc đặt tên file Media**:
        - Ảnh Thumbnail: `{chapterID}_{idStory}_{Tên_Gợi_Nhớ}.png`.
        - Ảnh Dialogue: `{chapterID}_s{idStory}_d{dialogueID}_{Tên_Gợi_Nhớ}.png`.
        - Âm thanh hiệu ứng Dialogue: `c{chapterID}_s{idStory}_d{dialogueID}_{Tên_Gợi_Nhớ}.mp3`.
        - Âm thanh nền Dialogue: `c{chapterID}_s{idStory}_d{dialogueID}_{Tên_Gợi_Nhớ}.mp3`.
    - **Quy trình CI/CD (GitHub Action)**: 
        - Kích hoạt khi file `.json` hoặc file media thay đổi.
        - Tự động lấy **mã Commit ID mới nhất** của từng file đó.
        - Chạy script parse JSON thành SQL (`INSERT OR REPLACE INTO...`). **Quan trọng**: Đổi tên file media theo commit ID và cấu hình lại URL trong file SQL để ứng dụng tải trực tiếp bằng raw link.
          *Ví dụ URL: `https://raw.githubusercontent.com/{owner}/{repo}/chapters/tênFile-commitID.mp3` (hoặc `.png`)*
        - Upload file SQL kết quả này lên thành `https://raw.githubusercontent.com/{owner}/{repo}/chapters/c{chapterID}-commitID.sql` (Ví dụ: `c001-commitID.sql`).

[ ] Task 2.3: Viết v2 gameStory/app.cpp - Xây dựng cơ chế đồng bộ CSDL SQL từ xa.
    - Khi module gameStory khởi động, dùng `curl` gọi GitHub API để lấy Commit ID mới nhất của chương.
    - Truy vết Commit ID hiện tại trong SQLite cục bộ.
    - Nếu Commit ID trên server mới hơn, tự động tải trực tiếp file `c{chapterID}-commitID.sql` từ Raw URL.
    - Chạy thẳng file SQL vừa tải vào SQLite cục bộ để cập nhật dữ liệu.

[ ] Task 2.4: viết v2 gameStory/app.cpp - tạo dialogue story đơn giản để giới thiệu game kèm nhạc nền phù hợp.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-phan-cot-game-05
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 04 và trên 06.
    - Cần hỗ trợ phím enter và space thay vì chỉ click next để chuyển tiếp cảnh.
    - Cần đa dạng chuyển cảnh, ví dụ làm 2 route đơn giản cho kịch bản truyện. Hỗ trợ phím tab để chuyển đổi các option.

[ ] Task 2.5: viết v2 gameStory/app.cpp - nút skip để bỏ qua phần cốt truyện.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-nut-bo-qua-cot-truyen-06
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 05 và trên 07.
    - Sau khi skip chuyển qua nhịp nhàng phần game console.

[ ] Task 2.6: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp
    - Nếu có viết thêm để hỗ trợ tích hợp, Comment codeblock này trong gameStory/app.cpp là: integration/v2
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau codeblock của integration/v1

### V3
[ ] Task 3.1: viết v3 gameStory/app.cpp - Cơ chế tải file Media độc lập từ Raw URL (Trace theo Commit ID).
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-tich-hop-backend-07
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 06 và trên 08.
    - Hệ thống C++ sẽ đọc trực tiếp CSDL SQLite (đã có đường dẫn dạng `https://raw.githubusercontent.com/{owner}/{repo}/chapters/tênFile-commitID.mp3`).
    - Ứng dụng duyệt qua các URL này, dùng `curl` để tải từng file đơn lẻ lưu vào cache cục bộ. Nếu file đã tồn tại trên disk với đúng định dạng tên `tênFile-commitID`, bỏ qua tải lại.
    - Tự động skip nếu không kết nối internet.

[ ] Task 3.2: viết v3 gameStory/app.cpp - thay tốc độ loading bar bằng tốc độ download, hiển thị phần dowload và tốc độ download và repeat hiệu ứng logo cho đến khi download xong hết.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-hieu-chinh-loading-bar-theo-download-speed-08
    - Đặt thứ tự codeblock này từ trên xuống ở vị trí sau 07 và trên 09.
    - Theo dõi tiến trình tải các file đơn lẻ (từ Task 3.1) và cập nhật thanh loading.
    - Đảm bảo cốt truyện và hình ảnh/âm thanh tải đầy đủ rồi mới bắt đầu chạy kịch bản.

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

# gameStory V2/V3 — Revised TODO (per latest taskStory.md)

## Key changes I caught in the new version

| Old plan I made | New spec |
|---|---|
| `chapters/c001/c001.{md,json,sql}` flat | `chapters/prompts/{json.md, c001.md}` + `chapters/src/c001/{c001.json, media/}` + `chapters/manifest.json` |
| Direct GitHub commit-API call per chapter | Single `manifest.json` (CI-rebuilt) as the only sync touchpoint |
| Reuse gameConsole's `shared_data.xmlDialogue` | **New normalized schema**: `meta` + `stories` + `dialogues` + `choices` |
| Codeblock `gamestory-dong-bo-sqlite-05` | `gamestory-dong-bo-sqlite-05a` |
| Codeblock `gamestory-phan-cot-game-05` | `gamestory-phan-cot-game-05b` |
| Workflow `chapters-build.yml` | `sync-chapters.yml` with `[skip ci]` loop guard |
| Media filenames include chapter/story IDs | **Memorable names only** + `_bgm`/`_sfx` suffix |

## Conflict with existing gameConsole code (flag now, fix at integration v2)

- `gameConsole/app.cpp::dbSeedSharedData()` reads `chapters/c001/c001.sql` (OLD path) and writes `shared_data` columns (`xmlDialogue`, `tableMatrix`, `requiredStories`...) the new JSON schema **doesn't carry**.
- Resolution: not my job now (it's `integration/v2`), but my `parse.py` will emit SQL that **adds** to the new tables and **leaves `shared_data` alone**. Integration step later reconciles.

---

## Revised TODO

### Phase A — Task 2.1: Folder reorg
- **A.1** Create `chapters/prompts/json.md` (schema reference & directory rules — single source of truth)
- **A.2** Create `chapters/prompts/c001.md` (story prompt for chapter 1)
- **A.3** Create `chapters/src/c001/c001.json` (sample data: 1 story, 3-5 dialogue nodes incl. one `choices` branch)
- **A.4** Create `chapters/src/c001/media/.gitkeep` (placeholders ok)
- **A.5** Add `chapters/manifest.json` placeholder + `.gitignore` note (`# auto-generated by CI`)
- **A.6** Update `app/CMakeLists.txt` to no longer copy `c001.sql` directly (now handled by C.5 sync)
- **I/O check**: `ls chapters/`, parse `c001.json` with jq

### Phase B — Task 2.2: CI/CD
- **B.1** Create `chapters/scripts/parse.py` (JSON → SQL emitter, rewrites `media/x.png` → raw GitHub URL)
- **B.2** Create `chapters/scripts/build_manifest.py` (collects per-file `git log` SHA → manifest.json)
- **B.3** Create `.github/workflows/sync-chapters.yml` (trigger, parse, commit `[skip ci]`, loop-guarded)
- **I/O check**: run `python3 parse.py c001.json` locally, assert output contains expected `INSERT OR REPLACE INTO stories`, `dialogues`, `choices` statements

### Phase C — Task 2.3: Remote sync `gamestory-dong-bo-sqlite-05a`
- **C.1** Add `gameStory_db.h` (new file) declaring `dbInitStoryTables()` + 4-table schema
- **C.2** Implement `dbInitStoryTables()` in `gameStory/app.cpp` (creates `meta`, `stories`, `dialogues`, `choices` if absent — does NOT touch gameConsole's `shared_data`)
- **C.3** Create `app/src/shared/http.h` + `http_native.cpp` (libcurl) + `http_wasm.cpp` (`emscripten_fetch`)
- **C.4** Add libcurl detection to `build.sh` (apt/brew/pacman) and `build.ps1` (vcpkg or pre-built)
- **C.5** Implement `syncChaptersFromManifest()` in gameStory/app.cpp — fetch manifest → diff SHA → fetch+exec changed SQL → update `meta`
- **C.6** WASM split: `#ifdef __EMSCRIPTEN__` calls `syncChapters_WASM()` (async emscripten_fetch + IDBFS persist); else `syncChapters_Desktop()`
- **C.7** Offline fallback: any HTTP failure → log + skip, never block startup
- **I/O check**: native run with `--offline` flag → assert no crash, log shows "offline"; native online → assert `meta` table has 1 row after first sync

### Phase D — Task 2.4: Dialogue engine `gamestory-phan-cot-game-05b`
- **D.1** Add `loadDialogueByChapter(chapterId, storyId)` → returns `std::map<int, DialogueNode>` from SQLite
- **D.2** Define `DialogueNode { int id; string speaker; string text; string imageUrl; string bgmUrl; string sfxUrl; int nextId; vector<Choice> choices; }`
- **D.3** Add `DialogueRuntime` state: `currentNodeId`, `selectedChoiceIdx`, `bgmStream`, `imageTexture`
- **D.4** Render: full-screen `imageUrl` (load via `loadImageFromUrl()` — cache to memory), text box bottom 30%, speaker label
- **D.5** Audio: `playBgm(url)` / `playSfx(url)` using `SDL_OpenAudioDeviceStream` (reuse gameConsole pattern)
- **D.6** Input: ENTER/SPACE/click → `currentNodeId = next_id`; TAB cycles choices when `has_choices=1`; ENTER on choice → `currentNodeId = choices[idx].next_id`
- **D.7** Terminate condition: `next_id = 0` (or no row) → break dialogue loop, return from gameStory
- **I/O check**: seed SQLite with 3 nodes (linear→branch→ending), walk through, assert correct path taken for each TAB choice

### Phase E — Task 2.5: Skip button `gamestory-nut-bo-qua-cot-truyen-06`
- **E.1** Add `SKIP_BTN` rect top-right (40×20 px), fade-in synced with logo intro
- **E.2** Event handler: hit-test + ESC key
- **E.3** On skip: stop BGM, free textures, `running = false`, return 0 → main.cpp falls through to gameConsole
- **I/O check**: native run, click skip during intro → window transitions to gameConsole within 1 frame, no error logs

### Phase F — V3 Task 3.1: Per-file media cache
- **F.1** After C.5 finishes, query distinct media URLs from `dialogues`
- **F.2** For each URL, derive local path = `SDL_GetPrefPath/cache/<basename>`; skip if exists
- **F.3** Download via `http.h` to that path
- **F.4** Substitute URLs in `DialogueNode` to local paths at load time
- **I/O check**: clear cache, run → assert 1 file per unique URL appears in cache dir; rerun → assert no new downloads

### Phase G — V3 Task 3.2: Download-speed loading bar
- **G.1** Track `bytesDone / bytesTotal` across all in-flight downloads
- **G.2** Compute speed (KB/s) over 500ms windows
- **G.3** Loop logo fade-in cycle while downloads active (currently single-shot fade)
- **G.4** Render `"X.X MB/s   Y/Z files"` under bar
- **I/O check**: throttle network, observe bar fills proportionally to bytes

---

## Execution order

```
A.1→A.6 → B.1→B.3 → C.1→C.7 → D.1→D.7 → E.1→E.3 → F.1→F.4 → G.1→G.4
```

Stop after each lettered item, verify I/O, then proceed.

---

**Confirm:**
- `agree` → start at **A.1** (create `chapters/prompts/json.md`)
- `agree, but skip Phase F+G` → V2 only
- `change X.Y to ...` → adjust before starting