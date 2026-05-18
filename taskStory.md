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
          ├── prompts/
          │   ├── json.md          ← schema reference (đã tạo C2)
          │   ├── c001.md          ← prompt tuyến truyện chapter 1
          │   └── c002.md
          └── src/
              ├── c001/
              │   ├── c001.json    ← đã tạo C1
              │   └── media/
              └── c002/
                  ├── c002.json
                  └── media/
      ```
    - Cập nhật toàn bộ đường dẫn liên quan trong logic xử lý của module gameStory.
    - Cập nhật CMakeLists.txt và build script để phản ánh đường dẫn mới.

[x] Task 2.2: Thiết lập quy chuẩn nội dung chapter và quy trình CI/CD qua GitHub Actions.
    - `chapters/prompts/json.md` đã tạo (C2) — nguồn tham chiếu schema duy nhất.
    - `chapters/src/c001/c001.json` đã tạo (C1) — chapter mẫu với 5 stories + audio.
    - `.github/workflows/sync-chapters.yml` đã implement — trigger trên push chapters/src,
      upsert chapter Gist, rebuild manifest Gist, không commit vào repo.
    - Guard `if: github.actor != 'github-actions[bot]'` đã có.

[x] Task 2.3: Viết v2 gameStory/app.cpp — Cơ chế đồng bộ CSDL SQLite từ xa dùng manifest.json.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-dong-bo-sqlite-05a
    - syncFromGist() + applyChapterJson() + metaGetSha/metaSetSha đã implement.
    - SHA-diff: chỉ fetch chapter có SHA mới, bỏ qua chapter khớp SHA.
    - Offline fallback: SYNC_OFFLINE khi manifest fetch fail, tiếp tục dùng local DB.

[x] Task 2.4: viết v2 gameStory/app.cpp - tạo dialogue story engine để hiển thị cốt truyện kèm nhạc nền.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-phan-cot-game-05b
    - dialRunLoop() + drawDialoguePage() đã implement.
    - Hỗ trợ tuyến tính (nextNodeId) và phân nhánh (choices).
    - Tab cycle choices, Enter/Space/click advance, ESC/SKIP thoát.
    - BGM stub: diagUpdateBgm() log URL thay đổi (playback thật ở V3 Task 3.1).

[x] Task 2.5: viết v2 gameStory/app.cpp - nút skip để bỏ qua phần cốt truyện.
    - Comment codeblock này trong gameStory/app.cpp là: gamestory-nut-bo-qua-cot-truyen-06
    - SKIP_BTN đã implement trong dialRunLoop() và logo phase.
    - BUG PENDING (Issue 2.6/2.12): skip hiện cả trong logo phase — fix ở bước B2.

[x] Task 2.6: tích hợp v2 với các modules còn lại trong app/src qua file app/main.cpp.
    - Comment codeblock này trong gameStory/app.cpp là: integration/v2
    - Dual-mode entry: storyId=0 → intro+sync; storyId>0 → dialogue trực tiếp.
    - main.cpp xử lý next==2 (story preview) và next==3 (DB missing → gameStory init).

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
[x] Issue 1.2: Bổ sung ký tự Copyright (©) trên thanh tiêu đề của hệ điều hành.
[x] Issue 1.3: Tinh chỉnh độ dày font chữ (Thinning Effect) bằng màu sắc.
[x] Issue 1.4: Tăng thời lượng Loading Bar (INTRO_DURATION) lên 8 giây.
[x] Issue 1.5: Sai lệch vai trò giữa Logo Game và Logo UIT so với Task 1.3.

### V2
[ ] Issue 2.1: Khởi tạo DB client khi chưa tồn tại — Prefix theo Device ID.
    - → DEFERRED TO V3: Phụ thuộc vào V3 OTP/auth flow. Hiện tại dùng idUser="default" hardcode.
    - Sẽ implement cùng Task 3.x khi có email-based identity.

[x] Issue 2.2: So sánh phiên bản manifest và cập nhật có chọn lọc — Flow A (DB đã tồn tại).
    - syncFromGist() thực hiện SHA diff per chapter.
    - applyChapterJson() DELETE removed idStory + UPSERT changed rows trong 1 transaction.
    - metaSetSha() update sau mỗi chapter processed thành công.

[x] Issue 2.3: GitHub Action chỉ cập nhật Gist — KHÔNG commit vào git repo.
    - sync-chapters.yml: PATCH chapter Gist + rebuild manifest Gist.
    - Không có git add/commit trong workflow.
    - Guard: `if: github.actor != 'github-actions[bot]'` đã có.

[ ] Issue 2.4: Stream ảnh trực tiếp từ URL — Không tải về disk.
    - → DEFERRED TO V3 Task 3.1: Cần libcurl media fetch + SDL_Texture in-memory decode.
    - V2 hiện tại: imageUrl lưu trong DB, placeholder grey box khi render.

[ ] Issue 2.5: Progressive Bar hiển thị tiến trình sync thực tế.
    - Partially done: SyncProgress.total/done tracking đã có, bar blends sync vs time progress.
    - → DEFERRED: Full per-chapter granularity cần async fetch (V3 Task 3.2).
    - Hiện tại: bar fill = max(syncProgress, timeProgress) hoạt động đúng cho case 0-chapter.

[x] Issue 2.6: Button "Skip" CHỈ hiện trong Dialogue Screen — Không hiện khi đang sync/load.
    - Fixed in B2: removed skipHoverI declaration + mouse hover/click from PHASE_LOGO.
    - Skip rendered ONLY by drawDialoguePage() inside dialRunLoop().
    - ESC key still exits logo phase without on-screen button.

[x] Issue 2.7: Chapter Flow — Story đầu tiên không có điều kiện, các story sau unlock tuần tự.
    - Fixed in D2: parseRequiredStories() + checkAllParentsQualify() helpers added.
    - postSyncConditionCheck() queries sd.requiredStories and verifies each parent in default_Stories.
    - c001.json stories 2-5 have sequential requiredStories gates enforced and tested.

[x] Issue 2.8: Kiểm tra điều kiện sau Progressive Bar — Chạy lại hoặc mở story mới.
    - Fixed in B1: storyUserTable() helper added; all 7 idUser_Stories replaced.
    - postSyncConditionCheck() queries default_Stories correctly.
    - YES/NO prompt for newly unlocked stories works as designed.

[x] Issue 2.9: Cập nhật DB khi user xác nhận mở story mới.
    - Implemented trong runGameStory(): UPDATE isSelected=0 story cũ, isSelected=1+isActivated=1 story mới.
    - WASM: EM_ASM syncfs sau UPDATE.

[ ] Issue 2.10: DB device-id namespacing and initialization behavior.
    - → DEFERRED TO V3: Phụ thuộc vào V3 auth/OTP flow giống Issue 2.1.
    - V2 giữ nguyên "default.sqlite" + idUser="default".

[ ] Issue 2.11: In-memory media streaming (images/bgm/sfx) — no disk cache.
    - → DEFERRED TO V3 Task 3.1: Cần async fetch layer + SDL_Texture/AudioStream decode.
    - V2: media URLs stored in DB only, no playback.

[x] Issue 2.12: Skip-button visibility differs from spec.
    - Fixed in B2 alongside Issue 2.6. See Issue 2.6 for full details.

[x] Issue 2.13: `requiredStories` unlock logic not enforced.
    - Fixed in D2: parseRequiredStories() + checkAllParentsQualify() enforce CSV gate.
    - Backward compatible: empty CSV falls back to legacy sel.maxScore check.
    - c001.json requiredStories data used for regression testing.

### V3
[ ] bổ sung sau

---

## Hướng dẫn thực hiện V2

### Thứ tự triển khai khuyến nghị

```
C1 (c001.json) → C2 (json.md) → B1 (fix postSyncConditionCheck) →
B2 (fix skip visibility) → D2 (requiredStories parse)
```

### Kiến trúc dữ liệu

**`chapters/manifest.json`** là điểm kết nối duy nhất giữa CI và runtime.
CI ghi (qua Gist), runtime đọc. MANIFEST_GIST_URL trong app/.env trỏ đến manifest Gist.

### Thiết kế bảng SQLite

```sql
CREATE TABLE IF NOT EXISTS shared_meta (
    chapter_id     TEXT PRIMARY KEY,
    sha            TEXT NOT NULL,
    updated_at     INTEGER,
    media_base_url TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS shared_data (
    idStory INTEGER NOT NULL, idChapter INTEGER NOT NULL,
    storyName TEXT, chapterName TEXT,
    minScore INTEGER DEFAULT 0, minSpeed REAL DEFAULT 0,
    minRetries INTEGER DEFAULT 0, requiredStories TEXT DEFAULT '',
    nextBlockScore INTEGER DEFAULT 0, nextBlockSpeed REAL DEFAULT 0,
    tableMatrix TEXT DEFAULT '', xmlDialogue TEXT DEFAULT '',
    thumbnailPath TEXT DEFAULT '',
    PRIMARY KEY (idStory, idChapter)
);

CREATE TABLE IF NOT EXISTS shared_dialogues (
    rowId INTEGER PRIMARY KEY AUTOINCREMENT,
    idStory INTEGER NOT NULL, idChapter INTEGER NOT NULL, nodeId INTEGER NOT NULL,
    speaker TEXT DEFAULT '', text TEXT DEFAULT '',
    imageUrl TEXT DEFAULT '', bgmUrl TEXT DEFAULT '', sfxUrl TEXT DEFAULT '',
    nextNodeId INTEGER DEFAULT 0, hasChoices INTEGER DEFAULT 0,
    UNIQUE(idStory, idChapter, nodeId)
);

CREATE TABLE IF NOT EXISTS shared_choices (
    rowId INTEGER PRIMARY KEY AUTOINCREMENT,
    idStory INTEGER NOT NULL, idChapter INTEGER NOT NULL,
    nodeId INTEGER NOT NULL, choiceIdx INTEGER NOT NULL,
    label TEXT DEFAULT '', nextNodeId INTEGER DEFAULT 0,
    UNIQUE(idStory, idChapter, nodeId, choiceIdx)
);
```

### Kiểm tra hoàn thành V2

Trước khi đánh dấu V2 done, xác nhận:
- [x] `chapters/src/c001/c001.json` tồn tại với 5 stories + audio fields
- [x] `chapters/prompts/json.md` tồn tại với schema + audio guide đầy đủ
- [x] `sync-chapters.yml` workflow không commit vào repo
- [x] `postSyncConditionCheck()` query đúng bảng `default_Stories` (B1)
- [x] Skip button ẩn trong logo phase (B2)
- [x] `requiredStories` CSV được parse và enforce (D2)
- [ ] Thêm chapter mới → CI tự push Gist → app sync khi restart

---

## Rules:
    - Chỉ có 1 file c++ (app/src/gameStory/app.cpp) duy nhất để viết.
    - Các *.h phải để trong thư mục include của ứng dụng (app/src/gameStory/include).
    - Cần tách 1 file layout.h (app/src/gameStory/include/layout.h) để đảm bảo ứng dụng chạy theo khung hình có tỷ lệ 9:16.
    - Các file hình ảnh, âm thanh, phim... của module app phải để trong chính thư mục đang làm việc và đặt tên bắt đầu bằng tiền tố là tên thư mục.
    - Nội dung chapter (JSON, media) được đặt trong `chapters/src/c{id}/`, không nằm trong thư mục `app/`.
    - Tên file media không chứa ID chapter hay ID story — đặt tên theo nội dung gợi nhớ kèm suffix `_sfx` hoặc `_bgm`.
    - File `chapters/prompts/json.md` là nguồn tham chiếu duy nhất cho cấu trúc JSON và quy tắc thư mục.
